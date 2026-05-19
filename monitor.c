#include "monitor.h"

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
