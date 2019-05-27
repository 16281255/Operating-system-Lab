#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<windows.h>
#define N 3
#define L 100
#define MAX 2000

int queue[L];
int list[N];//内存页面队列
int count = 0;//内存页面使用统计
int que_num = 0;//访问队列正在使用的值
int flag = 0;//每置换一次flag就+1
int first_come[N];//进入内存页面的时间
int last_use[N];//上次页面使用时间
int state[N][2];//在改进型clock算法中依次表示访问位修改位
int clock_num;//在改进clock算法中表示当前位置，类似指针
int count2 = 0;//统计空闲队列中q中已有页面数目
int m_count = 0;
int i,m;


typedef struct QNode  //链队列结点的定义
{
	int val;
	int num;
	struct QNode *next;
}QNode, *Linklist;
Linklist p, q;//p代表已修改页面链表，q代表空闲页面链表

void sequence_generate(int p,int e,double t,int m)
//序列随机生成,其中p为工作集起始位置，e为工作集包含页数，m为工作集移动率
{
	int choice;
	double r;
	srand(time(0));
	for (int i = 0; i < m; i++,que_num++)
		//生成m个取值范围在p和p+e之间的随机数作为访问序列
		queue[que_num] = rand() % e + p;
	m_count++;
	printf("序列生成成功！随机访问序列为：\n");
	for (i = 0; i < m*m_count; i++)
		printf("%d ",queue[i]);
	printf("\n");
	
	printf("是否继续增加访问序列长度\n【1】是\n【2】否\n");
	printf("请选择：");
	scanf("%d",&choice);
	if (choice == 1)
	{
		if (L < (m_count+1)*m)
		{
			printf("已超出访问序列最大长度，自动返回！\n");
			return;
		}
		else
		{
			r = (rand() % 100)*0.01;
			if (r < t)
			{
				printf("请重新输入p的值：\n");
				scanf("%d", &p);
			}
			else
				p = (p + 1) % N;
			sequence_generate(p, e, t, m);
		}
	}
	if(choice==2)
		return;
}
int find_num(int a)//距离下一次使用间隔
{
	int i;
	for (i = que_num; i < L; i++)
	{
		if (queue[i] == a)
			return	i - que_num;
	}
	return MAX;//返回最大值
}

void opt(int num) //最佳置换算法,num是访问序列中的一个值
{
	int i, max = 0, rep_num, temp = 0;
	if (count != N)//如果内存页面存在空位置，则加入至此
	{
		list[count] = num;
		count++;
		flag++;
	}
	else
	{
		for (i = 0; i < N; i++)
		{
			if (list[i] == num)
				return;
		}
		for(i=0;i<N;i++)
		{
			{
				temp = find_num(list[i]);
				if (temp > max)
				{
					max = temp;
					rep_num = i;
				}
			}
		}
		list[rep_num] = num;
		flag++;
	}
}

int find_first()//找到最先进入内存页面的那一个
{
	int i,result=0,min=first_come[0];
	for (i = 0; i < N; i++)
	{
		if (first_come[i] < min)
		{
			min = first_come[i];
			result = i;
		}
	}
	return result;
}

void fifo(int num)//先进先出置换算法
{
	int i,rep_num,min=MAX;
	if (count != N)//如果内存页面存在空位置，则加入至此
	{
		list[count] = num;
		first_come[count] = count;
		count++;
		flag++;
	}
	else
	{
		for (i = 0; i < N; i++)
		{
			if (list[i] == num)
				return;
		}
		rep_num = find_first();
		list[rep_num] = num;
		first_come[rep_num] = que_num;
		flag++;
	}
	
}

int find_last()//找到最久未使用的那一个
{
	int i, result = 0, min = last_use[0];
	for (i = 0; i < N; i++)
	{
		if (last_use[i] < min)
		{
			min = last_use[i];
			result = i;
		}
	}
	return result;
}
void lru(int num)//最近最久未使用算法
{
	int i, rep_num = 0;
	if (count != N)//如果内存页面存在空位置，则加入至此
	{
		list[count] = num;
		last_use[count] = count;
		count++;
		flag++;
	}
	else
	{
		for (i = 0; i < N; i++)
		{
			if (list[i] == num)
			{
				last_use[i] = que_num;
				return;
			}
		}
		rep_num = find_last();
		list[rep_num] = num;
		last_use[rep_num] = que_num;
		flag++;
	}
}

