#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef _WIN32
    #include <windows.h>
    typedef HANDLE serial_t;
    #define INVALID_SERIAL INVALID_HANDLE_VALUE
    
    typedef HANDLE thread_t;
    #define THREAD_FUNC DWORD WINAPI
    typedef LPVOID thread_arg_t;
#else
    #include <fcntl.h>
    #include <unistd.h>
    #include <termios.h>
    #include <pthread.h>
    #include <glob.h>
    #include <string.h>
    #include <stdint.h>
    
    #ifdef __APPLE__
        #include <sys/ioctl.h>
        #ifndef IOSSIOSPEED
            #define IOSSIOSPEED _IOW('T', 2, speed_t)
        #endif
    #endif
    
    typedef int serial_t;
    #define INVALID_SERIAL (-1)
    
    typedef pthread_t thread_t;
    #define THREAD_FUNC void*
    typedef void* thread_arg_t;
#endif

#include <stdio.h>
#include <stdlib.h>

// ESP32 Application Descriptor parser structure
typedef struct {
    char version[32];
    char project_name[32];
    char time[16];
    char date[16];
    char idf_ver[32];
} esp_info_t;

// Thread operations
static inline int thread_create(thread_t* thread, THREAD_FUNC (*start_routine)(thread_arg_t), thread_arg_t arg) {
#ifdef _WIN32
    *thread = CreateThread(NULL, 0, start_routine, arg, 0, NULL);
    return (*thread != NULL) ? 0 : -1;
#else
    return pthread_create(thread, NULL, start_routine, arg);
#endif
}

static inline void thread_close(thread_t thread) {
#ifdef _WIN32
    CloseHandle(thread);
#else
    pthread_detach(thread);
#endif
}

// Serial operations
static inline serial_t serial_open(const char* port_name, int baud_rate) {
#ifdef _WIN32
    char fullPortName[64];
    // Ports above COM9 need a prefix "\\.\"
    snprintf(fullPortName, sizeof(fullPortName), "\\\\.\\%s", port_name);
    serial_t hSerial = CreateFile(fullPortName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hSerial == INVALID_HANDLE_VALUE) {
        return INVALID_SERIAL;
    }
    
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        CloseHandle(hSerial);
        return INVALID_SERIAL;
    }
    
    dcbSerialParams.BaudRate = baud_rate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    if (!SetCommState(hSerial, &dcbSerialParams)) {
        CloseHandle(hSerial);
        return INVALID_SERIAL;
    }
    
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    SetCommTimeouts(hSerial, &timeouts);
    
    return hSerial;
#else
    serial_t fd = open(port_name, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
        return INVALID_SERIAL;
    }
    
    // Clear O_NDELAY to make read blocking/timeout-based
    fcntl(fd, F_SETFL, 0);
    
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        close(fd);
        return INVALID_SERIAL;
    }
    
    speed_t speed;
    switch(baud_rate) {
        case 9600:   speed = B9600;   break;
        case 19200:  speed = B19200;  break;
        case 38400:  speed = B38400;  break;
        case 57600:  speed = B57600;  break;
        case 115200: speed = B115200; break;
#ifdef B230400
        case 230400: speed = B230400; break;
#endif
#ifdef B460800
        case 460800: speed = B460800; break;
#endif
#ifdef B921600
        case 921600: speed = B921600; break;
#endif
        default:     speed = B115200; break;
    }
    
    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);
    
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
    tty.c_iflag &= ~IGNBRK;                     // disable break processing
    tty.c_lflag = 0;                            // no signaling chars, no echo, no canonical processing
    tty.c_oflag = 0;                            // no remapping, no delays
    tty.c_cc[VMIN]  = 0;                        // read blocks until at least 1 character is available, or timeout
    tty.c_cc[VTIME] = 5;                        // 0.5 seconds read timeout (in deciseconds)
    
    tty.c_cflag |= (CLOCAL | CREAD);            // ignore modem controls, enable reading
    tty.c_cflag &= ~(PARENB | PARODD);          // shut off parity
    tty.c_cflag &= ~CSTOPB;                     // 1 stop bit
    tty.c_cflag &= ~CRTSCTS;                    // no flow control
    
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        close(fd);
        return INVALID_SERIAL;
    }
    
#ifdef __APPLE__
    // Native macOS arbitrary speed setting via ioctl
    speed_t apple_speed = baud_rate;
    if (ioctl(fd, IOSSIOSPEED, &apple_speed) < 0) {
        // Fallback silently if the port doesn't support IOSSIOSPEED
    }
#endif
    
    return fd;
#endif
}

static inline int serial_read(serial_t fd, char* buf, int size) {
#ifdef _WIN32
    DWORD bytesRead = 0;
    if (ReadFile(fd, buf, size, &bytesRead, NULL)) {
        return (int)bytesRead;
    }
    return -1;
#else
    return read(fd, buf, size);
#endif
}

