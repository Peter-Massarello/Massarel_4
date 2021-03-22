#include "oss.h"

int shmid, log_id, sem_id;
int* shmptr;
char* log_ptr;
char* log_name = "logfile"; // Default

void create_file(char* name) {
	FILE* fp;
	char buf[50];
	
	// Put Log File shared memory declaration inside creation function for ease of use

	//key_t log_key = ftok(".", 'a');
	//log_id = shmget(log_key, sizeof(char) * 50, IPC_CREAT | 0666);

	if (log_id < 0)
	{
		errno = 5;
		perror("monitor: Error: Could not create shared memory for file");
		//end_func();
		exit(0);
	}

	log_ptr = (char*)shmat(log_id, 0, 0);

	if (log_ptr == (char*)-1)
	{
		errno = 5;
		perror("monitor: Error: Could not attach shared memory to file");
		//end_func();
		exit(0);
	}

	strcpy(buf, name);
	strcat(buf, ".txt");
	strcpy(log_ptr, buf);

	fp = fopen(buf, "w");
	fclose(fp);
}

int main(int argc, char* argv[]) {

	int opt;
	char* opt_buf;
	bool file_given = false;

	while ((opt = getopt(argc, argv, "hs:l:")) != -1)
	{
		switch (opt)
		{
		case 'h':
			//help_menu();
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

}
