#include <cstdlib> 
#include <ctime> 
#include <string>
#include <iostream>
#include <time.h>
#include <algorithm>
#include <cmath>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <climits>

#include "BinSearchInterface.h"
#include "BFSBinarySearch.h"
#include "DFSBinarySearch.h"
#include "BinarySearch.h"
#include "LinearSearch.h"
//#include "vEBBinarySearch.h"

using namespace std;

const int NUM_EXPERIMENTS = 100;
const int RUN_TIMES = 10;

timespec startTime, endTime;
double timeDiff;

int getRandomNumber(int low, int high, int seed);
void fillArrayWithRandom(int * array, int size, int low, int high, int seed);
void experimentLinearSearch(int elem);
void experimentBasicAlgorithm(int elem);
void experimentBFS(int elem);
void experimentDFS(int elem);
void experimentVEB(int elem);

void create_all_data_structures(BinSearchInterface *algo_arrray, int *array, int arrSize);
long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
		     int cpu, int group_fd, unsigned long flags);
int make_perf_event(int conf, int type);
void perf_event_array(int conf_array[], int type_array[], int* fd_array, int nStats);
void perf_event_reset(int fd_array[], int nStats);
void perf_event_enable(int fd_array[], int nStats);
void perf_event_disable(int fd_array[], int nStats);
void read_all(long long stat_row[], int* fd_array, int nStats);

const int nAlgos = 3;
const char *algo_labels[nAlgos] = {"Linear search", "BFS", "DFS"};
const string output_dir = "Measurements";

int main(int argc, char **argv)
{
  LinearSearch ls = LinearSearch();
  BinarySearch inorder = BinarySearch();
  BFSBinarySearch bfs = BFSBinarySearch();
  DFSBinarySearch dfs = DFSBinarySearch();
  //vEBBinarySearch veb = vEBBinarySearch();
  BinSearchInterface *algo_array[nAlgos] = {&ls, &bfs, &dfs};
  

  int conf_array[] = {PERF_COUNT_HW_BRANCH_MISSES,
		      PERF_COUNT_HW_INSTRUCTIONS,
		      PERF_COUNT_SW_TASK_CLOCK,
		      PERF_COUNT_SW_CPU_CLOCK,
		      PERF_COUNT_HW_CACHE_REFERENCES,
		      PERF_COUNT_HW_CACHE_MISSES};
  int HW = PERF_TYPE_HARDWARE;
  int SW = PERF_TYPE_SOFTWARE;
  int type_array[] = {HW,
		      HW,
		      SW,
		      SW,
		      HW,
		      HW};
  const int nStats = sizeof(conf_array)/sizeof(int);
  const string conf_labels[nStats] = {"Branch misses.csv",
				      "Instructions.csv",
				      "Task clock.csv",
				      "CPU clock.csv",
				      "Cache refs.csv",
				      "Cache misses.csv"};
  int fd_array[nStats];
  perf_event_array(conf_array, type_array, fd_array, nStats);

  long long stat_array[nAlgos][RUN_TIMES][nStats];
	

  FILE *files[nStats];
  {int i; for (i=0; i<nStats; i++) {
      files[i] = fopen((output_dir + "/" + conf_labels[i]).c_str(), "w");
      fprintf(files[i], "Array size,");
      int j; for (j=0; j<nAlgos; j++) {
	fprintf(files[i], "%s,", algo_labels[j]);
      }
      fprintf(files[i], "\n");
    }}

	int arrSize, low, high, searchFor;
	for (int i = 0; i < NUM_EXPERIMENTS; i++) {
		//cout << "Experiment " << i << endl;
		
		// Create random array
		arrSize = i+1;
		
		//cout << "Arrsize " << arrSize << endl;
		
		high = 100;
		low = 1;
		int array[arrSize];
		fillArrayWithRandom(array, arrSize, low, high, i+1);
		// Sort array
		sort(array, array + arrSize);
		
		// Set up algorithms
		{int i;
		 for (i=0; i<nAlgos; i++) {
		   (*algo_array[i]).createDataStructure(array, arrSize);
		 }}
		
		// Repeat experiments
		for (int j = 0; j < RUN_TIMES; j++) {
			searchFor = getRandomNumber(low, high, INT_MAX - j);	
			
			int oldRes = -1;
			int newRes;
			
			// Perform experiments
			{int iAlg;
			  for (iAlg=0; iAlg<nAlgos; iAlg++){
			    perf_event_reset(fd_array, nStats);
			    // Start all the stat-counters:
			    perf_event_enable(fd_array, nStats);

			    // Perform the binary search:
			    newRes = (*algo_array[iAlg]).binSearch(searchFor);

			    // Stop all the stat-counters:
			    perf_event_disable(fd_array, nStats);
			    
			    // Check result
			    if (oldRes != -1 && oldRes != newRes) {
					printf("Wrong result. prev-alg:%s, this-alg:%s ArrSize %d, searchFor %d\n", 
						algo_labels[iAlg-1], algo_labels[iAlg], arrSize, searchFor); 			
				}
				oldRes = newRes;
			    
			    //printf("iAlg=%d done", iAlg); // TODO: remove print
			    // Store the stats in the stat_array
			    read_all(stat_array[iAlg][j], fd_array, nStats);
			  }}
		}

		// Loop through all the implementations and calculate the average of every stat for searches with this array size.
		{for (int iStat = 0; iStat<nStats; iStat++){
		    fprintf(files[iStat], "%d,",arrSize);
		    for (int iAlg=0; iAlg<nAlgos; iAlg++){
		      long long stat_sum = 0;
		      for (int j=0; j<RUN_TIMES; j++){
			stat_sum += stat_array[iAlg][j][iStat];
			//cout << stat_sum << endl;
		      }
		      long long stat_avg = stat_sum / RUN_TIMES;
		      fprintf(files[iStat], "%lld,", stat_avg);
		    }
			fprintf(files[iStat], "\n");
		  }}
	}
	
	// TODO: Perhaps close the fd_array and files...
	return 0;
}

