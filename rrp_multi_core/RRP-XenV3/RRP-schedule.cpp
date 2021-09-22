#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <cmath>
#include <algorithm>
// Customized Includes
#include "rapidjson/document.h"
using namespace std;
using namespace rapidjson;

#include <xenctrl.h>
#include <xen/sysctl.h>
#include <uuid/uuid.h>

#define TIME_SLICE_LENGTH 30
// Structure Definitions
typedef struct {
    string id;
    int wcet; // availability factor numerator
    int period; // availability factor denominator
    int pcpu; // the pcpu this sched_entry_t will be executed on

    double getAF()
    {
    	return static_cast<double>(wcet) / static_cast<double>(period);
    }
}sched_entry_t;

typedef struct {
	string id;
	int vcpuNum;
}domain;

typedef struct {
    int cpu_id;
    double factor;
    double rest;
}pcpu;

typedef struct {
    unordered_set<string> ids;
    double af_sum;
}dp_return;


// -------------------------------------------------------------------
//                       Function Prototypes
// --------------------------------------------------------------------

// information retrieval functions
string readJsonFile(string fileName);
vector<sched_entry_t> parseJSON(string jsonContents);
int getPoolID(string poolFileName);
vector<domain> getDomainInfo(string domainsFileName);
unordered_set<int> getPCPUs(string CPUsFileName);
bool validateVCPUInfo(const vector<sched_entry_t> & VCPUs, int vcpuNum, const unordered_set<int>& PCPUs);
string generateNewID(string domainID, string VCPUId);
void parseID(string IDNow, string& domainID, int &VCPUID);

// schedule generation functions --> to be added
bool cmpVCPUs(const sched_entry_t& a, const sched_entry_t& b);
int lcm(int a, int b);
bool MulZ_ILO(vector<sched_entry_t>& VCPUs, vector<int>& PCPUs, int time_slice_length);
dp_return dp_single(vector<sched_entry_t> partition_list, int factor);
void z_approx(sched_entry_t& partition, int factor);
double approximate_value(double value);
vector<string> CSG(vector<sched_entry_t> partition_list, int factor);

bool check_delta(unordered_set<int> avail_set, vector<int> standard_p, int delta, int p);
int find_delta(unordered_set<int> avail_timeslices, int p, int q, int q_left);




// test functions, only used for testing purposes.
void printEntries(vector<sched_entry_t> target);
void test_approx(int wcet, int period);



/* ********* main function ************
 * ONLY A TEST VERSION NOW.
 * This function is the entry of the program.
 * It reads in system data and user input first.
 * It then invokes MulZ to generate a schedule and send the schedule to the kernel.
 * *********************************/
