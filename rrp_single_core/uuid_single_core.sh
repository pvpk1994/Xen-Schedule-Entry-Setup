#!/bin/bash
# ***********************************
# RRP Single Core Automation Script
# ***********************************
# Author: Pavan Kumar Paluri @UH-2019
# ***********************************

# Shell script to automatically add a specified cpupool's uuid to the simulation file

# Root access only, else libxl functions shall not work in the simulation

DEFAULT_DIR=/home/rtlabuser
UUID_INFO=$DEFAULT_DIR/pool_uuid/uuid_info.txt
POOLID_INFO=$DEFAULT_DIR/pool_uuid/pool_id.txt
CPU_LIST=$DEFAULT_DIR/pool_uuid/cpu_list.txt


if [ $(whoami) != "root" ]; then
        echo "only root access!!!"
        exit 1
else
        # ******* Extraction for CPU-IDs for RRP-XEN 2.0 ************* #
	 echo " "
        echo "************************ NOTE *************************************"
        echo "************ SINGLE CORE RRP SIMULATORS ***************************"
        echo "***** CPUs excluding 1 and 3 are available for RRP-Xen v1.0******"
        echo "***** CPUs 1 and 3 are reserved for RRP-Xen v2.0 ****************"
        echo "************************ END **************************************"

        xl cpupool-list -c > $CPU_LIST
        # ****** Extractions for Pool-ID and UUIDs ********* #
        xl list -v -c > $DEFAULT_DIR/pool_uuid/list_info.txt
        NUM_RECORDS=$(cat $DEFAULT_DIR/pool_uuid/list_info.txt | awk 'END{print NR}')
        echo "total number: $NUM_RECORDS"
        #cat list_info.txt | awk '{ for(counter = 0; counter < 3; counter++) {if ($10=="aaf_pool") {print $7}}}' >$UUID_INFO
        cat $DEFAULT_DIR/pool_uuid/list_info.txt | awk '{if($10=="aaf_pool") {print $11}}' >${POOLID_INFO}
        # ******** FOR PCPUs **************** #
        cat $CPU_LIST | awk '{if($1=="aaf_pool") {print $2}}' > $DEFAULT_DIR/pool_uuid/rrp_cpus_list.txt
        # *********************************** #
        for i in $(seq 1 $NUM_RECORDS); do
                 cat $DEFAULT_DIR/pool_uuid/list_info.txt | awk '{if($10=="aaf_pool") {print $7}}' >$UUID_INFO
        done
        if [ $? -eq 0 ]; then
                echo " UUIDs of domain(s) and cpupool ID bought into simulation file"
               gcc $(pwd)/rrp_sched_entry_sc.c -lxenctrl -lm -luuid -o sched_entry_sc -L /usr/local/lib 2>/dev/null
               # gcc $(pwd)/aaf_prefinal_init_version.c -lxenctrl -lm -luuid -o sched_entry_sc -L /usr/local/lib 2>/dev/null
                if [ $? -eq 0 ]; then
                        echo "aaf schedul entries are set successfully!!"
                        echo "----proceeding with execution---"
                        ./sched_entry_sc
			echo "******************* NOTE *******************************************"
			echo "* If Domain is not set into blocked state, follow these steps below:"
			echo "* Step 1:Destroy the domain(s) and removing the cpu from its cpupool"
			echo "* Step 2:Destroy the cpupool (aaf_pool)"
			echo "* Step 3:Re-create the environment again in the following order"
			echo "* for(X in range(#domains under aaf_pool)) do"
			echo "* xl cpupool-create --> xl cpupool-cpu-add --> xl cpupool-list --> xl create domX.cfg"
			echo "* done"
			echo "* Re-run this script again"
			echo "******************* END ********************************************"
                        if [ $? -ne 0 ]; then
                                echo "runtime error while running arinc653 simulation"
                        fi
                else
                        echo "simulation file compilation failure!!!"
                fi
        else
                echo "UUID(s) and pool-id import failure"
        fi
fi

