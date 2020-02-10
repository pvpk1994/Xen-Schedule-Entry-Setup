# Xen-Schedule-Entry-Setup
Schedule Entry Configuration Settings for ARINC653 and RRP

# All the pre_*.sh files automate the process of setting up physical, volume group and logical volumes for All the guest domains
# All the uuid_*aaf.sh files automate the process of prepping RRP and send it to the Hypervisor space and make guest domains ready for execution
# All the uuid_*arinc.sh files correspondingly automate the process of prepping guest schedule entries for execution based on the 
schedule sent from Dom-0 userspace to the kernel space.
