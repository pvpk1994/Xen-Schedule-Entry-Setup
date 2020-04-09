/* RRP MULTI CORE Schedule Generator (MulZ)
 * Author:: Pavan Kumar Paluri, Guangli Dai
 * Copyright 2019-2020 - RTLAB UNIVERSITY OF HOUSTON */
// Created by Pavan Kumar  Paluri
// Year: 2020
// Month: Feb-Mar
// Last Updated: April-9, 2020

/* ********** UPDATE *************
 * Runnable Version: Xen_Supported Schedule Entries file for RRP_MULZ_Multi_Core residing in Xen hypervisor
 * Otherwise working perfectly alright!
 * ******************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <assert.h>
// Customized Includes
#include "Hash_Map.h"
#include "Vector.h"

// Xen Includes
#include <xenctrl.h>
#include <xen/sysctl.h>
#include <uuid/uuid.h>


// --------- ifdef Zone -------------
#define PRINT_ENABLE
#define RRP_XEN_VERSION_TWO
#define RRP_SCHED_SET_ENABLE
#define RRP_SCHED_GET_ENABLE
// ---------------------------------


#define NUM_ENTRIES (num_minor_frames)
/* TODO: Automation of RRP_PCPUS pending... */
#define RRP_PCPUS (cpu_counter)


/* ********** XEN #DEFINEs ************** */
// #define NUM_MINOR_FRAMES 2
typedef int64_t s_time_t;
#define MICROSECS(_us)    ((s_time_t)((_us) * 1000ULL))
// #define MILLISECS(_ms)          (((s_time_t)(_ms)) * 1000000UL )
#define MILLISECS(_ms)  ((s_time_t)((_ms) * 1000000ULL))
#define DOMN_RUNTIME(_runtime) (_runtime*MICROSECS(1000)) // Example: DOMN_RUNTIME(30) ~30ms ~ 30*pow(10,3)ns
#define DOMN_RUNTIME_US (DOMN_RUNTIME*MICROSECS(1000))
#define TIME_SLICE_LENGTH MILLISECS(30)
#define LIBXL_CPUPOOL_POOLID_ANY 0xFFFFFFFF
#define UUID_READ 1024
/* *************************************** */


// Global Inits
static int arr[SIZE]; // init A_val with number of schedule entries
int counter;
char **uuid;
char **uuid_new;
static int num_minor_frames;
static int rrp_cpus[10]; // Assumption: Only assumes to have at the Max 10 PCPUs in aaf_pool
static int cpu_counter;

// Structure Definitions
typedef struct {
    // int id;
    char* id;
    int wcet; // getAAF_numerator
    int period; // getAAF_denominator
    int index; // Need an index since id represents char*
}sched_entry_t;

// Maintain a single linked list of timeslices
struct node{
    int ts;
    // int id;
    char* id;
    struct node *next;
};

/* ******* MULZ Modifications *******
 * factor: Keep track of factors per-CPU
 * rest: Keep track of rest(s) per-CPU
 * **********************************/
struct pcpu{
    int cpu_id;
    double factor;
    double rest;
};

/* ********* Partition-CPU Map *******
 * @param: partition_ID: Sched_entry_ID
 * @param: CPU ID
 * ***********************************/

// -------------------------------------------------------------------
//                       Function Prototypes
// --------------------------------------------------------------------
void swap(int,int);
int lcm(int, int);
int hyper_period(sched_entry_t *);
sched_entry_t* dom_comp(sched_entry_t *);
sched_entry_t* dom_af_comp(sched_entry_t *);
struct node* load_timeslices(struct node*,int );
void getA_calc(sched_entry_t *, int, int);
struct node *partition_single(sched_entry_t *partitions, int hp, struct node* avail, int count);
struct pcpu* load_pcpus(struct pcpu*);
bool Mul_Z(sched_entry_t*, struct pcpu*, int);
int MulZ_FFD_Alloc(int *, int *, struct pcpu*);
Vector z_approx(double, int);
// ------ MERGE SORT FUNCTION PROTOTYPES ---------
void Half_Split(struct node*, struct node**, struct node**);
struct node* Sorted_Merge(struct node*, struct node*);
struct node* Merge_Sort(struct node**);
// ------------------------------------------------
// ------------------------- DOMAIN-0 FN PROTOTYPES -------------
int poolid(FILE *);
char **uu_id(FILE *);
void rrp_valid_cpus(FILE *);
// ------------------------------------------------------------------------

