//
// Created by Pavan Kumar  Paluri  on 2019-07-22.
//
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

#define NUM_ENTRIES 2
#define RRP_PCPUS 3

// Global Inits
static int arr[SIZE]; // init A_val with number of schedule entries
int counter;
 

// Structure Definitions
typedef struct {
    int id;
    int wcet; // getAAF_numerator
    int period; // getAAF_denominator
}sched_entry_t;

// Maintain a single linked list of timeslices
struct node{
    int ts;
    int id;
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


// Function Prototypes
void swap(int,int);
int lcm(int, int);
int hyper_period(sched_entry_t *);
sched_entry_t* dom_comp(sched_entry_t *);
sched_entry_t* dom_af_comp(sched_entry_t *);
struct node* load_timeslices(struct node*,int );
void getA_calc(sched_entry_t *, int, int);
struct node* Partition_single(sched_entry_t*,int,struct node*, int);
struct pcpu* load_pcpus(struct pcpu*);
bool Mul_Z(sched_entry_t*, struct pcpu*);
int MulZ_FFD_Alloc(double, struct pcpu*);
Vector z_approx(double, int);

// Append time-slice at the end of the ts list
void append(struct node** header, int ts, int id)
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
        printf("timeslice:%d\n",noder->ts);
        printf("DomID:%d\n",noder->id);
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
    sched_entry_t schedule[NUM_ENTRIES], *scheduler, *scheduler_mul_z;
    int A_val[NUM_ENTRIES];

    printf("Enter the schedule WCET:");
    for(int i=0; i<NUM_ENTRIES; i++)
    {
        scanf("%d",&schedule[i].wcet);
    }
    printf("Enter the schedule's Periods:");
    for(int i=0; i<NUM_ENTRIES; i++)
    {
        scanf("%d",&schedule[i].period);
    }

    for(int i=0; i< NUM_ENTRIES; i++)
    {
        schedule[i].id = i;
        printf("WCET of schedule[%d] is %d\n ",i, schedule[i].wcet);
        printf("Period of schedule[%d] is %d\n",i, schedule[i].period);
    }

    int wcet[2] ={13,15};
    printf("Return Val:%d\n",domain_handle_comp(wcet[0],wcet[1]));
    int a = 34;
    int b = 45;
    int *avail_ts;
    swap(a,b);
  //  scheduler = dom_comp(schedule);
  /*
    for(int i=0;i <NUM_ENTRIES;i++) {
        printf(" schedule WCET[%d]= %d\n", i, scheduler[i].wcet);
        printf(" schedule Period[%d]= %d\n", i, scheduler[i].period);
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
 //   head_2 = Partition_single(scheduler, hyper_period(scheduler), head_1);


    // ---------------- MULZ ZONE -------------------
    struct pcpu* pcpu_t;

    // Allocate mem for RRP_PCPUs count
    pcpu_t = (struct pcpu*)malloc(RRP_PCPUS*sizeof(struct pcpu));

    // Load PCPUs now
    pcpu_t = load_pcpus(pcpu_t);
    
    // Priority Q for Schedule Entries ( Sorted in Non-Increasing Order)
    scheduler_mul_z = dom_af_comp(schedule);

    for(int i=0;i <NUM_ENTRIES;i++) {
        printf(" Partition[%d] has AF: %f\n", i, (double)scheduler_mul_z[i].wcet/scheduler_mul_z[i].period);

        if((double)scheduler_mul_z[i].wcet/scheduler_mul_z[i].period);
    }
    // Time to Invoke Mul-Z
    Mul_Z(scheduler_mul_z, pcpu_t);

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

sched_entry_t *dom_af_comp(sched_entry_t sched[])
{
    int i,j;
    for(i=0;i<NUM_ENTRIES;i++) {
        for (j = i + 1; j < NUM_ENTRIES; j++) {
            int k;
            k = avail_factor_cmp((double)sched[i].wcet/sched[i].period, (double)sched[j].wcet/sched[j].period);
            if (k == 0) {
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
    for(int i=0;i<hp;++i)
        append(&head,i,0);
    return head;
}

struct node *Partition_single(sched_entry_t *sched, int hp, struct node* Node, int count)
{

    struct node *Reader = NULL;
    // iterate through sorted schedule entry list
    for(int i=0;i< count;i++)
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
         append(&occupied_time_index,index_now,0);
         //printf("Index Now:%d\n",index_now);

         printf("Assigning %d to %d\n",get_i(Node,index_now),sched[i].id);
         append(&Reader,get_i(Node,index_now),sched[i].id);
         
         // Assign timeslices calculated above to Partition_i
         //  under progress...

        }
        struct node *temp=NULL;
        for(int x=0; x < hp_now; x++)
        {
            if(search(occupied_time_index, x) == false)
                // insert the ith element of linked list with head "Node"
                append(&temp, get_i(Node, x), 0);
        }
        // copy(old, new) {old->new}
        //print_list(temp);
        Node=copy(temp,Node);
        //print_list(Node);


    }
    print_list(Reader);
    return Reader;
}


/* ********* Load PCPUS ************
 * @param: Void 
 * @return: returns array of struct pcpus
 * *********************************/
struct pcpu* load_pcpus(struct pcpu *pcpu_t)
{