int find_clock()
{
	int i = clock_num;
	while (true)//如果没有找到第二类页面，则重新开始寻找第一类页面
	{
		while(true)//寻找第一类页面
		{
			if (state[i][0] == 0 && state[i][1] == 0)
				return i;
			i = (i + 1) % N;
			if (i == clock_num)
				break;
		}
		while(true)//如果没有找到第一类页面，则开始找第二类页面
		{
			if (state[i][0] == 0 && state[i][1] == 1)
				return i;
			state[i][0] = 0;
			i = (i + 1) % N;
			if (i == clock_num)
				break;
		}
	}
}
void clock_pro(int num)//CLOCK改进算法
{
	int i,rep_num;
	for (i = 0; i < N; i++)
	{
		if (list[i] == num)
		{
			state[i][0] = 1;
			state[i][1] = 0;
			clock_num = i;
			return;
		}
	}
	rep_num = find_clock();
	clock_num = (rep_num+1)%N;
	list[rep_num] = num;
	state[rep_num][0] = 1;
	state[rep_num][1] = 1;
	flag++;
}

void create_linklist(Linklist &p, int num)
{
	p = (Linklist)malloc(sizeof(QNode));
	if (!p)
		exit (- 1);
	p->val = 0;
	p->next = p;
}
void Insert_LNode(Linklist &m, int e,int f)//在循环链表中插入新的结点，从L头结点开始依次向后插入
{
	Linklist p, q;
	p = (Linklist)malloc(sizeof(QNode));
	q = (Linklist)malloc(sizeof(QNode));
	q->val = e;
	q->num = f;		
	p = m;
	while (p->next != m)
	{
		p = p->next;
	}
	p->next = q;
	q->next = m;
}
void Exchange_LNode(Linklist &m, int e, int f,int i)//将链表L中序号为i的结点替换为val为e，num为f的结点
{
	if (m->next == m) 
		exit(-1);
	Linklist p, q;
	int j = 0;
	p = (Linklist)malloc(sizeof(QNode));
	q = (Linklist)malloc(sizeof(QNode));
	q->val = e;
	q->num = f;
	p = m;
	for (j = 0; j < i; j++)//使p为待更换结点的前一个结点，故应保证，删除第一个非头结点时i=0，以此类推
		p = p->next;
	q->next = p->next->next;
	p->next = q;		
}
void pba(int number)//p代表已修改页面链表，q代表空闲页面链表
{
	int i,min=MAX,rep_num=0,j=0;
	QNode *t = p->next;
	QNode *r = q->next;
	if (count != N)//如果内存页面存在空位置，则加入至此
	{
		Insert_LNode(p, number,que_num);
		flag++;
		count++;
	}
	else
	{
		for (i = 0; i < N; i++)//如果numer已经存在修改队列p中
		{
			if (t->val == number)
				return;
			t = t->next;
		}
		for (i = 0; i < 2; i++)//如果空闲队列中有num
		{
			if (r->val == number)
			{
				Insert_LNode(p, number, r->num);//将其从q中加入p的队尾
				QNode *s = q;//将其从q中移除
				for (int ii = 0; ii < 2; ii++)
				{
					if (s->next->val == number)
					{
						s->next = s->next->next;
						break;
					}
					s = s->next;
				}
				//将p中最早进入的移除
				t = p->next;
				for (i = 0; i < N; i++)//找到最先进入的那一个
				{
					if (t->num < min)
						min = t->num;
					t = t->next;
				}
				Insert_LNode(q, t->val, t->num);//将其插入到q中
				t = p;
				for (i = 0; i < N; i++)
				{
					if (t->next->num == min)
					{
						t->next = t->next->next;
						break;
					}
					t = t->next;
				}
				return;
			}
			r = r->next;
		}
		//如果空闲队列中没有number
		t=p->next;
		for (i = 0; i < N; i++)//找到最先进入的那一个
		{
			if (t->num < min)
				min = t->num;
			t = t->next;
		}
		t = p->next;
		for (i = 0; i < N; i++)//将最先进入的那一个页面淘汰，加入到空闲页面链表
		{
			if (t->num == min)
			{
				Insert_LNode(q, t->val,min);//将其加入空闲队列q的队尾
				count2++;
				if(count2==2)
					q->next = q->next->next;//移除q中的第一个链表
				Exchange_LNode(p, number, que_num,j);//在修改队列p中对应位置替换
				flag++;
				break;
			}
			t = t->next;
			j++;
		}
	}
}