// Append time-slice at the end of the ts list
void append(struct node** header, int ts, char* id)
{
    struct node* new_node = (struct node*)malloc(sizeof(struct node));
    struct node *last = *header;

    // Assignment(s)
    new_node->ts = ts;
    new_node->id = id;
   // printf("time slice added:%d\n",new_node->ts);
    // Make next of this new_node to NULL since its being appended at the end of the list
    new_node->next=NULL;

    // If list is empty, make the head as last node
    if(*header == NULL)
    {
        *header = new_node;
       // printf("Head's timeslice:%d\n",(*header)->ts);
        return;
    }
    //If list is not empty, traverse the entire list to append the ts at the end
    while(last->next !=NULL)
        last = last->next;
    last->next = new_node;

}

// Print contents of linked list
void print_list(struct node* noder)
{
    while(noder!= NULL)
    {
        printf("timeslice:%d -> DomID:%s\n",noder->ts, noder->id);
    //  printf("DomID:%d\n",noder->id);
        noder = noder->next;
    }
    printf("\n");
}

// Get the dynamic length of the list of timeslices
int list_length(struct node *header)
{
    int counter=0;
    while(header != NULL)
    {
        counter++;
        header = header->next;
    }
    return counter;
}


// Remove a certain node from the list given the node
void delete_entry(struct node **head, int position)
{
    // If linked list is empty
    if(*head == NULL)
        return;

    struct node* temp = *head;

    //If only a single entry in the list
    if(position == 0)
    {
        *head = temp->next;
        free(temp);
        return;
    }

    for(int i=0; temp!=NULL && i<position-1;i++)
    {
        temp = temp->next;
    }

    if(temp == NULL || temp->next == NULL)
        return;

    struct node *next = temp->next->next;

    free(temp->next);

    temp->next = next;

}

// Search an element by its value to check its presence
/* Checks whether the value x is present in linked list */
bool search(struct node* head, int x)
{
    struct node* current = head;  // Initialize current
    while (current != NULL)
    {
        if (current->ts == x)
            return true;
        current = current->next;
    }
    return false;
}


// get the ith entry of a linked list
int get_i(struct node* head, int index)
{
    struct node* current = head;
    int count = 0; /* the index of the node we're currently
                  looking at */
    while (current != NULL)
    {
        if (count == index)
            return(current->ts);
        count++;
        current = current->next;
    }

    /* if we get to this line, the caller function was asking
       for a non-existent element so we assert fail */
    assert(0);
}

// Copy elements of one linked list into another
struct node *copy(struct node *org, struct node *new)
{
    new = NULL;
    struct node **tail = &new;

    for( ;org; org = org->next) {
        *tail = malloc (sizeof **tail );
        (*tail)->ts = org->ts;
        (*tail)->next = NULL;
        tail = &(*tail)->next;
    }
    return new;
}



/* ********* main function ************
 * This function is the entry of the program.
 * It reads in system data and user input first.
 * It then invokes MulZ to generate a schedule and send the schedule to the kernel.
 * *********************************/
int main()
{
    //retrieve the system information.
    FILE *file;
    int pool_id = poolid(file);
    uuid = uu_id(file);
    rrp_valid_cpus(file);

    //retrieve the user input.
    sched_entry_t partitions[NUM_ENTRIES], *sorted_partitions;
    //sched_entry_t *scheduler;//Tobe delted: never used.
    //int A_val[NUM_ENTRIES];//To be deleted: never used.
    printf("Enter the partitions' WCET:");
    for(int i=0; i<NUM_ENTRIES; i++)
    {
        scanf("%d",&partitions[i].wcet);
    }
    printf("Enter the partitions' Periods:");
    for(int i=0; i<NUM_ENTRIES; i++)
    {
        scanf("%d",&partitions[i].period);
    }

    for(int i=0; i< NUM_ENTRIES; i++)
    {
         partitions[i].id = malloc(UUID_READ);
         strncpy(partitions[i].id, uuid[i], UUID_READ);
         partitions[i].index = i;
       // partitions[i].id = i;
        printf("WCET of partitions[%d] is %d\n ",i, partitions[i].wcet);
        printf("Period of partitions[%d] is %d\n",i, partitions[i].period);
    }

    /* To be deleted: unused.
    int wcet[2] ={13,15};
    printf("Return Val:%d\n",domain_handle_comp(wcet[0],wcet[1]));
    int a = 34;
    int b = 45;
    int *avail_ts;
    swap(a,b);
    */
  //  scheduler = dom_comp(partitions);
  /*
    for(int i=0;i <NUM_ENTRIES;i++) {
        printf(" partitions WCET[%d]= %d\n", i, scheduler[i].wcet);
        printf(" partitions Period[%d]= %d\n", i, scheduler[i].period);
    }
  */
  //  getA_calc(scheduler, hyper_period(scheduler));
   // for( int i=0;i< NUM_ENTRIES;i++)
   //     printf("arr[%d]:%d\n",i,arr[i]);
 //   struct node *head = NULL;
 //   struct node *head_1, *head_2;
  //  head_1 = load_timeslices(head, hyper_period(scheduler));
    //print_list(load_timeslices(head, hyper_period(scheduler)));
    // Returns an unsorted timeslice-domain Pair
    // Need to sort it based on the timeslices....
 //   head_2 = partition_single(scheduler, hyper_period(scheduler), head_1);


    // ---------------- MULZ ZONE -------------------
    struct pcpu* pcpu_t;

    // Allocate mem for RRP_PCPUs count, the current version assumes the number of pcpu is less than 10.
    pcpu_t = (struct pcpu*)calloc(10, sizeof(struct pcpu));

    // Load PCPUs into pcpu_t
    pcpu_t = load_pcpus(pcpu_t);
    // Priority Q for Schedule Entries ( Sorted in Non-Increasing Order)
    sorted_partitions = dom_af_comp(partitions);

    for(int i=0;i <NUM_ENTRIES;i++) {
        printf(" Partition[%d] has AF: %f\n", i, (double)sorted_partitions[i].wcet/sorted_partitions[i].period);

        if((double)sorted_partitions[i].wcet/sorted_partitions[i].period);
    }
    // Time to Invoke Mul-Z
    Mul_Z(sorted_partitions, pcpu_t, pool_id);

    // Deleting an entry
  //  head =delete_entry(head_1,0);
    return 0;
}

