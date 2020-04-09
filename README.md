Xen Schedule Entries Setup Automation (RRP-XEN Versions 1 and 2 included):
--------------------------------------------------------------------------
Note: harcoded path for this setting lies in all of .sh files, do change those to your local file destinations.

1. Firstly, setup the desired Physical Volumes (PVs), Volume Groups (VGs) and Logical volumes (LVs) by executing any of pre_dom*.sh files based on the requirements. 

2. After setting up the desired logical volumes, remove CPUs 1 and 3 from Pool-0 and navigate to /etc/xen to run cpupool_rrp.cfg file to automate the process of RRP-Scheduler Operated cpupool with inclusion of cpus 1 and 3.

3. After successfull creation of cpupool, now proceed with creation of DomUs using xl create dom_x.cfg pool=\"aaf_pool\"

4. Navigate back to /home/* directory where the resident uuid_* files reside.

5. For RRP-Xen V2.0, the shell script needs to invoke rrp_sched_entry_mc.c for multi core implementation and for RRP-Xen V1.0 the shell script accordingly has to invoke rrp_single_resource.c which includes single-cpu implementation.
   NOTE: For single core, only 1 cpu needs to be setup in the RRP-Scheduler operated cpupool 

6. run ./uuid_automation.sh script which initially sets up the DomUs in a blocked state ready for the next stage, building process.

----------------------------------------------------------------------------------------------------------------------------

Case Study of Schedule Entry Setup for RRP-Xen 2.0 Multi-Core Scheduler:
------------------------------------------------------------------------
Case Study on Domains/Schedule Entries with following Configurations: {DomU4 af: 1/7, DomU5 af: 2/7, DomU6 af: 3/7, DomU7: 4/5}

CPU-ID -- DOMAIN-ID MAP
--------------------------
  1    --  7,
  3    --  4,
  3    --  5,
  3    --  6
------------------------
-> we know 4/5 is going to take up a dedicated CPU, which is 1 here. Therefore navigate to DomuX.cfg that takes this setting and add the following: cpus="1" #implies DomuX_0 is bound to only run on PCPU #1

-> For other 3 domains with af: (1,2,3)/7, we are sure they are going to run on CPU #3. Do the following for these 3 domuX as well cpus="3" #for {DomuX_1,DomuX_2,DomuX_3}.cfg

xl command execution sequences for these Domain Config Settings:
----------------------------------------------------------------
Follow this execution order after accomplishing the above steps:
-> xl cpupool-create for aaf_pool
-> xl cpupool-cpu-remove Pool-0 3
-> xl cpupool-cpu-add aaf_pool 3
-> cd /etc/xen
-> xl create {domuX_1, domuX_2, domuX_3}.cfg for aaf_pool
-> xl cpupool-cpu-remove Pool-0 1
-> xl cpupool-cpu-add aaf_pool 1
-> cd /etc/xen
-> xl create domuX_0.cfg for aaf_pool
-> Now uncomment the schedule_set fn in rrp schedule entry simulator and run the shell script to set all the domains into blocked state

Desired Result:
--------------
Domu4   - Blocked (b)
Domu5   - Blocked (b)
Domu6   - Blocked (b)
Domu7   - Blocked (b)
