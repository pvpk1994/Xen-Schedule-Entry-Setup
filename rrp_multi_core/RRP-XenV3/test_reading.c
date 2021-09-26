#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_CPU_NUM        5
#define LINE_SIZE	       2048
#define TIME_SLICE_LENGTH  50

/*
void getCPUInfo(char* buff, struct xen_sysctl_aaf_schedule* s_now)
{
	printf("Getting CPU Info\n");
	char line[LINE_SIZE];
	strcpy(line, buff);
	char s[2] = ",";
	char* token;
	printf("Rertrieving info from %s\n", line);
	token = strtok(line, s);
	printf("Rertrieving info from %s\n", line);
	printf("%llu\n", token);
	printf("%s\n", token);
	if(token==NULL)
		printf("%s not found in %s\n", s, line);
	printf("%s; %s\n", line, s);
	s_now->cpu_id = atoi(token);
	token = strtok(NULL, s);
	s_now->num_schedule_entries = atoi(token);
	s_now->hyperperiod = s_now->num_schedule_entries * TIME_SLICE_LENGTH;
}

void getScheduleEntry(char* line, struct xen_sysctl_aaf_schedule* s_now)
{
	printf("Rertrieving entry\n");
	char s[2] = ",";
	char* token;
	int counter = 0, ret_val;
	token = strtok(line, s);
	while(token)
	{
		printf("%s; %s\n", line, token);
		if(strcmp(token, "-1")==0)
		{
			counter ++;
			token = strtok(NULL, s);
		}
		char sep[2] = ":";
		char* temp;
		temp = strtok(token, sep);
		ret_val = uuid_parse(temp, s_now->schedule[counter].dom_handle);
		if(ret_val < 0)
		{
			printf("Wrong in parsing the uuid!");
			return;
		}
		temp = strtok(NULL, sep);
		s_now->schedule[counter].vcpu_id = atoi(temp);
		s_now->schedule[counter].wcet = TIME_SLICE_LENGTH;
		counter ++;
		token = strtok(NULL, s);
	}

}
*/

void parse(char* fileName)
{
	FILE *fp = NULL;
	fp = fopen(fileName, "r");
	if (fp==NULL)
	{
		printf("Cannot open %s!\n", fileName);
		return;
	}
	char buff[LINE_SIZE];
	char* token;
	int lineCounter = 0;
	//while (!feof(fp))
	while(fgets(buff, LINE_SIZE, fp))
	{
		int index = lineCounter / 2;
		printf("%s\n", buff);
		if(lineCounter%2 == 0)
		{
			// extract CPU info
			//getCPUInfo(buff, &sched_aaf[lineCounter / 2]);
			
			char s[2] = ",";
    		printf("Rertrieving info from %s\n", buff);
    		token = strtok(buff, s);
    		printf("Rertrieving info from %s\n", buff);
    		printf("%s\n", token);
			/*
        		sched_aaf[index].cpu_id = atoi(token);
        		token = strtok(NULL, s);
        		sched_aaf[index].num_schedule_entries = atoi(token);
        		sched_aaf[index].hyperperiod = sched_aaf[index].num_schedule_entries * TIME_SLICE_LENGTH;
			*/
		}
		else
		{
			// extract schedule info
			//getScheduleEntry(buff, &sched_aaf[lineCounter / 2]);
		}
		lineCounter ++;
	}


}

int main()
{
	parse("out.txt");
}
