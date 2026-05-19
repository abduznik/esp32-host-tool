#include "common.h"
#include "setup.h"
#include "monitor.h"
#include "flasher.h"

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