int domain_handle_comp(int wcet1, int wcet2)
{
    return wcet1 > wcet2 ? 0 : 1;
}

int avail_factor_cmp(double domain_a, double domain_b)
{
    // Sort domains in Descending (Non-Increasing order)
    // If 0: perform swap since second param > first param
    // If 1: No swap as first param > second param
    return domain_b > domain_a ? 0 : 1;
}
void swap(int a, int b)
{
    int temp;
    temp = a;
    a = b;
    b = temp;
 //   printf(" Value of a and b are: %d %d respectively\n", a,b);

}

sched_entry_t *dom_comp(sched_entry_t sched[])
{
    int i,j;
    for(i=0;i<NUM_ENTRIES;i++) {
        for (j = i + 1; j < NUM_ENTRIES; j++) {
            int k;
            k = domain_handle_comp(sched[i].wcet, sched[j].wcet);
            if (k != 0) {
               // swap(sched[i].wcet, sched[j].wcet);
               int temp,temp1,temp2;
               temp = sched[i].wcet;
               sched[i].wcet = sched[j].wcet;
               sched[j].wcet = temp;
                //return sched;
                temp1 = sched[i].period;
                sched[i].period = sched[j].period;
                sched[j].period = temp1;

                temp2 = sched[i].id;
                sched[i].id = sched[j].id;
                sched[j].id = temp2;
            }
        }
    }
    return sched;
}

/* ********* dom_af_comp ************
 * @param: sched_entry_t sched[]: An array of partitions
 * @return: Returns an array of sched_entry_t, 
 *          which is sorted by availability factor in non-increasing order.
 * *********************************/
sched_entry_t *dom_af_comp(sched_entry_t sched[])
{
    int i,j;
    for(i=0;i<NUM_ENTRIES;i++) {
        for (j = i + 1; j < NUM_ENTRIES; j++) {
            int k;
            k = avail_factor_cmp((double)sched[i].wcet/sched[i].period, (double)sched[j].wcet/sched[j].period);
            if (k == 0) {
                // swap(sched[i].wcet, sched[j].wcet);
                int temp,temp1, temp3;
                char temp2[1024];
                temp = sched[i].wcet;
                sched[i].wcet = sched[j].wcet;
                sched[j].wcet = temp;
                //return sched;
                temp1 = sched[i].period;
                sched[i].period = sched[j].period;
                sched[j].period = temp1;

		/*
        		temp3 = sched[i].index;
        		sched[i].index = sched[j].index;
        		sched[j].index = temp3;
		*/
                /*
                    temp2 = sched[i].id;
                    sched[i].id = sched[j].id;
                    sched[j].id = temp2;
                */
                strcpy(temp2, sched[i].id);
                strcpy(sched[i].id, sched[j].id);
                strcpy(sched[j].id, temp2);
            }
        }
    }
    return sched;
}

int lcm(int num1, int num2)
{
  int minMultiple;

  minMultiple = (num1 > num2) ? num1 : num2;

  while(1)
  {
      if( minMultiple%num1==0 && minMultiple%num2==0 )
      {
        //  printf("The LCM of %d and %d is %d.\n", num1, num2,minMultiple);
          break;
      }
      ++minMultiple;
  }
   return minMultiple;
}


