#include <xenctrl.h>
#include <xen/sysctl.h>
#include <uuid/uuid.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CPU_NUM        5
#define LINE_SIZE	       2048
// #define TIME_SLICE_LENGTH  50

/* ********** XEN #DEFINEs ************** */
// #define NUM_MINOR_FRAMES 2
typedef int64_t s_time_t;
#define MICROSECS(_us)    ((s_time_t)((_us) * 1000ULL))
// #define MILLISECS(_ms)          (((s_time_t)(_ms)) * 1000000UL )
#define MILLISECS(_ms)  ((s_time_t)((_ms) * 1000000ULL))
#define DOMN_RUNTIME(_runtime) (_runtime*MICROSECS(1000)) // Example: DOMN_RUNTIME(30) ~30ms ~ 30*pow(10,3)ns
#define DOMN_RUNTIME_US (DOMN_RUNTIME*MICROSECS(1000))
#define TIME_SLICE_LENGTH MILLISECS(1)
#define LIBXL_CPUPOOL_POOLID_ANY 0xFFFFFFFF
#define UUID_READ 1024
/* *************************************** */



int poolid(char* fileName)
{
     int pool_id;
     FILE* filer = fopen(fileName, "r");
     if(filer == NULL)
     {
         perror("fopen(pool_id.txt)\n");
     }
     fscanf(filer, "%d", &pool_id);
     printf("Reading... cpupool ID is: %d\n", pool_id);
     fclose(filer);
     return pool_id;
}

void getCPUInfo(char* buff, struct xen_sysctl_aaf_schedule* s_now)
{
	//printf("Getting CPU Info\n");
	char line[LINE_SIZE];
	strcpy(line, buff);
	char s[2] = ",";
	char* token;
	token = strtok(line, s);
	if(token==NULL)
		printf("%s not found in %s\n", s, line);
	s_now->cpu_id = atoi(token);
	token = strtok(NULL, s);
	s_now->num_schedule_entries = atoi(token);
	s_now->hyperperiod = s_now->num_schedule_entries * TIME_SLICE_LENGTH;
}

void getScheduleEntry(char* line, struct xen_sysctl_aaf_schedule* s_now)
{
	//printf("Rertrieving entries\n");
	char s[2] = ",";
	char* token;
	int counter = 0, ret_val;
	token = strtok(line, s);
	while(token)
	{
		//printf("%s; %s\n", line, token);
		//printf("Entry now: %s\n", token);
		//printf("Length: %u\n", strlen(token));
		if(strlen(token)>36)
			printf("%d\n", token[36]);
		if(strcmp(token, "-1")==0 || strcmp(token, " -1")==0)
		{
			strcpy(s_now->schedule[counter].dom_handle, "");
		}
		else
		{
			//strcpy(s_now->schedule[counter].dom_handle, token);
			char temp[100];
			strcpy(temp, token);
			printf("This entry: %s\n", temp);
			char domId[100], vcpu[100];
			char * mid = strchr(temp, ':');
			*mid = '\0';
			strcpy(domId, temp);
			strcpy(vcpu, mid+1);	
			printf("DomID:%s; ", domId);
			printf("VCPU:%s\n", vcpu);
			ret_val = uuid_parse(domId, s_now->schedule[counter].dom_handle);
			s_now->schedule[counter].vcpu_id = atoi(vcpu);
			//ret_val = uuid_parse(token, s_now->schedule[counter].dom_handle);
                	if(ret_val < 0)
                	{
                        	printf("Wrong in parsing the uuid: %s\n", token);
				return;
                	}

		}
		//printf("Entry stored: %s\n", s_now->schedule[counter].dom_handle);
		//printf("Entry stored: %X\n", *s_now->schedule[counter].dom_handle);
		/*
		char temp[20];
		sprintf(temp, "%X", *s_now->schedule[counter].dom_handle);
		int CPU = s_now->cpu_id;
		//printf("Test: %s", temp);
		// hardcoding VCPU ID for each domain and each PCPU
		if(strcmp(temp, "B")==0)
		{
			if(CPU==1)
			{
				s_now->schedule[counter].vcpu_id = 0;
			}
			else if(CPU==3)
			{
				s_now->schedule[counter].vcpu_id = 1;
			}
			else if(CPU==5)
			{
				s_now->schedule[counter].vcpu_id = 2;
			}
			else
			{
				s_now->schedule[counter].vcpu_id = 100;
			}
		}
		else if(strcmp(temp, "DE")==0)
		{
                        if(CPU==1)
                        {
                                s_now->schedule[counter].vcpu_id = 0;
                        }
                        else if(CPU==3)
                        {
                                s_now->schedule[counter].vcpu_id = 1;
                        }
                        else if(CPU==5)
                        {
                                s_now->schedule[counter].vcpu_id = 2;
                        }
                        else
                        {
                                s_now->schedule[counter].vcpu_id = 100;
                        }
		}
		else if(strcmp(temp, "C1")==0)
		{
                        if(CPU==1)
                        {
                                s_now->schedule[counter].vcpu_id = 1;
                        }
                        else if(CPU==3)
                        {
                                s_now->schedule[counter].vcpu_id = 2;
                        }
                        else if(CPU==5)
                        {
                                s_now->schedule[counter].vcpu_id = 0;
                        }
                        else
                        {
                                s_now->schedule[counter].vcpu_id = 100;
                        }
		}
		*/
		s_now->schedule[counter].wcet = TIME_SLICE_LENGTH;
		//s_now->schedule[counter].vcpu_id = 0;
		counter ++;
		token = strtok(NULL, s);

		/*
		char sep[2] = ":";
		char* temp;
		temp = strtok(token, sep);
		ret_val = uuid_parse(temp, s_now->schedule[counter].dom_handle);
		if(ret_val < 0)
		{
			printf("Wrong in parsing the uuid!");
			return;
		}
		printf("Parsed: %u\n", s_now->schedule[counter].dom_handle);
		strcpy(s_now->schedule[counter].dom_handle, temp);
		printf("Parse: %s\n", s_now->schedule[counter].dom_handle);
		temp = strtok(NULL, sep);
		s_now->schedule[counter].vcpu_id = atoi(temp);
		s_now->schedule[counter].wcet = TIME_SLICE_LENGTH;
		counter ++;
		token = strtok(NULL, s);
		*/
	}

}

