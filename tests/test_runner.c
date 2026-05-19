#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifndef _WIN32
    #include <unistd.h>
#endif

#define main app_main
#include "../main/main.c"
#undef main

// Helper to write a dummy file
void write_dummy_file(const char* path, const char* content) {
    FILE* f = fopen(path, "wb");
    assert(f != NULL);
    fwrite(content, 1, strlen(content), f);
    fclose(f);
}

// Dummy thread target
THREAD_FUNC DummyThreadFunc(thread_arg_t arg) {
    int* val = (int*)arg;
    *val = 42;
    return 0;
}

void test_threads() {
    printf("Running test_threads...\n");
    thread_t thread;
    int test_val = 0;
    int res = thread_create(&thread, DummyThreadFunc, &test_val);
    assert(res == 0);
    
    // Give the thread a tiny bit of time to execute
#ifndef _WIN32
    usleep(10000);
#else
    Sleep(10);
#endif

    assert(test_val == 42);
    thread_close(thread);
    printf("  [PASS] Thread operations succeeded.\n");
}

void test_serial_invalid() {
    printf("Running test_serial_invalid...\n");
    // Attempting to open a port that doesn't exist
    serial_t port = serial_open("NON_EXISTENT_PORT_xyz_123", 115200);
    assert(port == INVALID_SERIAL);
    printf("  [PASS] Invalid serial open failed gracefully.\n");
}

void test_flash_pipeline() {
    printf("Running test_flash_pipeline...\n");

    // Set environment variable ESPTOOL to point to our mock esptool
#ifdef _WIN32
    _putenv("ESPTOOL=tests\\mock_esptool.exe");
#else
    setenv("ESPTOOL", "./tests/mock_esptool", 1);
#endif

    // Create dummy firmware files
    write_dummy_file("tests/dummy_good.bin", "VALID_ESP32_BOOTLOADER_IMAGE_DATA_12345");
    write_dummy_file("tests/dummy_corrupt.bin", "CORRUPT_IMAGE_HEADER_ERR_000");

    // Scenario A: Valid firmware, valid port (Should succeed)
    printf("  Subtest A: Good flash\n");
    flash_firmware("COM3", 460800, "tests/dummy_good.bin");

    // Scenario B: Port connection failure (Should fail)
    printf("  Subtest B: Port failure\n");
    flash_firmware("FAIL_PORT", 460800, "tests/dummy_good.bin");

    // Scenario C: Corrupted firmware image (Should fail)
    printf("  Subtest C: Corrupt firmware\n");
    flash_firmware("COM3", 460800, "tests/dummy_corrupt.bin");

    // Scenario D: Missing firmware image (Should fail)
    printf("  Subtest D: Missing firmware\n");
    flash_firmware("COM3", 460800, "tests/dummy_missing.bin");

    // Clean up files
    remove("tests/dummy_good.bin");
    remove("tests/dummy_corrupt.bin");

    printf("  [PASS] Flashing pipeline simulations complete.\n");
}

void test_cli_parsing() {
    printf("Running test_cli_parsing...\n");

    // Test ports listing command
    char* argv_ports[] = {"esp32-tool", "ports"};
    int res = app_main(2, argv_ports);
    assert(res == 0);

    // Test monitor error handling (missing port)
    char* argv_monitor_err[] = {"esp32-tool", "monitor"};
    res = app_main(2, argv_monitor_err);
    assert(res == 1);

    // Test flash error handling (missing arguments)
    char* argv_flash_err[] = {"esp32-tool", "flash", "COM3"};
    res = app_main(3, argv_flash_err);
    assert(res == 1);

    // Test invalid command
    char* argv_invalid[] = {"esp32-tool", "invalid_command"};
    res = app_main(2, argv_invalid);
    assert(res == 1);

    printf("  [PASS] CLI parsing and validation logic tests succeeded.\n");
}

void test_esp_bin_parser() {
    printf("Running test_esp_bin_parser...\n");
    
    // Create a mock firmware buffer of 1024 bytes
    uint8_t buffer[1024] = {0};
    // Let's place the magic word 0xabcd5432 at offset 0x100
    // In little-endian: 32 54 cd ab
    size_t offset = 0x100;
    buffer[offset] = 0x32;
    buffer[offset+1] = 0x54;
    buffer[offset+2] = 0xCD;
    buffer[offset+3] = 0xAB;
    
    // Copy fake version starts at offset 16 (0x100 + 16 = 0x110)
    strcpy((char*)&buffer[offset + 16], "v1.2.3-test");
    // Copy project name starts at offset 48 (0x100 + 48 = 0x130)
    strcpy((char*)&buffer[offset + 48], "mock-project");
    // Copy time at 80, date at 96, idf_ver at 112
    strcpy((char*)&buffer[offset + 80], "12:34:56");
    strcpy((char*)&buffer[offset + 96], "May 19 2026");
    strcpy((char*)&buffer[offset + 112], "v5.2-dirty");
    
    // Write it to a file
    FILE* f = fopen("tests/dummy_desc.bin", "wb");
    assert(f != NULL);
    fwrite(buffer, 1, sizeof(buffer), f);
    fclose(f);
    
    // Test the parser
    esp_info_t info;
    int parsed = detect_esp_bin_info("tests/dummy_desc.bin", &info);
    assert(parsed == 1);
    
    assert(strcmp(info.version, "v1.2.3-test") == 0);
    assert(strcmp(info.project_name, "mock-project") == 0);
    assert(strcmp(info.time, "12:34:56") == 0);
    assert(strcmp(info.date, "May 19 2026") == 0);
    assert(strcmp(info.idf_ver, "v5.2-dirty") == 0);
    
    remove("tests/dummy_desc.bin");
    printf("  [PASS] ESP32 app descriptor parsing test passed.\n");
}

int main() {
    printf("=================================\n");
    printf("   ESP32 HOST TOOL TEST SUITE    \n");
    printf("=================================\n");

    test_threads();
    test_serial_invalid();
    test_flash_pipeline();
    test_cli_parsing();
    test_esp_bin_parser();

    printf("\nAll unit tests passed successfully!\n");
    return 0;
}
