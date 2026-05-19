#include "common.h"

volatile int monitor_running = 1;
const char* g_exe_path = NULL;

line_ending_t g_line_ending = LINE_ENDING_LF; // Default: LF (\n)

const char* g_line_ending_names[] = {
    "Newline (\\n)",
    "CRLF (\\r\\n)",
    "Carriage Return (\\r)",
    "None / Raw"
};

const char* g_line_ending_strings[] = {
    "\n",
    "\r\n",
    "\r",
    ""
};

int file_exists(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (f) {
        fclose(f);
        return 1;
    }
    return 0;
}

void resolve_path_relative_to_exe(const char* relative_path, char* resolved_path, size_t max_len) {
    if (g_exe_path) {
        const char* last_slash = strrchr(g_exe_path, '/');
        const char* last_backslash = strrchr(g_exe_path, '\\');
        const char* last_sep = (last_slash > last_backslash) ? last_slash : last_backslash;
        
        if (last_sep) {
            size_t dir_len = last_sep - g_exe_path + 1;
            if (dir_len < max_len) {
                strncpy(resolved_path, g_exe_path, dir_len);
                resolved_path[dir_len] = '\0';
                strncat(resolved_path, relative_path, max_len - dir_len - 1);
                return;
            }
        }
    }
    strncpy(resolved_path, relative_path, max_len - 1);
    resolved_path[max_len - 1] = '\0';
}

int resolve_file_if_exists(const char* input_path, char* output_path, size_t max_len) {
    if (file_exists(input_path)) {
        strncpy(output_path, input_path, max_len - 1);
        output_path[max_len - 1] = '\0';
        return 1;
    }
    if (input_path[0] != '/' && !(input_path[0] != '\0' && input_path[1] == ':')) {
        char resolved[512];
        resolve_path_relative_to_exe(input_path, resolved, sizeof(resolved));
        if (file_exists(resolved)) {
            strncpy(output_path, resolved, max_len - 1);
            output_path[max_len - 1] = '\0';
            return 1;
        }
    }
    return 0;
}

void open_url(const char* url) {
    char cmd[512];
#ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "start %s", url);
#elif defined(__APPLE__)
    snprintf(cmd, sizeof(cmd), "open %s", url);
#else
    snprintf(cmd, sizeof(cmd), "xdg-open %s", url);
#endif
    system(cmd);
}