int main(int argc, char* argv[])
{
	string poolIDFile = "./config_files/pool_id.txt";
	string CPUFile = "./config_files/rrp_cpus_list.txt";
	string domainFile = "./config_files/uuid_info.txt";
	if(argc >= 2)
		poolIDFile = string(argv[1]);
	if(argc >= 3)
		CPUFile = string(argv[2]);
	if(argc >= 4)
		domainFile = string(argv[3]);
	int poolID = getPoolID(poolIDFile);
	unordered_set<int> PCPUs = getPCPUs(CPUFile);
	vector<domain> domains = getDomainInfo(domainFile);
	vector<sched_entry_t> VCPUs;
	for(int i=0; i<domains.size(); i++)
	{
		cout<<"Please input the VCPU configuration file for "<<domains[i].id<<endl;
		string jsonFileName;
		cin>>jsonFileName;
		string contents = readJsonFile(jsonFileName);
		auto tempVCPUs = parseJSON(contents);
		assert(validateVCPUInfo(tempVCPUs, domains[i].vcpuNum, PCPUs));
		for(auto& vcpu : tempVCPUs)
		{
			vcpu.id = generateNewID(domains[i].id, vcpu.id);
		}
		VCPUs.insert(VCPUs.end(), tempVCPUs.begin(), tempVCPUs.end());
	}	
	vector<int> PCPUVec;
	for(auto id:PCPUs)
		PCPUVec.push_back(id);
	MulZ_ILO(VCPUs, PCPUVec, 30);
	/*
	// preparations needed for MulZ, not needed for MulZ-ILO
	// sort the vcpus by their availability factors 
	sort(VCPUs.begin(), VCPUs.end(), cmpVCPUs);
	printEntries(VCPUs);
	// initialize the PCPUs
	vector<pcpu> PCPUObjs;
	for(auto pcpuID : PCPUs)
	{
		pcpu temp;
		temp.cpu_id = pcpuID;
		temp.factor = 0;
		temp.rest = 1;
		PCPUObjs.push_back(temp);
	}
	cout<< "Number of CPUs used for schedule now:" << PCPUObjs.size()<<endl;
	*/

	/* test codes
	// test JSON reading
	string fileName = "sample.json";
	string contents = readJsonFile(fileName);	
	//cout << contents << endl;
	auto VCPUs = parseJSON(contents);
	printEntries(VCPUs);

	//test system infor retrieval
	cout << getPoolID("./config_files/pool_id.txt");
	auto result = getDomainInfo("./config_files/uuid_info.txt");
	for(auto now:result)
	{
		cout<<now.id<<","<<now.vcpuNum<<endl;
	}
	getPCPUs("./config_files/rrp_cpus_list.txt");

	// test z_approx
	test_approx(1, 15);
	test_approx(1, 2);
	test_approx(4, 5);
	return 0;	
	*/

	
}





/* ********* readJSONFile ************
 * This function reads in the contents of a JSON file 
 * and returns the contents in string.
 *
 * @param: string fileName: The name of the JSON file to be read.
 * @return: string contents: The contents inside the JSON file.
 * *********************************/
string readJsonFile(string fileName)
{
	ifstream test_file(fileName);
	string contents;
	if(!test_file.is_open())
	{
		cerr << "File "<< fileName << " cannot be opened." << endl;
		exit(EXIT_FAILURE);
	}
	contents = string((std::istreambuf_iterator<char>(test_file)), std::istreambuf_iterator<char>());
	return contents;
}

/* ********* praseJSON ************
 * This function parses the contents, given in the form of string
 * and stores the information of VCPUs into a list of sched_entry_t.
 * This function may terminate the program if the contents given are in wrong formats.
 *
 * @param: string jsonContents: The contents of a JSON file.
 * @retrun: vector<sched_entry_t>: A list of struct sched_entry_t storing the infomration of each VCPU.
 * *********************************/
vector<sched_entry_t> parseJSON(string jsonContents)
{
	Document dom;
	dom.Parse(jsonContents.c_str());
	assert(dom.IsObject()); // the document should be an object
	assert(dom.HasMember("VCPUs")); // should include member VCPUs
	const Value& VCPUList = dom["VCPUs"];
	assert(VCPUList.IsArray()); // VCPUs should be an array
	vector<sched_entry_t> VCPUs;
	for(int i=0; i<VCPUList.Size(); i++)
	{
		// read in the value in each element
		auto& VCPU = VCPUList[i];
		// validate the format of each VCPU in the file
		assert(VCPU.HasMember("ID"));		
		assert(VCPU.HasMember("WCET"));		
		assert(VCPU.HasMember("period"));		
		assert(VCPU.HasMember("PCPU"));		
		assert(VCPU["ID"].IsInt());		
		assert(VCPU["WCET"].IsInt());		
		assert(VCPU["period"].IsInt());		
		assert(VCPU["PCPU"].IsInt());		

		sched_entry_t VCPUObj;
		VCPUObj.id= to_string(VCPU["ID"].GetInt());
		VCPUObj.wcet = VCPU["WCET"].GetInt();
		VCPUObj.period= VCPU["period"].GetInt();
		VCPUObj.pcpu= VCPU["PCPU"].GetInt();
		VCPUs.push_back(VCPUObj);
	}
	return VCPUs;
}

/* ********* getPoolID ************
 * Retreives the Pool ID.
 * @param: string poolFileName: The name of the file storing pool ID.
 * @return: The pool_id of the cpu pool target scheduler controls.
 * *********************************/