int hyper_period(sched_entry_t *sched)
{
    int final_val=1;
    for (int i=0; i< NUM_ENTRIES; i++)
    {

            final_val = lcm(final_val, sched[i].period);
         //   printf("Final Value: %d\n", final_val);

    }
    return final_val;
}

int hyper_period_MulZ(sched_entry_t *sched, int limit)
{
    int final_val = 1;
    for(int i=0; i< limit; i++)
    {
        final_val = lcm(final_val, sched[i].period);
    }
    return final_val;
}

void getA_calc(sched_entry_t *sched, int hp, int count)
{
    int i;


    for(i=0;i< count;i++)
    {
        arr[i] = (int)(hp/sched[i].period)*(sched[i].wcet);
    }
}

struct node* load_timeslices(struct node* head, int hp)
{
    for(int i=1;i<=hp;++i)
        append(&head,i,0);
    return head;
}

/* ********* check_delta ************
 * @param: node* avail_set: A list of time slices available now.
 * @param: int * standard_p: An array representing T(p, q, 0)
 * @param: int wcet: The length of standard_p.
 * @param: int delta: The right shifted value of the standar_p to be tested.
 * @param: int period: The period of the current partition.
 * @return: Returns true if T(p, q, delta) is available in the avail_set,
 *          Otherwise, return false.
 * *********************************/
bool check_delta(struct node* avail_set, int * standard_p, int wcet, int delta, int period)
{
    for(int i=0; i < wcet; i++)
    {
        int t_now = (standard_p[i] + delta)%period+1;
        if(search(avail_set, t_now)==false)
            return false;
    }
    return true;
}

/* ********* find_delta ************
 * @param: node* avail_set: A list of time slices available now.
 * @param: int period: The period of the current partition.
 * @param: int wcet: The wcet of the current partition.
 * @param: int wcet_left: The wcet_left after this partition is allocated.
 * @return: Returns the delta1 so that T(period, wcet, delta1) U T(period, wcet_left, delta2) = avail_set
            and the two partitions do not overlap. If no such delta1 and delta2 can be found, return -1.
 * *********************************/
int find_delta(struct node* avail_set, int period, int wcet, int wcet_left)
{
    //construct standard regular partitions (needs to add 1 because time slice index counts from 1
    //printf("find_delta: %d, %d, %d.\n", period, wcet, wcet_left);
    int *standard_p1 = malloc(sizeof(int)*period);
    for(int i=0; i < wcet; i++)
    {
        standard_p1[i] = (int)(floor(i*period/wcet))%period;
    }
    int *standard_p2 = malloc(sizeof(int)*period);
    for(int i=0; i < wcet_left; i++)
    {
        standard_p2[i] = (int)(floor(i*period/wcet_left))%period;
    }
    for(int delta1=0; delta1 < period; delta1++)
    {
        if(check_delta(avail_set, standard_p1, wcet, delta1, period))
        {
	    //printf("delta1 found: %d.\n", delta1);
            for(int delta2=0; delta2 < period; delta2++)
            {
                if(check_delta(avail_set, standard_p2, wcet_left, delta2, period))
                    return delta1;
            }
        }
    }
    return -1;
}

/* ********* partition_single ************
 * @param: sched_entry_t * partitions: An array of sched_entry_t, stores the partitions' information.
 * @param: int hp: The hyper-period calculated based on partitions' approximate availability factors.
 * @param:  node* avail: Available time slices now, ranges from 0 to hp - 1.
 * @return: returns array of struct pcpus
 * *********************************/
