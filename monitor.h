#ifndef MONITOR_H
#define MONITOR_H

#include "common.h"

THREAD_FUNC SerialReader(thread_arg_t lpParam);
void run_monitor(const char* portName, int baudRate);

#endif // MONITOR_H
