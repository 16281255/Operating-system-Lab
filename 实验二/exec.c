#include<stdio.h>
#include <unistd.h>
int main()
{
	int n=execl("/usr/bin/vi","vi","-al","/media/huangdong/09899376F10C5AD7/收藏夹/Desktop/操作系统/实验二/for.c",NULL);
	if(n==-1)
		printf("执行错误 !");
	else
		printf("已经执行!");
	return 0;
}
