//
/* RRP SINGLE CORE Schedule Generator (Magic7)
 * Author:: Pavan Kumar Paluri, Guangli Dai
 * Copyright 2019-2020 - RTLAB UNIVERSITY OF HOUSTON */
// Created by Pavan Kumar  Paluri  on 2019-07-22.
// --------------------------------------------------
// Last upadted on April-11, 2020 -[fixed cpu ID issue]
// --------------------------------------------------------------------------------------
//  NOTE:: now() in Xen hypervisor space always measures time in Nanoseconds time units.
// Therefore, wcet being passed to the hypervisor space should also be in Nanoseconds
// --------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
//#include </usr/local/include/xenctrl.h>
#include <uuid/uuid.h>
//#include </usr/local/include/xen/sysctl.h>
#include <xenctrl.h>
#include <xen/sysctl.h>
#define NUM_MINOR_FRAMES 2
typedef int64_t s_time_t;
#define MICROSECS(_us)    ((s_time_t)((_us) * 1000ULL))
// #define MILLISECS(_ms)          (((s_time_t)(_ms)) * 1000000UL )
#define MILLISECS(_ms)  ((s_time_t)((_ms) * 1000000ULL))
#define DOMN_RUNTIME(_runtime) (_runtime*MICROSECS(1000)) // Example: DOMN_RUNTIME(30) ~30ms ~ 30*pow(10,3)ns
#define DOMN_RUNTIME_US (DOMN_RUNTIME*MICROSECS(1000))
#define LIBXL_CPUPOOL_POOLID_ANY 0xFFFFFFFF
#define UUID_READ 1024

#define NUM_ENTRIES 3

// Global Inits
static int arr[1024]; // init A_val with number of schedule entries
int counter;
char **uuid;
char **uuid_new;
static int num_minor_frames;
// Structure Definitions
typedef struct {
    char *id;
    int wcet; // getAAF_numerator
    int period; // getAAF_denominator
}sched_entry_t;

// Maintain a single linked list of timeslices
struct node{
    int ts;
    char *id;
    struct node *next;
};


// Function Prototypes
void swap(int,int);
int lcm(int, int);
int hyper_period(sched_entry_t *);
sched_entry_t* dom_comp(sched_entry_t *);
struct node* load_timeslices(struct node*,int );
void getA_calc(sched_entry_t *, int);
struct node* partition_single(sched_entry_t*,int,struct node*);
struct node* SortedMerge(struct node* , struct node* );
void HalfSplit(struct node*, struct node**,
               struct node**);

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
    num_minor_frames = i;
    return buffer;
}

// Perform merge sort on unsorted ts-dom pair based on ts Complexity : O(nlogn)
struct node* MergeSort(struct node ** HeadRef)
{
    struct node* head = *HeadRef;
    // Create 2 nodes a and b for splitting
    struct node* a;
    struct node* b;

    // Trivial case test: what if the there is only 1 element in the list?
    if((head->next == NULL) || (head == NULL))
        return NULL;

    // Split the unsorted list into 2 halves
    HalfSplit(head, &a, &b);

    // Sort the halves recursively
    MergeSort(&a);
    MergeSort(&b);

    // Merge the sorted halves and point it back to headref
    *HeadRef = SortedMerge(a,b);
    return *HeadRef;

}

// Sorted Merge to sort the already sorted halves
struct node* SortedMerge(struct node* a, struct node* b)
{
    // init a temp node
    struct node* result = NULL;

    // Trivial checks
    // if only on half is present, no need to sort, just return that valid half
    if(a == NULL)
        return b;
    else if(b == NULL)
        return a;

    // Actual sort happens here, sort it in ascending order
    if(a->ts <= b->ts)
    {
        result = a;
        result->next = SortedMerge(a->next, b);
    }
    else
    {
        result = b;
        result->next = SortedMerge(a,b->next);
    }
    return result;
}

// Now, all we are left with is the completion of HalfSplit(...)
void HalfSplit(struct node* header, struct node** first_half,
               struct node** second_half)
{
    struct node* fast_divider;
    struct node* slow_divider;
    slow_divider = header;
    fast_divider = header->next;
    // while slow_divider increments one node at a time, fast_divider advances 2 nodes at a time
    // thereby staying ahead and checking if list is not null
    while(fast_divider != NULL)
    {
        fast_divider = fast_divider->next;
        if(fast_divider != NULL)
        {
            slow_divider = slow_divider->next;
            fast_divider = fast_divider->next;
        }
    }

    // By the time control is out of above while loop, fast_divider should have reached
    // the end of list while slow_divider should have reached node before midpoint
    *first_half = header;
    *second_half = slow_divider->next;
    slow_divider->next = NULL;
}


