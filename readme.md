# ESP32 Host Tool

A lightweight, zero-dependency, cross-platform CLI companion tool for ESP32 devices, written in Pure C. 

The application serves as a generic diagnostic and deployment utility for flashing any `.bin` firmware and interacting with ESP32 serial interfaces.

---

## Supported Platforms
- **Windows** (using Win32 API and Registry scanning)
- **Linux** (using termios serial communications and POSIX threads)
- **macOS** (using termios with IOKit high baud-rate support and POSIX threads)

---

## Technical Architecture
* **Language:** Pure C (no C++ standard library dependencies).
* **Dependencies:** Zero external libraries. Uses native OS primitives: Win32 API on Windows, and POSIX `pthread`/`termios`/`glob` on Linux/macOS.
* **Modular Structure:** The codebase is fully modularized into isolated functional domains:
  - `main.c`: CLI router and main menu controller.
  - `common.c` / `common.h`: Shared resources, platform helpers, and path resolvers.
  - `monitor.c` / `monitor.h`: Dual-thread non-blocking serial communication driver.
  - `flasher.c` / `flasher.h`: Firmware signature verification and flashing pipeline wrapper.
  - `setup.c` / `setup.h`: Automated installation and setup script.
  - `platform.h`: Unified cross-platform serial and threading abstraction layer.
* **Build System:** Makefile with automatic platform detection (works with GCC/Clang on Unix, and MinGW on Windows).

---

## Core Features
* **Cross-Platform Port Detection:** 
  - **Windows:** Scans the Registry (`HARDWARE\DEVICEMAP\SERIALCOMM`)
  - **Linux:** Scans `/dev/ttyUSB*` and `/dev/ttyACM*`
  - **macOS:** Scans `/dev/cu.usbserial*` and `/dev/cu.SLAB*`
* **Stabilized Bidirectional Monitor:** Utilizes high-precision hardware serial buffer purging (`tcflush` / `PurgeComm`) and startup newline handshaking to eliminate flashing residuals and synchronize the console instantly.
* **Custom Line Endings:** Built-in configurator to dynamically toggle outgoing line endings on the fly:
  - Newline (`\n`)
  - CRLF (`\r\n`)
  - Carriage Return (`\r`)
  - None / Raw (Direct byte-stream transmission)
* **ESP32 Binary Descriptor Parsing:** Reads and extracts signature metadata directly from compiled firmware `.bin` files (Project name, App version, Compile date/time, and IDF version) prior to flashing.
* **Automated Tooling Setup:** Interactive downloader to retrieve and unpack platform-matched `esptool` executable wrappers and official companion firmware releases using standard system curl.
* **Dual Interface Mode:** Full command-line argument interface with automatic fallback to the interactive menu when executed without parameters.

---

## CLI Interface & Usage

### 1. List Detected Serial Ports
```bash
./esp32-tool ports
```

### 2. Launch Serial Monitor
```bash
./esp32-tool monitor <port> [--baud 115200]
```

### 3. Flash Firmware Directly
```bash
./esp32-tool flash <port> <path/to/firmware.bin>
```
> [!NOTE]
> You must have `esptool` installed and available on your system `PATH`. Alternatively, configure its location explicitly by setting the `ESPTOOL` environment variable, e.g., `export ESPTOOL=/path/to/esptool`.

### 4. Interactive Configuration Menu
```bash
./esp32-tool
```

---

## UART Companion Example & Limitations

To test interactive serial communications using this tool, upload the following non-blocking UART parser sketch to your microcontroller (e.g., ESP32, Arduino).

### High-Reliability Non-Blocking Sketch

```cpp
#define LED_PIN 2
const long SERIAL_BAUD = 115200;

char rxBuffer[64];
int rxIndex = 0;
bool ledState = false;

void setup() {
  Serial.begin(SERIAL_BAUD);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  Serial.println("System Ready. Type 'HELLO' and press ENTER.");
}

void loop() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (rxIndex > 0) {
        rxBuffer[rxIndex] = '\0';
        String input = String(rxBuffer);
        input.trim();
        
        if (input == "HELLO") {
          ledState = !ledState;
          digitalWrite(LED_PIN, ledState ? HIGH : LOW);
          Serial.print("LED toggled! Current state: ");
          Serial.println(ledState ? "ON" : "OFF");
        } else {
          Serial.print("Unknown command: ");
          Serial.println(input);
        }
        rxIndex = 0;
      }
    } else {
      if (rxIndex < (int)sizeof(rxBuffer) - 1) {
        rxBuffer[rxIndex++] = c;
      }
    }
  }
}
```

### Technical Limitations of Blocking Serial API
Many basic microcontroller tutorials recommend standard blocking utilities like `Serial.readStringUntil('\n')`. However, those methods introduce significant limitations when operating with low-level host serial drivers:
1. **Timeout-Driven Delays:** Blocking read calls default to a 1-second timeout. If a line terminator is delayed or missing, the microcontroller halts execution for a full second, causing severe UI latency.
2. **Buffer Corruption / Infinite Echo Loops:** Under heavy data streams or legacy core implementations (e.g., older ESP32 Arduino Core versions), blocking routines can fail to flush internal registers properly upon timeout, returning stale buffer content in an infinite feedback loop.
3. **Line Ending Sensitivity:** Simple parsers frequently fail to strip carriage returns (`\r`), leading to parsing mismatches. 

Always use a non-blocking character-by-character buffer structure (as shown above) to ensure stable, robust, and zero-latency serial interfaces.

---

## Getting Started & Compilation

### Prerequisites
- **GCC / Clang** (for Linux/macOS)
- **MinGW-w64** (for Windows)

### Compilation
The included `Makefile` automatically detects the host operating system, configures standard optimization flags, and links relevant platform dependencies:

```bash
# Clone the repository
git clone https://github.com/abduznik/esp32-host-tool.git
cd esp32-host-tool

# Compile the production binary
make
```

To run unit tests:
```bash
make test
```

To clean build output:
```bash
make clean
```