#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

// Resource IDs
#define ID_ESPTOOL 101
#define ID_FIRMWARE 102

HANDLE hSerial;

// Helper to list ports on Windows using the Registry
void list_ports() {
    char path[256];
    char port_name[256];
    HKEY hKey;
    
    printf("--- Available Serial Ports ---\n");

    // Open the Registry key where Windows stores serial port mappings
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        printf("Error: Could not access registry to list ports.\n");
        return;
    }

    DWORD index = 0;
    DWORD name_len = sizeof(port_name);
    DWORD data_len = sizeof(path);
    DWORD type;

    // Iterate through the registry values
    while (RegEnumValue(hKey, index, port_name, &name_len, NULL, &type, (LPBYTE)path, &data_len) == ERROR_SUCCESS) {
        printf("[%d] %s (Internal: %s)\n", (int)index, path, port_name);
        
        // Reset buffers for next iteration
        index++;
        name_len = sizeof(port_name);
        data_len = sizeof(path);
    }

    RegCloseKey(hKey);
    
    if (index == 0) {
        printf("No serial ports found.\n");
    }
}

void configure_port(const char* portName)
{
    char fullPortName[32];
    // Ports above COM9 need a prefix "\\.\"
    snprintf(fullPortName, sizeof(fullPortName), "\\\\.\\%s", portName);

    hSerial = CreateFile(fullPortName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    if (hSerial == INVALID_HANDLE_VALUE)
    {
        printf("Error: Could not open %s (Code: %lu)\n", portName, GetLastError);
        exit(1);
    }

    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(hSerial, &dcbSerialParams))
    {
        printf("Error: Could not get state.\n");
        CloseHandle(hSerial);
        exit(1);
    }

    dcbSerialParams.BaudRate = CBR_115200;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(hSerial, &dcbSerialParams))
    {
        printf("Error: Could not set serial params.\n");
        CloseHandle(hSerial);
        exit(1);
    }

    // Set Timeouts
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    SetCommTimeouts(hSerial, &timeouts);

    printf("Connected to %s at 115200 baud.\n", portName);
}   


DWORD WINAPI SerialReader(LPVOID lpParam)
{
    char buffer[128];
    DWORD bytesRead;

    printf("--- Monitor (Press CTRL+C to Exit) ---\n");

    while(1)
    {
        //Readfile will wait based on our timeouts
        if (ReadFile(hSerial, buffer, sizeof(buffer) - 1, &bytesRead, NULL))
        {
            if (bytesRead > 0)
            {
                buffer[bytesRead] = '\0';
                printf("%s", buffer);
            }
        }
    }
    return 0;
}

void run_monitor(const char* portName)
{
    char inputBuffer[128];
    DWORD bytesWritten;

    printf("Initializing Monitor on %s... \n", portName);
    configure_port(portName);

    HANDLE hThread = CreateThread(NULL, 0, SerialReader, NULL, 0, NULL);

    printf("--- CLI Ready. Type commands (e.g., 'ping', 'led on') and press ENTER ---\n");

    // Main thread handling
    while (1)
    {
        if (fgets(inputBuffer, sizeof(inputBuffer), stdin))
        {
            WriteFile(hSerial, inputBuffer, strlen(inputBuffer), &bytesWritten, NULL);
        }
    }

    CloseHandle(hSerial);
    CloseHandle(hThread);
}

// Resource extraction
int extract_resource(int resource_id, const char* output_filename)
{
    HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(resource_id), RT_RCDATA);
    if (!hRes) return 0;

    HGLOBAL hMem = LoadResource(NULL, hRes);
    if (!hMem) return 0;

    DWORD size = SizeofResource(NULL, hRes);
    void* data = LockResource(hMem);

    FILE* f = fopen (output_filename, "wb");
    if (!f) return 0;

    fwrite(data, 1, size, f);
    fclose(f);
    return 1;
}

// Flash Logic with extraction
void flash_firmware(const char* portName)
{
    char tempPath[MAX_PATH];
    char esptoolPath[MAX_PATH];
    char firmwarePath[MAX_PATH];
    char command[2048];

    printf("\nInfo: Unpacking Internal tools...\n");

    GetTempPath(MAX_PATH, tempPath);

    snprintf(esptoolPath, sizeof(esptoolPath), "%sesptool_internal.exe", tempPath);
    snprintf(firmwarePath, sizeof(firmwarePath), "%sfirmware_internal.bin", tempPath);


    if (!extract_resource(ID_ESPTOOL, esptoolPath))
    {
        printf("Error! Failed to unpack internal esptool!\n");
        getchar();
        return;
    }
    if (!extract_resource(ID_FIRMWARE, firmwarePath))
    {
        printf("Error! Failed to unpack internal firmware!\n");
        getchar();
        return;
    }

    // Using python -m esptool for this
    snprintf(command, sizeof(command),
        "\"\"%s\" --chip esp32 --port %s --baud 460800 write_flash -z 0x0 \"%s\"\"",
        esptoolPath, portName, firmwarePath);
    printf("\nInfo: Starting Flash Process on %s...\n", portName);
    printf("Command: %s\n", command);
    fflush(stdout);

    int result = system(command);

    remove(esptoolPath);
    remove(firmwarePath);

    if (result == 0)
    {
        printf("\nSuccess! Firmware flashed successfully!\n");
    }
    else
    {
        printf("Fail! Flashing failed with code %d.\n", result);
    }
    
    printf("Press Enter to exit...");
    getchar();
}

int main() {
    char portName[16];
    int choice;

    list_ports();

    printf("\nEnter COM port (e.g, COM3): ");
    scanf("%s", portName);

    printf("\n--- Select Action ---\n"
        "1. Serial Monitor\n"
        "2. Flash Firmware\n"
        "Choice: \n");

    scanf("%d", &choice);
    getchar();

    switch (choice)
    {
        case 1:
            run_monitor(portName);
            break;
        case 2:
            flash_firmware(portName);
            break;
        default:
            printf("Invalid Choice. Exiting. \n");
            break;
    }

    return 0;
}