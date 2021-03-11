#ifndef DPDK_LOGGER_CONFIG_H
#define DPDK_LOGGER_CONFIG_H

#include "dpdk_metric_interface.h"

#define LOG_OUTPUT_FILE stderr
// #define LOG_OUTPUT_FILE fopen("test_file.txt", "a+")

#define debug_print(FILE_OUT, fmt, ...) \
        do { if (DEBUG) fprintf(FILE_OUT, "%s:%d:%s(): " fmt, __FILE__, \
                                __LINE__, __func__, __VA_ARGS__); } while (0)

#define CURRENT_METRIC_HANDLER DPDKMetricInterface

#ifndef TIMER_RESOLUTION_CYCLES
    #define TIMER_RESOLUTION_CYCLES 10000000ULL // Around 5ms for 2Ghz
#endif


// Min Max Macros. These macros are not type safe.
#define GENERIC_MAX(x, y) ((x) > (y) ? (x) : (y))
#define GENERIC_MIN(x, y) ((x) > (y) ? (y) : (x))



#define PRACH_DEDICATED 1U
#define PRACH_RAND_LOW 2U
#define PRACH_RAND_HIGH 3U

#endif