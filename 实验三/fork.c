#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/sem.h>

union semun
{
 	int val;
 	struct semid_ds *buf;
 	unsigned short *arry;
};

void set_value_of_sem(int sem_id, int sem_num,  int val) {
	union semun sem_union;
	sem_union.val = val;
	if (semctl(sem_id, sem_num, SETVAL, sem_union) == -1)
	{
		printf("%d %s\n", sem_id, "error - set value of semaphore");
		exit(1);
	}
}

void set_values_of_sem(int sem_id, unsigned short *arry) {
	union semun sem_union;
	sem_union.arry = arry;
	if (semctl(sem_id, sizeof(arry) - 1, SETALL, sem_union) == -1)
	{
		printf("%d %s\n", sem_id, "error - set values of semaphore");
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

void signal(int sem_id, int sem_num) {
	struct sembuf sem_buff;
	sem_buff.sem_num = sem_num;
	sem_buff.sem_op = 1;
	sem_buff.sem_flg = 0;

	if (semop(sem_id, &sem_buff, 1) == -1)
	{
		printf("%d %s\n", sem_id, "error - signal");
		exit(1);
	}
}

void wait(int sem_id, int sem_num) {
	struct sembuf sem_buff;
	sem_buff.sem_num = sem_num;
	sem_buff.sem_op = -1;
	sem_buff.sem_flg = 0;

	if (semop(sem_id, &sem_buff, 1) == -1)
	{
		printf("%d %s\n", sem_id, "error - wait");
		exit(1);
	}
}

int main(int argc, char const *argv[])
{

	pid_t pid1, pid2, pid3;
	int sem_id1, sem_id2, sem_id3;
	sem_id1 = semget(1000, 2, 0666 | IPC_CREAT);
	sem_id2 = semget(1001, 1, 0666 | IPC_CREAT);
	sem_id3 = semget(1002, 2, 0666 | IPC_CREAT);
	unsigned short init_arry[2] = {0, 0};
	set_values_of_sem(sem_id1, init_arry);
	set_value_of_sem(sem_id2, 0, 1);
	set_values_of_sem(sem_id3, init_arry);

	while ((pid1 = fork()) == -1);
	if (pid1 > 0)
	{
		while ((pid2 = fork()) == -1);
		if (pid2 > 0)
		{
			wait(sem_id1, 0);
			wait(sem_id2, 0);
			printf("I am the process P2:pid= %d ppid= %d\n", getpid(), getppid() );
			signal(sem_id2, 0);
			signal(sem_id3, 0);
			exit(0);
		}
		else
		{
			wait(sem_id3, 0);
			wait(sem_id3, 1);
			printf("I am the process P4:pid= %d ppid= %d\n", getpid(), getppid() );

			delete_sem(sem_id1);
			delete_sem(sem_id2);
			delete_sem(sem_id3);
			exit(0);
		}
	}
	if (pid1 == 0)
	{
		while ((pid3 = fork()) == -1);
		if (pid3 > 0)
		{
			printf("I am the process P1:pid= %d ppid= %d\n", getpid(), getppid() );
			unsigned short arry[2] = {1, 1};
			set_values_of_sem(sem_id1, arry);
			exit(0);
		}
		else
		{
			wait(sem_id1, 1);
			wait(sem_id2, 0);
			printf("I am the process P3:pid= %d ppid= %d\n", getpid(), getppid() );
			signal(sem_id2, 0);
			signal(sem_id3, 1);
			exit(0);
		}
	}

	return 0;
}