int getPoolID(string poolFileName)
{
     int pool_id;
     FILE* filer = fopen(poolFileName.c_str(), "r");
     if(filer == NULL)
     {
         fprintf(stderr, "Cannot open file %s\n", poolFileName.c_str());
     }
     fscanf(filer, "%d", &pool_id);
     printf("Reading... cpupool ID is: %d\n", pool_id);
     fclose(filer);
     return pool_id;
}


/* ********* getDomainInfo ************
 * Retreives the information of domains in the target CPU pool.
 * @param: string domainsFileName: The name of the file storing domains' information.
 * @return: vector<domain> domains: A list of domain struct storing each domain's information.
 * *********************************/
vector<domain> getDomainInfo(string domainsFileName)
{
	FILE* filer = fopen(domainsFileName.c_str(), "r");
	if(filer == NULL)
         fprintf(stderr, "Cannot open file %s\n", domainsFileName.c_str());
     char *buffer = (char*)malloc(128* sizeof(char));
     vector<domain> domains;
     while(fscanf(filer, "%s", buffer)== 1)
     {
         printf("Reading .... uuid of Domain is: %s\n", buffer);
         domain now;

         now.id = string(buffer);
         // also put the vcpuNum later
         if(fscanf(filer, "%s", buffer)!=1)
         	break;
         now.vcpuNum = atoi(buffer);
         domains.push_back(now);
     }
     printf("Number of Entries are: %lu\n", domains.size());
     fclose(filer);
     return domains;
}

/* ********* getPCPUs ************
 * Retreives the information of domains in the target CPU pool.
 * @param: string CPUsFileName: The name of the file storing the CPUs in the RRP cpupool.
 * @return: A set of integers representing the ID of the PCPU in the pool.
 * *********************************/
unordered_set<int> getPCPUs(string CPUsFileName)
{
    unordered_set<int> RRPCPUs;
    FILE* filer = fopen(CPUsFileName.c_str(), "r");
    if(filer == NULL)
    {
        fprintf(stderr, "Cannot open file %s\n", CPUsFileName.c_str());
    }

    const char s[2] = ",";
    char *token;
    if(filer != NULL)
    {
        char line[20];
        while(fgets(line, sizeof(line), filer)!= NULL)
        {
            token = strtok(line, s);
            RRPCPUs.insert(atoi(token));
            token = strtok(NULL, s);
	        while(token != NULL)
	        {
	            RRPCPUs.insert(atoi(token));
	            token = strtok(NULL, s);
	        }
        }
    }
    printf("Total Number of CPUs are: %lu\n", RRPCPUs.size());
    fclose(filer);
    return RRPCPUs;
}

/* ********* validateVCPUInfo ************
 * Validates whether the VCPU information given is valid
 * @param: vector<sched_entry_t> VCPUs: The information of VCPUs read from the file.
 * @param: int vcpuNum: 				The number of VCPUs in the domain.
 * @param: unordered_set<int> PCPUs:    The list of PCPUs in the pool.
 * @return: A boolean variable indicating whether the VCPUs are valid or not.
 * *********************************/
bool validateVCPUInfo(const vector<sched_entry_t>& VCPUs, int vcpuNum, const unordered_set<int>& PCPUs)
{
	for(auto vcpu : VCPUs)
	{
		if(atoi(vcpu.id.c_str()) >= vcpuNum)
		{
			printf("The VCPU information is wrong with an invalid ID.\n");
			return false;
		}
		if(PCPUs.find(vcpu.pcpu)==PCPUs.end())
		{
			printf("The PCPU specified does not exist in the pool.\n");
			return false;
		}
	}
	return true;
}

/* ********* generateNewID ************
 * Generates a new string ID based on the domain and the VCPU
 * @param: string domainID: the ID of the domain
 * @param: string VCPUId: 	the ID of the VCPU
 * @return: A string combining both domainID and VCPUId that is unique
 * *********************************/
string generateNewID(string domainID, string VCPUId)
{
	return domainID + ":" + VCPUId;
}

