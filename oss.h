#define _XOPEN_SOURCE 700

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

	bool wait_on_oss;					
	bool blocked;				
	bool early_term;				

	int pid;				
	int index;				

	int proc_type;

	unsigned int prev_burst;			
	unsigned int start_sec;					
	unsigned int start_nano;				
	unsigned int blocked_until_sec;			
	unsigned int blocked_until_nano;		
	float system_time;				
	float cpu_time;				

} pcb; 

typedef struct {

	int user_count;						

	int scheduled_pid;					
	int scheduled_index;			

	unsigned int next_fork_sec;			
	unsigned int next_fork_nano;		
	unsigned int clock_nano;			
	unsigned int clock_seconds;			


	pcb pcb_arr[MAX];	

} memory_container; 


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
