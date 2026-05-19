#ifndef COMMON_H
#define COMMON_H

#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern volatile int monitor_running;
extern const char* g_exe_path;

int file_exists(const char* filename);
void resolve_path_relative_to_exe(const char* relative_path, char* resolved_path, size_t max_len);
int resolve_file_if_exists(const char* input_path, char* output_path, size_t max_len);
void open_url(const char* url);

#endif // COMMON_H
