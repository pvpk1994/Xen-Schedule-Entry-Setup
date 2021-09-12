/*************************************************
 Name: ARINC653 Simulation file for  ARINC653 scheduler
 purpose: Sets the schedule entries for arinc scheduler
 To compile: gcc arinc_sample.c -lxenctrl -luuid -o global (OR) run uuid_automation.sh which 
             includes compilation of this simulation file as well.
 Author: Pavan Kumar Paluri
 Copyright @ RTLABS -2019 - University of Houston 
 *************************************************/
#include <stdio.h>
#include<stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xenctrl.h>
#include <uuid/uuid.h>
#define NUM_MINOR_FRAMES 2
typedef int64_t s_time_t;
#define MILLISECS(_ms)          (((s_time_t)(_ms)) * 1000000UL )
#define MICROSECS(_ms)          (((s_time_t)(_ms)) * 1000UL )
#define DOMN_RUNTIME MILLISECS(30)
//#define DOMN_RUNTIME MILLISECS(0.1)
//#define DOMN_RUNTIME MICROSECS(10)
#define LIBXL_CPUPOOL_POOLID_ANY 0xFFFFFFFF
#define UUID_READ 1024
#define QUANTUM 50000
char **dom_uuid;
static int minor_frames;
//FILE *filer;
int poolid(FILE *filer)
{
  int pool_id;
  filer = fopen("/home/rtlabuser/pool_uuid/pool_id.txt","r");
   if(filer == NULL)
	perror("fopen(pool_id.txt)\n");
  fscanf(filer,"%d",&pool_id);
  printf("Reading.. cpupool id is:%d\n",pool_id);
  fclose(filer);
   return pool_id;
}

char **uu_id(FILE *filer)
{
  filer = fopen("/home/rtlabuser/pool_uuid/uuid_info.txt","r");
  if(filer == NULL)
	perror("fopen(uuid_info.txt");
  int i=0;
  char **buffer = malloc(128*sizeof(char *));
  int k;
   for(k=0;k<128;k++)
	buffer[k] = malloc(128*sizeof(char));
  
 while(fscanf(filer,"%s",buffer[i])==1)
 {
  printf("Reading.. uuid of domain is: %s\n",buffer[i]);
  i++;
 }
 printf("Number of entries are:%d\n",i);
  fclose(filer);
  minor_frames = i;
  return buffer;
}

//char dom_uuid[40]="49f86d53-3298-43a6-b8e6-a5aec2d4d3c2"; 
int main()
{
    FILE *filer;
   FILE *filer1;
   int pool_id;
    pool_id = poolid(filer1);
    dom_uuid = uu_id(filer);
    struct xen_sysctl_scheduler_op ops;
    struct xen_sysctl_arinc653_schedule sched;
    xc_interface *xci = xc_interface_open(NULL, NULL, 0);
    int i;
    int result, result_get ;
    /* Init all the wcets and periods with 0s */
    int *wcet=(int*)malloc(minor_frames*sizeof(int));
   /***************************************/

    sched.major_frame = 0;
    sched.num_sched_entries = minor_frames;
    for (i = 0; i < sched.num_sched_entries; ++i)
    {
        //strncpy((char *)sched.sched_entries[i].dom_handle,dom_uuid[i],sizeof(dom_uuid[i]));
        int uuid =  uuid_parse(dom_uuid[i], sched.sched_entries[i].dom_handle);
       printf("Size of Domain: %ld\n",sizeof(sched.sched_entries[1].dom_handle));
	printf("DomUUID return:%d\n",uuid);
	// enabling only single vcpu to run 
       sched.sched_entries[i].vcpu_id = 0;
 
       // sched.sched_entries[0].runtime = 50000; 
       //sched.sched_entries[0].runtime = 50000;
     //   sched.major_frame += sched.sched_entries[i].runtime;
    }

     
     for(int k =0; k<sched.num_sched_entries; k++)
     {
        printf("Enter the WCET for schedule entry: %d :",k+1);
	scanf("%llu",&wcet[k]);
	sched.sched_entries[k].runtime = DOMN_RUNTIME*wcet[k];
     }

      long long  hp;
      printf("Enter the Major Frame of all schedule entries put together::");
      scanf("%llu",&hp);
      sched.major_frame = DOMN_RUNTIME*hp;
 

    result = xc_sched_arinc653_schedule_set(xci,pool_id, &sched);
    printf("The cpupoolid is: %d\n",pool_id);
   printf("The result of set is %d\n",result);
//   sleep(50000000);
   // result_get = xc_sched_arinc653_schedule_get(xci,pool_id,&sched);
    printf("The number of entries is %d\n",sched.num_sched_entries);
    for(i = 0;i < sched.num_sched_entries;i++)
    {
	printf("=====%d======\n",i);
	printf("The major_frame is %llu\n",sched.major_frame);
	printf("The dom_handle is %X\n",*sched.sched_entries[i].dom_handle);
	printf("The vcpuid is %d\n",sched.sched_entries[i].vcpu_id);
	printf("The minor_frame runtime is %llu\n",sched.sched_entries[i].runtime);
	printf("==============\n");
    } 

     result_get = xc_sched_arinc653_schedule_get(xci,pool_id,&sched);
     printf("Status of xc_get:%d\n",result_get);
   return 0;
}

