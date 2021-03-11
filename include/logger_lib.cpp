#include "logger_lib.h"
#include "logger_config.h"
#include <string>
#include <inttypes.h>

#define FIRST_21_MASK  0x00000000001FFFFFUL
#define SECOND_21_MASK 0x000003FFFFE00000UL
#define THIRD_21_MASK  0xFFFFFC0000000000UL



static void ssb_timer_callback(struct rte_timer *tim, void *arg){
    LoggerLib *logger = (LoggerLib *)arg;
    logger->per_ssb_timer_callback(tim, NULL);
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
            debug_print("Adding A New SSB Timer for core %d\n", current_core_id);

            rte_timer_reset(&per_ssb_timer, rte_get_timer_hz(), PERIODICAL, current_core_id, ssb_timer_callback, this);
        }
        
        debug_print("A new SSB Device Has been created with ID: %d\n", id);
        
    }else{
        printf("SSB Device Registration has failed.\n");
        ssb_first_available_id--;

        return false;
    }

    // free(name_str);
    return true;
}

bool LoggerLib::update_metric_value(int metric_id, int value, bool absolute){
    return metric_handler.update_metric(metric_id, value, absolute);
}

// Each SSB Has three possible PRACH. Instead of preserving a metric for each one,
// values will be decoded into 64bit integer. So maximum value of the PRACH will be
// 21 bits. After That, values would overflow.
bool LoggerLib::on_ssb_prach_receive(int id, uint8_t type, uint32_t count){
    uint64_t current_ssb_prach; 
    bool result = metric_handler.get_metric(id, current_ssb_prach);

    // debug_print("Current SSB Prach Metric ID is %d and its value is %d\n", id, current_ssb_prach);

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
        // debug_print("Current SSB Dedicated Count is %d\n", result);
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

        // debug_print("Current SSB Rand HIGH Count is %d\n", result);


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

        // debug_print("Current SSB Rand LOW Count is %d\n", result);


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

    // debug_print("Current SSB Prach Metric ID is %d and its value is %d\n", ssb_id, current_ssb_prach);

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
    //debug_print("Per SSB Timer Callback\n", NULL);

    std::map<int, per_ssb_measurements>::iterator iterator;

    for(iterator = per_ssb_data.begin(); iterator != per_ssb_data.end(); iterator++){
        iterator->second.values[0] = get_ssb_message_count(iterator->first, PRACH_DEDICATED);
        iterator->second.values[1] = get_ssb_message_count(iterator->first, PRACH_RAND_HIGH);
        iterator->second.values[2] = get_ssb_message_count(iterator->first, PRACH_RAND_LOW);

        update_metric_value(iterator->first, 0, true);
        debug_print("Per SSB Values ID: %d, PRACH_DEDICATED %d, PRACH_RAND_HIGH %d, PRACH_RAND_LOW %d\n", 
                            iterator->first, iterator->second.values[0], iterator->second.values[1], iterator->second.values[2]);
    }
}


bool LoggerLib::add_new_drb(int &id, int ue_sample_frequency){
    struct per_drb_measurements drb;
    ue_sampling_frequency = ue_sample_frequency;

    // We can use size as ID since we don't support any deletion since rte_metrics does not support it.
    id = drb_measurement_map.size();

    drb_measurement_map.insert(std::make_pair(id, empty_measurements));
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
    it->second.cell_ids.push_back(cell_id);

    // If we have pushed a cell_id for the first time, then we need to activate the sampling to UE contexts.
    
    return true;
}