/* ********* parseID ************
 * Parses the ID and retrieve the domain ID and the VCPU ID
 * @param: string IDNow: the ID of the domain
 * @param: string& domainID: the ID of the domain
 * @param: int& VCPUId:   the ID of the VCPU
 * *********************************/
void parseID(string IDNow, string& domainID, int &VCPUID)
{
    int pos = IDNow.find(":");
    domainID = IDNow.substr(0, pos);
    VPUCID = atoi(IDNow.substr(pos+1).c_str());
}

/* ************* MULZ_ILO ************
 * @param: sched_entry_t* partitions: An array of struct sched_entry_t, which stores the information of partitions.
 * @param: pcpu* pc: An array of struct pcpu.
 * @param: int time_slice_length: The length of each time slice.
 * @param: int cpu_pool_id: The id of the current cpu pool.
 * @return: unordered_map<int, vector<sched_entry_t>> schedule: The final schedule for each cpu
 * Credits: Guangli Dai, Pavan Kumar Paluri
 * ********************************/
bool MulZ_ILO(vector<sched_entry_t>& VCPUs, vector<int>& PCPUs, int time_slice_length)
{
    int factors[4]= {3,4,5,7};
    unordered_map<int, vector<string>> results;
    unordered_map<string, sched_entry_t> VCPUMap;
    for(auto v:VCPUs)
    {
        VCPUMap[v.id] = v;
    }
    for(int i=0; i<PCPUs.size(); i++)
    {
        //push in empty schedules if no partitions left
        if(VCPUs.size()==0)
        {
            vector<string> result;
            results[PCPUs[i]] = result;
            continue;
        }

        //choose a set of partitions for a certain CPU
        double max_s = 0;
        int factor_used = 0;
        unordered_set<string> chosen_ps;
        for(int factor : factors)
        {
            dp_return temp_result = dp_single(VCPUs, factor);
            if(temp_result.af_sum > max_s)
            {
                max_s = temp_result.af_sum;
                chosen_ps = temp_result.ids;
                factor_used = factor;
            }
        }

        //use partitions with id in chosen_ps to generate a schedule
        //filter used partition out from the partitions
        vector<sched_entry_t> temp_VCPUs;
        vector<sched_entry_t> VCPUs_now;
        //qDebug()<<"Chosen partitions for CPU #"<<i<<" with factor "<<factor_used<<endl;
        for(auto p : VCPUs)
        {
            if(chosen_ps.find(p.id) == chosen_ps.end())
                temp_VCPUs.push_back(p);
            else
            {
            	p.pcpu = PCPUs[i];
                VCPUs_now.push_back(p);
            }
        }
        VCPUs = temp_VCPUs;
        cout << "For CPU #" << PCPUs[i] << ": " << endl;
        printEntries(VCPUs_now);
        vector<string> result = CSG(VCPUs_now, factor_used);
        results[PCPUs[i]] = result;
    }

    for(int i=0; i<PCPUs.size(); i++)
    {
    	cout << endl << "For CPU #" << PCPUs[i] << endl;
    	for(auto id:results[PCPUs[i]])
    	{
    		cout << id << " , ";
    	}
    	cout << endl;
    	//printEntries(results[PCPUs[i].cpu_id]);
    }
    if (VCPUs.size()!=0)
        return false;

    // convert to the data structure sched_set can accept
    for(int i=0; i<PCPUs.size(); i++)
    {
        struct xen_sysctl_scheduler_op ops;
        // can we keep using the same sched_aaf?
        struct xen_sysctl_aaf_schedule sched_aaf; 
        int counter = 0;
        sched_aaf.cpu_id = PCPUs[i];
        sched_aaf.hyperperiod = results[PCPUs[i]].size()*TIME_SLICE_LENGTH;
        sched_aaf.num_schedule_entries = results[PCPUs[i]].size();
        // check num_schedule_entries fit the requirments
        for(auto id:results[PCPUs[i]])
        {
            auto temp = VCPUMap[id];
            sched_aaf.schedule[counter].wcet = TIME_SLICE_LENGTH;
            parseID(temp.id, sched_aaf.schedule[counter].domain_handle, sched_aaf.schedule[counter].vcpu_id); 
            counter ++;
        }

        cout << "------------------------------" << endl;
        cout << "CPU #" << sched_aaf.cpu_id << endl;
        // set_result = xc_sched_aaf_schedule_set(xci, cpu_pool_id, &sched_aaf[i]);
        for(int i=0; i<counter; i++)
        {
            cout << sched_aaf.schedule[i].domain_handle << " ----- " << sched_aaf.schedule[i].vcpu_id << endl;
        }

    }


    return true;
}

