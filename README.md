# _logger_lib_dpdk_
### Basic Purpose:
This is a logger library built on top of DPDK for use in 5G applications. Its main purpose is to create a logger class capable of handling all the logging related utilities in the 3GPP TS 38.314 V16.2.0 (2020-12) technical specifications. Library uses meson to configure itself and adds dpdk as system dependency. Current meson configuration also creates a .so file so it can be added as a library easily.

## Usage:
Class uses _rte_metrics_ and _rte_timer_ libraries in the background and handles interaction with these libraries. All these libraries require initialization in a primary process so logger library should be initialized after EAL is initialized.

Example:
```cpp
    ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_panic("Cannot init EAL\n");
	logger = new LoggerLib(rte_socket_id());
```

Class also provides a convenient `Tick` function to be called to handle necessary timer callbacks. This callback internally uses callbacks from the _rte_timer_ library. This means calling this function handles all timer callbacks for all registered timers inside or outside of the logger class. 

```cpp
    static int lcore_task(void *arg){
        // Handle Task related initializations
        ...
        while (1)
	    {
	        // Handle other recurring tasks
	        ...
	        // Handle Timer Callbacks
		    logger->LoggerTick();
	    }
	}
```

### Current Capabilities
Class is now able to handle PRACH request made to each SSB or cell. It is section 4.2.2.1 and 4.2.2.2 in the technical specifications. There is only a single interface for SSB's in the API but cell functionality is exactly the same so they can be used interchangibly. Functions to handle these logging activities can be found at the `logger_lib.h` header file. Sampling of these values are fixed at 1Hz and a callback is triggered every second to generate the logged data. User can access the last sampled data or current data if needed but specifications only mentions the sampled data.

Library also has a prototype for handling logging of UE contexes per DRB per cell. This is section 4.2.1.3 in the technical specification. Library currently handles the metric registration and adding new active or inactive UE contexes. It does not handle deletion and timer callback still has couple of things more to handle. 

#### Metric Interface Class
Library uses an adapter class called `MetricInterface` to handle any requests to raw metric storage. logger_lib handles the access to raw metric storage and extracts necessary data from stored information. Right now, a metric interface for _rte_metrics_ library is implemented. This library is a wrapper around _rte_mempool_ provided by dpdk and simplifies memory access for metric handling. Using this library however, results in a larger memory footprint and in the future, implementing a metric interface directly on top of _rte_mempool_ can be considered. 





