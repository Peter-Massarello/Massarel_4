#include "oss.h"
extern errno;

int shm_id, sem_id;
shmptr_t* shm_ptr;
int user_index;
int user_pid;
int sec_diff;
FILE* file_ptr;

int main(int argc, char* argv[]) {

	cleanup();
	return 0;
}

int create_shm() {
	key_t key = ftok("Makefile", 'a');
	if ((shm_id = shmget(key, (sizeof(pcb_t) * MAX) + sizeof(shmptr_t), IPC_CREAT | 0666)) == -1) {
		errno = 5;
		perror("oss.c: Error: Could not create shared memory in create_shm()");
		return -1;
	}
	if ((shm_ptr = (shmptr_t*)shmat(shm_id, 0, 0)) == (shmptr_t*)-1) {
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

void normalize_clock() {
	unsigned int nano = shm_ptr->pcb_arr[user_index].start_nano;
	int sec;
	if (nano >= 1000000000) {
		shm_ptr->pcb_arr[user_index].start_sec += 1;
		shm_ptr->pcb_arr[user_index].start_nano -= 1000000000;

	}
}