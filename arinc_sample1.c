   #include <xenctrl.h>
   #include <uuid/uuid.h>
   
void main()
{
  struct xen_sysctl_arinc653_schedule sched;
  xc_interface *xci = xc_interface_open(NULL, NULL, 0);
  int i;

    /* initialize major frame and number of minor frames */
  sched.major_frame = 0;
  sched.num_sched_entries = NUM_MINOR_FRAMES;

 for (i = 0, i < sched.num_sched_entries; ++i)
  {
        /* identify domain by UUID */
        uuid_parse(DOMN_UUID_STRING, sched.sched_entries[i].dom_handle);

        sched.sched_entries[i].vcpu_id = DOMN_VCPUID;

        /* runtime in ms */
        sched.sched_entries[i].runtime = DOMN_RUNTIME;

        /* updated major frame time */
        sched.major_frame += sched.sched_entries[i].runtime;
    }

    xc_sched_arinc653_schedule_set(xci, SCHED_POOLID, &sched);
}
