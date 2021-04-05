#include "oss.h"
extern errno;

int shm_id, sem_id;
memory_container* shm_ptr;
int user_index;
int user_pid;
int sec_diff;
FILE* file_ptr;

int main(int argc, char* argv[]) {

	srand(time(NULL));
	int rand_num = rand() % 10000;
	char log_name[20];
	sprintf(log_name, "%d.log", rand_num);

	signal(SIGKILL, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	int num;
	unsigned int time;
	unsigned int burst;
	bool finished = false;

	float sec_to_ms;
	float nano_to_ms;

	if ((create_shm() == -1) || (create_sem() == -1))
	{
		cleanup();
		exit(0);
	}

	user_pid = getpid();

	while (1) {


		while (shm_ptr->scheduled_pid != user_pid);

		shm_ptr->scheduled_pid = 0;

		user_index = shm_ptr->scheduled_index;

		shm_ptr->pcb_arr[user_index].blocked = false;

		if (shm_ptr->pcb_arr[user_index].start_nano == 0 && shm_ptr->pcb_arr[user_index].start_sec == 0) {
			srand(user_pid);
			num = rand() % (QUANTUM + 1);
			if (num % 2 == 0) {
				time = rand() % 11;
				burst = rand() % ((10000000 + 1) - 1) + 1;
				shm_ptr->pcb_arr[user_index].early_term = true;
				shm_ptr->pcb_arr[user_index].prev_burst = burst;
				shm_ptr->pcb_arr[user_index].cpu_time += time;
				shm_ptr->pcb_arr[user_index].system_time += time;
				sem_signal(shm_id);
				cleanup();
				exit(0);
			}
			shm_ptr->pcb_arr[user_index].start_nano = shm_ptr->clock_nano;
			shm_ptr->pcb_arr[user_index].start_sec = shm_ptr->clock_seconds;
			num = rand() % (100 + 1);
			if (num % 3 == 0) {
				shm_ptr->pcb_arr[user_index].proc_type = IO;
			}
			else {
				shm_ptr->pcb_arr[user_index].proc_type = CPU;
			}
		}
		if (shm_ptr->pcb_arr[user_index].proc_type == IO) {
			num = rand() % 13;
			if (num % 4 == 0) {
				shm_ptr->pcb_arr[user_index].blocked = true;
				shm_ptr->pcb_arr[user_index].blocked_until_sec = shm_ptr->clock_seconds + (rand() % ((5 + 1) - 1) + 1);
				shm_ptr->pcb_arr[user_index].blocked_until_nano = shm_ptr->clock_nano + (rand() % 1001);
				if (shm_ptr->pcb_arr[user_index].blocked_until_nano > 1000000000) {
					shm_ptr->pcb_arr[user_index].blocked_until_nano -= 1000000000;
					shm_ptr->pcb_arr[user_index].blocked_until_sec += 1;
				}

			}
			else {
				finished = true;
			}
		}
		else {
			finished = true;
		}

		burst = rand() % ((10000000 + 1) - 1) + 1;
		shm_ptr->pcb_arr[user_index].prev_burst = burst;

		shm_ptr->pcb_arr[user_index].cpu_time += (float)burst / 1000000;

		if (finished) {
			sec_diff = shm_ptr->clock_seconds - shm_ptr->pcb_arr[user_index].start_sec;
			shm_ptr->pcb_arr[user_index].start_nano += shm_ptr->clock_nano;
			normalize_time();
			shm_ptr->pcb_arr[user_index].start_nano += burst;
			normalize_time();
			shm_ptr->pcb_arr[user_index].start_sec = sec_diff;
			sec_to_ms = (float)shm_ptr->pcb_arr[user_index].start_sec * 1000;
			nano_to_ms = (float)shm_ptr->pcb_arr[user_index].start_nano / 1000000;
			shm_ptr->pcb_arr[user_index].system_time = sec_to_ms + nano_to_ms;
			sem_signal(shm_id);
			break;
		}
		sem_signal(shm_id);
	}
	cleanup();
	return 0;
}

int create_shm() {
	key_t key = ftok("Makefile", 'a');
	if ((shm_id = shmget(key, (sizeof(pcb) * MAX) + sizeof(memory_container), IPC_EXCL)) == -1) {
		perror("user.c: shmget failed:");
		return -1;
	}
	if ((shm_ptr = (memory_container*)shmat(shm_id, 0, 0)) == (memory_container*)-1) {
		perror("user.c: shmat failed:");
		return -1;
	}
	return 0;
}

int create_sem() {
	key_t key = ftok(".", 'a');
	if ((sem_id = semget(key, NUM_SEMS, 0)) == -1) {
		perror("user.c: semget failed:");
		return -1;
	}
	return 0;
}

void sem_signal() {;
	struct sembuf op;
	op.sem_num = 0;
	op.sem_op = 1;
	op.sem_flg = 0;
	semop(sem_id, &op, 1);
}

void signal_handler() {
	cleanup();

}

void cleanup() {
	shmdt(shm_ptr);
}

void normalize_time() {
	unsigned int nano = shm_ptr->pcb_arr[user_index].start_nano;
	int sec;
	if (nano >= 1000000000) {
		shm_ptr->pcb_arr[user_index].start_sec += 1;
		shm_ptr->pcb_arr[user_index].start_nano -= 1000000000;

	}
}