// Append timeslice at the end of the ts list
void append(struct node** header, int ts, char* id)
{
    struct node* new_node = (struct node*)malloc(sizeof(struct node));
    struct node *last = *header;

    // Assignment(s)
    new_node->ts = ts;
    char buf[30];
   // buf = strdup(id);
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
    // printf("Tail Timeslice:%d\n",last->ts);
    return;

}


// Print contents of linked list
void print_list(struct node* noder)
{
    while(noder!= NULL)
    {
        printf("timeslice:%d\n",noder->ts);
        printf("DomID:%s\n",noder->id);
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

// MAIN
int main()
{
    FILE *file;
    int pool_id = poolid(file);
    uuid = uu_id(file);
    struct xen_sysctl_scheduler_op ops;
    struct xen_sysctl_aaf_schedule sched_aaf;
    sched_entry_t schedule[num_minor_frames], *scheduler;
    int A_val[num_minor_frames];
    xc_interface *xci = xc_interface_open(NULL,NULL,0);

    int cpu_id = get_cpu_id();
    if(cpu_id!=-1)
        sched_aaf.cpu_id = cpu_id;

    printf("Enter the schedule WCET:");
    for(int i=0; i<num_minor_frames; i++)
    {
        scanf("%d",&schedule[i].wcet);
    }
    printf("Enter the schedule's Periods:");
    for(int i=0; i<num_minor_frames; i++)
    {
        scanf("%d",&schedule[i].period);
    }

    for(int i=0; i< num_minor_frames; i++)
    {
         schedule[i].id = malloc(UUID_READ);
        //strcpy(schedule[i].id ,uuid[i]);
        strncpy(schedule[i].id, uuid[i],UUID_READ);
        //schedule[i].id[19] = 0;
        printf("WCET of schedule[%d] is %d\n ",i, schedule[i].wcet);
        printf("Period of schedule[%d] is %d\n",i, schedule[i].period);
    }

    int *avail_ts;
    scheduler = dom_comp(schedule);
    for(int i=0;i <num_minor_frames;i++) {
        printf(" schedule WCET[%d]= %d\n", i, scheduler[i].wcet);
        printf(" schedule Period[%d]= %d\n", i, scheduler[i].period);
    }
    getA_calc(scheduler, hyper_period(scheduler));
    //for( int i=0;i< num_minor_frames;i++)
    //    printf("arr[%d]:%d\n",i,arr[i]);
    struct node *head = NULL;
    struct node *head_1, *head_2;

    head_1 = load_timeslices(head, hyper_period(scheduler));
    //print_list(load_timeslices(head, hyper_period(scheduler)));
    // Returns an unsorted timeslice-domain Pair
    // Need to sort it based on the timeslices....
    head_2 = partition_single(scheduler, hyper_period(scheduler), head_1);
    sched_aaf.hyperperiod = hyper_period(scheduler)*DOMN_RUNTIME(30);

    head_2= MergeSort(&head_2);
    printf("\nSorted list\n");
    print_list(head_2);
    // set the number of schedule entries dynamically to be equal to the number of entries in timeslice-dom table
    sched_aaf.num_schedule_entries = list_length(head_2);
    //sched_aaf.num_sched_entries = 1;
    printf("Number of schedule entries in Kernel:%d\n",sched_aaf.num_schedule_entries);
    printf("Hyperperiod of the schedule:%d\n",sched_aaf.hyperperiod);

    // Testing phase to see if int dom_id can be converted back to xen_dom_handle for launching into kernel...
    uuid_new = (char**)calloc(num_minor_frames,sizeof(char*));
    for(int i=0;i<num_minor_frames;i++)
    {
        uuid_new[i] = (char*)calloc(5,sizeof(char));
    }
    int k=0;

    while(head_2 != NULL)
    {
        char buf[30];
       // itoa(head_2->id,uuid_new[k],64);
       printf("Return Val:%d\n",uuid_parse(head_2->id,sched_aaf.schedule[k].dom_handle));
       sched_aaf.schedule[k].wcet = DOMN_RUNTIME(30);

      // if(sched_aaf.schedule[k].wcet ==0)
      //     sched_aaf.schedule[k].wcet =1;
       sched_aaf.schedule[k].vcpu_id = 0;
        head_2 = head_2->next;
        k++;
    }

    /*
    printf("k value:%d\n",k);

    for(int z=0;z<k;z++)
    {
        //printf("uuid_string:%s\n",uuid_new[z]);
        //uuid_parse(uuid_new[z],sched_aaf.schedule[z].dom_handle);
        printf("Dom_Handle:%X\n",*sched_aaf.schedule[z].dom_handle);
        printf("Domain Runtime:%d\n",sched_aaf.schedule[z].wcet);
    }
    */

   // sched_aaf has now all the required fields to prepare sched_entries and send them into runnable state ...
    int set_result;
 //  printf(" \n !!!!! SCHEDULE SET AND GET FUNCTIONS NOT INVOKED !!!!!\n");
   set_result = xc_sched_aaf_schedule_set(xci,pool_id,&sched_aaf);
   printf("Schedule Set for AAF result is: %d\n",set_result);
    	//set_result = xc_sched_arinc653_schedule_set(xci,pool_id,&sched_aaf);
   int get_result =32 ;
   get_result = xc_sched_aaf_schedule_get(xci,pool_id,&sched_aaf);
    printf(" Schedule Get for AAF result is: %d\n",get_result);

    return 0;
}

int domain_handle_comp(int wcet1, int wcet2)
{
    return wcet1 > wcet2 ? 0 : 1;
}

void swap(int a, int b)
{
    int temp;
    temp = a;
    a = b;
    b = temp;
    printf(" Value of a and b are: %d %d respectively\n", a,b);

}

sched_entry_t *dom_comp(sched_entry_t sched[])
{
    int i,j;
    for(i=0;i<num_minor_frames;i++) {
        for (j = i + 1; j < num_minor_frames; j++) {
            int k;
            k = domain_handle_comp(sched[i].wcet, sched[j].wcet);
            if (k != 0) {
                // swap(sched[i].wcet, sched[j].wcet);
                int temp,temp1;
                char temp2[1024];
                temp = sched[i].wcet;
                sched[i].wcet = sched[j].wcet;
                sched[j].wcet = temp;
                //return sched;
                temp1 = sched[i].period;
                sched[i].period = sched[j].period;
                sched[j].period = temp1;

                strcpy(temp2 , sched[i].id);
                strcpy(sched[i].id , sched[j].id);
                strcpy(sched[j].id ,temp2);
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
            //printf("The LCM of %d and %d is %d.\n", num1, num2,minMultiple);
            break;
        }
        ++minMultiple;
    }
    return minMultiple;
}


int hyper_period(sched_entry_t *sched)
{
    int final_val=1;
    for (int i=0; i< num_minor_frames; i++)
    {

        final_val = lcm(final_val, sched[i].period);
        //printf("Final Value: %d\n", final_val);

    }
    return final_val;
}


void getA_calc(sched_entry_t *sched, int hp)
{
    int i;


    for(i=0;i< num_minor_frames;i++)
    {
        arr[i] = (int)(hp/sched[i].period)*(sched[i].wcet);
    }
}

struct node* load_timeslices(struct node* head, int hp)
{
    for(int i=1;i<=hp;++i)
        append(&head,i,NULL);
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
struct node *partition_single(sched_entry_t *partitions, int hp, struct node* avail)
{
    //Initialize the time slice list
    struct node *result = NULL;
    // iterate through sorted schedule entry list (partitions)
    for(int i=0;i< num_minor_frames;i++)
    {
        // Init a new list to record the time slices allocated.
        struct node *occupied_time_index=NULL;
        //allocate time slices based on different wcet of the current partition
        if(partitions[i].wcet!=1)
        {
            //find available delta first
            int avail_length = list_length(avail);
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
            //retrieve the smallest time slice available now
            int index = get_i(avail, 0);
            //check whether time is in feasible range.
            if(index >= partitions[i].period)
            {
                printf("Unschedulable partitions with no available time slices!\n");
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

/* ********* get_cpu_id ************
 * @return: Returns the id of the first CPU in the pool (if there are multiple).
 *          The cpu ids must be written to /home/rtlabuser/pool_uuid/rrp_cpus_list.txt in advance.
 *          If no available CPU id is found there, the function returns -1.
 * *********************************/
int get_cpu_id()
{
    FILE *filer;
    filer = fopen("/home/rtlabuser/pool_uuid/rrp_cpus_list.txt", "r");
    if(filer == NULL)
    {
        perror("fopen(rrp_cpus_list.txt)\n");
    }
    char *token;
    const char s[2] = ",";
    //read in the first cpu id and return.
    if(filer != NULL)
    {
        char line[20];
        while(fgets(line, sizeof(line), filer)!= NULL)
        {
            token = strtok(line, s);
            int cpu_id = atoi(token);
            fclose(filer);
            return cpu_id;
        }
    }
    return -1;
    fclose(filer);
}