struct node *partition_single(sched_entry_t *partitions, int hp, struct node* avail,int count)
{
    //Initialize the time slice list
    struct node *result = NULL;
    // iterate through sorted schedule entry list (partitions)
    for(int i=0;i< count;i++)
    {
	printf("Handling partition %s.\n", partitions[i].id);
        // Init a new list to record the time slices allocated.
        struct node *occupied_time_index=NULL;
        //allocate time slices based on different wcet of the current partition
        if(partitions[i].wcet!=1)
        {
            //find available delta first
            int avail_length = list_length(avail);
	    //print_list(avail);
	    //printf("wcet now: %d, %d.\n", (int)(avail_length * partitions[i].period/hp), partitions[i].wcet);
            int delta1 = find_delta(avail, partitions[i].period, partitions[i].wcet, (int)(avail_length * partitions[i].period/hp) - partitions[i].wcet);
            if(delta1 == -1)
            {
                printf("Unschedulable partitions with no available delta!\n");
                return NULL;
            }
            //utilize the delta found to allocate time slices to partitions[i]
            for(int l=0; l < hp/partitions[i].period; l++)
            {
                for(int k=0; k < partitions[i].wcet; k++)
                {
                    int index_now = (int)(floor(k*partitions[i].period/partitions[i].wcet) + delta1)
                                    %partitions[i].period + l * partitions[i].period + 1;
                    if(search(avail, index_now)==false)
                    {
                        printf("Time slice %d is allocated redundantly!\n", index_now);
                        return NULL;
                    }
                    append(&occupied_time_index, index_now, 0);
                    append(&result, index_now, partitions[i].id);
                }
            }
        }
        else
        {
	    printf("In else.\n");
            //retrieve the smallest time slice available now
            int index = get_i(avail, 0);
            //check whether time is in feasible range.
            if(index > partitions[i].period)
            {
                printf("Unschedulable partitions with no available time slices!\n");
		printf("Partition failed: %d, %d\n", partitions[i].period, partitions[i].wcet);
                return NULL;
            }
            //allocate time slices to partition[i]
            for(int l= 0;l<hp/partitions[i].period;l++)
            {
                int index_now = index + l * partitions[i].period;
                append(&occupied_time_index, index_now, 0);
                append(&result, index_now, partitions[i].id);
            }
        }
        //update time slices left
        struct node *temp=NULL;
        for(int i=1;i<=hp;i++)
        {
            if(search(avail,i)==true && search(occupied_time_index,i)== false)
                // insert the ith element of linked list with head "Node"
                append(&temp,i,0);
        }
        //print_list(temp);
        avail=copy(temp,avail);
        //print_list(Node);


    }
    //printf("The result from partition_single:\n");
    //print_list(result);
    return result;
}

/* ********* Load PCPUS ************
 * @param: Void
 * @return: returns array of struct pcpus
 * *********************************/
struct pcpu* load_pcpus(struct pcpu *pcpu_t)
{
    // Since memory at other indices are unfilled, Enforce a MEMSET here.

    for(int iter=0; iter<RRP_PCPUS; iter++)
    {
        // Loading begins - Init Phase
        pcpu_t[rrp_cpus[iter]].cpu_id = rrp_cpus[iter];
        // Init factors with 0
        pcpu_t[rrp_cpus[iter]].factor = 0;
        // Init rest with 1
        pcpu_t[rrp_cpus[iter]].rest = 1;
    }
    return pcpu_t;
}

/* ************* MUL-Z ************
 * @param: sched_entry_t* partitions: An array of struct sched_entry_t, which stores the information of partitions.
 * @param: pcpu* pc: An array of struct pcpu.
 * @param: int cpu_pool_id: The id of the current cpu pool.
 * @return: Returns true when schedulable and false otherwise.
 * ********************************/
