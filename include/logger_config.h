#ifndef DPDK_LOGGER_CONFIG_H
#define DPDK_LOGGER_CONFIG_H

#include "dpdk_metric_interface.h"


#define debug_print(fmt, ...) \
        do { if (DEBUG) fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, \
                                __LINE__, __func__, __VA_ARGS__); } while (0)

#define CURRENT_METRIC_HANDLER DPDKMetricInterface

#ifndef TIMER_RESOLUTION_CYCLES
    #define TIMER_RESOLUTION_CYCLES 10000000ULL // Around 5ms for 2Ghz
#endif

#define PRACH_DEDICATED 1U
#define PRACH_RAND_LOW 2U
#define PRACH_RAND_HIGH 3U

#endif