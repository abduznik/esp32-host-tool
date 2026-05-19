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
    char esptool_ver[64] = "v5.2.0";
    char firmware_ver[64] = "v1.0.4";
    char temp[64];

    printf("\n=== Setup & Download Tools ===\n");
    printf("Enter esptool version to use [default: v5.2.0]: ");
    if (fgets(temp, sizeof(temp), stdin)) {
        temp[strcspn(temp, "\r\n")] = 0;
        if (temp[0] != '\0') {
            strncpy(esptool_ver, temp, sizeof(esptool_ver) - 1);
        }
    }

    printf("Enter companion firmware version to use [default: v1.0.4]: ");
    if (fgets(temp, sizeof(temp), stdin)) {
        temp[strcspn(temp, "\r\n")] = 0;
        if (temp[0] != '\0') {
            strncpy(firmware_ver, temp, sizeof(firmware_ver) - 1);
        }
    }

    // Auto-normalize version tags to start with 'v'
    if (esptool_ver[0] != 'v' && esptool_ver[0] != 'V') {
        char temp_ver[64];
        snprintf(temp_ver, sizeof(temp_ver), "v%s", esptool_ver);
        strncpy(esptool_ver, temp_ver, sizeof(esptool_ver) - 1);
    }
    if (firmware_ver[0] != 'v' && firmware_ver[0] != 'V') {
        char temp_ver[64];
        snprintf(temp_ver, sizeof(temp_ver), "v%s", firmware_ver);
        strncpy(firmware_ver, temp_ver, sizeof(firmware_ver) - 1);
    }

    printf("\nOptions with versions: esptool %s, firmware %s\n"
           "1. Open esptool Releases page in browser\n"
           "2. Open companion firmware releases page in browser\n"
           "3. Auto-download esptool via system curl\n"
           "4. Auto-download companion firmware via system curl\n"
           "5. Back to Main Menu\n"
           "Choice: ", esptool_ver, firmware_ver);

    if (scanf("%d", &choice) != 1) {
        return;
    }
    // Consume leftover newline
    int c;
    while ((c = getchar()) != '\n' && c != EOF);

    char url[512];
    char cmd[1024];

    switch (choice) {
        case 1:
            snprintf(url, sizeof(url), "https://github.com/espressif/esptool/releases/tag/%s", esptool_ver);
            printf("Opening %s...\n", url);
            open_url(url);
            break;
        case 2:
            snprintf(url, sizeof(url), "https://github.com/abduznik/esp32-uart-cli/releases/tag/%s", firmware_ver);
            printf("Opening %s...\n", url);
            open_url(url);
            break;
        case 3:
            printf("Auto-downloading esptool %s...\n", esptool_ver);
#ifdef _WIN32
            snprintf(cmd, sizeof(cmd), "curl -L -o esptool.zip https://github.com/espressif/esptool/releases/download/%s/esptool-%s-windows-amd64.zip", esptool_ver, esptool_ver);
            printf("Executing: %s\n", cmd);
            system(cmd);
            printf("Extracting ZIP archive...\n");
            system("powershell -Command \"Expand-Archive -Force esptool.zip .\"");
            system("powershell -Command \"if (Test-Path esptool-windows-amd64\\esptool.exe) { Move-Item -Force esptool-windows-amd64\\esptool.exe . }\"");
            system("powershell -Command \"Remove-Item -Recurse -Force esptool.zip, esptool-windows-amd64 -ErrorAction SilentlyContinue\"");
            printf("\nExtracted and flattened esptool successfully!\n");
#elif defined(__APPLE__)
            snprintf(cmd, sizeof(cmd), 
                "ARCH=$(uname -m); "
                "if [ \"$ARCH\" = \"arm64\" ]; then "
                "  curl -L -o esptool.tar.gz https://github.com/espressif/esptool/releases/download/%s/esptool-%s-macos-arm64.tar.gz && "
                "  tar -xzf esptool.tar.gz && "
                "  mv esptool-macos-arm64/esptool . && "
                "  chmod +x esptool; "
                "else "
                "  curl -L -o esptool.tar.gz https://github.com/espressif/esptool/releases/download/%s/esptool-%s-macos-amd64.tar.gz && "
                "  tar -xzf esptool.tar.gz && "
                "  mv esptool-macos-amd64/esptool . && "
                "  chmod +x esptool; "
                "fi; "
                "rm -rf esptool.tar.gz esptool-macos-arm64 esptool-macos-amd64", esptool_ver, esptool_ver, esptool_ver, esptool_ver);
            printf("Executing curl & tar extraction...\n");
            system(cmd);
            printf("\nDone! esptool is now installed locally.\n");
#else
            snprintf(cmd, sizeof(cmd), 
                "curl -L -o esptool.tar.gz https://github.com/espressif/esptool/releases/download/%s/esptool-%s-linux-amd64.tar.gz && "
                "tar -xzf esptool.tar.gz && "
                "mv esptool-linux-amd64/esptool . && "
                "chmod +x esptool; "
                "rm -rf esptool.tar.gz esptool-linux-amd64", esptool_ver, esptool_ver);
            printf("Executing: %s\n", cmd);
            system(cmd);
            printf("\nDone! esptool is now installed locally.\n");
#endif
            break;
        case 4:
            printf("Auto-downloading companion firmware %s...\n", firmware_ver);
            snprintf(cmd, sizeof(cmd), "curl -L -o firmware.bin https://github.com/abduznik/esp32-uart-cli/releases/download/%s/firmware.bin", firmware_ver);
            printf("Executing: %s\n", cmd);
            system(cmd);
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
        } else if (file_exists("esptool-windows-amd64\\esptool.exe")) {
            esptool_cmd = "esptool-windows-amd64\\esptool.exe";
        } else if (file_exists("esptool.py")) {
            esptool_cmd = "python esptool.py";
        } else {
            esptool_cmd = "esptool";
        }
