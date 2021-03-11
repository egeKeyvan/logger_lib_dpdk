#include "logger_lib.h"
#include "logger_config.h"
#include <string>
#include <inttypes.h>

#define FIRST_21_MASK  0x00000000001FFFFFUL
#define SECOND_21_MASK 0x000003FFFFE00000UL
#define THIRD_21_MASK  0xFFFFFC0000000000UL

#define FIRST_32_MASK  0x00000000FFFFFFFFUL
#define LAST_32_MASK   0xFFFFFFFF00000000UL

static void ssb_timer_callback(struct rte_timer *tim, void *arg){
    LoggerLib *logger = (LoggerLib *)arg;
    logger->per_ssb_timer_callback(tim, NULL);
}

static void per_drb_per_cell_callback(struct rte_timer *tim, void *arg){
    LoggerLib *logger = (LoggerLib *)arg;
    logger->per_drb_per_cell_timer_callback(tim, NULL);
}

LoggerLib::LoggerLib(int core_socket_id) : current_core_id(core_socket_id), ssb_first_available_id(-1){
    rte_timer_subsystem_init();
    rte_metrics_init(core_socket_id);

    metric_handler.initialize_metrics((void *) &core_socket_id);
}

LoggerLib::~LoggerLib(){
    
}


void LoggerLib::LoggerTick(){
    uint64_t cur_tsc = rte_rdtsc();
	uint64_t diff_tsc = cur_tsc - prev_tsc;
	
    if (diff_tsc > TIMER_RESOLUTION_CYCLES) {
		rte_timer_manage();
		prev_tsc = cur_tsc;
	}
}

bool LoggerLib::add_new_ssb(int &id){
    // if(ssb_first_available_id == TOTAL_SSB_COUNT - 1){
    //     return false;
    // }

    ssb_first_available_id++;

    std::string str;

    str += "per_ssb_log_";
    str += std::to_string(ssb_first_available_id);

    if(metric_handler.register_metric(str.c_str(), id) == true){
        per_ssb_data.insert(std::make_pair(id, empty_ssb_measurements));

        // We received our first SSB Callback. We need to start the Timer callbacks
        if(ssb_first_available_id == 0){
            debug_print(LOG_OUTPUT_FILE, "Adding A New SSB Timer for core %d\n", current_core_id);

            rte_timer_reset(&per_ssb_timer, rte_get_timer_hz(), PERIODICAL, current_core_id, ssb_timer_callback, this);
        }
        
        debug_print(LOG_OUTPUT_FILE, "A new SSB Device Has been created with ID: %d\n", id);
        
    }else{
        printf("SSB Device Registration has failed.\n");
        ssb_first_available_id--;

        return false;
    }

    // free(name_str);
    return true;
}

bool LoggerLib::update_metric_value(int metric_id, int64_t value, bool absolute){
    return metric_handler.update_metric(metric_id, value, absolute);
}

// Each SSB Has three possible PRACH. Instead of preserving a metric for each one,
// values will be decoded into 64bit integer. So maximum value of the PRACH will be
// 21 bits. After That, values would overflow.
bool LoggerLib::on_ssb_prach_receive(int id, uint8_t type, uint32_t count){
    uint64_t current_ssb_prach; 
    bool result = metric_handler.get_metric(id, current_ssb_prach);

    // debug_print(LOG_OUTPUT_FILE,"Current SSB Prach Metric ID is %d and its value is %d\n", id, current_ssb_prach);

    if(!result){
        return false;
    }

    uint64_t mask;

    switch (type)
    {
    
    // First 21 is for PRACH_DEDICATED Messages
    case PRACH_DEDICATED: {
        mask = FIRST_21_MASK;
        uint64_t result = current_ssb_prach & mask;

        

        result += count;
        // debug_print(LOG_OUTPUT_FILE,"Current SSB Dedicated Count is %d\n", result);
        current_ssb_prach &= (~mask);

        current_ssb_prach += result;

        return metric_handler.update_metric(id, current_ssb_prach, true);
    }

    // Middle 21 bits are for PRACH_HIGH
    case PRACH_RAND_HIGH: {
        mask = SECOND_21_MASK;
        uint64_t result = current_ssb_prach & mask;

        result >>= 21;
        result += count;

        // debug_print(LOG_OUTPUT_FILE,"Current SSB Rand HIGH Count is %d\n", result);


        result <<= 21;

        current_ssb_prach &= (~mask);
        current_ssb_prach += (result);

        return metric_handler.update_metric(id, current_ssb_prach, true);
    }

    // Remaining bits are for PRACH_RAND_LOW
    case PRACH_RAND_LOW: {
        mask = THIRD_21_MASK;
        uint64_t result = current_ssb_prach & mask;

        result >>= 42;
        result += count;

        // debug_print(LOG_OUTPUT_FILE,"Current SSB Rand LOW Count is %d\n", result);


        result <<= 42;

        current_ssb_prach &= (uint64_t)(~mask);
        current_ssb_prach += (result);

        return metric_handler.update_metric(id, current_ssb_prach, true);
    }

        
    
    default:
        return false;
    }
}



