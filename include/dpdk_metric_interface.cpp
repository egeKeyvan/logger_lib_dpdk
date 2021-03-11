#include "dpdk_metric_interface.h"



DPDKMetricInterface::DPDKMetricInterface(){

}


DPDKMetricInterface::~DPDKMetricInterface(){
    rte_metrics_deinit();
    printf("Metric Interface is destroyed\n");
}


bool DPDKMetricInterface::initialize_metrics(void *data){
    socket_id = *((int *)data);
    rte_metrics_init(socket_id);

    return true;
}


bool DPDKMetricInterface::register_metric(const char *name_str, int &id){
    id = rte_metrics_reg_name(name_str);
    // printf("String is %s and length is %d\n\n", name_str, strlen(name_str));

    if(!(id >= 0)){
        return false;
    }

    //print_metrics();
    return true;
}


bool DPDKMetricInterface::update_metric(int metric_id, int64_t value, bool absolute){
    if(absolute){
        return (rte_metrics_update_value(socket_id, metric_id, value)) >= 0;
    }else{
        uint64_t current_value;
        if(get_metric(metric_id, current_value)){
            current_value += value;
            return (rte_metrics_update_value(socket_id, metric_id, value)) >= 0;
        }

        return false;
    }
}

bool DPDKMetricInterface::get_metric(int metric_id, uint64_t &metric_value){
    struct rte_metric_value *metrics;
    int len;
    int ret;
    int i;

    len = rte_metrics_get_names(NULL, 0);
    
    if (len < 0) {
        printf("Cannot get metrics count\n");
        return false;
    }
    if (len == 0) {
        printf("No metrics to display (none have been registered)\n");
        return false;
    }

    metrics = (struct rte_metric_value*) malloc(sizeof(struct rte_metric_value) * len);

    if (metrics == NULL) {
        printf("Cannot allocate memory\n");
        free(metrics);
        return false;
    }

    ret = rte_metrics_get_values(socket_id, metrics, len);
    if (ret < 0 || ret > len) {
        printf("Cannot get metrics values\n");
        free(metrics);
        return false;
    }

    for(i = 0; i < len; i++){
        if(metrics[i].key == metric_id){
            metric_value = metrics[i].value;
            break;
        }
    }

    return true;
}


void DPDKMetricInterface::print_metrics(){
    struct rte_metric_value *metrics;
    struct rte_metric_name *names;
    int len;
    int ret;

    len = rte_metrics_get_names(NULL, 0);
    if (len < 0) {
        printf("Cannot get metrics count\n");
        return;
    }
    if (len == 0) {
        printf("No metrics to display (none have been registered)\n");
        return;
    }


    metrics = (struct rte_metric_value*) malloc(sizeof(struct rte_metric_value) * len);
    names =   (struct rte_metric_name*) malloc(sizeof(struct rte_metric_name) * len);
    
    if (metrics == NULL || names == NULL) {
        printf("Cannot allocate memory\n");
        free(metrics);
        free(names);
        return;
    }
    
    ret = rte_metrics_get_values(socket_id, metrics, len);
    if (ret < 0 || ret > len) {
        printf("Cannot get metrics values\n");
        free(metrics);
        free(names);
        return;
    }
    
    printf("Metrics for port %i is %d units long\n", socket_id, len);
    // for (int i = 0; i < len; i++)
    //     printf("  %s: %llu -> %zu\n",
    //         names[metrics[i].key].name, metrics[i].value, strlen(names[metrics[i].key].name));
    //free(metrics);
    //free(names);
}
