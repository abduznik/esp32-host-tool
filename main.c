#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global control to run the serial monitor read loop
volatile int monitor_running = 1;

const char* g_exe_path = NULL;

// Helper to check if a file exists
int file_exists(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (f) {
        fclose(f);
        return 1;
    }
    return 0;
}

void resolve_path_relative_to_exe(const char* relative_path, char* resolved_path, size_t max_len) {
    if (g_exe_path) {
        const char* last_slash = strrchr(g_exe_path, '/');
        const char* last_backslash = strrchr(g_exe_path, '\\');
        const char* last_sep = (last_slash > last_backslash) ? last_slash : last_backslash;
        
        if (last_sep) {
            size_t dir_len = last_sep - g_exe_path + 1;
            if (dir_len < max_len) {
                strncpy(resolved_path, g_exe_path, dir_len);
                resolved_path[dir_len] = '\0';
                strncat(resolved_path, relative_path, max_len - dir_len - 1);
                return;
            }
        }
    }
    strncpy(resolved_path, relative_path, max_len - 1);
    resolved_path[max_len - 1] = '\0';
}

int resolve_file_if_exists(const char* input_path, char* output_path, size_t max_len) {
    if (file_exists(input_path)) {
        strncpy(output_path, input_path, max_len - 1);
        output_path[max_len - 1] = '\0';
        return 1;
    }
    if (input_path[0] != '/' && !(input_path[0] != '\0' && input_path[1] == ':')) {
        char resolved[512];
        resolve_path_relative_to_exe(input_path, resolved, sizeof(resolved));
        if (file_exists(resolved)) {
            strncpy(output_path, resolved, max_len - 1);
            output_path[max_len - 1] = '\0';
            return 1;
        }
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

    char choice_str[64];
    if (!fgets(choice_str, sizeof(choice_str), stdin)) {
        return;
    }
    choice = atoi(choice_str);

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

    printf("--- Monitor (Type 'exit' and press ENTER to go back to menu) ---\n");

    while (monitor_running) {
        int bytesRead = serial_read(port, buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            printf("%s", buffer);
            fflush(stdout);
        } else {
            usleep(10000); // 10ms sleep to prevent spinning on non-blocking reads/errors
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
        return;
    }

    printf("Connected to %s at %d baud.\n", portName, baudRate);

    monitor_running = 1;
    thread_t thread;
    if (thread_create(&thread, SerialReader, &port) != 0) {
        printf("Error: Could not create thread.\n");
        serial_close(port);
        return;
    }

    // Send a "fake enter" (handshake newline) to synchronize and request prompt immediately
    usleep(50000); 
    serial_write(port, "\n", 1);

    printf("--- CLI Ready. Type commands (e.g., 'ping', 'led on') and press ENTER. Type 'exit' to go back ---\n");

    // Main thread handling
    while (monitor_running) {
        if (fgets(inputBuffer, sizeof(inputBuffer), stdin)) {
            // Check if this is a real user entry (must contain a newline/carriage return)
            if (strchr(inputBuffer, '\n') == NULL && strchr(inputBuffer, '\r') == NULL) {
                usleep(10000); // Sleep and skip spurious empty/non-blocking reads
                continue;
            }
            inputBuffer[strcspn(inputBuffer, "\r\n")] = 0;
            if (strcmp(inputBuffer, "exit") == 0) {
                monitor_running = 0;
                break;
            }
            // Restore newline
            strcat(inputBuffer, "\n");
            serial_write(port, inputBuffer, strlen(inputBuffer));
        } else {
            usleep(10000); // Sleep briefly on stdin EOF or error
        }
    }

    serial_close(port);
    thread_close(thread);
    printf("Disconnected from monitor. Returning to main menu.\n");
}

void flash_firmware(const char* portName, int baudRate, const char* binPath) {
    char command[2048];
    const char* esptool_cmd = getenv("ESPTOOL");
    char resolved_esptool[512] = {0};
    char resolved_bin[512] = {0};

    // Auto-discover local esptool
    if (!esptool_cmd) {
#ifdef _WIN32
        if (resolve_file_if_exists("esptool.exe", resolved_esptool, sizeof(resolved_esptool))) {
            esptool_cmd = resolved_esptool;
        } else if (resolve_file_if_exists("esptool.py", resolved_esptool, sizeof(resolved_esptool))) {
            esptool_cmd = "python esptool.py";
        } else {
            esptool_cmd = "esptool";
        }
#else
        if (resolve_file_if_exists("esptool", resolved_esptool, sizeof(resolved_esptool))) {
            esptool_cmd = resolved_esptool;
        } else if (resolve_file_if_exists("esptool-macos-arm64/esptool", resolved_esptool, sizeof(resolved_esptool))) {
            esptool_cmd = resolved_esptool;
        } else if (resolve_file_if_exists("esptool-macos-amd64/esptool", resolved_esptool, sizeof(resolved_esptool))) {
            esptool_cmd = resolved_esptool;
        } else if (resolve_file_if_exists("esptool-linux-amd64/esptool", resolved_esptool, sizeof(resolved_esptool))) {
            esptool_cmd = resolved_esptool;
        } else if (resolve_file_if_exists("esptool.py", resolved_esptool, sizeof(resolved_esptool))) {
            esptool_cmd = "python3 ./esptool.py";
        } else {
            esptool_cmd = "esptool";
        }
#endif
    }

    if (resolve_file_if_exists(binPath, resolved_bin, sizeof(resolved_bin))) {
        binPath = resolved_bin;
    }

    printf("\nInfo: Starting Flash Process on %s...\n", portName);
#ifdef _WIN32
    snprintf(command, sizeof(command), "\"\"%s\" --chip esp32 --port %s --baud %d write_flash -z -fm dio 0x0 \"%s\"\"",
             esptool_cmd, portName, baudRate, binPath);
#else
    snprintf(command, sizeof(command), "\"%s\" --chip esp32 --port \"%s\" --baud %d write_flash -z -fm dio 0x0 \"%s\"",
             esptool_cmd, portName, baudRate, binPath);
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
    g_exe_path = argv[0];
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
            flash_firmware(port, 460800, bin);
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
    char portName[256] = {0};
    int baudRate = 115200;
    int choice;

    while (1) {
        printf("\n=== ESP32 Host Tool Main Menu ===\n");
        if (portName[0] != '\0') {
            printf("1. Select Serial Port [Current: %s]\n", portName);
        } else {
            printf("1. Select Serial Port [Current: None]\n");
        }
        printf("2. Configure Baud Rate [Current: %d]\n", baudRate);
        printf("3. Serial Monitor\n"
               "4. Flash Firmware\n"
               "5. Setup & Download Tools\n"
               "6. Quit\n"
               "Choice: ");

        char choice_str[64];
        if (!fgets(choice_str, sizeof(choice_str), stdin)) {
            continue;
        }
        choice_str[strcspn(choice_str, "\r\n")] = 0;
        if (choice_str[0] == '\0') {
            continue;
        }
        choice = atoi(choice_str);

        if (choice == 6) {
            printf("Exiting application. Goodbye!\n");
            break;
        }

        switch (choice) {
            case 1: {
                while (1) {
                    list_ports();
                    printf("\nEnter port index, port name (e.g., COM3 or /dev/ttyUSB0), or 'b' to go back: ");
                    char input[256];
                    if (!fgets(input, sizeof(input), stdin)) {
                        break;
                    }
                    input[strcspn(input, "\r\n")] = 0;

                    if (strcmp(input, "b") == 0 || strcmp(input, "B") == 0) {
                        break;
                    }

                    // Check if numeric index
                    int is_numeric = 1;
                    if (input[0] == '\0') {
                        is_numeric = 0;
                    } else {
                        for (int i = 0; input[i] != '\0'; ++i) {
                            if (input[i] < '0' || input[i] > '9') {
                                is_numeric = 0;
                                break;
                            }
                        }
                    }

                    if (is_numeric) {
                        int idx = atoi(input);
                        char ports[32][256];
                        int count = get_ports_list(ports, 32);
                        if (idx >= 0 && idx < count) {
                            strncpy(portName, ports[idx], sizeof(portName) - 1);
                            portName[sizeof(portName) - 1] = '\0';
                            printf("Resolved index %d to: %s\n", idx, portName);
                            break;
                        } else {
                            printf("Error: Invalid port index %d. Try again.\n\n", idx);
                            continue;
                        }
                    } else {
                        strncpy(portName, input, sizeof(portName) - 1);
                        portName[sizeof(portName) - 1] = '\0';
                        break;
                    }
                }
                break;
            }
            case 2: {
                printf("\n=== Configure Baud Rate ===\n"
                       "[1] 9600\n"
                       "[2] 19200\n"
                       "[3] 38400\n"
                       "[4] 57600\n"
                       "[5] 74880\n"
                       "[6] 115200 (Default)\n"
                       "[7] 230400\n"
                       "[8] 460800\n"
                       "[9] 921600\n"
                       "Enter choice (1-9) or type a custom baud rate: ");
                
                char baud_str[64];
                if (fgets(baud_str, sizeof(baud_str), stdin)) {
                    baud_str[strcspn(baud_str, "\r\n")] = 0;
                    if (baud_str[0] != '\0') {
                        int val = atoi(baud_str);
                        if (val >= 1 && val <= 9) {
                            int rates[] = {9600, 19200, 38400, 57600, 74880, 115200, 230400, 460800, 921600};
                            baudRate = rates[val - 1];
                        } else if (val > 0) {
                            baudRate = val;
                        } else {
                            printf("Invalid selection. Keeping %d baud.\n", baudRate);
                        }
                    }
                }
                printf("Baud rate configured to: %d\n", baudRate);
                break;
            }
            case 3: {
                if (portName[0] == '\0') {
                    printf("Error: No serial port selected! Please select a port first.\n");
                    break;
                }
                run_monitor(portName, baudRate);
                break;
            }
            case 4: {
                if (portName[0] == '\0') {
                    printf("Error: No serial port selected! Please select a port first.\n");
                    break;
                }
                
                char bin_files[16][256];
                int bin_count = get_bin_files(bin_files, 16);
                char chosen_bin[512] = {0};

                while (1) {
                    if (bin_count > 0) {
                        printf("\n--- Discovered firmware .bin files (local or near tool) ---\n");
                        for (int i = 0; i < bin_count; ++i) {
                            esp_info_t info;
                            if (detect_esp_bin_info(bin_files[i], &info)) {
                                printf("[%d] %s (App: %s, Ver: %s, Built: %s %s, IDF: %s)\n",
                                       i, bin_files[i], info.project_name, info.version, info.date, info.time, info.idf_ver);
                            } else {
                                printf("[%d] %s (Generic ESP32 Bin)\n", i, bin_files[i]);
                            }
                        }
                        printf("\nEnter index of .bin to flash, type a custom file path, or 'b' to go back: ");
                        
                        char choice_str[256];
                        if (fgets(choice_str, sizeof(choice_str), stdin)) {
                            choice_str[strcspn(choice_str, "\r\n")] = 0;
                            
                            if (strcmp(choice_str, "b") == 0 || strcmp(choice_str, "B") == 0) {
                                break;
                            }
                            
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
                                    printf("Invalid index. Try again.\n");
                                    continue;
                                }
                            } else {
                                strncpy(chosen_bin, choice_str, sizeof(chosen_bin) - 1);
                                chosen_bin[sizeof(chosen_bin) - 1] = '\0';
                            }
                        }
                    } else {
                        printf("Enter firmware .bin path or 'b' to go back: ");
                        if (fgets(chosen_bin, sizeof(chosen_bin), stdin)) {
                            chosen_bin[strcspn(chosen_bin, "\r\n")] = 0;
                            if (strcmp(chosen_bin, "b") == 0 || strcmp(chosen_bin, "B") == 0) {
                                break;
                            }
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
                        flash_firmware(portName, baudRate, chosen_bin);
                        break;
                    }
                }
                break;
            }
            case 5: {
                run_setup();
                break;
            }
            default:
                printf("Invalid Choice. Please select 1-6.\n");
                break;
        }
    }

    return 0;
}