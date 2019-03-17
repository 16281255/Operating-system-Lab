#include <stdio.h>
#include<unistd.h>
#include<string.h>
void C_Hello()
{
	char* s="Hello world!";
	printf("%s\n",s);
}
int H_Hello()
{
	char* s="Hello World!";
	int len=12;
	int result=0;
	__asm__ __volatile__("movl %2,%%edx;\n\r"
	"movl %1,%%ecx;\n\r"
	"movl $1,%%ebx;\n\r"
	"movl $4,%%eax;\n\r"
	"int $0x80"
	:"=m"(result)
	:"m"(s),"r"(len)
	:"%eax");
	return 0;
}

int main()
{
	printf("C语言:");
	C_Hello();
	printf("汇编语言:");
	H_Hello();
	printf("\n");
	return 0;
}

