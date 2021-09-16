# RRP-Xen userspace tools V3.0

## Features
- __MulZ\_ILO__ implemented
- Configuration files taken as command line arguments
- Configuration file for each domain taken as inputs, specify them based on the uuid of the domain
- sched_set and sched_get invokes not added yet

## Usage

```
g++ RRP-schedule.cpp -std=c++11 -o sched
./sched ./config_files/pool_id.txt ./config_files/rrp_cpus_list.txt ./config_files/uuid_info.txt 
```

Input the VCPU information using JSON files when asked.
The format of the JSON file should be the same as that in *config_files/sample.json*