void parse(char* fileName)
{
	xc_interface *xci = xc_interface_open(NULL, NULL, 0);
	FILE *fp = NULL;
	fp = fopen(fileName, "r");
	if (fp==NULL)
	{
		printf("Cannot open %s!\n", fileName);
		return;
	}
    	struct xen_sysctl_scheduler_op ops;
    	struct xen_sysctl_aaf_schedule sched_aaf[MAX_CPU_NUM];
	char buff[LINE_SIZE];
	char* token;
	int lineCounter = 0;
	//while (!feof(fp))
	while(fgets(buff, LINE_SIZE, fp))
	{
		buff[strlen(buff) - 1] = 0;
		int index = lineCounter / 2;
		//printf("%s\n", buff);
		if(lineCounter%2 == 0)
		{
			// extract CPU info
			getCPUInfo(buff, &sched_aaf[lineCounter / 2]);

		}
		else
		{
			// extract schedule info
			getScheduleEntry(buff, &sched_aaf[lineCounter / 2]);
		}
		lineCounter ++;
	}
	for(int i=0; i<lineCounter/2; i++)
	{
		printf("CPU: %d, Entries: %d \n", sched_aaf[i].cpu_id, sched_aaf[i].num_schedule_entries);
		for(int j=0; j<sched_aaf[i].num_schedule_entries; j++)
		{
			printf("%X : %d , ",*sched_aaf[i].schedule[j].dom_handle, sched_aaf[i].schedule[j].vcpu_id);
		}
		printf("\n");
		int cpu_pool_id = poolid("config_files/pool_uuid/pool_id.txt");
		// set schedule here
//#define RRP_SCHED_SET_ENABLE
#ifdef RRP_SCHED_SET_ENABLE
            	int set_result = xc_sched_aaf_schedule_set(xci, cpu_pool_id, &sched_aaf[i]);
	    	printf("------------------------------\n");
            	printf("Schedule Set for LT PCPU: %d is: %d\n", sched_aaf[i].cpu_id, set_result);
	    	printf("------------------------------\n");
#endif
            	// GET-ZONE
#ifdef RRP_HED_GET_ENABLE
            	int get_result = xc_sched_aaf_schedule_get(xci, cpu_pool_id, &sched_aaf[i]);
#endif
	}


}

int main()
{
	parse("out.txt");
}
