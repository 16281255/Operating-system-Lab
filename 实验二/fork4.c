#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

int main()
{
	pid_t p1,p2,p3,p4,p5;
	while((p1=fork()) == -1);//p1进程运行
	if(!p1)
	{
  	while((p2=fork()) == -1);//p2进程运行
  		if(!p2)
    		{
        		while((p3=fork())==-1);//p3进程运行
        	if (!p3)
       			{
             		while(1){
              	printf("P4:子ID==%d，父ID==%d\n",getpid(),getppid());}
       			}
       else
      {
           while ((p4=fork())==-1);
           if (!p4)
          {
               while(1){
                 printf("P5:子ID==%d，父ID==%d\n",getpid(),getppid());}
          }
       }
      while(1){
        printf("P2：子ID==%d，父ID==%d\n",getpid(),getppid());
	//printf("P2：子ID==%d，父ID==%d\n",getpid(),getppid());exit(getpid()); //正常退出p2进程
	//printf("P2：子ID==%d，父ID==%d\n",getpid(),getppid());unsigned char *ptr = 0x00;*ptr = 0x00;//段错误退出p2进程
	}
   }
 else
 {
  while ((p5=fork())==-1);
           if (!p5)
          {
               while(1){
                 printf("P3:子ID==%d，父ID==%d\n",getpid(),getppid());}
          }
 }
   while(1){
    printf("P1:子ID==%d，父ID==%d\n",getpid(),getppid());}
}

return 0;
}
