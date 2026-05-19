#include "setup.h"
#include "common.h"

void run_setup(void) {
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
            snprintf(url, sizeof(url), "https://github.com/abduznik/esp32-host-tool/releases/tag/%s", firmware_ver);
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
