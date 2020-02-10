// arinc653_sample.c
// arinc653_schedule_set
// to run this program, do the following:
// gcc arinc653_sample.c -lxenctrl -luuid

#include <stdio.h>
#include <xenctrl.h>
#include <uuid/uuid.h>

int main()
{
        struct xen_sysctl_arinc653_schedule sched;
        xc_interface *xci = xc_interface_open(NULL, NULL, 0);
        int i,j;

        /* initialize major frame and number of minor frames */
        sched.major_frame = 0;
        sched.num_sched_entries = 2;

        uuid_parse("1b1d800f-2f8d-4bbd-9237-058b9be6b179", sched.sched_entries[0].dom_handle);
        sched.sched_entries[0].vcpu_id = 0;
        sched.sched_entries[0].runtime = 10;
        sched.major_frame += sched.sched_entries[0].runtime;
        // hardcoded UUID of the sample program on the internet,
        // change it to the UUID of domU when PS AAF RRP scheduler is to be run
       
         // PArt below is  being commented for testing purposes.... 
       /* 
         uuid_parse("39caaf80-37b6-4a73-8cdb-b21fad5476f3",sched.sched_entries[1].dom_handle);
        sched.sched_entries[1].vcpu_id = 0;
        sched.sched_entries[1].runtime = 10;
        sched.major_frame += sched.sched_entries[1].runtime;
        */
       // i = xc_sched_arinc653_schedule_set(xci, 2, &sched);
        // Testing purposes -- Pavan!
        // convert poolid to poolname to see whats turning up ....
	uint32_t cpupool_id = 2;
	xc_cpupoolinfo_t *pool_aaf;
	
        pool_aaf = xc_cpupool_getinfo(xci,cpupool_id);
	printf("Scheduler_ID is:%d\n",pool_aaf->sched_id);
	printf("CPUPool ID is:%d\n",pool_aaf->cpupool_id);
        printf("Domain ID is:%d\n",pool_aaf->n_dom);
         
        i = xc_sched_arinc653_schedule_set(xci,pool_aaf->cpupool_id, &sched);
	
	if (i)
        {
                printf("true\n");
        } else {
                printf("false\n");
        }

for(;;)

/*
       j = xc_sched_arinc653_schedule_get(xci,pool_aaf->cpupool_id, &sched);

       if (j)
	{
		printf("True\n");
	}	
      else {
		printf("False\n");
        }
*/
        return 0;
}