/* ************* dp_single ************
 * @param: vector<sched_entry_t> partition_list: An array of sched_entry_t, which stores the information of VCPUs/partitions.
 * @param: int factor: 							 The factor to be used, the number should be one of 3, 4, 5, 7
 * @return: dp_return result:					 The final schedule for a CPU and the sum of the availability factors
 * Credits: Guangli Dai, Pavan Kumar Paluri
 * ********************************/
dp_return dp_single(vector<sched_entry_t> partition_list, int factor)
{
    //approximate all partitions using the factor, note that af stays the same while WCET and period are updated so that WCET/period = aaf
    int largest_period = 1;
    for(int i=0; i<partition_list.size(); i++)
    {
        //approximates app_result = z_approx(partition_list[i].getAF(), factor);
        //partition_list[i].reset(app_result.WCET, app_result.period);
        // define z_approx so that the partitions are directly approximated
        z_approx(partition_list[i], factor);
        if(partition_list[i].period > largest_period)
            largest_period = partition_list[i].period;
    }
    //convert all wcet and period according to the largest_period
    for(int i=0; i<partition_list.size(); i++)
    {
        long long current_period = partition_list[i].period;
        long long current_WCET = partition_list[i].wcet;
        //partition_list[i].reset(current_WCET * largest_period / current_period, largest_period);
        partition_list[i].wcet = current_WCET * largest_period / current_period;
        partition_list[i].period = largest_period;
    }

    int n = partition_list.size()+1;
    int m = largest_period + 1;
    //use 2d array dp[i][j] to represent the total availability factor accommodated by partition i-1 with j* 1/largest_period space used
    vector<vector<double>> dp;
    //dp_result stores the set of ids of partitions chosen to reach dp[i][j]
    vector<vector<unordered_set<string>>> dp_result;
    //initialize the two 2D arrays
    for(int i=0; i<n; i++)
    {
        vector<double> temp_dp;
        vector<unordered_set<string>> temp_result;
        for(int j=0; j<m; j++)
        {
            temp_dp.push_back(0);
            unordered_set<string> temp;
            temp_result.push_back(temp);
        }
        dp.push_back(temp_dp);
        dp_result.push_back(temp_result);
    }

    for(int i=0; i<n; i++)
    {
       if(i==0)
            continue;
       //the index of the partition exploring now has an index of i-1
       int WCET_now = partition_list[i-1].wcet;
       double af_now = partition_list[i-1].getAF();
       string id_now = partition_list[i-1].id;
        for(int j=0; j<m; j++)
        {
            if(j==0)
                continue;
            double temp1 = dp[i-1][j];
            double temp2 = 0;
            if(j >= WCET_now)
            {
                temp2 = dp[i-1][j-WCET_now] + af_now;
            }

            if(temp1 >= temp2)
            {
                dp[i][j] = temp1;
                dp_result[i][j] = dp_result[i-1][j];
            }
            else
            {
                dp[i][j] = temp2;
                dp_result[i][j] = dp_result[i-1][j-WCET_now];
                dp_result[i][j].insert(id_now);
            }
        }
    }
    dp_return result;
    result.ids = dp_result[n-1][m-1];
    result.af_sum = dp[n-1][m-1];
    return result;
}

/* ************* z_approx ************
 * @param: sched_entry_t partition: The partition to be approximated
 * @param: int factor:				The factor to be used, the number should be one of 3, 4, 5, 7
 * @return: 						Nothing, but partition will be changed in place
 * ********************************/
