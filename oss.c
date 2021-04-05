#include "oss.h"
extern errno;

int shm_id;
int sem_id;
int proc_used[MAX];
shmptr_t* shm_ptr;
FILE* file_ptr;
char log_buffer[200];
int ready_pids[MAX];
int blocked_queue[MAX];
int ready_in, ready_out, blocked_in, blocked_out;
int temp_pid;
int line_count = 0;

void init_table();

int main(int argc, char* argv[]) {

//*************************************************************************************************
//
//	Signal Handling And Variable Initialization
//
//*************************************************************************************************
	srand(time(NULL));

	signal(SIGKILL, signal_handler);
	signal(SIGALRM, signal_handler);
	signal(SIGINT, signal_handler);

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = child_handler;
	sigaction(SIGCHLD, &sa, NULL);

	char log_name[25];
	int max_time = 100;
	int opt;
	int temp, ready;
	unsigned int nano_change;
	int rand_time;
	char* default_log = "logfile";
	ready_in = ready_out = blocked_in = blocked_out = 0;
	bool new_name = false;

//*************************************************************************************************
//
//	GetOpt
//
//*************************************************************************************************

	while ((opt = getopt(argc, argv, "hs:l:")) != -1) {
		switch (opt) {
		case 'h':
			display_help();
			break;
		case 's':
			max_time = atoi(optarg);
			break;
		case 'l':
			if (strlen(optarg) > 19) {
				printf("Given string too long, returning to default\n\n");
				new_name = false;
			}
			else {
				strcpy(log_name, optarg);
				strcat(log_name, ".txt");
				new_name = true;
			}
			break;
		default:
			printf("Error: Invalid Option, please us ./oss -h for help\n\n");
			display_help();
			exit(0);
		}
	}

	if (new_name == false)
	{
		strcpy(log_name, default_log);
		strcat(log_name, ".txt");
	}

	file_ptr = fopen(log_name, "a");

	if ((create_shm() == -1) || (create_sem() == -1)) 
	{
		cleanup();
		exit(0);
	}

	init_table();
	init_sems();
	init_shm();

	set_fork();

//*************************************************************************************************
//
//	Main While Loop
//
//*************************************************************************************************

	int ready_count;
	int temp_pid;
	while (1) {

		sleep(1);
		ready_count = count_ready();

		shm_ptr->user_count = get_user_count();

		if (shm_ptr->user_count < MAX) {

			if ((shm_ptr->next_fork_sec == shm_ptr->clock_seconds) &&
				(shm_ptr->next_fork_nano <= shm_ptr->clock_nano)) 
			{
				spawn();
				set_fork();
			}
			else if (shm_ptr->next_fork_sec < shm_ptr->clock_seconds) 
			{
				spawn();
				set_fork();
			}
		}

		clear_blocked();

		if (ready_count > 0)
		{
			temp_pid = ready_pids[ready_out];
			temp = get_index_by_pid(temp_pid);

			shm_ptr->scheduled_index = temp;

			rand_time = rand() % (10001);
			shm_ptr->clock_nano += rand_time;
			normalize_clock();

			sprintf(log_buffer, "OSS: Dispatching PID %d\n", temp_pid);
			write_to_log(log_buffer);
	
			shm_ptr->scheduled_pid = temp_pid;

			ready_pids[ready_out] = 0;
			ready_out = (ready_out + 1) % MAX;

			sem_wait(sem_id);
		
			if (shm_ptr->pcb_arr[temp].early_term) 
			{
			
				sprintf(log_buffer, "OSS: PID: %d has terminated early after spending %d ns in the cpu/system\n", temp_pid, shm_ptr->pcb_arr[temp].prev_burst);
				write_to_log(log_buffer);
				proc_used[temp] = 0;
			}
			else if (shm_ptr->pcb_arr[temp].blocked) 
			{
				blocked_queue[blocked_in] = temp_pid;
				blocked_in = (blocked_in + 1) % MAX;
				sprintf(log_buffer, "OSS: PID: %d has been blocked and is going into blocked queue. Last Burst: %d ns\n", temp_pid, shm_ptr->pcb_arr[temp].prev_burst);
				write_to_log(log_buffer);
				sprintf(log_buffer, "OSS: entire quantum not used by process, PID was blocked before it could use it all\n");
				write_to_log(log_buffer);
			}
			else 
			{		
				proc_used[temp] = 0;
				sprintf(log_buffer, "OSS: PID: %d has finished. CPU time: %.2f ms System time: %.2f ms Last burst: %d ns\n", temp_pid, shm_ptr->pcb_arr[temp].cpu_time, shm_ptr->pcb_arr[temp].system_time, shm_ptr->pcb_arr[temp].prev_burst);
				write_to_log(log_buffer);
			}
		}
		shm_ptr->clock_seconds += 1;
		nano_change = rand() % 1001;
		shm_ptr->clock_nano += nano_change;
		normalize_clock();
	}
	cleanup();
	return 0;
}

