//
// Created by Pavan Kumar  Paluri  on 2019-07-22.
// ---------------------------------------------
// Upadte on Dec-5, 2019
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
#define TIME_SLICE_LENGTH MILLISECS(30)
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
struct node* Partition_single(sched_entry_t*,int,struct node*);
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

    int wcet[2] ={13,15};
    printf("Return Val:%d\n",domain_handle_comp(wcet[0],wcet[1]));
    int a = 34;
    int b = 45;
    int *avail_ts;
    swap(a,b);
    scheduler = dom_comp(schedule);
    for(int i=0;i <num_minor_frames;i++) {
        printf(" schedule WCET[%d]= %d\n", i, scheduler[i].wcet);
        printf(" schedule Period[%d]= %d\n", i, scheduler[i].period);
    }
    getA_calc(scheduler, hyper_period(scheduler));
    for( int i=0;i< num_minor_frames;i++)
        printf("arr[%d]:%d\n",i,arr[i]);
    struct node *head = NULL;
    struct node *head_1, *head_2;

    head_1 = load_timeslices(head, hyper_period(scheduler));
    //print_list(load_timeslices(head, hyper_period(scheduler)));
    // Returns an unsorted timeslice-domain Pair
    // Need to sort it based on the timeslices....
    head_2 = Partition_single(scheduler, hyper_period(scheduler), head_1);
    //sched_aaf.hyperperiod = hyper_period(scheduler)*DOMN_RUNTIME(30);
    sched_aaf.hyperperiod = hyper_period(scheduler)*TIME_SLICE_LENGTH;

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
       //sched_aaf.schedule[k].wcet = DOMN_RUNTIME(30);
       sched_aaf.schedule[k].wcet = TIME_SLICE_LENGTH;

      // if(sched_aaf.schedule[k].wcet ==0)
      //     sched_aaf.schedule[k].wcet =1;
       sched_aaf.schedule[k].vcpu_id = 0;
        head_2 = head_2->next;
        k++;
    }
    printf("k value:%d\n",k);

    for(int z=0;z<k;z++)
    {
        //printf("uuid_string:%s\n",uuid_new[z]);
        //uuid_parse(uuid_new[z],sched_aaf.schedule[z].dom_handle);
        printf("Dom_Handle:%X\n",*sched_aaf.schedule[z].dom_handle);
        printf("Domain Runtime:%d\n",sched_aaf.schedule[z].wcet);
    }

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
            printf("The LCM of %d and %d is %d.\n", num1, num2,minMultiple);
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
        printf("Final Value: %d\n", final_val);

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

struct node *Partition_single(sched_entry_t *sched, int hp, struct node* Node)
{

    struct node *Reader = NULL;
    // iterate through sorted schedule entry list
    for(int i=0;i< num_minor_frames;i++)
    {
        // init hp_now to the current size of timeslice list
        int hp_now = list_length(Node);
        // printf("SIZE:%d\n",hp_now);

        // Init a new list
        struct node *occupied_time_index=NULL;
        for(int k=0;k <arr[i];k++)
        {
            // Assignment of calculated timeslices to a partition
            int index_now = (int)(floor(k*hp_now/arr[i]))%hp_now;
            append(&occupied_time_index,index_now,NULL);
            //printf("Index Now:%d\n",index_now);

            printf("Assigning %d to %s\n",get_i(Node,index_now),sched[i].id);
            // sched[i].id is in integer but append takes in
            append(&Reader,get_i(Node,index_now),sched[i].id);

            // Assign timeslices calculated above to Partition_i
            //  under progress...

        }
        struct node *temp=NULL;
        for(int i=0;i<hp_now;i++)
        {
            if(search(occupied_time_index,i)== false)
                // insert the ith element of linked list with head "Node"
                append(&temp,get_i(Node,i),NULL);
        }
        // copy(old, new) {old->new}
        //print_list(temp);
        Node=copy(temp,Node);
        //print_list(Node);


    }
    print_list(Reader);
    return Reader;
}
