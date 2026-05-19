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