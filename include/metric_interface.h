#ifndef DPDK_LOGGER_CLASS_METRIC_INTERFACE_H
#define DPDK_LOGGER_CLASS_METRIC_INTERFACE_H

#include "rte_eal.h"

class MetricInterface{
    public:
        MetricInterface(){}

        virtual ~MetricInterface(){};

        // Initialize the Metrics library. A pointer can be supplied as a parameter
        virtual bool initialize_metrics(void *data) = 0;

        // Register a new metric to system. Its ID is returned with ID parameter.
        virtual bool register_metric(const char *metric_name, int &id) = 0;

        /** Update The Registered Metric Value. 
         * @param absolute: If true, metric value is set to @param value. Else value is added to current value
         * 
         * **/
        virtual bool update_metric(int metric_id, uint64_t value, bool absolute) = 0;

        // Get the metric value by ID.
        virtual bool get_metric(int metric_id, uint64_t &metric_value) = 0;


    protected:
        // Print the Currently Registered Metrics to screen. Should be used for testing purposes. Real Outputting will be done in
        // Logger class itself since this is the class that knows the structure of the registered metrics.
        virtual void print_metrics() = 0;

    private:
};



#endif