#ifndef COMMON_H
#define COMMON_H

#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern volatile int monitor_running;
extern const char* g_exe_path;

typedef enum {
    LINE_ENDING_LF = 0,   // \n
    LINE_ENDING_CRLF,     // \r\n
    LINE_ENDING_CR,       // \r
    LINE_ENDING_NONE      // None
} line_ending_t;

extern line_ending_t g_line_ending;
extern const char* g_line_ending_names[];
extern const char* g_line_ending_strings[];

int file_exists(const char* filename);
void resolve_path_relative_to_exe(const char* relative_path, char* resolved_path, size_t max_len);
int resolve_file_if_exists(const char* input_path, char* output_path, size_t max_len);
void open_url(const char* url);

#endif // COMMON_H
