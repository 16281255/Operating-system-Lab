#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <time.h>

#define N 10

union semun
{
 int val;
 struct semid_ds *buf;
 unsigned short *arry;
};

void set_init_value_of_sem(int sem_id, int init_value) {
	union semun sem_union;
	sem_union.val = init_value;
	if (semctl(sem_id, 0, SETVAL, sem_union) == -1)
	{
		printf("%s\n", "error - inital semaphore");
		exit(1);
	}
}

void delete_sem(int sem_id) {
	union semun sem_union;
	if (semctl(sem_id, 0, IPC_RMID, sem_union) == -1)
	{
		printf("%s\n", "error - delete semaphore");
		exit(1);
	}
}

void signal(int sem_id) {
	struct sembuf sem_buff;
	sem_buff.sem_num = 0;
	sem_buff.sem_op = 1;
	sem_buff.sem_flg = SEM_UNDO;

	if (semop(sem_id, &sem_buff, 1) == -1)
	{
		printf("%s\n", "error - signal");
		exit(1);
	}
}

void wait(int sem_id) {
	struct sembuf sem_buff;
	sem_buff.sem_num = 0;
	sem_buff.sem_op = -1;
	sem_buff.sem_flg = SEM_UNDO;

	if (semop(sem_id, &sem_buff, 1) == -1)
	{
		printf("%s\n", "error - wait");
		exit(1);
	}
}


int share_memory_id;
void *share_memory;
struct share_data
{
	int in, out;
	char buffer[N];
	FILE *fp;
	FILE *fp_out;
};
struct share_data *shared;
int empty, full, mutex;


int producer() {
	wait(empty);
	wait(mutex);
	// printf("%p\n", shared->fp);
	char ch = fgetc(shared->fp);
	printf("%c", ch);
	if (ch == EOF) {
		signal(mutex);
		// signal(full);
		return 0;
	}
	shared->buffer[shared->in] = ch;
	shared->in = (shared->in + 1) % N;
	signal(mutex);
	signal(full);

	return 1;
}

int comsumer() {
	wait(full);
	wait(mutex);
	printf("out: %d ", shared->out);
	char out_char = shared->buffer[shared->out];
	if (out_char == EOF)
	{
		signal(mutex);
		// signal(empty);	
		return 0;
	}
	shared->out = (shared->out + 1) % N;
	fprintf(shared->fp_out, "%c", out_char);
	fflush(shared->fp_out);
	printf("%c", out_char);

	signal(mutex);
	signal(empty);

	return 1;	
}

int main(int argc, char const *argv[])
{

	share_memory_id = shmget(12345, sizeof(struct share_data), 0666|IPC_CREAT);
	share_memory = shmat(share_memory_id, 0, 0);
	shared = (struct share_data *)share_memory;
	shared->fp = fopen("test.txt", "r");
	shared->fp_out = fopen("test_out.txt", "w");
	if (shared->fp == NULL)
	{
		printf("%s\n", "file open fail");
		return 0;
	}
	// printf("%p\n", shared->fp);
	shared->in = 0; shared->out = 0;

	empty = semget(3000, 1, 0666 | IPC_CREAT); 
	full = semget(3001, 1, 0666 | IPC_CREAT);
	mutex = semget(3002, 1, 0666 | IPC_CREAT);
	set_init_value_of_sem(empty, N);
	set_init_value_of_sem(full, 0);
	set_init_value_of_sem(mutex, 1);

	pid_t pid1, pid2;
	while ((pid1 = fork()) == -1);
	if (pid1 > 0)
	{ 
		while ((pid2 = fork()) == -1);
		if (pid2 > 0)
		{
			while (producer()) {
				// usleep(100000);
			}
		}
		else {
			while (producer()) {
				// usleep(100000);
			}
		}
	}
	else
	{
		while (comsumer()) 
			;
		printf("\n");
	}
	
	fclose(shared->fp_out);
	fclose(shared->fp);

	return 0;
}
