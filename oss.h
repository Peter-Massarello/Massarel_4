#define _XOPEN_SOURCE 700
#define MAX 18
#define NUM_SEMS 1
#define QUANTUM 10

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

typedef struct {
	bool blocked;
	bool waiting;
	bool early_term;

	int pid;
	int index;

	int proc_type;
	
	unsigned int prev_burst;
	unsigned int start_sec;
	unsigned int start_nano;
	unsigned int blocked_until_seconds;
	unsigned int blocked_until_nano;
	float cpu_time;
	float system_time;
} pcb_t;

typedef struct {
	int user_count;

	unsigned int clock_seconds;
	unsigned int clock_nano;
	unsigned int next_fork_sec;			
	unsigned int next_fork_nano;

	int current_pid;
	int current_index;

	pcb_t pcb_arr[MAX];
} shmptr_t;

/*typedef struct {

	bool wait_on_oss;				//flag that will be used for oss.c to tell the user to go			
	bool blocked;					//flag to return to oss.c that the process has been blocked
	bool early_term;				//flag to return to oss.c that the process has terminated early

	int pid;					//pid of process in question 
	int index;					//index of process in question (may not even need this idk yet)

	int proc_type;

	unsigned int prev_burst;				//time elapsed in previous burst in nano
	unsigned int start_sec;					//start time of first dispatch, used in total system time elapsed calculation
	unsigned int start_nano;				//start time of first dispatch, used in total system time elapsed calculation
	unsigned int blocked_until_seconds;			//used to hold the time that marks whem a process can be set to ready again 
	unsigned int blocked_until_nano;		//used to hold the time that marks whem a process can be set to ready again
	float system_time;				//total time the process has existed in the system in ms
	float cpu_time;					//total time that the process has actively been "doing work" in ms

} pcb_t; //process control block

typedef struct {

	int user_count;						// used by oss.c for keeping track of the number of processes in the system

	int current_pid;					//holds the dispatched processes pid
	int current_index;				// holds the index of the pcb for that given pid

	//keeps track of various times
	unsigned int next_fork_sec;			//tells oss.c when the earliest it can fork another user process is
	unsigned int next_fork_nano;		//tells oss.c when the earliest it can fork another user process is	
	unsigned int clock_nano;			//holds nanosecond component of the simulated clock
	unsigned int clock_seconds;			//holds seconds component of the simulated clock


	pcb_t pcb_arr[MAX];	// array of PCBs   (holds stuff regaurding the currently running processes

} shmptr_t;*/

void write_to_log(char* log);
void create_file(char* name);
void help_menu();
void sig_handler();
void cleanup();
bool create_shared_memory(key_t shmkey, key_t semkey);
void init_shm();
void init_pcb(int i);
void init_sems();
void child_handler();
void init_table();
void set_fork();
int get_num_of_ready();
int get_num_of_users();
void spawn();
int find_next_user();
void fork_user(int user_id);
void move_blocked();
int get_index(int pid);
void reset_nano();
void sem_wait(int sem);