bool Mul_Z(sched_entry_t* partitions, struct pcpu *pc, int cpu_pool_id)
{
    int cpu_return_id;

    struct xen_sysctl_scheduler_op ops;
    struct xen_sysctl_aaf_schedule sched_aaf[RRP_PCPUS];
    xc_interface *xci = xc_interface_open(NULL, NULL, 0);

   // For Resource Partitions
    for(int i=0; i<NUM_ENTRIES; i++)
    {
        // cpu_return_id =1;
        printf("Partition wcet and period are %d, %d.\n", partitions[i].wcet, partitions[i].period);
        cpu_return_id = MulZ_FFD_Alloc(&partitions[i].wcet, &partitions[i].period, pc);
        printf("New partition wcet and period are %d, %d.\n", partitions[i].wcet, partitions[i].period);
        // For PCPUs

        if(cpu_return_id == -1)
            return false;
        else
        {
            // CPU_ID : Partition_ID
            printf("CPU return ID for partition #%d is: %d\n", partitions[i].index, cpu_return_id);
            insert_key_val(cpu_return_id, partitions[i].index);
        }
    }

    // Execute Partition-Single for each PCPU here
    for(int i=0; i<RRP_PCPUS; i++){
        int set_result= -2, get_result= -2;
        // Maintain a Vector of Partitions..
       // VECTOR_INIT(current_sched_entries);

       // --------------- RRP-XEN DOMAIN-0 INITs ZONE -----------------------
   /*
        struct xen_sysctl_scheduler_op ops;
        struct xen_sysctl_aaf_schedule sched_aaf;
        xc_interface *xci = xc_interface_open(NULL, NULL, 0);
   */
      //-------------------------------------------------------------------
       sched_entry_t RRP_MulZ[SIZE];
       int count = 0; // Make sure count is always less than SIZE
        for(int j=0; j< SIZE; j++)
        {
            if(hashArray[j] != NULL)
            {
                if(hashArray[j]->Key == pc[rrp_cpus[i]].cpu_id)
                {
		    printf("Key and value now are: %d, %d. \n", hashArray[j]->Key, hashArray[j]->Value);
                    // Add to vector current_sched_entries the Partition-ID
                    // VECTOR_ADD(current_sched_entries, &paritions[hashArray[j]->Value]);
                    // Perform Copy of every entity of that instance
		            RRP_MulZ[count].id = malloc(1024);
                    strncpy(RRP_MulZ[count].id, partitions[hashArray[j]->Value].id, 1024);
                    RRP_MulZ[count].wcet = partitions[hashArray[j]->Value].wcet;
                    RRP_MulZ[count].period = partitions[hashArray[j]->Value].period;
        		    RRP_MulZ[count].index = partitions[hashArray[j]->Value].index;
        		    printf("----------RRP_MULZ[%d] INFO --------------------\n",count);
                    printf("RRP_MulZ[%d].id: %s\n", count, RRP_MulZ[count].id);
                    printf("RRP_MulZ[%d].wcet: %d\n", count, RRP_MulZ[count].wcet);
                    printf("RRP_MulZ[%d].period: %d\n", count, RRP_MulZ[count].period);
        		    printf("RRP_MulZ[%d].index: %d\n", count, RRP_MulZ[count].index);
                    printf("RRP_MulZ[%d] lies in CPU %d.\n", count, pc[rrp_cpus[i]].cpu_id);
        		    printf("------------------------------\n");
                    count++;
                }
            }
        }
        printf("Count Val:%d\n", count);
        printf("Partition Single invoked for CPU: %d..\n", pc[rrp_cpus[i]].cpu_id);
        // Invoke Partition-Single for Each PCPU
        /* *************** VALID PER-CPU ZONE ************** */
        if(count != 0) {
            struct node *head = NULL;
            struct node *head_1, *head2;

            head_1 = load_timeslices(head, hyper_period_MulZ(RRP_MulZ, count));
            getA_calc(RRP_MulZ, hyper_period_MulZ(RRP_MulZ, count), count);
            head2 = partition_single(RRP_MulZ, hyper_period_MulZ(RRP_MulZ, count), head_1, count);
            // Sort the time-slices using Merge Sort
            head2 = Merge_Sort(&head2);
            printf("------------------------------\n");
            printf(" Sorted Launch Table for CPU:%d \n", pc[rrp_cpus[i]].cpu_id);
            printf("------------------------------\n");
            print_list(head2);

            // Copy CPU-ID , hyperperiod and num_sched_entries for this LT per-cpu
            /* TODO: Set these PCPU IDs in Xen in a continous range
             * TODO: Example: PCPUs 2-4 {2,3,4} so it will be easy to modify it here.. */

#ifdef RRP_XEN_VERSION_TWO
            sched_aaf[i].cpu_id = pc[rrp_cpus[i]].cpu_id;
            sched_aaf[i].hyperperiod = hyper_period_MulZ(RRP_MulZ, count)*TIME_SLICE_LENGTH;
            sched_aaf[i].num_schedule_entries = list_length(head2);
            printf("CPU ID copied into kernel: %d\n", sched_aaf[i].cpu_id);

            // UUID-Passage
            uuid_new = (char**)calloc(num_minor_frames,sizeof(char*));
    	    for(int i=0;i<num_minor_frames;i++)
    		{
        		uuid_new[i] = (char*)calloc(5,sizeof(char));
    		}

            int k =0, ret_val;
            while(head2 != NULL)
            {
                // char buf[30];
                ret_val = uuid_parse(head2->id, sched_aaf[i].schedule[k].dom_handle);
                printf("Return UUID_PARSE val: %d\n", ret_val);
                if(ret_val < 0)
                {
                    EXIT_FAILURE;
                }
                sched_aaf[i].schedule[k].wcet = TIME_SLICE_LENGTH;
                sched_aaf[i].schedule[k].vcpu_id = 0;
                head2 = head2->next;
                k++;
            }
#ifdef PRINT_ENABLE
            printf("K value: %d\n", k);
#endif
            for(int z=0;z<k;z++)
            {
                //printf("uuid_string:%s\n",uuid_new[z]);
                //uuid_parse(uuid_new[z],sched_aaf.schedule[z].dom_handle);
                printf("Dom_Handle:%X\n",*sched_aaf[i].schedule[z].dom_handle);
                printf("Domain Runtime:%d nanoseconds\n",sched_aaf[i].schedule[z].wcet);
            }

            // Xen Hypercall invocation Zone

            // SET-ZONE
#ifdef RRP_SCHED_SET_ENABLE
            set_result = xc_sched_aaf_schedule_set(xci, cpu_pool_id, &sched_aaf[i]);
	    printf("------------------------------\n");
            printf("Schedule Set for LT PCPU: %d is: %d\n", pc[rrp_cpus[i]].cpu_id, set_result);
	    printf("------------------------------\n");
#endif
            // GET-ZONE
#ifdef RRP_SCHED_GET_ENABLE
            get_result = xc_sched_aaf_schedule_get(xci, cpu_pool_id, &sched_aaf[i]);
#ifdef PRINT_ENABLE
	    printf("---- RRP-SCHED-GET STILL UNDER CONSTRUCTION----  \n");
#endif
	    printf("------------------------------\n");
            printf("Schedule Get for LT PCPU: %d is: %d\n", pc[rrp_cpus[i]].cpu_id, get_result);
	    printf("------------------------------\n");
#endif
#endif
            //  free(RRP_MulZ);
        }
    }
    printf("------------------------------\n");
    printf("CPU-PARTITION MAP\n");
    display();
    printf("------------------------------\n");
    // If here and control hasnt returned false..
    return true;
}

