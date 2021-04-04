#include "oss.h"

extern int errno;
int shmid, sem_id;
int proc_used[MAX];		
int ready_pids[MAX];
int blocked_queue[MAX];
FILE* fp;
int ready_in, ready_out, blocked_in, blocked_out, temp_pid;

shmptr_t* shmptr;
char* log_name = "logfile"; // Default
char log_buf[200];
int alarm_time = 100; // Default

void kill_pids();

int main(int argc, char* argv[]) {

	//*************************************************************************************************
	//
	//	Signal Handling And Variable Initialization
	//
	//*************************************************************************************************

	signal(SIGINT, sig_handler);
	signal(SIGKILL, sig_handler);
	signal(SIGALRM, sig_handler);

	srand(time(NULL));

	ready_in = ready_out = blocked_in = blocked_out = 0;

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = child_handler;
	sigaction(SIGCHLD, &sa, NULL);

	//*************************************************************************************************
	//
	//	GetOpt
	//
	//*************************************************************************************************

	int opt;
	char* opt_buf;
	bool file_given = false;

	if (argc == 1)
	{
		system("clear");
		errno = 1;
		perror("oss: Error: Program called without agruments, use ./monitor -h for help");
		return 0;
	}

	//system("clear");
	while ((opt = getopt(argc, argv, "hs:l:")) != -1)
	{
		switch (opt)
		{
		case 'h':
			help_menu();
			break;
		case 's':
			
		case 'l':
			opt_buf = optarg;
			if (strlen(opt_buf) > 20)
			{
				printf("File name given is too long, defaulting to logfile.txt\n");
				create_file(log_name);
			}
			else
			{
				log_name = opt_buf;
				create_file(log_name);
			}
			file_given = true;
			break;
		default:
			errno = 1;
			perror("monitor: Error: Wrong option given");
			break;
		}
	}

	if (!file_given)
		create_file(log_name);

	//*************************************************************************************************
	//
	//	Generating Shared memory
	//
	//*************************************************************************************************
	
	key_t shmkey = ftok("./README.md", 'a');
	key_t semkey = ftok("Makefile", 'a');

	printf("before initialization\n");

	if (!create_shared_memory(shmkey, semkey))
		cleanup();

	printf("Initializing now\n");

	init_table();
	init_sems();
	init_shm();

	printf("Out of initialize\n");
	set_fork();
	
	/*while (1) {
		sleep(1);

		int ready_count = get_num_of_ready();
		shmptr->user_count = get_num_of_users();
		int user_count_debug = shmptr->user_count;

		if (shmptr->user_count < MAX)
		{
			// If enough time has passed for next process to be created.
			if (shmptr->next_fork_sec == shmptr->clock_seconds)
			{
				if (shmptr->next_fork_nano <= shmptr->next_fork_sec)
				{
					spawn();
					set_fork();
				}
			}
			else if (shmptr->next_fork_sec < shmptr->clock_seconds)
			{
				spawn();
				set_fork();
			}

			move_blocked();

			if (ready_count > 0)
			{
				temp_pid = ready_pids[ready_out];
				int temp = get_index(temp_pid);

				shmptr->current_index = temp;

				int rand_time = rand() % ((10001) - 100) + 100;
				shmptr->clock_nano += rand_time;
				reset_nano();

				//sprintf(log_buf, "OSS: Dispactching PID %d", temp_pid);
				//write_to_log(log_buf);

				shmptr->current_pid = temp_pid;
				ready_pids[ready_out] = 0;
				ready_out = (ready_out + 1) % MAX;

				sem_wait(sem_id);

				if (shmptr->pcb_arr[temp].early_term)
				{
					//sprintf(log_buf, "OSS: PID: %d has terminated early after spending %d ns in the cpu/system\n", temp_pid, shmptr->pcb_arr[temp].prev_burst);
					//write_to_log(log_buf);
					proc_used[temp] = 0;
				}
				else if (shmptr->pcb_arr[temp].blocked)
				{
					blocked_queue[blocked_in] = temp_pid;
					blocked_in = (blocked_in + 1) % MAX;
					//sprintf(log_buf, "OSS: PID: %d has been blocked and is going into blocked queue. Last Burst: %d ns\n", temp_pid, shmptr->pcb_arr[temp].prev_burst);
					//write_to_log(log_buf);
				}
				else
				{
					proc_used[temp] = 0;
					//sprintf(log_buf, "OSS: PID: %d has finished. CPU time: %.2f ms System time: %.2f ms Last burst: %d ns\n", temp_pid, shmptr->pcb_arr[temp].cpu_time, shmptr->pcb_arr[temp].system_time, shmptr->pcb_arr[temp].prev_burst);
					//write_to_log(log_buf);
				}
			}
			shmptr->clock_seconds += 1;
			int nano_add = (rand() % 1001);
			shmptr->clock_nano += nano_add;
			reset_nano();
		}
		
	}*/

	while (1) {

		sleep(1);
		int ready_count = get_num_of_ready();

		shmptr->user_count = get_num_of_users();

		if (shm_ptr->user_count < MAX) {

			if (shm_ptr->next_fork_sec == shm_ptr->clock_seconds) {
				if (shm_ptr->next_fork_nano <= shm_ptr->clock_nano) {
					spawn();
					set_fork();

				}
			}
			else if (shm_ptr->next_fork_sec < shm_ptr->clock_seconds) {
				spawn();
				set_fork();
			}
		}

		purge_blocked();

		if (ready_count > 0) {

			temp_pid = ready_pids[ready_out];
			temp = get_index_by_pid(temp_pid);
			shm_ptr->scheduled_index = temp;

			rand_time = rand() % ((10000 + 1) - 100) + 100;
			shm_ptr->clock_nano += rand_time;
			normalize_clock();

			sprintf(log_buffer, "OSS: Dispatching PID %d to do work\n", temp_pid);
			log_string(log_buffer);

			shm_ptr->scheduled_pid = temp_pid;

			ready_pids[ready_out] = 0;
			ready_out = (ready_out + 1) % MAX;

			sem_wait(sem_id);

			if (shm_ptr->pcb_arr[temp].early_term) {
				sprintf(log_buffer, "OSS: PID: %d has terminated early after spending %d ns in the cpu/system\n", temp_pid, shm_ptr->pcb_arr[temp].prev_burst);
				log_string(log_buffer);
				proc_used[temp] = 0;
			}
			else if (shm_ptr->pcb_arr[temp].blocked) {
				blocked_queue[blocked_in] = temp_pid;
				blocked_in = (blocked_in + 1) % MAX;
				sprintf(log_buffer, "OSS: PID: %d has been blocked and is going into blocked queue. Last Burst: %d ns\n", temp_pid, shm_ptr->pcb_arr[temp].prev_burst);
				log_string(log_buffer);
				sprintf(log_buffer, "OSS: entire quantum not used by process, PID was blocked before it could use it all\n");
				log_string(log_buffer);
			}
			else {
				proc_used[temp] = 0;
				sprintf(log_buffer, "OSS: PID: %d has finished. CPU time: %.2f ms System time: %.2f ms Last burst: %d ns\n", temp_pid, shm_ptr->pcb_arr[temp].cpu_time, shm_ptr->pcb_arr[temp].system_time, shm_ptr->pcb_arr[temp].prev_burst);
				log_string(log_buffer);
			}


		}


		//increment time at end of loop to simulate the scheduler taking time
		shm_ptr->clock_seconds += 1;
		nano_change = rand() % 1001;
		shm_ptr->clock_nano += nano_change;
		normalize_clock();

	}

	cleanup();
	return 0;
}

