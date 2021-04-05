#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/sem.h>

#define MAX 18
#define NUM_SEMS 1
#define QUANTUM 10
#define IO 1
#define CPU 2

typedef struct {

	bool wait_on_oss;				//flag that will be used for oss.c to tell the user to go			
	bool blocked;					//flag to return to oss.c that the process has been blocked
	bool early_term;				//flag to return to oss.c that the process has terminated early

	int pid;					//pid of process in question 
	int index;					//index of process in question (may not even need this idk yet)

	int proc_type;

	unsigned int prev_burst;				//time elapsed in previous burst in nano
	unsigned int start_sec;					//start time of first dispatch, used in total system time elapsed calculation
	unsigned int start_nano;				//start time of first dispatch, used in total system time elapsed calculation
	unsigned int blocked_until_sec;			//used to hold the time that marks whem a process can be set to ready again 
	unsigned int blocked_until_nano;		//used to hold the time that marks whem a process can be set to ready again
	float system_time;				//total time the process has existed in the system in ms
	float cpu_time;					//total time that the process has actively been "doing work" in ms

} pcb; //process control block

typedef struct {

	int user_count;						// used by oss.c for keeping track of the number of processes in the system

	int scheduled_pid;					//holds the dispatched processes pid
	int scheduled_index;				// holds the index of the pcb for that given pid

	//keeps track of various times
	unsigned int next_fork_sec;			//tells oss.c when the earliest it can fork another user process is
	unsigned int next_fork_nano;		//tells oss.c when the earliest it can fork another user process is	
	unsigned int clock_nano;			//holds nanosecond component of the simulated clock
	unsigned int clock_seconds;			//holds seconds component of the simulated clock


	pcb pcb_arr[MAX];	// array of PCBs   (holds stuff regaurding the currently running processes

} memory_container; //used to hold anything that we put in shared memory


int create_shm();
int create_sem();
void cleanup();
void signal_handler();
void sem_wait(int sem);
void sem_signal();
void display_help();
void child_handler();
void fork_user(int);
void init_table();
void init_sems();
void init_shm();
void init_pcb(int);
void write_to_log(char*);
int get_next_location();
void clear_blocked();
void set_fork();
void spawn();
int get_user_count();
int get_index_by_pid(int);
void normalize_clock();
void normalize_fork();
void kill_pids();
int count_ready();
void normalize_time();