void print()//打印输出内存页面队列当前情况
{
	for (int i = 0; i < N&& i <= que_num; i++)
		printf("%d ", list[i]);
	printf("\n");
}
void Introduction_Author()//简介信息
{
	time_t Now; 
    Now  = time(NULL); 
	printf("************************************************************************\n" 
		"*                            页面置换算法                              *\n"
		"*                       16281255  黄东   安全1601                      *\n"
		"************************************************************************\n                        %s",ctime(&Now));
}
void Introduction_Function()//功能信息
{
	
	printf("************************************************************************\n" 
		"*                                 算法                                 *\n"
		"*                                                                      *\n"
		"*                         【1】最佳置换算法                            *\n"
		"*                         【2】先进先出算法                            *\n"
		"*                         【3】最近最久算法                            *\n"
		"*                         【4】改进CLOCK算法                           *\n"
		"*                         【5】页面缓冲算法                            *\n"
		"*                         【0】退出算法程序                            *\n"
		"************************************************************************\n");
	
}
void Sequence()
{
	printf("************************************************************************\n"
		"*                           产生随机访问序列                           *\n"
		"*                        P:起始位置  T:最大块数                        *\n"
		"*                        E:包含页数  M:序列长度                        *\n"
		"************************************************************************\n");
	int p, e ;
	double t;
	printf("请依次输入p,e,t,m的值：");
	scanf("%d%d%lf%d", &p, &e, &t, &m);
	sequence_generate(p, e, t, m);//产生随机数列
}
int main()
{
	int menu;
	Introduction_Author();
	Introduction_Function();
	system("pause");
	system("cls");
	Sequence();
	printf("序列已经存储，");
	system("pause");
	while (true)
	{
		system("cls");
		Introduction_Function();
		printf("请选择算法：");
		scanf("%d", &menu);
		if (menu == 1)
		{
			count = 0;
			flag = 0;
			for (que_num = 0; que_num < m*m_count; que_num++)
			{
				opt(queue[que_num]);
				printf("第%2d次页面情况:", que_num + 1);
				print();
			}
			printf("OPT：缺页数：%d   缺页率：%.2f\n", flag,(float)flag / (m * m_count));
			system("pause");
		}
		if (menu == 2)
		{
			count = 0;
			flag = 0;
			for (que_num = 0; que_num < m*m_count; que_num++)
			{
				fifo(queue[que_num]);
				printf("第%2d次页面情况:", que_num + 1);
				print();
			}
			printf("FIFO：缺页数：%d   缺页率：%.2f\n", flag,(float)flag / (m * m_count));
			system("pause");
		}
		if (menu == 3)
		{
			count = 0;
			flag = 0;
			for (que_num = 0; que_num < m*m_count; que_num++)
			{
				lru(queue[que_num]);
				printf("第%2d次页面情况:", que_num + 1);
				print();
			}
			printf("LRU：缺页数：%d   缺页率：%.2f\n", flag,(float)flag / (m * m_count));
			system("pause");
		}
		if (menu == 4)
		{
			count = 0;
			flag = 0;
			clock_num = 0;
			for (que_num = 0; que_num < m*m_count; que_num++)
			{
				clock_pro(queue[que_num]);
				printf("第%2d次页面情况:", que_num + 1);
				print();
			}
			printf("CLOCK：缺页数：%d   缺页率：%.2f\n", flag,(float)flag / (m * m_count));
			system("pause");
		}
		if (menu == 5)
		{
			count = 0;
			flag = 0;
			create_linklist(p, N);
			create_linklist(q, 2);
			for (que_num = 0; que_num < m*m_count; que_num++)
			{
				pba(queue[que_num]);
				printf("第%2d次页面情况:", que_num + 1);
				QNode *t = p->next;
				for (int i = 0; i < N && i <= que_num; i++)
				{
					printf("%d ", t->val);
					t = t->next;
				}
				printf("\n");
			}
			printf("PBA：缺页数：%d   缺页率：%.2f\n", flag,(float)flag / (m * m_count));
			system("pause");
		}
		if (menu == 0) //结束程序
		{
			system("cls");
			Introduction_Author();
			printf("************************************************************************\n"
				"*                            欢迎再次使用                              *\n"
				"************************************************************************\n");
			return 0;
		}
		
	}
}