int get_index(int pid) {
	for (int i = 0; i < MAX; i++)
	{
		if (shmptr->pcb_arr->pid == pid)
			return shmptr->pcb_arr->index;
	}
	return -1;
}

void move_blocked() {
	int pid = blocked_queue[blocked_out];
	int index = get_index(pid);

	if (pid != 0)
	{
		if ((shmptr->pcb_arr[index].blocked_until_seconds == shmptr->clock_seconds) &&
			(shmptr->pcb_arr[index].blocked_until_nano == shmptr->clock_nano))
		{
			blocked_queue[blocked_out] = 0;
			blocked_out = (blocked_out + 1) % MAX;
			ready_pids[ready_in] = pid;
			ready_in = (ready_in + 1) % MAX;
		}
		else if (shmptr->next_fork_sec < shmptr->clock_seconds)
		{
			blocked_queue[blocked_out] = 0;
			blocked_out = (blocked_out + 1) % MAX;
			ready_pids[ready_in] = pid;
			ready_in = (ready_in + 1) % MAX;
		}
	}
}

void spawn() {
	int next_user = find_next_user();
	fork_user(next_user);
}

int find_next_user() {
	for (int i = 0; i < MAX; i++)
	{
		if (proc_used[i] == 0)
			return i;
	}
}

