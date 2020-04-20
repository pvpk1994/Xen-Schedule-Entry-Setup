/********** EDF ARINC SCHEDULE ENTRY SIMULATOR ********
 * Author: Pavan Kumar Paluri
 * Developed @ RT-LAB @ UNIVERSITY OF HOUSTON
 * Month/Year: Apr/2020
 * ****************************************************/

/************ ASSUMPTIONS ************
 Update: This Schedule entry simulator assumes partitions to follow implicit-deadline nature
          i.e., period_of_partition_i == deadline_of_partition_i
 Update: All Domains are assumed to have arrival times to be 0 for simplicity.
 Update: There will not be more than 1000 processes (For simplicity)
 **************************************/

// Last Updated on Apr-20,2020
// TODO: In Xen Compliant ARINC EDF simulator, change partition_id to Domain's UUID
// TODO: function xc_sched_arinc653_schedule_get() still needs to be developed.
// TODO: Timeslice assignment to IDLE Domain/VCPU still under progress...
// [FIXED]: Fixed timeslice assignment to IDLE Domain/VCPu problem as idle vcpu assignment gives an invalid Domain-handler,
//          This invalid domain handler cannot fetch a runnable vcpu in ARINC scheduler, so do_schedule() then picks up
//          a corresponding IDLE VCPU for this time-slice entry. In this way, we resolve the IDLE time-slice issue.


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Hash_Map.h"
// Xen Includes
#include <uuid/uuid.h>
#include <xenctrl.h>

typedef int64_t s_time_t;
#define MILLISECS(_ms)  (((s_time_t)(_ms)) * 1000000UL )
#define DOMN_RUNTIME    MILLISECS(30) // Default runtime of 30ms timeslice
#define UUID_READ 1024

struct partition {
    char* id;
    int wcet;
    int period; // period == deadline
};

// Conditionals
#define PRINT_ENABLE
#define PRINT_ENABLE_NI

// Function Declarations
char** edf_schedule(struct partition*, int, int);
int poolid(FILE*);
char** uu_id(FILE *);
void rrp_valid_cpus(FILE *);


// Global Variables
static int arinc_cpus[10]; // Assumption: Assumes only there are MAXiMUM of 10 cpus in Xen operating under ARINC
static int num_minor_frames;
static int cpu_counter;
char **uuid;
char **uuid_new;

/******** HELPER FUNCTIONS ***********/
int raise_error(double result)
{
    printf("Schedulability is: %f >= 1.00\n", result);
    printf("Exiting the program ....\n");
    return EXIT_FAILURE;
}

int lcm(int num1, int num2)
{
    int minMultiple;
    minMultiple = (num1 > num2) ? num1 : num2;
    while (1)
    {
        if(minMultiple%num1 == 0 && minMultiple%num2 == 0)
            break;
        ++minMultiple;
    }
    return minMultiple;
}

int cal_hp(struct partition *sched_entry, int total_entries)
{
    int final_val = 1;
    for(int i=0; i< total_entries; i++)
        final_val = lcm(final_val, sched_entry[i].period);
    return final_val;
}
/*********** END OF HELPER FNS ******************/

int main()
{
    FILE *file;
    int pool_id = poolid(file);
    uuid = uu_id(file);
//    rrp_valid_cpus(file);
    char **result;
    int number_of_partitions, hyperperiod_res;
    double sched_result = 0.0;
    number_of_partitions = num_minor_frames;
    struct xen_sysctl_scheduler_op ops;
    struct xen_sysctl_arinc653_schedule sched;
    xc_interface *xci = xc_interface_open(NULL, NULL, 0);
    struct partition *partitions;
    partitions = (struct partition*) malloc(number_of_partitions * sizeof(struct partition));
    for(int i =0; i <number_of_partitions; i++)
    {
        // Partition_ID loader
        printf("Enter the wcet of Partition %d: ", i);
        scanf("%d", &partitions[i].wcet);

        printf("Enter the period of Partition %d: ", i);
        scanf("%d", &partitions[i].period);

        partitions[i].id = malloc(UUID_READ);
        strncpy(partitions[i].id, uuid[i], UUID_READ);
    }

    // Print
#ifdef PRINT_ENABLE
for(int i=0; i < number_of_partitions; i++)
    {
        printf("Partition %s:  wcet: %d  period: %d\n", partitions[i].id, partitions[i].wcet, partitions[i].period);
    }
#endif

    // necessary and sufficient schedulability test
    // if Sum of (c_i/p_i) <= 1: then schedulable, else No
    for(int i=0; i < number_of_partitions; i++)
    {
        sched_result += (double)partitions[i].wcet/ partitions[i].period;
    }

    sched_result <= 1 ? printf("Schedulable and Utilization is: %f \n", sched_result) : raise_error(sched_result);

    // Calculate Hyper-period for the schedule
    hyperperiod_res = cal_hp(partitions, number_of_partitions);

    #ifdef PRINT_ENABLE
    printf("Hyperperiod of the schedule is: %d\n", hyperperiod_res);
#endif

    // Compute EDF schedule
    result = edf_schedule(partitions, number_of_partitions, hyperperiod_res);

    /* Xen Env Zone Begins... */
    sched.num_sched_entries = hyperperiod_res;
    sched.major_frame = hyperperiod_res*DOMN_RUNTIME;
    for(int i=0; i<sched.num_sched_entries; ++i)
    {
        int uuid;
        //if(!strcmp(result[i], "NONE"))
            uuid = uuid_parse(result[i], sched.sched_entries[i].dom_handle);
#ifdef PRINT_ENABLE
        printf("Domain Handle Copied: %X\n", *sched.sched_entries[i].dom_handle);
        printf("uuid_parse return val: %d\n", uuid);
#endif
        sched.sched_entries[i].vcpu_id = 0;
        sched.sched_entries[i].runtime = DOMN_RUNTIME;
    }
    int set_result;
    set_result = xc_sched_arinc653_schedule_set(xci, pool_id, &sched);
#ifdef PRINT_ENABLE
    printf("CPU Pool ID is: %d\n", pool_id);
    printf("Result of set is: %d\n", set_result);
    printf("num schedule entries: %d\n", sched.num_sched_entries);
#endif



    return 0;
}