/* *************** MulZ_FFD_Alloc ********
 * @param: int *wcet: The value of wcet of the current partition.
 * @param: int *period: The value of period of the current partition.
 * @param: pcpu_t: An array of struct pcpu, it stores the information of all pcpus.
 * @return: Returns the id of pcpu current partition is allocated to.
 *          If no available pcpu is found, return -1.
 * Credits: Yu Li, Guangli Dai, Pavan Kumar Paluri
 * ***************************************************/
int MulZ_FFD_Alloc(int *wcet, int *period, struct pcpu* pcpu_t) {
    int fixed_list[4] = {3, 4, 5, 7};
    Vector num_frac;
    int x_index = 5;
    double af = (double)(*wcet)/(*period);
    double smallest = INT_MAX;
    int smallest_up = 0, smallest_down = INT_MAX;
    for (int i = 0; i < 4; i++)
    {
        num_frac = z_approx(af, fixed_list[i]);
        int first_elem = (int)vector_get(&num_frac, 0);
        int second_elem = (int)vector_get(&num_frac, 1);
        //printf("num_frac[0]: %d\n", first_elem);
        //printf("num_frac[1]: %d\n", second_elem);
        double num = (double)first_elem/second_elem;
        if(num < smallest)
        {
            smallest = num;
            smallest_up = first_elem;
            smallest_down = second_elem;
            x_index = fixed_list[i];
        }
    }

    double r = smallest;
    #ifdef PRINT_ENABLE
           printf("RRP_PCPUS: %d\n", RRP_PCPUS);
    #endif


    for(int i=0; i<RRP_PCPUS; i++)
    {
        #ifdef PRINT_ENABLE
		printf("CPU_ID in MulZ_alloc: %d\n", pcpu_t[rrp_cpus[i]].cpu_id);
        #endif
        int pcpu_id_now = pcpu_t[rrp_cpus[i]].cpu_id;
        if(pcpu_t[pcpu_id_now].factor == 0)
        {
            //assign this partition to pcpu_id_now
            pcpu_t[pcpu_id_now].factor = x_index;
            pcpu_t[pcpu_id_now].rest = 1-r;
            *wcet = smallest_up;
            *period = smallest_down;
            printf("Final Factor is: %lf\n", r);
            printf("Allocated to %d.\n", pcpu_id_now);
            return pcpu_id_now;
        }
        else
        {
            Vector tempFrac;
            tempFrac = z_approx(af, pcpu_t[pcpu_id_now].factor);
            int temp_wcet = (int)vector_get(&tempFrac, 0);
            int temp_period = (int)vector_get(&tempFrac, 1);
            double temp_num = (double)temp_wcet / (double)temp_period;
            if(temp_num <= pcpu_t[pcpu_id_now].rest)
            {
                //assign this partition to pcpu_id_now
                *wcet = temp_wcet;
                *period = temp_period;
                pcpu_t[pcpu_id_now].rest -= temp_num;
                printf("Final Factor is: %lf\n", temp_num);
                printf("Allocated to %d.\n", pcpu_id_now);
                return pcpu_id_now;
            }
        }
        /*
        else if(pcpu_t[pcpu_t[rrp_cpus[i]].cpu_id].rest >= r)
        {
            Vector rfrac;
            // factors[pcpu_id_now]
            rfrac = z_approx(af, (int)pcpu_t[pcpu_id_now].factor);
            int first_elem_r = (int)vector_get(&rfrac, 0);
            int second_elem_r = (int)vector_get(&rfrac, 1);

            r = (double)first_elem_r/second_elem_r;
            printf("Final Factor is: %lf\n", r);
            printf("Allocated to %d.\n", pcpu_id_now);
            pcpu_t[pcpu_id_now].rest -= r;
            return pcpu_id_now;
        }
        */
    }

    return -1;
}