int LoggerLib::get_ssb_message_count(int ssb_id, uint8_t prach_type){
    uint64_t current_ssb_prach; 
    bool result = metric_handler.get_metric(ssb_id, current_ssb_prach);

    // debug_print(LOG_OUTPUT_FILE,"Current SSB Prach Metric ID is %d and its value is %d\n", ssb_id, current_ssb_prach);

    if(!result){
        return -1;
    }

    uint64_t mask;

    switch (prach_type)
    {
    
    // First 21 is for PRACH_DEDICATED Messages
    case PRACH_DEDICATED: {
        mask = FIRST_21_MASK;
        uint64_t result = current_ssb_prach & mask;

        return result;
    }

    // Middle 21 bits are for PRACH_HIGH
    case PRACH_RAND_HIGH: {
        mask = SECOND_21_MASK;
        uint64_t result = current_ssb_prach & mask;

        result >>= 21;

        return result;
    }

    // Remaining bits are for PRACH_RAND_LOW
    case PRACH_RAND_LOW: {
        mask = THIRD_21_MASK;
        uint64_t result = current_ssb_prach & mask;

        result >>= 42;

        return result;
    }

    default:
        return -1;
    }
}



void LoggerLib::per_ssb_timer_callback(__rte_unused struct rte_timer *tim, void *arg){
    //debug_print(LOG_OUTPUT_FILE,"Per SSB Timer Callback\n", NULL);

    std::map<int, per_ssb_measurements>::iterator iterator;

    for(iterator = per_ssb_data.begin(); iterator != per_ssb_data.end(); iterator++){
        iterator->second.values[0] = get_ssb_message_count(iterator->first, PRACH_DEDICATED);
        iterator->second.values[1] = get_ssb_message_count(iterator->first, PRACH_RAND_HIGH);
        iterator->second.values[2] = get_ssb_message_count(iterator->first, PRACH_RAND_LOW);

        update_metric_value(iterator->first, 0, true);
        debug_print(LOG_OUTPUT_FILE,"Per SSB Values ID: %d, PRACH_DEDICATED %d, PRACH_RAND_HIGH %d, PRACH_RAND_LOW %d\n", 
                            iterator->first, iterator->second.values[0], iterator->second.values[1], iterator->second.values[2]);
    }
}


void LoggerLib::per_drb_per_cell_timer_callback(__rte_unused struct rte_timer *tim, void *arg){
    debug_print(LOG_OUTPUT_FILE, "Per DRB Per Cell Timer Callback\n", NULL);
    std::map<int, per_drb_measurements>::iterator it;
    

    uint32_t active_count;
    uint32_t inactive_count;

    for(it = drb_measurement_map.begin(); it != drb_measurement_map.end(); it++){
        for(unsigned int i = 0; i < it->second.cell_ids.size(); i++){
            get_active_inactive_ue_count(it->first, i, active_count, inactive_count);
            
            it->second.max_active_ue_count = GENERIC_MAX(it->second.max_active_ue_count, active_count);
            it->second.min_active_ue_count = GENERIC_MIN(it->second.min_active_ue_count, active_count);

            it->second.max_inactive_ue_count = GENERIC_MAX(it->second.max_inactive_ue_count, active_count);
            it->second.min_inactive_ue_count = GENERIC_MIN(it->second.min_inactive_ue_count, active_count);

            it->second.total_active_ue_count += active_count;
            it->second.total_inactive_ue_count += inactive_count;
        }
    }
}

// Use the same trick to store two 32 bit numbers for active and inactive UE's. This way, we don't have to manage additional metrics and callbacks.
// This simplifies management and also provides a more performant logging.
bool LoggerLib::add_new_active_ue_to_cell(int drb_id, int cell_id, uint32_t count, bool new_ue){
    std::map<int, per_drb_measurements>::iterator it = drb_measurement_map.find(drb_id);
    
    if(it == drb_measurement_map.end()){
        printf("DRB with ID %d is not found\n", drb_id);
        return false;
    }

    if(cell_id >= (int) it->second.cell_ids.size()){
        printf("Cell with ID %d is not found within DRB with ID %d\n", cell_id, drb_id);
        return false;
    }

    int cell_metric_id = it->second.cell_ids[cell_id];

    uint64_t metric_value;
    if(metric_handler.get_metric(cell_metric_id, metric_value)){
        uint32_t active_count = metric_value & FIRST_32_MASK;
        active_count += count;

        metric_value &= (~FIRST_32_MASK);
        metric_value += active_count;



        if(!new_ue){
            uint64_t inactive_count = metric_value & LAST_32_MASK;
            
            inactive_count >>= 32;
            inactive_count -= count;
            inactive_count <<= 32;

            metric_value &= (~LAST_32_MASK);
            metric_value += inactive_count;
        }
        
        return metric_handler.update_metric(cell_metric_id, metric_value, true);
        // debug_print(LOG_OUTPUT_FILE,"New Cell Metric Value %llu\n", metric_value);
    }

    return false;
}



