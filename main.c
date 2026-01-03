#include <windows.h>
#include <stdio.h>

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

int main() {
    printf("ESP32 Host Tool v1.0 (Windows Native)\n");
    list_ports();
    printf("\nPress Enter to exit...");
    getchar(); // Keep window open
    return 0;
}