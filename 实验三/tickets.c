#include<sys/types.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include <fcntl.h> 
#include <sys/stat.h>
#include <semaphore.h>
#include <sched.h>
sem_t* mySem = NULL;
int tickets = 1000;//初始票数
int num1 = 1, num2 = 1; //售/退票次数
void*sold(){
    int i = 100;
    while(i--){
        // sem_wait(mySem);         
	printf("第%d次售出后剩余: %d.\n",num1,tickets);
	num1++;
        int temp = tickets;
        // sched_yield();         
	temp = temp - 1;
        // sched_yield();         
	tickets = temp;
        // sem_post(mySem);
     }
}
void*returnT(){
    int i = 100;
    while(i--){
        // sem_wait(mySem);
         printf("第%d次退票后剩余: %d.\n",num2,tickets);
	num2++;
        int temp = tickets;
        // sched_yield();
         temp = temp + 1;
        // sched_yield();
         tickets = temp;
        // sem_post(mySem);
     }
}
int main(){
    pthread_t p1,p2;
    mySem = sem_open("Ticket", O_CREAT, 0666, 1);
    pthread_create(&p1,NULL,sold,NULL);
    pthread_create(&p2,NULL,returnT,NULL);
    pthread_join(p1,NULL);
    pthread_join(p2,NULL);
    sem_close(mySem);
    sem_unlink("Ticket");
    printf("最后的票数: %d.\n",tickets);
    return 0;
}