void z_approx(sched_entry_t& partition, int factor)
{
	 double factor_double = static_cast<double>(factor);
	 int result_WCET = 1, result_period = 1;
	 double af = partition.getAF();
	 if(af==0)
	 {
	 	return;
	 }
	 else if (af > 0 && af < 1.0 / factor_double)
	 {
	 	int n = floor(approximate_value(log(factor_double*af)/log(0.5)));
        result_WCET = 1;
        result_period = factor * pow(2,n);
	 }
	 else if (af >= 1.0 / factor_double && af <= (factor_double - 1) / factor_double)
	 {
	 	result_period = factor;
        result_WCET = ceil(approximate_value(factor * af));	
	 }
	 else if (af > (factor_double - 1) / factor_double && af < 1)
     {
        int n = ceil(approximate_value(log(factor * (1 - af)) / log(0.5)));
        result_WCET = factor * pow(2, n) - 1;
        result_period = factor * pow(2, n);
     }
	 partition.wcet = result_WCET;
	 partition.period = result_period;
}

/* ************* approximate_value ************
 * @param:  double value: The value to be approximated
 * @return: A double value approximated to nearby values to bypass the precision problem
 * ********************************/
double approximate_value(double value)
{
	double result = floor(value);
    if(value - result > 0.9999)
        return result + 1;
    else if(value - result > 0.49999 && value - result < 0.5)
        return result + 0.5;
    else if (value - result > 0 && value - result < 0.00001)
        return result;
    return value;
}

/* ************* CSG ************
 * @param: vector<sched_entry_t> partition_list: An array of sched_entry_t, which stores the information of VCPUs/partitions.
 * @param: int factor: 							 The factor to be used, the number should be one of 3, 4, 5, 7
 * @return: vector<string> schedule:			 The schedule on the current PCPU.
 * ********************************/
vector<string> CSG(vector<sched_entry_t> partition_list, int factor)
{
    vector<string> empty_result;
    vector<string> schedule;
    unordered_set<int> avail_timeslices;
    //approximate partitions using the factor, calculate the hyper-period meanwhile
    int hyper_period = 0;
    for(int i=0; i<partition_list.size(); i++)
    {
    	z_approx(partition_list[i], factor);
        if(hyper_period==0)
            hyper_period = partition_list[i].period;
        else
        {
            hyper_period = lcm(hyper_period, partition_list[i].period);
        }
    }

    //use CSG-AZ algorithm to generate a schedule
    //initialize the schedule with empty time slices
    for(int i=0; i<hyper_period; i++)
    {
        schedule.push_back("-1");
        avail_timeslices.insert(i);
    }
    //sort the partition by their aaf!
    sort(partition_list.begin(), partition_list.end(), cmpVCPUs);

    for(sched_entry_t p : partition_list)
    {
        if(p.wcet!=1)
        {

            int delta1 = find_delta(avail_timeslices, p.period, p.wcet, avail_timeslices.size()*p.period / hyper_period - p.wcet);
            if(delta1==-1)
            {
                cout<<"Unschedulable Resource Specifications!";
                return empty_result;
            }
            //allocate time slices according to delta1
            for(int l=0; l<hyper_period / p.period; l++)
            {
                for(int k=0; k<p.wcet; k++)
                {
                    int index_now = int(floor(k*p.period / p.wcet + delta1)) % p.period + l * p.period;
                    if(avail_timeslices.find(index_now)==avail_timeslices.end())
                    {
                        cout<<"Something wrong with time slice "<<index_now;
                        return empty_result;
                    }
                    schedule[index_now] = p.id;
                    avail_timeslices.erase(index_now);
                }
            }
        }
        else
        {
            int index = -1;
            for(int time : avail_timeslices)
            {
                if(index==-1 || time < index)
                    index = time;
            }
            if(index >= p.period)
            {
                cout<<"Unschedulable Resource Specifications!";
                return empty_result;
            }
            for(int l=0; l<hyper_period/p.period; l++)
            {
                int index_now = index + l*p.period;
                schedule[index_now] = p.id;
                avail_timeslices.erase(index_now);
            }

        }
    }
    return schedule;
}


