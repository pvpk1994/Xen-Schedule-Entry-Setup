#!/bin/bash

# ***********************************
# Author: Pavan Kumar Paluri @UH-2019
# ***********************************

# Shell script to automatically add a specified cpupool's uuid to the simulation file

# Root access only, else libxl functions shall not work in the simulation

UUID_INFO=$(pwd)/pool_uuid/uuid_info.txt
POOLID_INFO=$(pwd)/pool_uuid/pool_id.txt
CPU_LIST=$(pwd)/pool_uuid/cpu_list.txt

if [ $(whoami) != "root" ]; then
	echo "only root access!!!"
	exit 1
else
        # ******* Extraction for CPU-IDs for RRP-XEN 2.0 ************* #
        xl cpupool-list -c > $CPU_LIST
        # ****** Extractions for Pool-ID and UUIDs ********* #
	xl list -v -c > $(pwd)/pool_uuid/list_info.txt
	NUM_RECORDS=$(cat $(pwd)/pool_uuid/list_info.txt | awk 'END{print NR}')
	echo "total number: $NUM_RECORDS"
	#cat list_info.txt | awk '{ for(counter = 0; counter < 3; counter++) {if ($10=="aaf_pool") {print $7}}}' >$UUID_INFO
	cat $(pwd)/pool_uuid/list_info.txt | awk '{if($10=="aaf_pool") {print $11}}' >${POOLID_INFO}
        # ******** FOR PCPUs **************** #
        cat $CPU_LIST | awk '{if($1=="aaf_pool") {print $2}}' > $(pwd)/pool_uuid/rrp_cpus_list.txt
        # *********************************** #
        for i in $(seq 1 $NUM_RECORDS); do
		 cat $(pwd)/pool_uuid/list_info.txt | awk '{if($10=="aaf_pool") {print $7}}' >$UUID_INFO
	done
	if [ $? -eq 0 ]; then
		echo " UUIDs of domain(s) and cpupool ID bought into simulation file"
		gcc $(pwd)/aaf_new_single_resource.c -lxenctrl -lm -luuid -o aaf -L /usr/local/lib 2>/dev/null
		if [ $? -eq 0 ]; then
			echo "aaf schedul entries are set successfully!!"
			echo "----proceeding with execution---"
			./aaf
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
