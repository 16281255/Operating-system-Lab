#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
int main()
{
	fork();
	printf("ID==%d\n",getpid());
	return 0;
}
