Xen Schedule Entries Setup Automation (RRP-XEN Versions 1 and 2 included):
--------------------------------------------------------------------------
Note: harcoded path for this setting lies in all of .sh files, do change those to your local file destinations.

1. Firstly, setup the desired Physical Volumes (PVs), Volume Groups (VGs) and Logical volumes (LVs) by executing any of pre_dom*.sh files based on the requirements. 

2. After setting up the desired logical volumes, remove CPUs 1 and 3 from Pool-0 and navigate to /etc/xen to run cpupool_rrp.cfg file to automate the process of RRP-Scheduler Operated cpupool with inclusion of cpus 1 and 3.

3. After successfull creation of cpupool, now proceed with creation of DomUs using xl create dom_x.cfg pool=\"aaf_pool\"

4. Navigate back to /home/* directory where the resident uuid_* files reside.

5. For RRP-Xen V2.0 the shell script needs to invoke rrp_multi_resource.c for multi core implementation and for RRP-Xen V1.0 the shell script accordingly has to invoke rrp_single_resource.c which includes single-cpu implementation.
   NOTE: For single core, only 1 cpu needs to be setup in the RRP-Scheduler operated cpupool 

6. run ./uuid_automation.sh script which initially sets up the DomUs in a blocked state ready for the next stage, building process.