static inline int serial_write(serial_t fd, const char* buf, int size) {
#ifdef _WIN32
    DWORD bytesWritten = 0;
    if (WriteFile(fd, buf, size, &bytesWritten, NULL)) {
        return (int)bytesWritten;
    }
    return -1;
#else
    return write(fd, buf, size);
#endif
}

static inline void serial_close(serial_t fd) {
#ifdef _WIN32
    CloseHandle(fd);
#else
    close(fd);
#endif
}

// Get list of ports
static inline int get_ports_list(char ports[][256], int max_ports) {
    int count = 0;
#ifdef _WIN32
    HKEY hKey;
    char path[256];
    char port_name[256];
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return 0;
    }

    DWORD index = 0;
    DWORD name_len = sizeof(port_name);
    DWORD data_len = sizeof(path);
    DWORD type;

    while (RegEnumValue(hKey, index, port_name, &name_len, NULL, &type, (LPBYTE)path, &data_len) == ERROR_SUCCESS) {
        if (count < max_ports) {
            strncpy(ports[count], path, 255);
            ports[count][255] = '\0';
            count++;
        }
        index++;
        name_len = sizeof(port_name);
        data_len = sizeof(path);
    }
    RegCloseKey(hKey);
#else
    glob_t glob_result;
    memset(&glob_result, 0, sizeof(glob_result));
    
    int glob_flags = 0;
    
#if defined(__APPLE__)
    glob("/dev/cu.usbserial*", glob_flags, NULL, &glob_result);
    glob("/dev/cu.SLAB*", glob_flags | GLOB_APPEND, NULL, &glob_result);
#else
    glob("/dev/ttyUSB*", glob_flags, NULL, &glob_result);
    glob("/dev/ttyACM*", glob_flags | GLOB_APPEND, NULL, &glob_result);
#endif

    for (size_t i = 0; i < glob_result.gl_pathc; ++i) {
        if (count < max_ports) {
            strncpy(ports[count], glob_result.gl_pathv[i], 255);
            ports[count][255] = '\0';
            count++;
        }
    }
    globfree(&glob_result);
#endif
    return count;
}

// Port scanner
static inline void list_ports() {
    printf("--- Available Serial Ports ---\n");
    char ports[32][256];
    int count = get_ports_list(ports, 32);
    for (int i = 0; i < count; ++i) {
        printf("[%d] %s\n", i, ports[i]);
    }
    if (count == 0) {
        printf("No serial ports found.\n");
    }
}

// Detect ESP32 app descriptor signature in local binary files
static inline int detect_esp_bin_info(const char* filepath, esp_info_t* info) {
    FILE* f = fopen(filepath, "rb");
    if (!f) return 0;

    // Read the first 4096 bytes of the firmware
    uint8_t buffer[4096];
    size_t bytesRead = fread(buffer, 1, sizeof(buffer), f);
    fclose(f);

    if (bytesRead < 256) return 0;

    // Search for the magic word 0xabcd5432 (little-endian: 32 54 cd ab)
    for (size_t i = 0; i < bytesRead - 256; ++i) {
        if (buffer[i] == 0x32 && buffer[i+1] == 0x54 && buffer[i+2] == 0xCD && buffer[i+3] == 0xAB) {
            size_t struct_base = i;
            memcpy(info->version, &buffer[struct_base + 16], 32);
            info->version[31] = '\0';

            memcpy(info->project_name, &buffer[struct_base + 48], 32);
            info->project_name[31] = '\0';

            memcpy(info->time, &buffer[struct_base + 80], 16);
            info->time[15] = '\0';

            memcpy(info->date, &buffer[struct_base + 96], 16);
            info->date[15] = '\0';

            memcpy(info->idf_ver, &buffer[struct_base + 112], 32);
            info->idf_ver[31] = '\0';

            return 1;
        }
    }
    return 0;
}

// Scan the current working directory for files matching *.bin
static inline int get_bin_files(char files[][256], int max_files) {
    int count = 0;
#ifdef _WIN32
    WIN32_FIND_DATA findData;
    HANDLE hFind = FindFirstFile("*.bin", &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (count < max_files) {
                strncpy(files[count], findData.cFileName, 255);
                files[count][255] = '\0';
                count++;
            }
        } while (FindNextFile(hFind, &findData) && count < max_files);
        FindClose(hFind);
    }
#else
    glob_t glob_result;
    memset(&glob_result, 0, sizeof(glob_result));
    if (glob("*.bin", 0, NULL, &glob_result) == 0) {
        for (size_t i = 0; i < glob_result.gl_pathc; ++i) {
            if (count < max_files) {
                strncpy(files[count], glob_result.gl_pathv[i], 255);
                files[count][255] = '\0';
                count++;
            }
        }
    }
    globfree(&glob_result);
#endif
    return count;
}

#endif // PLATFORM_H