#else
        if (file_exists("./esptool")) {
            esptool_cmd = "./esptool";
        } else if (file_exists("./esptool-macos-arm64/esptool")) {
            esptool_cmd = "./esptool-macos-arm64/esptool";
        } else if (file_exists("./esptool-macos-amd64/esptool")) {
            esptool_cmd = "./esptool-macos-amd64/esptool";
        } else if (file_exists("./esptool-linux-amd64/esptool")) {
            esptool_cmd = "./esptool-linux-amd64/esptool";
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
            continue; // Re-run main selection/port list loop
        }

        switch (choice) {
            case 1:
                run_monitor(portName, 115200);
                break;
            case 2: {
                char bin_files[16][256];
                int bin_count = get_bin_files(bin_files, 16);
                char chosen_bin[512] = {0};

                if (bin_count > 0) {
                    printf("\n--- Found firmware .bin files in current directory ---\n");
                    for (int i = 0; i < bin_count; ++i) {
                        esp_info_t info;
                        if (detect_esp_bin_info(bin_files[i], &info)) {
                            printf("[%d] %s (App: %s, Ver: %s, Built: %s %s, IDF: %s)\n",
                                   i, bin_files[i], info.project_name, info.version, info.date, info.time, info.idf_ver);
                        } else {
                            printf("[%d] %s (Generic ESP32 Bin)\n", i, bin_files[i]);
                        }
                    }
                    printf("\nEnter index of .bin to flash, or type a custom file path: ");
                    
                    char choice_str[256];
                    if (fgets(choice_str, sizeof(choice_str), stdin)) {
                        choice_str[strcspn(choice_str, "\r\n")] = 0;
                        
                        // Check if numeric index
                        int is_num = 1;
                        if (choice_str[0] == '\0') {
                            is_num = 0;
                        } else {
                            for (int i = 0; choice_str[i] != '\0'; ++i) {
                                if (choice_str[i] < '0' || choice_str[i] > '9') {
                                    is_num = 0;
                                    break;
                                }
                            }
                        }
                        
                        if (is_num) {
                            int b_idx = atoi(choice_str);
                            if (b_idx >= 0 && b_idx < bin_count) {
                                strncpy(chosen_bin, bin_files[b_idx], sizeof(chosen_bin) - 1);
                                chosen_bin[sizeof(chosen_bin) - 1] = '\0';
                                printf("Selected binary: %s\n", chosen_bin);
                            } else {
                                printf("Invalid index. Falling back to typing custom path.\n");
                            }
                        } else {
                            strncpy(chosen_bin, choice_str, sizeof(chosen_bin) - 1);
                            chosen_bin[sizeof(chosen_bin) - 1] = '\0';
                        }
                    }
                }
                
                // If no binary files found, or custom path selected
                if (chosen_bin[0] == '\0') {
                    printf("Enter firmware .bin path: ");
                    if (fgets(chosen_bin, sizeof(chosen_bin), stdin)) {
                        chosen_bin[strcspn(chosen_bin, "\r\n")] = 0;
                    }
                }

                if (chosen_bin[0] != '\0') {
                    esp_info_t info;
                    if (detect_esp_bin_info(chosen_bin, &info)) {
                        printf("\nDetected ESP32 Firmware Signature:\n"
                               "  Project Name: %s\n"
                               "  App Version:  %s\n"
                               "  Compile Date: %s %s\n"
                               "  IDF Version:  %s\n",
                               info.project_name, info.version, info.date, info.time, info.idf_ver);
                    }
                    flash_firmware(portName, chosen_bin);
                }
                break;
            }
            default:
                printf("Invalid Choice. Exiting.\n");
                break;
        }
        break;
    }

    return 0;
}