void fork_user(int user_id) {
	int pid;

	if ((pid = fork()) == -1)
	{
		perror("oss.c: Error: Could not fork child in function fork_user()");
		cleanup();
	}
	else
	{
		if (pid != 0)
		{
			printf("in forked spot\n");
			shmptr->pcb_arr[user_id].pid = pid;
			proc_used[user_id] = 1;// maybe change this to boolean
			//init_pcb(user_id);
			ready_pids[ready_in] = shmptr->pcb_arr[user_id].pid;
			//sprintf(log_buf, "OSS: PID: %d was forked and placed in the ready queue\n", shmptr->pcb_arr[user_id].pid);
			//write_to_log(log_buf);
			ready_in = (ready_in + 1) % MAX;
		}
		else {
			execl("./uproc", "./uproc", (char*)0);
		}
	}
}

int get_num_of_users() {
	int count = 0;
	for (int i = 0; i < MAX; i++)
	{
		if (ready_pids[i] != 0)
			count++;
	}
	for (int i = 0; i < MAX; i++)
	{
		if (blocked_queue[i] != 0)
			count++;
	}
	return count;
}

int get_num_of_ready() {
	int count = 0;
	for (int i = 0; i < MAX; i++)
	{
		if (ready_pids[i] != 0)
			count++;
	}
	return count;
}

void set_fork() {
	shmptr->next_fork_sec = (rand() % 2) + shmptr->clock_seconds;
	shmptr->next_fork_nano = (rand() % 1001) + shmptr->clock_nano;

	reset_nano();
}

void reset_nano() {
	if (shmptr->clock_nano >= 1000000000) // If nanoseconds reaches one second, adjust clock
	{
		shmptr->clock_seconds += 1;
		shmptr->clock_nano -= 1000000000;
	}
}

void init_table() {
	printf("Init table\n");
	for (int i = 0; i < MAX; i++)
	{
		proc_used[i] = 0;
		ready_pids[i] = 0;
	}
}

void child_handler() {
	pid_t pid;
	while ((pid = waitpid((pid_t)(-1), 0, WNOHANG)) > 0) {
		// Child Died, do something
	}

}

void init_sems() {
	printf("Init sems\n");
	semctl(sem_id, 0, SETVAL, 1);
}

void init_pcb(int i) {
	printf("Init pcb\n");
	shmptr->pcb_arr[i].blocked = false;
	shmptr->pcb_arr[i].early_term = false;
	shmptr->pcb_arr[i].waiting = true;
	shmptr->pcb_arr[i].start_nano = 0;
	shmptr->pcb_arr[i].start_sec = 0;
	shmptr->pcb_arr[i].cpu_time = 0;
	shmptr->pcb_arr[i].system_time = 0;
	shmptr->pcb_arr[i].prev_burst = 0;
	shmptr->pcb_arr[i].index = i;

}

