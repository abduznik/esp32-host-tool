#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        printf("Error: Could not open %s\n", portName);
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
    if (!esptool_cmd) {
        esptool_cmd = "esptool";
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

    list_ports();

    printf("\nEnter port (e.g., COM3 or /dev/ttyUSB0): ");
    if (scanf("%255s", portName) != 1) {
        return 1;
    }

    printf("\n--- Select Action ---\n"
           "1. Serial Monitor\n"
           "2. Flash Firmware\n"
           "Choice: ");

    if (scanf("%d", &choice) != 1) {
        return 1;
    }
    
    // Consume leftover characters in input buffer including newline
    int c;
    while ((c = getchar()) != '\n' && c != EOF);

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

    return 0;
}