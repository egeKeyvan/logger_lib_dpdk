#ifndef DPDK_LOGGER_LIB_H
#define DPDK_LOGGER_LIB_H

#include "rte_timer.h"
#include "rte_metrics.h"
#include "rte_malloc.h"
#include "logger_config.h"
#include "vector"
#include "map"

#define DEBUG 1

struct per_ssb_measurements
{
    int values[3];
};

per_ssb_measurements empty_ssb_measurements = {0, 0, 0};

// Structure to hold necessary data about per DRB measurements.
// Assumption about this structure is that one DRB holds multiple cells.
struct per_drb_measurements{
    
    uint64_t max_active_ue_count;

    uint64_t min_active_ue_count;

    uint64_t max_inactive_ue_count;

    uint64_t min_inactive_ue_count;

    // Accumulate Total Active UE Count to take an average
    uint64_t total_active_ue_count;

    // Accumulate Total Inactive UE Count to take an average
    uint64_t total_inactive_ue_count;

    // Cell IDs
    std::vector<int> cell_ids;
};

struct per_drb_measurements empty_measurements;

/** This is a class to handle necessary logging in 5G context. Currently, it uses rte_metrics backed metric services.
 * This service is somewhat convienient to use but also inefficient. By using mempool structures of the DPDK directly,
 * memory performance of the class can be improved significantly. Also, the class does not support deleting a added metric since
 * this function is also absent in the rte_metrics library.
 * */
class LoggerLib {
    public:
        /** This function handles all the initialization necessary for logging library. This function
         * should be called from a main lcore thread. Please call
         * before delegating tasks to lcores. 
        **/
        LoggerLib(int core_socket_id);

        /** Add a new SSB PRACH for Logging. Return true if successfull. ID parameter is filled with
        * correct ID of the SSB. ID's are simply next index in the array. Library currently
        * does not support deleting SSB's. This interface is exactly the same as PRACH per cell measurements.
        * Without the need of additional interface, this functions can be used for per cell as well.
        * @returns True if successful and ID of the new SSB
        * **/
        bool add_new_ssb(int &id);

        /** This function updates the received PRACH count for the given SSB with ID 
        * @param id ID of the SSB.
        * @param type Type of the PRACH can be:
        * - PRACH_DEDICATED -
        * - PRACH_RAND_LOW -
        * - PRACH_RAND_HIGH -
        * 
        * **/
        bool on_ssb_prach_receive(int id, uint8_t type, uint32_t count = 1);

        /** Tick function for the timers. When this is called, timer callbacks are executed and values
        *   of the metrics are updated. Sensitivity of the callbacks are handled by: 
        * @param TIMER_RESOLUTION_CYCLES_LOGGER_LIB: Cycle count to check new callbacks.
        *  **/ 
        void LoggerTick();

        /** Update the Metric Values.
         * @param metric_id ID of the metric to update
         * @param value Updated Value
         * @param absolute If this is true, metric value is set to @param value. If false, @param value is added to the current value.
         * **/
        bool update_metric_value(int metric_id, int64_t value, bool absolute = true);


        // Get of the Three SSB Values for given ID. These values are the message counts after last iteration.
        int get_ssb_message_count(int ssb_id, uint8_t prach_type);

        // Get of the Three SSB Values for given ID. These values are the measured frequency in the last iteration and before values are reset.
        int get_ssb_message_frequency(int ssb_id, uint8_t prach_type);

        /** Add a new DRB measurement metric. Return its ID in parameter
         * @param drb_id ID of the parent DRB
         * @param ue_sampling_frequency Frequency of the polling about UE's in the DRB. Must be at least 10Hz.
         * **/
        bool add_new_drb(int &id, int ue_sampling_frequency);

        /** Add a new cell to DRB.
         * @param drb_id ID of the parent DRB
         * @param ue_sampling_frequency Frequency of the polling about UE's in the DRB. Must be at least 10Hz.
         * @param cell_id ID of the newly created cell
         * **/
        bool add_new_cell_to_drb(int drb_id, int &cell_id);


        /** Add new Active UE's to Cell contained under DRB with @param drb_id
         * @param drb_id ID of the parent DRB
         * @param cell_id ID of the Cell
         * @param count Number of the UE to add
         * @param new_ue If this is true, count will be added to current Active UE count. If false, same amount of UE will also be substracted from the inactive UE's.
         * **/
        bool add_new_active_ue_to_cell(int drb_id, int cell_id, uint32_t count, bool new_ue = true);

        /** Add new Inactive UE's to Cell contained under DRB with @param drb_id
         * @param drb_id ID of the parent DRB
         * @param cell_id ID of the Cell
         * @param count Number of the UE to add
         * @param new_ue If this is true, count will be added to current Inactive UE count. If false, same amount of UE will also be substracted from the active UE's.
         * **/
        bool add_new_inactive_ue_to_cell(int drb_id, int cell_id, uint32_t count, bool new_ue = true);


        // Per SSB Calculation Callback
        void per_ssb_timer_callback(__rte_unused struct rte_timer *tim, void *arg);

        // Per DRB Per Cell Measurement Callback
        void per_drb_per_cell_timer_callback(__rte_unused struct rte_timer *tim, void *arg);

        // Public Deconstructor
        ~LoggerLib();

    private:    
    // Timers to periodically sample SSB values. They are always sampled with 1sec period
    rte_timer per_ssb_timer;

    // For holding data, maps are preferred since they allow for O(log n) search which I believe will happen quite often.
    // ID's that belong to per_ssb_metrics. 
    std::map<int, per_ssb_measurements> per_ssb_data;
    
    std::map<int, per_drb_measurements> drb_measurement_map;
    
    CURRENT_METRIC_HANDLER metric_handler;
    
    // Which core logger is attached to
    int current_core_id;

    // Sampling Frequency for measurements. Must be at least 10Hz
    int ue_sampling_frequency;

    // When was the last reading of the UE values occured for per drb per cell measurements.
    uint64_t ue_last_sampled_time;

    // Timer to generate callbacks for sampling for per drb per cell measurements.
    rte_timer per_drb_per_cell_measurement_timer;

    // Available ID for the SSB.    
    int ssb_first_available_id;

    // Previos Timer Tick Time
    int prev_tsc;

    // A helper function to retrieve Active and Inactive UE's for per DRB per Cell.
    bool get_active_inactive_ue_count(int drb_id, int cell_id, uint32_t &active_count, uint32_t &inactive_count);
};



// Callback to calculate ssb Values


#endif