#include <windows.h>
#include <stdio.h>

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

void read_loop()
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
}

int main() {
    char portName[16];

    list_ports();

    printf("\nEnter COM port (e.g, COM3): ");
    scanf("%s", portName);

    configure_port(portName);

    // While Loop until window closed
    read_loop();

    CloseHandle(hSerial);

    return 0;
}