/* ********* lcm ************
 * Returns the least common multiple of integers num1 and num2.
 * *********************************/
int lcm(int a, int b)
{
    int mul = a*b;
    int larger = max(a, b);
    int smaller = min(a, b);
    while(smaller != 0)
    {
        int temp = larger % smaller;
        larger = smaller;
        smaller = temp;
    }
    mul /= larger;
    return mul;
}

/* ********* cmpVCPUs ************
 * Compares the availability factors of the two VCPUs
 * @param: sched_entry_t a: the first VCPU
 * @param: sched_entry_t b: the second VCPU
 * @return: A boolean variable indicating whether the first VCPU has a larger availability factor
 * *********************************/
bool cmpVCPUs(const sched_entry_t& a, const sched_entry_t& b)
{
	return ((double)a.wcet/a.period) >= ((double)b.wcet/b.period);
}

/* ************* find_delta ************
 * @param: unordered_set<int> avail_timeslices: The available time slices
 * @param: int p: 							 	The approximated period
 * @param: int q: 							 	The approximated wcet	
 * @param: int q_left: 							The time slices left in each p after using q
 * @return: int delta1:			 				The right shifting index of the time slice sets assigned to this partition
 * ********************************/
int find_delta(unordered_set<int> avail_timeslices, int p, int q, int q_left)
{
    vector<int> standard_p1, standard_p2;
    for(int k=0; k<q; k++)
    {
        int t_now = int(floor(k*p/q)) % p;
        standard_p1.push_back(t_now);
    }
    for(int k=0; k<q_left; k++)
    {
        int t_now = int(floor(k*p/q_left)) % p;
        standard_p2.push_back(t_now);
    }

    //find potential delta1 (delta is always smaller than p)
    for(int delta1 = 0; delta1<p; delta1++)
    {
        if(check_delta(avail_timeslices, standard_p1, delta1, p))
        {
            if(standard_p2.size()==0)
                return delta1;
            //check delta2, if compatible, return delta1
            unordered_set<int> temp_avail = avail_timeslices;
            unordered_set<int> partition1_set;
            for(int time : standard_p1)
            {
                temp_avail.erase((time + delta1)%p);
            }
            for(int delta2 = 0; delta2<p; delta2++)
            {
                if(check_delta(temp_avail, standard_p2, delta2, p))
                    return delta1;
            }
        }
    }
    cout<<"Cannot find delta!"<<endl;
    for(auto time : avail_timeslices)
        cout<<time<<endl;
    cout<<p<<","<<q<<","<<q_left<<standard_p2.size()<<endl;
    return -1;
}

/* ************* check_delta ************
 * @param: unordered_set<int> avail_set: 		The available time slices
 * @param: vector<int> standard_p: 				The time slices of a standard regular partition
 * @param: int delta:							The right shifting index of the current partition	
 * @param: int p: 								The period of the current partition
 * @return: bool result:			 			True if the delta is valid, false otherwise
 * ********************************/
bool check_delta(unordered_set<int> avail_set, vector<int> standard_p, int delta, int p)
{
    for(int time : standard_p)
    {
        int time_now = (time + delta)%p;
        if(avail_set.find(time_now) == avail_set.end())
            return false;
    }
    return true;
}

// test function that prints out the info of a list of sched_entry_t
void printEntries(vector<sched_entry_t> target)
{
	for(auto sched:target)
	{
		printf("ID: %s, wcet: %d, period: %d, pcpu: %d\n", \
			sched.id.c_str(), sched.wcet, sched.period, sched.pcpu);
	}
}

// unit test function of z_approx
void test_approx(int wcet, int period)
{
	sched_entry_t now;
	now.wcet = wcet;
	now.period = period;
	int factors[4] = {3,4,5,7};
	cout << "Target: " << now.wcet << " / " << now.period << endl;
	for(int i=0; i<4; i++)
	{
		now.wcet = wcet;
		now.period = period;
		z_approx(now, factors[i]);
		cout<<"Using factor " << factors[i] << " results: " << now.wcet << " / " << now.period << endl;
	}

}