int getRandomNumber(int low, int high, int seed) {
	srand(seed + (unsigned) time(0));
	return rand() % ( (high+1) - low ) + low;
}

void fillArrayWithRandom(int * array, int size, int low, int high, int seed)
{
	srand(seed + (unsigned) time(0));
    for(int i=0; i < size; i++){ 
        array[i] = rand() % ( (high+1) - low ) + low;
	} 
}

/*
void create_all_data_structures(BinSearchInterface *algo_array, int *array, int arrSize) {
  int i;
  for (i=0; i<nAlgos; i++){
    (*algo_array[i]).createDataStructure(array, arrSize);
  }
}
*/


long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
		int cpu, int group_fd, unsigned long flags)
{
  int ret;

  ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
		group_fd, flags);
  return ret;
}

int make_perf_event(int conf, int type){
  struct perf_event_attr pe;
  int fd;

  memset(&pe, 0, sizeof(struct perf_event_attr));
  pe.type = type;
  pe.size = sizeof(struct perf_event_attr);
  pe.config = conf;
  pe.disabled = 1;
  pe.exclude_kernel = 1;
  pe.exclude_hv = 1;

  fd = perf_event_open(&pe, 0, -1, -1, 0);
  if (fd == -1) {
    fprintf(stderr, "Error opening leader %llx\n", pe.config);
    exit(EXIT_FAILURE);
  }
  return fd;
}

void perf_event_array(int conf_array[], int type_array[], int* fd_array, int nStats){
  int i;
  for (i=0; i<nStats; i++) {
    fd_array[i] = make_perf_event(conf_array[i], type_array[i]);
  }
}

void perf_event_reset(int fd_array[], int nStats){
  int i;
  for (i=0; i<nStats; i++) {
    ioctl(fd_array[i], PERF_EVENT_IOC_RESET, 0);
  }
}

void perf_event_enable(int fd_array[], int nStats){
  int i;
  for (i=0; i<nStats; i++) {
    ioctl(fd_array[i], PERF_EVENT_IOC_ENABLE, 0);
  }
}

void perf_event_disable(int fd_array[], int nStats){
  int i;
  for (i=0; i<nStats; i++) {
    ioctl(fd_array[i], PERF_EVENT_IOC_DISABLE, 0);
  }
}

void read_all(long long stat_row[], int* fd_array, int nStats){
  int i;
  for (i=0; i<nStats; i++){
    long long stat = 0;
    read(fd_array[i], &stat, sizeof(long long));
    stat_row[i] = stat;
    //fprintf(file, "%lld,", stat);
  }
  //fprintf(file, "\n");
}