int count_ready() {
	int count = 0;
	int i;
	for (i = 0; i < MAX; i++) {
		if (ready_pids[i] > 0)
			count++;
	}
	return count;
}

void display_help() {
	printf("HELP MENU\n\n");
	printf("The goal of this homework is to learn about process scheduling inside an operating system. You will work on the specified\n");
	printf("scheduling algorithmand simulate its performance.\n");
	printf("PROGRAM OPTIONS:\n\n");
	printf("OPTION [-h]:\n");
	printf("           When called, will display help menu\n\n");
	printf("OPTION [-s][argument]:\n");
	printf("           When called, will allow you to change the length of time program runs for. (in seconds)\n\n");
	printf("OPTION [-l][argument]:\n");
	printf("           When called, will allow you to replace the default logfile.txt with your own name.\n");
	printf("           If the log file goes over 1000 lines, which it will with the default time limit, it gets terminated early\n\n");
	exit(0);
}

void normalize_clock() {
	unsigned int nano = shm_ptr->clock_nano;
	int sec;
	if (nano >= 1000000000) {
		shm_ptr->clock_seconds += 1;
		shm_ptr->clock_nano -= 1000000000;
	}
}

void normalize_fork() {
	unsigned int nano = shm_ptr->next_fork_nano;
	int sec;
	if (nano >= 1000000000) {
		shm_ptr->next_fork_sec += 1;
		shm_ptr->next_fork_nano -= 1000000000;
	}
}


void set_fork() {
	shm_ptr->next_fork_sec = (rand() % 2) + shm_ptr->clock_seconds;
	shm_ptr->next_fork_nano = (rand() % 1001) + shm_ptr->clock_nano;
	normalize_fork();
}

int create_shm() {
	key_t key = ftok("Makefile", 'a');
	if ((shm_id = shmget(key, (sizeof(pcb) * MAX) + sizeof(memory_container), IPC_CREAT | 0666)) == -1) {
		errno = 5;
		perror("oss.c: Error: Could not create shared memory in create_shm()");
		return -1;
	}
	if ((shm_ptr = (memory_container*)shmat(shm_id, 0, 0)) == (memory_container*)-1) {
		errno = 5;
		perror("oss.c: Error: Could not attach shared memory in create_shm()");
		return -1;
	}

	return 0;
}

int create_sem() {
	key_t key = ftok(".", 'a');
	if ((sem_id = semget(key, NUM_SEMS, IPC_CREAT | 0666)) == -1) {
		errno = 5;
		perror("oss.c: Error: Could not create semaphores in create_sem()");
		return -1;
	}
	return 0;
}

void sem_wait(int sem) {
	struct sembuf op;
	op.sem_num = sem;
	op.sem_op = -1;
	op.sem_flg = 0;
	semop(sem_id, &op, 1);
}



void write_to_log(char* string) {
	fputs(string, file_ptr);
	line_count++;
	if (line_count >= 1000)
	{
		printf("LogFile Limit exceeded, exiting...\n\n");
		cleanup();
		exit(0);
	}
}


void init_sems() {
	//sets the initial values in the semaphores
	semctl(sem_id, 0, SETVAL, 0);
}

void init_table() {
	for (int i = 0; i < MAX; i++)
	{
		proc_used[i] = 0;
		ready_pids[i] = 0;
	}
}

