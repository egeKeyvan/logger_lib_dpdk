#ifndef DPDPK_METRIC_INTERFACE_LOGGER_H
#define DPDPK_METRIC_INTERFACE_LOGGER_H

#include "metric_interface.h"
#include "rte_eal.h"
#include "rte_metrics.h"

class DPDKMetricInterface : public MetricInterface {
    public:
        DPDKMetricInterface();

        ~DPDKMetricInterface();

        bool initialize_metrics(void *data);

        bool register_metric(const char *metric_name, int &id);

        bool update_metric(int metric_id, int64_t value, bool absolute);

        bool get_metric(int metric_id, uint64_t &metric_value);

    protected:
        void print_metrics();

    private:
        int socket_id;
};

#endif