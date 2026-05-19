#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper to check if a file exists
int file_exists(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (f) {
        fclose(f);
        return 1;
    }
    return 0;
}

// Helper to open a URL in default browser
void open_url(const char* url) {
    char cmd[512];
#ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "start %s", url);
#elif defined(__APPLE__)
    snprintf(cmd, sizeof(cmd), "open %s", url);
#else
    snprintf(cmd, sizeof(cmd), "xdg-open %s", url);
#endif
    system(cmd);
}

// Setup and auto-downloader menu
void run_setup() {
    int choice;
    printf("\n=== Setup & Download Tools ===\n"
           "1. Open esptool Releases page in browser\n"
           "2. Open companion firmware releases page in browser\n"
           "3. Auto-download esptool (v5.2.0) via system curl\n"
           "4. Auto-download companion firmware (v1.0.4) via system curl\n"
           "5. Back to Main Menu\n"
           "Choice: ");
    
    if (scanf("%d", &choice) != 1) {
        return;
    }
    // Consume leftover newline
    int c;
    while ((c = getchar()) != '\n' && c != EOF);

    switch (choice) {
        case 1:
            printf("Opening esptool releases page...\n");
            open_url("https://github.com/espressif/esptool/releases/tag/v5.2.0");
            break;
        case 2:
            printf("Opening companion firmware releases page...\n");
            open_url("https://github.com/abduznik/esp32-uart-cli/releases/tag/v1.0.4");
            break;
        case 3:
            printf("Auto-downloading esptool v5.2.0...\n");
#ifdef _WIN32
            system("curl -L -o esptool.zip https://github.com/espressif/esptool/releases/download/v5.2.0/esptool-v5.2.0-win64.zip");
            printf("\nDownloaded esptool.zip! You can extract it in this folder.\n");
#elif defined(__APPLE__)
            system("curl -L -o esptool.zip https://github.com/espressif/esptool/releases/download/v5.2.0/esptool-v5.2.0-macos.zip");
            printf("\nUnzipping esptool...\n");
            system("unzip -o esptool.zip && chmod +x esptool");
            printf("Done! esptool is now installed locally.\n");
#else
            system("curl -L -o esptool.zip https://github.com/espressif/esptool/releases/download/v5.2.0/esptool-v5.2.0-linux-amd64.zip");
            printf("\nUnzipping esptool...\n");
            system("unzip -o esptool.zip && chmod +x esptool");
            printf("Done! esptool is now installed locally.\n");
#endif
            break;
        case 4:
            printf("Auto-downloading companion firmware v1.0.4...\n");
            system("curl -L -o firmware.bin https://github.com/abduznik/esp32-uart-cli/releases/download/v1.0.4/firmware.bin");
            printf("Done! firmware.bin is now downloaded in this folder.\n");
            break;
        case 5:
        default:
            break;
    }
}

THREAD_FUNC SerialReader(thread_arg_t lpParam) {
    char buffer[128];
    serial_t port = *(serial_t*)lpParam;

    printf("--- Monitor (Press CTRL+C to Exit) ---\n");

    while (1) {
        int bytesRead = serial_read(port, buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            printf("%s", buffer);
            fflush(stdout);
        }
    }
    return 0;
}

void run_monitor(const char* portName, int baudRate) {
    char inputBuffer[128];

    printf("Initializing Monitor on %s... \n", portName);
    serial_t port = serial_open(portName, baudRate);

    if (port == INVALID_SERIAL) {
#ifdef _WIN32
        printf("Error: Could not open %s (Code: %lu)\n", portName, GetLastError());
#else
        printf("Error: Could not open %s\n", portName);
#endif
        exit(1);
    }

    printf("Connected to %s at %d baud.\n", portName, baudRate);

    thread_t thread;
    if (thread_create(&thread, SerialReader, &port) != 0) {
        printf("Error: Could not create thread.\n");
        serial_close(port);
        exit(1);
    }

    printf("--- CLI Ready. Type commands (e.g., 'ping', 'led on') and press ENTER ---\n");

    // Main thread handling
    while (1) {
        if (fgets(inputBuffer, sizeof(inputBuffer), stdin)) {
            serial_write(port, inputBuffer, strlen(inputBuffer));
        }
    }

    serial_close(port);
    thread_close(thread);
}

