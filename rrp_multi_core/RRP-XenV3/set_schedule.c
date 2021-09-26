#include <xenctrl.h>
#include <xen/sysctl.h>
#include <uuid/uuid.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CPU_NUM        5
#define LINE_SIZE	       2048
#define TIME_SLICE_LENGTH  50

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
			ret_val = uuid_parse(token, s_now->schedule[counter].dom_handle);
                	if(ret_val < 0)
                	{
                        	printf("Wrong in parsing the uuid: %s\n", token);
				return;
                	}
			
		}
		//printf("Entry stored: %s\n", s_now->schedule[counter].dom_handle);
		//printf("Entry stored: %X\n", *s_now->schedule[counter].dom_handle);
		s_now->schedule[counter].wcet = TIME_SLICE_LENGTH;
		s_now->schedule[counter].vcpu_id = 0;
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
		// set schedule here
#ifdef RRP_SCHED_SET_ENABLE
            	set_result = xc_sched_aaf_schedule_set(xci, cpu_pool_id, &sched_aaf[i]);
	    	printf("------------------------------\n");
            	printf("Schedule Set for LT PCPU: %d is: %d\n", pc[rrp_cpus[i]].cpu_id, set_result);
	    	printf("------------------------------\n");
#endif
            	// GET-ZONE
#ifdef RRP_HED_GET_ENABLE
            	get_result = xc_sched_aaf_schedule_get(xci, cpu_pool_id, &sched_aaf[i]);
#endif
	}

	


}

int main()
{
	parse("out.txt");
}
