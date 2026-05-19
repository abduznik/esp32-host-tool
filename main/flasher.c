#include "flasher.h"
#include "common.h"

void flash_firmware(const char* portName, int baudRate, const char* binPath) {
    char command[2048];
    const char* esptool_cmd = getenv("ESPTOOL");
    char resolved_esptool[512] = {0};
    char resolved_bin[512] = {0};

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