void init_shm() {
	printf("Init shm\n");
	shmptr->user_count = 0;
	shmptr->clock_nano = 0;
	shmptr->clock_seconds = 0;
	shmptr->current_pid = 0;
	shmptr->current_index = -1;

	for (int i = 0; i < MAX; i++)
		init_pcb(i);
}

bool create_shared_memory(key_t shmkey, key_t semkey) {
	printf("Creating shared Memory\n");
	key_t key = ftok("Makefile", 'a');
	shmid = shmget(key, (sizeof(pcb_t) * MAX) + sizeof(shmptr_t), IPC_CREAT | 0666);
	if (shmid == -1)
	{
		errno = 5;
		perror("oss: Error: Could not created shared memory of struct");
		return false;
	}

	shmptr = (shmptr_t*)(shmat(shmid, 0, 0));
	if (shmptr == (shmptr_t*)-1)
	{
		errno = 5;
		perror("oss: Error: Could not attach shared memory of struct");
		return false;
	}

	key_t key1 = ftok(".", 'a');
	sem_id = semget(key1, NUM_SEMS, IPC_CREAT | 0666);

	if (sem_id < 0)
	{
		errno = 5;
		perror("oss: Error: Could not create shared memory of semaphores");
		return false;
	}

	/*key_t key = ftok("Makefile", 'a');
	//gets chared memory
	if ((shmid = shmget(key, (sizeof(pcb_t) * MAX) + sizeof(shmptr_t), IPC_CREAT | 0666)) == -1) {
		perror("oss: Error: Could not created shared memory of struct");
		return false;
	}
	//tries to attach shared memory
	if ((shmptr = (shmptr_t*)shmat(shmid, 0, 0)) == (shmptr_t*)-1) {
		perror("oss: Error: Could not attach shared memory of struct");
		return false;
	}
	key_t key1 = ftok(".", 'a');
	//gets chared memory
	if ((sem_id = semget(key1, NUM_SEMS, IPC_CREAT | 0666)) == -1) {
		perror("oss: Error: Could not create shared memory of semaphores");
		return false;
	}*/

	return true;
}

void cleanup() {
	kill_pids();
	sleep(1);
	shmdt(shmptr);
	shmctl(shmid, IPC_RMID, NULL);
	semctl(sem_id, 0, IPC_RMID, NULL);
	fclose(fp);
	exit(0);
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

void sig_handler() {
	cleanup();
}

void write_to_log(char* log) {
	fputs(log, fp);
}

void create_file(char* name) {
	char buf[50];

	strcpy(buf, name);
	strcat(buf, ".txt");
	strcpy(log_name, buf);

	fp = fopen(buf, "w");
	fclose(fp);
}

void help_menu() {
	printf("HELP MENU\n\n");
	printf("Program wil take in four arguments, filename, consumers, producers and time\n");
	printf("Defaults are logfile.txt, 2 producers, 6 consumers, and 100 seconds\n");
	printf("PROGRAM OPTIONS:\n\n");
	printf("OPTION [-o]:\n");
	printf("           When called, will overwrite the default logfile with given logfile name\n");
	printf("           If given filename is longer than 20, will default to logfile.txt\n");
	printf("           EX: ./montior -o newfilename\n\n");
	printf("OPTION [-c]:\n");
	printf("           When called, will overwrite the default value of consumers with given value\n");
	printf("           EX: ./montior -c 20\n\n");
	printf("OPTION [-p]:\n");
	printf("           When called, will overwrite the default value of producers with given value\n");
	printf("           EX: ./monitor -p 20\n\n");
	printf("OPTION [-t]:\n");
	printf("           When called, will overwtie the default value of time with given value\n");
	printf("           EX: ./monitor -t 20\n\n");
	exit(0);
}


void sem_wait(int sem) {
	struct sembuf op;
	op.sem_num = sem;
	op.sem_op = -1;
	op.sem_flg = 0;
	semop(sem_id, &op, 1);
}

