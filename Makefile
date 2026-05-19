CC = gcc
CFLAGS = -Wall -Wextra -O2

# Detect OS
ifeq ($(OS),Windows_NT)
    TARGET = esp32-tool.exe
    LDFLAGS = 
    TEST_MOCK_ESPTOOL = tests/mock_esptool.exe
    TEST_RUNNER = tests/test_runner.exe
else
    UNAME_S := $(shell uname -s)
    TARGET = esp32-tool
    TEST_MOCK_ESPTOOL = tests/mock_esptool
    TEST_RUNNER = tests/test_runner
    ifeq ($(UNAME_S),Darwin)
        # On macOS, build a Universal Binary for both Intel (x86_64) and Apple Silicon (arm64)
        CFLAGS += -arch x86_64 -arch arm64
        LDFLAGS = -lpthread -arch x86_64 -arch arm64
    else
        LDFLAGS = -lpthread
    endif
endif

all: $(TARGET)

$(TARGET): main.c platform.h
	$(CC) $(CFLAGS) main.c -o $(TARGET) $(LDFLAGS)

# Compile tests
$(TEST_MOCK_ESPTOOL): tests/mock_esptool.c
	$(CC) $(CFLAGS) tests/mock_esptool.c -o $(TEST_MOCK_ESPTOOL)

$(TEST_RUNNER): tests/test_runner.c platform.h main.c
	$(CC) $(CFLAGS) tests/test_runner.c -o $(TEST_RUNNER) $(LDFLAGS)

test: all $(TEST_MOCK_ESPTOOL) $(TEST_RUNNER)
	@echo "================================="
	@echo "Running Test Suite..."
	@echo "================================="
	./$(TEST_RUNNER)

clean:
	rm -f esp32-tool esp32-tool.exe tests/mock_esptool tests/mock_esptool.exe tests/test_runner tests/test_runner.exe

.PHONY: all clean test
