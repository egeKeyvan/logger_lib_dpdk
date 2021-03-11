#include <iostream>
#include <logger_lib.h>

#include <rte_memory.h>
#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_debug.h>
#include <rte_metrics.h>
#include <rte_timer.h>

LoggerLib *logger;

static int
lcore_hello(__rte_unused void *arg)
{
	unsigned lcore_id;
	lcore_id = rte_lcore_id();
	unsigned socket_id = rte_socket_id();
	printf("hello from core %u\nSocket ID is %u\n", lcore_id, socket_id);
	
    
	int id = 5;
	bool result = logger->add_new_ssb(id);
	printf("Result is %d and ID is %d\n", result, id);

	logger->on_ssb_prach_receive(id, PRACH_RAND_LOW, 43);

	logger->on_ssb_prach_receive(id, PRACH_DEDICATED, 560);

	logger->on_ssb_prach_receive(id, PRACH_RAND_HIGH, 240);

	logger->on_ssb_prach_receive(id, PRACH_DEDICATED, 35);

	logger->on_ssb_prach_receive(id, PRACH_RAND_HIGH, 240);
	

	printf("Dedicated %d, Rand HIGH %d Rand LOW %d\n", 
					logger->get_ssb_message_count(id, PRACH_DEDICATED), 
						logger->get_ssb_message_count(id, PRACH_RAND_HIGH), 
							logger->get_ssb_message_count(id, PRACH_RAND_LOW));

	
	int drb_id;
	logger->add_new_drb(drb_id, 20);

	int cell_id;
	logger->add_new_cell_to_drb(drb_id, cell_id);
	

	while (1)
	{
		logger->LoggerTick();
	}
	
	// delete logger;
	return 0;
}

int main(int argc, char **argv){
    int ret;
	// unsigned lcore_id;

	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_panic("Cannot init EAL\n");

	logger = new LoggerLib(rte_socket_id());
	

	/* call lcore_hello() on every worker lcore */
	// RTE_LCORE_FOREACH_WORKER(lcore_id) {
	// 	rte_eal_remote_launch(lcore_hello, NULL, lcore_id);
	// }

	/* call it on main lcore too */
	lcore_hello(NULL);

	rte_eal_mp_wait_lcore();
    return 0;
}   