#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
    printf("[Mock Esptool] Executed with args:\n");
    for (int i = 0; i < argc; ++i) {
        printf("  argv[%d]: %s\n", i, argv[i]);
    }

    // Parse and validate arguments
    int has_chip = 0;
    int has_port = 0;
    int has_write_flash = 0;
    const char* port_val = NULL;
    const char* bin_val = NULL;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--chip") == 0 && i + 1 < argc) {
            if (strcmp(argv[i+1], "esp32") == 0) {
                has_chip = 1;
            }
            i++;
        } else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port_val = argv[i+1];
            has_port = 1;
            i++;
        } else if (strcmp(argv[i], "write_flash") == 0) {
            has_write_flash = 1;
            // The file path is usually the last argument
            if (argc > 5) {
                bin_val = argv[argc - 1];
            }
        }
    }

    if (!has_chip || !has_port || !has_write_flash) {
        fprintf(stderr, "[Mock Esptool] Error: Invalid argument structure!\n");
        return 3;
    }

    if (port_val && strcmp(port_val, "FAIL_PORT") == 0) {
        fprintf(stderr, "[Mock Esptool] Error: Could not connect to serial port %s!\n", port_val);
        return 1;
    }

    // Check if firmware file is valid/exists
    if (!bin_val) {
        fprintf(stderr, "[Mock Esptool] Error: No firmware binary specified!\n");
        return 2;
    }

    FILE* f = fopen(bin_val, "rb");
    if (!f) {
        fprintf(stderr, "[Mock Esptool] Error: Firmware file '%s' does not exist!\n", bin_val);
        return 2;
    }
    
    // Check if firmware is a mock invalid file
    char buffer[64];
    size_t read_bytes = fread(buffer, 1, sizeof(buffer) - 1, f);
    fclose(f);

    if (read_bytes > 0) {
        buffer[read_bytes] = '\0';
        if (strstr(buffer, "CORRUPT") != NULL) {
            fprintf(stderr, "[Mock Esptool] Error: Corrupted firmware image detected!\n");
            return 2;
        }
    }

    printf("[Mock Esptool] Flashing success!\n");
    return 0;
}