/* *************** z_approx ********
 * @param: double avail_factor: Avalability factor of each partition
 * @param: int num: The value of m in function Z(m, 2).
 * @return: Returns the approximate wcet and period calculated based on Z(m,2)
 * ***************************************************/
Vector z_approx(double avail_factor, int num)
{
    int i = 1, j=0, m=2;
    double largest = 1;
    VECTOR_INIT(v);
    while(true)
    {
        if((double)(num-i)/num >= avail_factor && (num-i)!= 1)
        {
            largest = (double)(num-i)/num;
             VECTOR_FREE(v);
             VECTOR_ADD(v, (int)num-i);
             VECTOR_ADD(v, (int)num);
            i+=1;
        } else{
            double denom = (int)(num*pow((double)m, (double)j));
            if(1/denom >= avail_factor)
            {
                largest = 1/denom;
                VECTOR_FREE(v);
                VECTOR_ADD(v, 1);
                 VECTOR_ADD(v, (int)(denom));
                j+= 1;
            } else
                return v;
        }
    }
   // return v;
}


/* *************** MERGE SORT FUNCTIONS ********* */
struct node* Merge_Sort(struct node **Header)
{
    struct node* head = *Header;

    // Create 2 Nodes for split and Merge
    struct node* a;
    struct node* b;

    // Trivial Case: If no elements?
    if(head->next == NULL)
        return NULL;

    // If not, split seq into halves
    Half_Split(head, &a, &b);

    // Sort the halves recursively
    Merge_Sort(&a);
    Merge_Sort(&b);

    // Merge the sorted halves and let pointer point to Header again ..
    // to enable a successful return
    *Header = Sorted_Merge(a, b);
    return *Header;
}

// Sort the already sorted halves
struct node* Sorted_Merge(struct node* a, struct node* b)
{
    // Create a temp Node
    struct node *temp = NULL;

    // If sample_node a is NULL, then simply return b and vice-versa
    if(a == NULL)
        return b;
    else if(b == NULL)
        return a;

    // Sort the elements based on their values of time-slices
    if(a->ts <= b->ts)
    {
        temp = a;
        // Now, for insertion of b, check it with next one of a
        temp->next = Sorted_Merge(a->next, b);
    }

    else{
        temp = b;
        temp->next = Sorted_Merge(a, b->next);
    }

    return temp;
}

// Half_Split using fast and slow pointer concept
void Half_Split(struct node* header, struct node** first_half,
                struct node** second_half)
{
    struct node *fast_divider;
    struct node *slow_divider;

    // Init slow_divider to point to the header initially
    // Init fast_divider to point to the header->next
    slow_divider = header;
    fast_divider = header->next;

    /* While slow divider proceeds one node at a time, fast divider..
     * on the other hand proceeds at double the rate as compared to
     * first one staying ahead of slow divider making sure the node is
     * not invalid
     * */
    while(fast_divider != NULL)
    {
        fast_divider = fast_divider->next;
        if(fast_divider != NULL)
        {
            slow_divider = slow_divider->next;
            fast_divider = fast_divider->next;
        }
    }

    // If control here, slow_divider must exactly be at the middle-1 of the list.
    *first_half = header;
    *second_half = slow_divider->next;
    slow_divider->next = NULL;
}

/* ********* FILE EXTRACTION OPERATIONS ************
 * RRP-Xen 2.0 Domains, CPU-Pool ID and CPU-IDs extractions
 * **************************************************/
/* ********* poolid ************
 * @param: FILE *filer: The file pointer to be used.
 * @return: The pool_id of the cpu pool target scheduler controls.
 * *********************************/
 int poolid(FILE *filer)
{
     int pool_id;
     filer = fopen("/home/rtlabuser/pool_uuid/pool_id.txt", "r");
     if(filer == NULL)
     {
         perror("fopen(pool_id.txt)\n");
     }
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
            rrp_cpus[cpu_counter] = atoi(token);
            cpu_counter += 1;
        for(x=0; x<10; x++)
        {
            printf("%s\n", token);
            token = strtok(NULL, s);
            if(token != NULL)
            {
                // valid strings
                rrp_cpus[cpu_counter] = atoi(token);
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
        printf("CPU[%d] #: %d\n",c,rrp_cpus[c]);
    }
    fclose(filer);
}