void init_pcb(int index) {
	shm_ptr->pcb_arr[index].blocked = false;
	shm_ptr->pcb_arr[index].early_term = false;
	shm_ptr->pcb_arr[index].wait_on_oss = true;
	shm_ptr->pcb_arr[index].start_nano = 0;
	shm_ptr->pcb_arr[index].start_sec = 0;
	shm_ptr->pcb_arr[index].cpu_time = 0;
	shm_ptr->pcb_arr[index].system_time = 0;
	shm_ptr->pcb_arr[index].prev_burst = 0;
	shm_ptr->pcb_arr[index].index = index;

}

void init_shm() {
	shm_ptr->user_count = 0;
	shm_ptr->clock_nano = 0;
	shm_ptr->clock_seconds = 0;
	shm_ptr->scheduled_pid = 0;
	shm_ptr->scheduled_index = -1;

	for (int i = 0; i < MAX; i++)
		init_pcb(i);
}

int get_next_location() {
	for (int i = 0; i < MAX; i++) 
	{
		if (proc_used[i] == 0)
			return i;
	}
	return -1;
}

int get_user_count() {
	int total = 0;
	for (int i = 0; i < MAX; i++)
	{
		if (ready_pids[i] != 0)
			total++;
	}
	for (int i = 0; i < MAX; i++) 
	{
		if (blocked_queue[i] != 0)
			total++;
	}
	return total;
}

void fork_user(int index) {

	int pid;

	if ((pid = fork()) == -1)
	{
		errno = 5;
		perror("oss.c: Error: Could not for child in fork_user()");
		cleanup();
		exit(1);
	}
	else 
	{
		if (pid != 0) 
		{
			shm_ptr->pcb_arr[index].pid = pid;
			proc_used[index] = 1;
			init_pcb(index);
			ready_pids[ready_in] = shm_ptr->pcb_arr[index].pid;
			sprintf(log_buffer, "OSS: PID: %d was forked and placed in the ready queue\n", shm_ptr->pcb_arr[index].pid);
			write_to_log(log_buffer);
			ready_in = (ready_in + 1) % MAX;
		}
		else
		{
			execl("./uproc", "./uproc", (char*)0);
		}
	}
}

void spawn() {
	int temp;
	temp = get_next_location();
	fork_user(temp);
}

int get_index_by_pid(int pid) {
	for (int i = 0; i < MAX; i++) {
		if (shm_ptr->pcb_arr[i].pid == pid) 
			return shm_ptr->pcb_arr[i].index;
	}
	return -1;
}

void clear_blocked() {
	int pid = blocked_queue[blocked_out];
	int i = get_index_by_pid(pid);
	if (pid != 0) {
		if ((shm_ptr->pcb_arr[i].blocked_until_sec == shm_ptr->clock_seconds) && 
			(shm_ptr->pcb_arr[i].blocked_until_nano <= shm_ptr->clock_nano))
		{
			blocked_queue[blocked_out] = 0;
			blocked_out = (blocked_out + 1) % MAX;
			ready_pids[ready_in] = pid;
			ready_in = (ready_in + 1) % MAX;
		}
		else if (shm_ptr->next_fork_sec < shm_ptr->clock_seconds) 
		{
			blocked_queue[blocked_out] = 0;
			blocked_out = (blocked_out + 1) % MAX;
			ready_pids[ready_in] = pid;
			ready_in = (ready_in + 1) % MAX;

		}
	}
}

void cleanup() {
	kill_pids();
	sleep(1);
	shmdt(shm_ptr);
	shmctl(shm_id, IPC_RMID, NULL);
	semctl(sem_id, 0, IPC_RMID, NULL);
	fclose(file_ptr);
}

void kill_pids() {
	int i;
	for (i = 0; i < MAX; i++) {
		if (blocked_queue[i] != 0) {
			kill(blocked_queue[i], SIGKILL);
		}
	}
	for (i = 0; i < MAX; i++) {
		if (ready_pids[i] != 0) {
			kill(ready_pids[i], SIGKILL);
		}
	}
}

void signal_handler() {
	cleanup();
	sleep(1);
	exit(0);
}

void child_handler() {
	pid_t pid;
	while ((pid = waitpid((pid_t)(-1), 0, WNOHANG)) > 0) {
	}

}