    for(int iter=0; iter<RRP_PCPUS; iter++)
    {
        // Loading begins - Init Phase
        pcpu_t[iter].cpu_id = iter;
        // Init factors with 0
        pcpu_t[iter].factor = 0;
        // Init rest with 1
        pcpu_t[iter].rest = 1;
    }
    return pcpu_t;
}

/* ************* MUL-Z ************
 * @param: sched_mulZ: pointer to struct sched_entry_t
 * @param: pcpu_t: pointer to struct pcpu
 * return: a bool param
 * ********************************/
bool Mul_Z(sched_entry_t* mulZ, struct pcpu *pc)
{
    int cpu_return_id;
    // For Resource Partitions
    for(int i=0; i<NUM_ENTRIES; i++)
    {
        // cpu_return_id =1;
        cpu_return_id = MulZ_FFD_Alloc((double)mulZ[i].wcet/mulZ[i].period, pc);
        // For PCPUs
        printf("CPU return ID is: %d\n", cpu_return_id);
        if(cpu_return_id == -1)
            return false;
        else
        {
            // CPU_ID : Partition_ID
            insert_key_val(cpu_return_id, mulZ[i].id);
        }
    }

    // Execute Partition-Single for each PCPU here
    for(int i=0; i<RRP_PCPUS; i++)
    {
        // Maintain a Vector of Partitions..
       // VECTOR_INIT(current_sched_entries);
       sched_entry_t RRP_MulZ[SIZE];
       int count = 0; // Make sure count is always less than SIZE
        for(int j=0; j< SIZE; j++)
        {
            if(hashArray[j] != NULL)
            {
                if(hashArray[j]->Key == pc[i].cpu_id)
                {
                    // Add to vector current_sched_entries the Partition-ID
                    // VECTOR_ADD(current_sched_entries, &mulZ[hashArray[j]->Value]);
                    // Perform Copy of every entity of that instance
                    RRP_MulZ[count].id = mulZ[hashArray[j]->Value].id;
                    RRP_MulZ[count].wcet = mulZ[hashArray[j]->Value].wcet;
                    RRP_MulZ[count].period = mulZ[hashArray[j]->Value].period;
                    printf("RRP_MulZ[%d].id: %d\n", count, RRP_MulZ[count].id);
                    printf("RRP_MulZ[%d].wcet: %d\n", count, RRP_MulZ[count].wcet);
                    printf("RRP_MulZ[%d].period: %d\n", count, RRP_MulZ[count].period);

                    count++;
                }
            }
        }
        printf("Count Val:%d\n", count);
        // Invoke Partition-Single for Each PCPU
        struct node *head = NULL;
        struct node *head_1, *head2;
        printf("Partition Singe invoked..\n");
        head_1 = load_timeslices(head, hyper_period_MulZ(RRP_MulZ, count));
        getA_calc(RRP_MulZ, hyper_period_MulZ(RRP_MulZ, count), count);
        head2=Partition_single(RRP_MulZ, hyper_period_MulZ(RRP_MulZ, count), head_1, count);

      //  free(RRP_MulZ);
    }

    display();
    // If here and control hasnt returned false..
    return true;
}

/* *************** HELPER-FUNCTION FOR MUL_Z ********
 * @param: double (AAF) of each partition
 * @param: pcpu_t: Pointer to struct pcpu
 * ***************************************************/
int MulZ_FFD_Alloc(double af, struct pcpu* pcpu_t) {
    int fixed_list[4] = {3, 4, 5, 7};
    Vector num_frac;
    int x_index = 5;
    double smallest = INT_MAX;
    int smallest_up = 0, smallest_down = INT_MAX;
    for (int i = 0; i < 4; i++)
    {
        num_frac = z_approx(af, fixed_list[i]);
        int first_elem = (int)vector_get(&num_frac, 0);
        int second_elem = (int)vector_get(&num_frac, 1);
    //    printf("num_frac[0]: %d\n", first_elem);
    //    printf("num_frac[1]: %d\n", second_elem);
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
    for(int i=0; i<RRP_PCPUS; i++)
    {
        int pcpu_id_now = pcpu_t[i].cpu_id;
        if(pcpu_t[pcpu_id_now].factor == 0)
        {
            pcpu_t[pcpu_id_now].factor = x_index;
            pcpu_t[pcpu_id_now].rest = 1-r;
            // TODO: set_AAF(rest) and set_AAF_Frac() need to be developed...
            // Do We need them though ???
            printf("Final Factor is: %lf\n", r);
            return pcpu_id_now;
        }
        else if(pcpu_t[pcpu_t[i].cpu_id].rest >= r)
        {
            Vector rfrac;
            // factors[pcpu_id_now]
            rfrac = z_approx(af, (int)pcpu_t[pcpu_id_now].factor);
            int first_elem_r = (int)vector_get(&rfrac, 0);
            int second_elem_r = (int)vector_get(&rfrac, 1);

            r = (double)first_elem_r/second_elem_r;
            printf("Final Factor is: %lf\n", r);
            pcpu_t[pcpu_id_now].rest -= r;
            return pcpu_id_now;
        }
    }

    return -1;
}

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
            double denom = (int)(num*pow(m, j));
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