char** edf_schedule(struct partition* domain, int num_of_domains, int hp)
{
    int earliest_ddl;
    int next_ddl[1000];
    int rem_wcet[1000];
    int period[1000];
    int sched_table[hp];
    int earliest_ddl_index;
    int process_period[1000];
    char **partition_list = (char**)malloc(hp * sizeof(char*));
    for(int i= 0; i< hp; i++)
        partition_list[i] = (char*)malloc(1024* sizeof(char));


    for(int i=0; i<num_of_domains; i++)
    {
        next_ddl[i] = domain[i].period;
        rem_wcet[i] = domain[i].wcet;
        period[i] = next_ddl[i];
        // Period Container
        process_period[i] = 0;
    }
#ifdef PRINT_ENABLE
    printf("\n*******************************\n");
    printf("P-1: Represents IDLE Domain/VCPU\n");
    printf("********************************\n");
#endif
    // Iterate through each time-slice..
    for(int i=0; i<hp; i++)
    {
#ifdef PRINT_ENABLE
        printf("TS: [%d to %d] - ", i, i+1);
#endif
        // Set init val of earliest_ddl to be the hp
        earliest_ddl = hp;
        earliest_ddl_index = -1;
        // Which process to choose ??
        for(int j=0; j<num_of_domains; j++)
        {
        if(rem_wcet[j] > 0)
            {
                if(earliest_ddl >= next_ddl[j])
                {
#undef PRINT_ENABLE_NI
#ifdef PRINT_ENABLE_NI
                    printf("earliest_ddl: %d\n", earliest_ddl);
#endif
                    // Set earliest ddl to be this process's deadline.
                    earliest_ddl = next_ddl[j];
                    earliest_ddl_index = j;

                }
            }
        }
#ifdef PRINT_ENABLE
        printf(" P%d \n", earliest_ddl_index);
#endif
        if(earliest_ddl_index != -1)
	{
            strcpy(partition_list[i], domain[earliest_ddl_index].id);
        }
        // reduce the wcet of that partition by 1 time unit now..
        else {
            strcpy(partition_list[i], "NONE");
	  //  printf("partition_list[%d]: %s\n", i, partition_list[i]);
        }
        rem_wcet[earliest_ddl_index]--;

        for(int j=0; j<num_of_domains; j++)
        {
            if(process_period[j] == period[j]-1)
            {
                // Resetting phase...
                next_ddl[j] = domain[j].period;
                rem_wcet[j] = domain[j].wcet;
                period[j] = next_ddl[j];
                process_period[j] = 0;
            }
            else{
            if(next_ddl[j] > 0)
                next_ddl[j]--;
            else {
                if (rem_wcet[j] > 0)
                    printf("\n the Process %d has exceeded the deadline \n", j);
            }
            process_period[j]++;
            }
        }
    }
    return partition_list;
}

/****************** Xen Domain Information Extraction **************/
/***************** Helper Functions ******************/
int poolid(FILE *filer)
{
    int pool_id;
    filer = fopen("/home/rtlabuser/pool_uuid/pool_id.txt", "r");
    if(!filer)
        perror("fopen(pool_id.txt");
    fscanf(filer, "%d", &pool_id);
    printf("Reading... cpupool ID is: %d\n", pool_id);
    fclose(filer);
    return pool_id;
}

char** uu_id(FILE* filer)
{
    filer = fopen("/home/rtlabuser/pool_uuid/uuid_info.txt", "r");
    if(filer == NULL)
        perror("fopen(uuid_info.txt)\n");
    int i =0;
    char **buffer = malloc(128* sizeof(char*));
    int k;
    for(k=0; k<128; k++)
        buffer[k] = malloc(128* sizeof(char));
    while(fscanf(filer, "%s", buffer[i])== 1)
    {
        printf("Reading .... uuid of Domain is: %s\n", buffer[i]);
        i++;
    }
    printf("Number of Entries are: %d\n", i);
    fclose(filer);
    num_minor_frames = i;
    return buffer;
}

void rrp_valid_cpus(FILE *filer)
{
    // int rrp_cpus[10];
    int x=0;
    filer = fopen("/home/rtlabuser/pool_uuid/rrp_cpus_list.txt", "r");
    if(filer == NULL)
    {
        perror("fopen(rrp_cpus_list.txt)\n");
    }

//    int cpu_counter = 0;
    const char s[2] = ",";
    char *token;
    int i;
    if(filer != NULL)
    {
        char line[20];
        while(fgets(line, sizeof(line), filer)!= NULL)
        {
            token = strtok(line, s);
            arinc_cpus[cpu_counter] = atoi(token);
            cpu_counter += 1;
            for(x=0; x<10; x++)
            {
                printf("%s\n", token);
                token = strtok(NULL, s);
                if(token != NULL)
                {
                    // valid strings
                    arinc_cpus[cpu_counter] = atoi(token);
                    ++cpu_counter;
                }
                else{
                    break;
                }
                // printf("%s\n", line);
            }
        }
    }
    printf("Total Number of CPUs are: %d\n", cpu_counter);
    for(int c=0 ; c<cpu_counter; c++)
    {
        printf("CPU[%d] #: %d\n",c, arinc_cpus[c]);
    }
    fclose(filer);
}