void flash_firmware(const char* portName, const char* binPath) {
    char command[2048];
    const char* esptool_cmd = getenv("ESPTOOL");
    
    // Auto-discover local esptool
    if (!esptool_cmd) {
#ifdef _WIN32
        if (file_exists("esptool.exe")) {
            esptool_cmd = "esptool.exe";
        } else if (file_exists("esptool.py")) {
            esptool_cmd = "python esptool.py";
        } else {
            esptool_cmd = "esptool";
        }
#else
        if (file_exists("./esptool")) {
            esptool_cmd = "./esptool";
        } else if (file_exists("./esptool.py")) {
            esptool_cmd = "python3 ./esptool.py";
        } else {
            esptool_cmd = "esptool";
        }
#endif
    }

    printf("\nInfo: Starting Flash Process on %s...\n", portName);
#ifdef _WIN32
    snprintf(command, sizeof(command), "\"\"%s\" --chip esp32 --port %s --baud 460800 write_flash -z 0x0 \"%s\"\"",
             esptool_cmd, portName, binPath);
#else
    snprintf(command, sizeof(command), "\"%s\" --chip esp32 --port \"%s\" --baud 460800 write_flash -z 0x0 \"%s\"",
             esptool_cmd, portName, binPath);
#endif
    printf("Command: %s\n", command);
    fflush(stdout);

    int result = system(command);

    if (result == 0) {
        printf("\nSuccess! Firmware flashed successfully!\n");
    } else {
        printf("Fail! Flashing failed with code %d.\n", result);
    }
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        if (strcmp(argv[1], "ports") == 0) {
            list_ports();
            return 0;
        } else if (strcmp(argv[1], "monitor") == 0) {
            if (argc < 3) {
                printf("Error: Monitor requires a port name.\n");
                printf("Usage: %s monitor <port> [--baud 115200]\n", argv[0]);
                return 1;
            }
            const char* port = argv[2];
            int baud = 115200;
            if (argc >= 5 && strcmp(argv[3], "--baud") == 0) {
                baud = atoi(argv[4]);
            }
            run_monitor(port, baud);
            return 0;
        } else if (strcmp(argv[1], "flash") == 0) {
            if (argc < 4) {
                printf("Error: Flash requires a port name and a firmware bin path.\n");
                printf("Usage: %s flash <port> <firmware.bin>\n", argv[0]);
                return 1;
            }
            const char* port = argv[2];
            const char* bin = argv[3];
            flash_firmware(port, bin);
            return 0;
        } else {
            printf("Unknown command: %s\n", argv[1]);
            printf("Usage:\n");
            printf("  %s ports\n", argv[0]);
            printf("  %s monitor <port> [--baud <baud>]\n", argv[0]);
            printf("  %s flash <port> <firmware.bin>\n", argv[0]);
            printf("  %s (no arguments for interactive menu)\n", argv[0]);
            return 1;
        }
    }

    // Interactive menu fallback
    char portName[256];
    char binPath[512];
    int choice;

    while (1) {
        while (1) {
            list_ports();
            printf("\nEnter port (e.g., COM3 or /dev/ttyUSB0, or select index e.g., 0) or 'r' to refresh: ");
            if (!fgets(portName, sizeof(portName), stdin)) {
                return 1;
            }
            portName[strcspn(portName, "\r\n")] = 0;

            if (strcmp(portName, "r") == 0 || strcmp(portName, "R") == 0) {
                printf("\nRefreshing port list...\n\n");
                continue;
            }

            // Check if input is a numeric index
            int is_numeric = 1;
            if (portName[0] == '\0') {
                is_numeric = 0;
            } else {
                for (int i = 0; portName[i] != '\0'; ++i) {
                    if (portName[i] < '0' || portName[i] > '9') {
                        is_numeric = 0;
                        break;
                    }
                }
            }

            if (is_numeric) {
                int idx = atoi(portName);
                char ports[32][256];
                int count = get_ports_list(ports, 32);
                if (idx >= 0 && idx < count) {
                    strncpy(portName, ports[idx], sizeof(portName) - 1);
                    portName[sizeof(portName) - 1] = '\0';
                    printf("Resolved index %d to: %s\n", idx, portName);
                } else {
                    printf("Error: Invalid port index %d. Try again.\n\n", idx);
                    continue;
                }
            }
            break;
        }

        printf("\n--- Select Action ---\n"
               "1. Serial Monitor\n"
               "2. Flash Firmware\n"
               "3. Setup & Download Tools\n"
               "Choice: ");

        if (scanf("%d", &choice) != 1) {
            return 1;
        }
        
        // Consume leftover characters in input buffer including newline
        int c;
        while ((c = getchar()) != '\n' && c != EOF);

        if (choice == 3) {
            run_setup();
            continue; // Re-run port selection / main loop after setup actions
        }

        switch (choice) {
            case 1:
                run_monitor(portName, 115200);
                break;
            case 2:
                printf("Enter firmware .bin path: ");
                if (fgets(binPath, sizeof(binPath), stdin)) {
                    // remove trailing newline
                    binPath[strcspn(binPath, "\r\n")] = 0;
                    flash_firmware(portName, binPath);
                }
                break;
            default:
                printf("Invalid Choice. Exiting.\n");
                break;
        }
        break;
    }

    return 0;
}