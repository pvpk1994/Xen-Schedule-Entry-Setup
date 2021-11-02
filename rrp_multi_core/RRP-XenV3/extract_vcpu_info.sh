#!/bin/bash
# *********************************
# Multi-VCPU per Domain Information
# *********************************
# Author: Pavan Kumar Paluri
# Shell script that extracts Multiple VCPUs per DomU information for aaf_pool controlled CPUs
# Root Access only, libxl functions do not work otherwise.

#DEF_DIR=/home/rtlabuser
DEF_DIR=$1
UUID_INFO=$DEF_DIR/uuid_info.txt
VCPU_INFO=$DEF_DIR/vcpu_info.csv

if [ $(whoami) != "root" ]; then
	echo "only root access!!"
	exit 1
else
	xl list -c -v > $DEF_DIR/list_info.txt
	NUM_DOMS=$(cat $DEF_DIR/list_info.txt | awk 'END{print NR}')
	echo "Number of Domains: $NUM_DOMS"
	# Iterate through the list of Domains to find the ones belonging to aaf_pool
	for i in $(seq 1 $NUM_DOMS); do
		cat $DEF_DIR/list_info.txt | awk '{if($10=="aaf_pool") {print $7}}' >$UUID_INFO
	done

	# UUID_INFO has all info about DomUs belonging to aaf_pool
	xl vcpu-list > $VCPU_INFO
	NUM_RRP_DOMS=$(cat $UUID_INFO | awk 'END{print NR}')
	echo "Number of RRP Domains: $NUM_RRP_DOMS"
	# cat $DEF_DIR/pool_uuid/list_info.txt | awk '{if($10=="aaf_pool") {print $2, $7}}'
	# Store domIDs of RRP-pool in bash array
	dom_id=($(cat $DEF_DIR/list_info.txt | awk '{if($10=="aaf_pool") {print $2,$7}}'))

	declare -A map
	while IFS=" " read -r fir sec thi fou fif six sev eig nin ten ele; do
		if [[ "$ten" = "aaf_pool" ]]; then
			echo "aaf_pool is read"
			map["$sec"]="$sev"
		fi
	done < $DEF_DIR/list_info.txt

	# Print the array for confirmation
	for key in ${!map[@]}; do
		echo ${key} ${map[${key}]}
	done

	# Iterate through the xl vcpu-list to gather VCPU-PCPU info for each RRP-operated DomU
	# cat $VCPU_INFO | awk ' {{ print $2,$3,$4}}' > $DEF_DIR/pool_uuid/dom_vcpu_cpu.txt
	echo "Name,Domain ID,Domain UUID,VCPU,CPU" >> $DEF_DIR/dom_vcpu_cpu.csv
	while IFS=" " read -r first second third fourth fifth sixth seventh; do
		# echo "First field: $first, second field: $second, third field: $third";
		if [[ " ${dom_id[*]} " =~ " ${second} " ]]; then
			echo "$first,$second,${map[${second}]},$third,$fourth" >> $DEF_DIR/dom_vcpu_cpu.csv;

		fi
	done < $VCPU_INFO
fi