bool LoggerLib::add_new_inactive_ue_to_cell(int drb_id, int cell_id, uint32_t count, bool new_ue){
    std::map<int, per_drb_measurements>::iterator it = drb_measurement_map.find(drb_id);
    
    if(it == drb_measurement_map.end()){
        printf("DRB with ID %d is not found\n", drb_id);
        return false;
    }

    if(cell_id >= (int) it->second.cell_ids.size()){
        printf("Cell with ID %d is not found within DRB with ID %d\n", cell_id, drb_id);
        return false;
    }

    int cell_metric_id = it->second.cell_ids[cell_id];

    uint64_t metric_value;
    if(metric_handler.get_metric(cell_metric_id, metric_value)){
        uint64_t inactive_count = metric_value & LAST_32_MASK;

        inactive_count >>= 32;
        inactive_count += count;
        inactive_count <<= 32;

        metric_value &= (~LAST_32_MASK);
        metric_value += inactive_count;

        if(!new_ue){
            uint32_t active_count = metric_value & FIRST_32_MASK;
            active_count -= count;

            metric_value &= (~FIRST_32_MASK);
            metric_value += active_count;
        }
        
        return metric_handler.update_metric(cell_metric_id, metric_value, true);
        // debug_print(LOG_OUTPUT_FILE,"New Cell Metric Value %llu\n", metric_value);
    }

    return false;
}




bool LoggerLib::add_new_drb(int &id, int ue_sample_frequency){
    struct per_drb_measurements drb;
    ue_sampling_frequency = ue_sample_frequency;

    // We can use size as ID since we don't support any deletion since rte_metrics does not support it.
    id = drb_measurement_map.size();

    drb_measurement_map.insert(std::make_pair(id, empty_measurements));
    // debug_print(LOG_OUTPUT_FILE,"A new DRB is added with ID %d\n", id);
    return true;
}

bool LoggerLib::add_new_cell_to_drb(int drb_id, int &cell_id){
    std::map<int, per_drb_measurements>::iterator it = drb_measurement_map.find(drb_id);

    if(it == drb_measurement_map.end()){
        printf("DRB with ID %d is not found\n", drb_id);
        return false;
    }

    // Use vector size again. 
    cell_id = it->second.cell_ids.size();
    int cell_metric_id;

    std::string str;

    str += "per_drb_per_cell_";
    str += std::to_string(drb_id);
    str += std::to_string(cell_id);

    if(metric_handler.register_metric(str.c_str(), cell_metric_id) == true){
        debug_print(LOG_OUTPUT_FILE,"A new per DRB per Cell metric has been created with ID: %d\n", cell_metric_id);
        it->second.cell_ids.push_back(cell_metric_id);
    }else{
        printf("Per DRB Per Cell Metric Registration has failed with DRB ID %d and Cell ID %d.\n", drb_id, cell_id);
        return false;
    }
    

    if(cell_id == 0){
        // If we have pushed a cell_id for the first time, then we need to activate the sampling to UE contexts.
        rte_timer_reset(&per_drb_per_cell_measurement_timer, rte_get_timer_hz() / ue_sampling_frequency, PERIODICAL, 
                    current_core_id, per_drb_per_cell_callback, this);
        ue_last_sampled_time = rte_rdtsc();
    }
   
    return true;
}





bool LoggerLib::get_active_inactive_ue_count(int drb_id, int cell_id, uint32_t &active_count, uint32_t &inactive_count){
    std::map<int, per_drb_measurements>::iterator it = drb_measurement_map.find(drb_id);
    
    if(it == drb_measurement_map.end()){
        printf("DRB with ID %d is not found\n", drb_id);
        return false;
    }

    if(cell_id >= (int) it->second.cell_ids.size()){
        printf("Cell with ID %d is not found within DRB with ID %d\n", cell_id, drb_id);
        return false;
    }

    int cell_metric_id = it->second.cell_ids[cell_id];

    uint64_t metric_value;
    if(metric_handler.get_metric(cell_metric_id, metric_value)){
        active_count = metric_value & FIRST_32_MASK;
        uint64_t place_holder = metric_value & LAST_32_MASK;

        inactive_count = place_holder >> 32;

        return true;
    }


    return false;
}