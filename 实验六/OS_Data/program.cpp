/************************************************************************/
/* ����޸ģ��ƶ�   16281255  ��ȫ1601                                  */
/* ��ϵ��ʽ��16281255@bjtu.edu.cn                                       */
/************************************************************************/

#include <stdio.h>
#include <memory.h>  
#include <string>   //"exit"���ʹ��
#include <string.h>
#include <iostream>
#include <windows.h>
#include <time.h>
using namespace std;

#define GENERAL 1 
#define DIRECTORY 2
#define Zero 0
//1����ͨ�ļ���2��Ŀ¼�ļ���0�����ļ�


/************************************************************************/
/* FCB�洢                                                              */
/************************************************************************/
struct FCB
{
	char fname[16]; //�ļ���
	char type;  // 0�����ļ�  1����ͨ�ļ� 2��Ŀ¼�ļ�   
	int size;    //�ļ���С
	int fatherBlockNum;    //��ǰ�ĸ�Ŀ¼�̿��
	int currentBlockNum;    //��ǰ���̿�
	char Dir_Time[24];//�ļ���ʱ��
	char File_Time[24];//�ļ�ʱ��
	void initialize() //��պ���
	{
		strcpy(fname,"\0");
		type = Zero;
		size =0;
		fatherBlockNum = currentBlockNum = 0;
	}
}; 

/************************************************************************/
/* �ļ�ϵͳ��ʼ�������趨                                               */
/************************************************************************/
const char* FilePath = "C:\\huangdong";//��ʼ�ļ�Ŀ¼
const int BlockSize = 512;       //�̿��С
const int OPEN_MAX = 5;          //�ܴ������ļ���
const int BlockCount = 128;   //�̿���
const int DiskSize = BlockSize * BlockCount;    //���̴�С
const int BlockFcbCount = BlockSize/sizeof(FCB);//Ŀ¼�ļ������FCB��
int OpenFileCount = 0; // ͳ�Ƶ�ǰ���ļ���Ŀ

struct OPENLIST      //�û��ļ��򿪱�
{
	int files;      //��ǰ���ļ���
	FCB f[OPEN_MAX];    //FCB����
	OPENLIST()
	{
		files=0;
		for(int i=0;i<OPEN_MAX;i++){
			f[i].fatherBlockNum = -1;//Ϊ�����
			f[i].type=GENERAL;
		}
	}
};

/************************************************************************/
/* Ŀ¼�ļ��ṹ                                                         */
/************************************************************************/
struct dirFile
{
	struct FCB fcb[BlockFcbCount];
	void init(int _FatherBlockNum,int _CurrentBlockNum,char *name)
		//����ţ���ǰ��ţ�Ŀ¼��
	{
		strcpy(fcb[0].fname,name); //�����FCB
		fcb[0].fatherBlockNum=_FatherBlockNum;
		fcb[0].currentBlockNum=_CurrentBlockNum;
		fcb[0].type=DIRECTORY;     //���Ŀ¼�ļ�
		for(int i=1;i<BlockFcbCount;i++){
			fcb[i].fatherBlockNum=_CurrentBlockNum; //���Ϊ����
			fcb[i].type=Zero;    // ���Ϊ�հ���
		}
	}
};

/************************************************************************/
/* ��Ŀ¼��ʼ������                                                     */
/************************************************************************/
struct DISK
{
	int FAT1[BlockCount];     //FAT1
	int FAT2[BlockCount];     //FAT2
	struct dirFile root;    //��Ŀ¼
	char data[BlockCount-3][BlockSize];
	void format(){
		memset(FAT1,0,BlockCount);     //FAT1
		memset(FAT2,0,BlockCount);     //FAT2
		FAT1[0]=FAT1[1]=FAT1[2]=-2; //0,1,2�̿�����δ���FAT1,FAT2,��Ŀ¼��
		FAT2[0]=FAT2[1]=FAT2[2]=-2;  //FAT������
		root.init(2,2,"C:\\");//��Ŀ¼��
		memset(data,0,sizeof(data));//������
	}
};

FILE *fp;      //�����ļ���ַ
char * BaseAddr;    //������̿ռ����ַ
string currentPath="C:\\huangdong\\";   //��ǰ·��
int current=2;    //��ǰĿ¼���̿��
string cmd;      //����ָ��
struct DISK *osPoint;    //���̲���ϵͳָ��
char command[16];    //�ļ�����ʶ
struct OPENLIST* openlist; //�û��ļ��б�ָ��

/************************************************************************/
/* ָ�����                                                           */
/************************************************************************/
int format();
int mkdir(char *sonfname);
int rmdir(char *sonfname);
int create(char *name);
int listshow();
int destroy(char *name);
int changePath(char *sonfname);
int write(char *name);
int exit();
int open(char *file);
int close(char *file);
int read(char *file);

/************************************************************************/
/* �ļ�ϵͳ��ʼ��/��ʽ��                                                */
/************************************************************************/
int format()
{
	current = 2;
	currentPath="C:\\";   //��ǰ·��
	osPoint->format();//���ļ��б��ʼ��
	delete openlist;
	openlist=new OPENLIST;
	fp = fopen(FilePath,"w+");
	fwrite(BaseAddr,sizeof(char),DiskSize,fp);
	fclose(fp);
	printf("�ɹ�����ʽ����ɣ�\n");
	return 1;
}

/************************************************************************/
/* �����ļ���                                                           */
/*�ж��Ƿ������������Ӹ���Ŀ¼�������Ŀ¼�̿飬���ҳ�ʼ���޸�fat��     */
/************************************************************************/
int  mkdir(char *sonfname)
{
	int i,temp,iFAT;
	time_t dirTime;
	dirTime = time(NULL);
	char DirTime[24];
	strcpy(DirTime,ctime(&dirTime));
	DirTime[24]=' ';

	struct dirFile *dir;     //��ǰĿ¼��ָ��
	if(current == 2) // ��Ŀ¼ 
		dir=&(osPoint->root);
	else
		dir=(struct dirFile *)(osPoint->data [current-3]);
	for(i = 1;i<BlockFcbCount;i++)//ͬ���ļ��м��
	{
		if(dir->fcb[i].type==DIRECTORY && strcmp(dir->fcb[i].fname,sonfname)==0 )
		{
			printf("ʧ�ܣ����ļ����Ѵ��ڣ���ѡ��򿪻�������������\n");
			return 0;
		}
	}
	for(i = 1;i < BlockFcbCount; i++){//���ҿհ�fcb���
		if(dir->fcb[i].type==Zero)
			break;
	}
	if(i == BlockFcbCount){
		printf("ʧ�ܣ���Ŀ¼��������ѡ���µ�Ŀ¼�´���!\n");
		return 0;
	}
	temp = i;
	for(i = 3;i < BlockCount;i++)
	{
		if(osPoint->FAT1[i] == 0)
			break;
	}
	if(i == BlockCount){
		printf("ʧ�ܣ����������������������ļ����ٴ�����\n");
		return 0;
	}

	iFAT=i;
	//�������
	osPoint->FAT1[iFAT]=osPoint->FAT2[iFAT] = 2;   //2��ʾ������¼�Ŀ¼�ļ�
	//��д�÷����µ��̿�Ĳ���
	strcpy(dir->fcb[temp].fname,sonfname);
	dir->fcb[temp].type=DIRECTORY;
	dir->fcb[temp].fatherBlockNum=current;
	dir->fcb[temp].currentBlockNum=iFAT;
	strcpy(dir->fcb[temp].Dir_Time,DirTime);
	//��ʼ����Ŀ¼�ļ��̿�
	dir=(struct dirFile*)(osPoint->data [iFAT-3]);   //��λ����Ŀ¼�̿��
	dir->init (current,iFAT,sonfname);//iFAT��Ҫ����Ŀ�ţ������current����ָҪ����Ŀ�ĸ����
	printf("�ɹ����ļ����Ѵ�����\n");
	return 1;
}
/************************************************************************/
/* ɾ���ļ���                                                           */
/************************************************************************/
int rmdir(char *sonfname)
{

	int i,temp,j;//ȷ����ǰĿ¼���и��ļ�,����¼�¸�FCB�±�
	struct dirFile *dir;     //��ǰĿ¼��ָ��
	if(current==2)
		dir=&(osPoint->root);
	else
		dir=(struct dirFile *)(osPoint->data [current-3]);

	for(i=1;i<BlockFcbCount;i++){     //���Ҹ�Ŀ¼�ļ�
		if(dir->fcb[i].type==DIRECTORY && strcmp(dir->fcb[i].fname,sonfname)==0){
			break;
		}
	}

	temp=i;

	if(i==BlockFcbCount){
		printf("ʧ�ܣ�δ�ҵ����ļ��У�\n");
		return 0;
	}

	j = dir->fcb[temp].currentBlockNum;
	struct dirFile *sonDir;     //��ǰ��Ŀ¼��ָ��
	sonDir=(struct dirFile *)(osPoint->data [ j - 3]);

	for(i=1;i<BlockFcbCount;i++) { //������Ŀ¼�Ƿ�Ϊ��Ŀ¼
		if(sonDir->fcb[i].type!=Zero)
		{
			printf("���棺�ǿ��ļ��У������ļ�����ɾ����\n");
			return 0;
		}
	}
	/*��ʼɾ����Ŀ¼����*/
	osPoint->FAT1[j] = osPoint->FAT2[j]=0;     //fat���
	char *p=osPoint->data[j-3];      //��ʽ����Ŀ¼
	memset(p,0,BlockSize);
	dir->fcb[temp].initialize();          //������Ŀ¼ռ��Ŀ¼��
    printf("�ɹ����ļ����ѱ�����\n");
	return 1;
}

/************************************************************************/
/* �����ļ�                                                             */
/************************************************************************/
int create(char *name){
	time_t fileTime;
	fileTime = time(NULL);
	char FileTime[24];
	strcpy(FileTime,ctime(&fileTime));
	FileTime[24]=' ';

	int i,iFAT;//temp,
	int emptyNum = 0,isFound = 0;        //����Ŀ¼�����
	struct dirFile *dir;     //��ǰĿ¼��ָ��
	if(current==2)
		dir=&(osPoint->root);
	else
		dir=(struct dirFile *)(osPoint->data [current-3]);
	for(i=1;i<BlockFcbCount;i++)//�鿴Ŀ¼�Ƿ�����//Ϊ�˱���ͬ�����ı��ļ�
	{
		if(dir->fcb[i].type == Zero && isFound == 0)	{
			emptyNum = i;
			isFound = 1;
		}
		else if(dir->fcb[i].type==GENERAL && strcmp(dir->fcb[i].fname,name)==0 ){
			printf("ʧ�ܣ����ļ��Ѵ��ڣ�\n");
			return 0;
		}
	}

	if(emptyNum == 0){
		printf("ʧ�ܣ��ļ�Ŀ¼������\n");
		return 0;
	}

	for(i = 3;i<BlockCount;i++)//����FAT��Ѱ�ҿհ���������������̿��j
	{
		if(osPoint->FAT1[i]==0)
			break;
	}
	if(i==BlockCount){
		printf("ʧ�ܣ�����������\n");
		return 0;
	}
	iFAT=i;
	//������̿�
	osPoint->FAT1[iFAT] = osPoint->FAT2[iFAT] = 1;
	//��д�÷����µ��̿�Ĳ���
	strcpy(dir->fcb[emptyNum].fname,name);
	dir->fcb[emptyNum].type=GENERAL;
	dir->fcb[emptyNum].fatherBlockNum=current;
	dir->fcb[emptyNum].currentBlockNum=iFAT;
	dir->fcb[emptyNum].size =0;
	strcpy(dir->fcb[emptyNum].File_Time,FileTime);
	char* p = osPoint->data[iFAT -3];
	memset(p,4,BlockSize);
	printf("�ɹ��������ļ���ɣ�\n");
	return 1;
}

/************************************************************************/
/* �ļ���ѯ                                                             */
/************************************************************************/
int listshow()
{
	int i,DirCount=0,FileCount=0;
	//������ǰĿ¼
	struct dirFile *dir;     //��ǰĿ¼��ָ��
	if(current==2)
		dir=&(osPoint->root);
	else
		dir=(struct dirFile *)(osPoint->data [current-3]);

	for(i=1;i<BlockFcbCount;i++)
	{
		if(dir->fcb[i].type==GENERAL){   //������ͨ�ļ�
			FileCount++;
		}
		if(dir->fcb[i].type==DIRECTORY){   //����Ŀ¼�ļ�
			DirCount++;
		}
	}

	if (FileCount+DirCount == 0)
	{
		printf("���ļ�Ŀ¼Ϊ�գ�\n");
		return 1;
	} 
	else
	{
		FileCount=0;
		DirCount=0;
		printf("\n�ļ�����\t�ļ�����\tӵ����\t\t����ʱ��\n");
	}

	for(i=1;i<BlockFcbCount;i++)
	{
		if(dir->fcb[i].type==GENERAL){   //������ͨ�ļ�
			FileCount++;
			printf("%s\t\t�ı��ļ�\thuangdong\t",dir->fcb[i].fname);
			for (int m=0;m<24;m++)
			{
				printf("%c",dir->fcb[i].File_Time[m]);
			}
			printf("\n");
		}
		if(dir->fcb[i].type==DIRECTORY){   //����Ŀ¼�ļ�
			DirCount++;
			printf("%s\t\t�ļ���\t\thuangdong\t%s\n",dir->fcb[i].fname,dir->fcb[i].Dir_Time);
		}
	}
	printf("\n�ɹ��� �ļ���Ŀ��%d ���ļ�����Ŀ��%d\n\n",FileCount,DirCount);
	return 1;
}

/************************************************************************/
/*ɾ���ļ�                                                              */
/************************************************************************/
int destroy(char *name)
{
	int i,temp,j;
	//ȷ����ǰĿ¼���и��ļ�,���Ҽ�¼������FCB�±�
	struct dirFile *dir;     //��ǰĿ¼��ָ��
	if(current == 2)
		dir=&(osPoint->root);
	else
		dir=(struct dirFile *)(osPoint->data [current-3]);
	for(i=1;i < BlockFcbCount;i++) //���Ҹ��ļ�
	{
		if(dir->fcb[i].type==GENERAL && strcmp(dir->fcb[i].fname,name)==0){
		break;
		}
	}

	if(i == BlockFcbCount){
		printf("ʧ�ܣ����ļ������ڣ�\n");
		return 0;
	}

	int k;
	for(k=0;k<OPEN_MAX;k++){
		if((openlist->f [k].type = GENERAL)&&
			(strcmp(openlist->f [k].fname,name)==0)){
			if(openlist->f[k].fatherBlockNum == current){
				break;
			}
			else{
				printf("ʧ�ܣ����ļ�δ�ҵ���\n");
				return 0;
			}
		}
	}

	if(k!=OPEN_MAX){
		close(name);
	}

	//�Ӵ��б���ɾ��
	temp=i;
	j = dir->fcb [temp].currentBlockNum ;    //�����̿��j
	osPoint->FAT1[j]=osPoint->FAT2[j]=0;     //fat1,fat2����Ϊ�հ�
	char *p=osPoint->data[j - 3];
	memset(p,0,BlockSize); //���ԭ�ı��ļ�������
	dir->fcb[temp].initialize(); //type=0;     //��Ǹ�Ŀ¼��Ϊ���ļ�
	printf("�ɹ������ļ���ɾ����\n");
	return 1;
}

/************************************************************************/
/* �����ļ���                                                           */
/************************************************************************/
int changePath(char *sonfname)
{
	struct dirFile *dir;     //��ǰĿ¼��ָ��
	if(current==2)
		dir=&(osPoint->root);
	else
		dir=(struct dirFile *)(osPoint->data [current-3]);
	//�ص���Ŀ¼
	if(strcmp(sonfname,"..")==0){
		if(current==2){
			printf("���棺�Ѿ���Ŀ���ļ��У�\n");
			return 0;
		}
		current = dir->fcb[0].fatherBlockNum ;
		currentPath = currentPath.substr(0,currentPath.size() - strlen(dir->fcb[0].fname )-1);
		return 1;
	}
	//������Ŀ¼
	//ȷ����ǰĿ¼���и�Ŀ¼,���Ҽ�¼������FCB�±�
	int i,temp;	
	for(i = 1; i < BlockFcbCount; i++){     //���Ҹ��ļ�
		if(dir->fcb[i].type==DIRECTORY&&strcmp(dir->fcb[i].fname,sonfname)==0){
			temp=i;
			break;
		}
	}

	if(i==BlockFcbCount){
		printf("ʧ�ܣ���Ŀ¼�����ڣ�\n");
		return 0;
	}

	//�޸ĵ�ǰ�ļ���Ϣ
	current=dir->fcb [temp].currentBlockNum ;
	currentPath = currentPath+dir->fcb [temp].fname +"\\";
	printf("�ɹ����ѵ�Ŀ���ļ��У�\n");
	return 1;
}

/************************************************************************/
/* ϵͳ�˳�����                                                         */
/************************************************************************/
int exit(){//���浽������C:\huangdong
	//�������ļ����ر�
	fp=fopen(FilePath,"w+");
	fwrite(BaseAddr,sizeof(char),DiskSize,fp);
	fclose(fp);
	//�ͷ��ڴ��ϵ��������
	free(osPoint);
	//�ͷ��û����ļ���
	delete openlist;
	printf("�ɹ����ļ��Ѿ����棬ϵͳ���˳�����ӭ�´�ʹ�ã�\n\n");
	return 1;
}
/************************************************************************/
/* д�ļ�                                                               */
/************************************************************************/
int write(char *name)
{
	int i;
	char *startPoint,*endPoint;
	//�ڴ��ļ��б��в��� file(����Ҫ����ͬ����ͬĿ¼�ļ������!!!)
    for(i=0;i<OPEN_MAX;i++){
		if(strcmp(openlist->f [i].fname,name)==0 ){
			if(openlist->f[i].fatherBlockNum ==current){
				break;
			}
			else{
				printf("ʧ�ܣ��ļ�δ�ҵ���\n");
				return 0;
			}
		}
	}

	if(i==OPEN_MAX){
		printf("���棺�ļ�δ�򿪣�\n");
		return 0;
	}
	int active=i;
	int fileStartNum = openlist->f[active].currentBlockNum - 3 ;
	startPoint = osPoint->data[fileStartNum];
	endPoint = osPoint->data[fileStartNum + 1];
	printf("�ɹ������������ݣ�������Ctrl+D��ɣ�\n");
	char input;
	while(((input=getchar())!=4))	{
		if(startPoint < endPoint-1)	{
			*startPoint++ = input;
		}
		else{
			printf("���棺�ļ�����������");
			*startPoint++ = 4;
			break;
		}
	}
	return 1;
}

/************************************************************************/
/* ��ȡ�ļ�                                                             */
/************************************************************************/
int  read(char *file)
{
	int i,fileStartNum;
	char *startPoint,*endPoint;
	//struct dirFile *dir;
	//�ڴ��ļ��б��в��� file(����Ҫ����ͬ����ͬĿ¼�ļ������!!!)
	for(i=0;i<OPEN_MAX;i++){
		if(strcmp(openlist->f [i].fname,file)==0 ){
			if(openlist->f[i].fatherBlockNum ==current){
				break;
			}
			else{
				printf("ʧ�ܣ����ļ�δ�򿪣�\n");
				return 0;
			}
		}
	}

	if(i==OPEN_MAX){
		printf("ʧ�ܣ����ļ�δ�򿪣�\n");
		return 0;
	}
	int active=i;//�����ļ������ַ
	fileStartNum = openlist->f[active].currentBlockNum - 3 ;
	startPoint = osPoint->data[fileStartNum];
	endPoint = osPoint->data[fileStartNum + 1];
	printf("�ɹ����ļ��Ѵ򿪣�����Ϊ:  \n");
	while((*startPoint)!=4&& (startPoint < endPoint)){
		putchar(*startPoint++);
	}
	printf("\n");
	return 1;
}

/************************************************************************/
/* ���ļ�                                                             */
/************************************************************************/
int open(char *file)
{
	int i,FcbIndex;
	//����Ƿ��Ѿ���
	for(i=0;i<OPEN_MAX;i++){
		if(openlist->f[i].type ==GENERAL && strcmp(openlist->f [i].fname,file)==0 
		&&openlist->f[i].fatherBlockNum == current)
		{
			printf("��ʾ�����ļ��Ѵ򿪣�\n");
			return 0;
		}
	}

	//ȷ���пյĴ��ļ���
	if(openlist->files == OPEN_MAX){
		printf("ʧ�ܣ��ļ��򿪴ﵽ���ޣ�\n");
		return 0;
	}
	//ȷ����ǰĿ¼���и��ļ�,���Ҽ�¼������FCB�±�
	struct dirFile *dir;     //��ǰĿ¼��ָ��
	if(current==2)
		dir=&(osPoint->root);
	else
		dir=(struct dirFile *)(osPoint->data [current-3]);

	for(i = 1;i< BlockFcbCount;i++){     //���Ҹ��ļ�
		if(dir->fcb[i].type==GENERAL && strcmp(dir->fcb[i].fname,file)==0 )	{
			FcbIndex=i;
			break;
		}
	}

	if(i==BlockFcbCount){
		printf("ʧ�ܣ��ļ�δ�ҵ���\n");
		return 0;
	}
	//װ�����ļ�������ļ��б�
	openlist->f[OpenFileCount] = dir->fcb[FcbIndex]; //FCB����
	openlist->files ++;
	printf("�ɹ����ļ��Ѵ򿪣�\n");
	OpenFileCount++;
	return 1;
}

/************************************************************************/
/* �ر��ļ�                                                             */
/************************************************************************/
int close(char *file)
{
	//�ͷŸ��ļ���ռ�ڴ�
	//�ͷ��û����ļ��б����
	int i;
	//�ڴ��ļ��б��в��� file
	for(i=0;i<OPEN_MAX;i++){
		if((openlist->f [i].type = GENERAL)&&
			(strcmp(openlist->f [i].fname,file)==0)){
			if(openlist->f[i].fatherBlockNum == current)	{
				break;
			}
			else{
				printf("���棺���ļ��Ѵ򿪣���δ�ڵ�ǰĿ¼�£��޷��رգ�\n");
				return 0;
			}
		}
	}

	if(i==OPEN_MAX){
		printf("ʧ�ܣ����ļ�δ�򿪣�\n");
		return 0;
	}
	int active=i;
	openlist->files --;
	openlist->f[active].initialize();
	OpenFileCount--;
	printf("�ɹ������ļ��ѹرգ�\n");
	return 1;
}
/************************************************************************/
/* ʵ����Ϣ                                                             */
/************************************************************************/
void AuthorShow()
{
	time_t now;
	now = time(NULL);
	char NOW[24];
	strcpy(NOW,ctime(&now));
	NOW[24]='*';
	printf(
		"************************************************************************\n"
		"*                            �ļ�ϵͳ                                  *\n"
		"*                       �ƶ� 16281255 ��ȫ1601                         *\n"
		"*                            2019.6.9                                  *\n"
		"************************************************************************\n"
		"***********************%s************************\n"
		"************************************************************************\n",NOW);
}
/************************************************************************/
/* ���ܽ���˵��                                                         */
/************************************************************************/
void MenuShow()
{
	printf(
		"************************************************************************\n"
		"*                            ����ָ�                                *\n"
		"*         �ļ���_������mkdir <dirname>                                 *\n"
		"*         �ļ���_ɾ����rmdir <dirname>                                 *\n"
		"*         �ļ���_��ʾ��ls <dirname>                                    *\n"
		"*         �ļ���_���ģ�cd <dirname>                                    *\n"
		"*         �ļ�_�����򿪣�create <filename>                             *\n"
		"*         �ļ�_ѡ��д�룺write <filename>                              *\n"
		"*         �ļ�_ѡ���ȡ��read <filename>                               *\n"
		"*         �ļ�_ѡ��򿪣�open <filename>                               *\n"
		"*         �ļ�_ѡ��رգ�close <filename>                              *\n"
		"*         �ļ�_ѡ��ɾ����rm <filename>                                 *\n"
		"*         ϵͳ_������գ�format                                        *\n"
		"*         ϵͳ_��ȫ�˳���exit                                          *\n"
		"************************************************************************\n");
}
/************************************************************************/
/* �˳�Ŀ¼                                                             */
/************************************************************************/
void Menu_Exit()
{
	time_t now;
	now = time(NULL);
	char NOW[24];
	strcpy(NOW,ctime(&now));
	NOW[24]='*';
	system("cls");
	printf(
		"************************************************************************\n"
		"*                            �ļ�ϵͳ                                  *\n"
		"*                       �ƶ� 16281255 ��ȫ1601                         *\n"
		"************************************************************************\n"
		"***********************%s************************\n"
		"************************************************************************\n"
		"*                          ��ӭʹ�ñ�ϵͳ                              *\n"
		"*                             �´��ټ�                                 *\n"
		"************************************************************************\n",NOW);
}
/************************************************************************/
/* ������                                                               */
/************************************************************************/
int main()
{	
	AuthorShow();
	system("PAUSE");
	system("cls");
	MenuShow();
	system("PAUSE");
	openlist=new OPENLIST;//�����û��ļ��򿪱�
	BaseAddr=(char *)malloc(DiskSize);//��������ռ䲢�ҳ�ʼ��
	osPoint=(struct DISK *)(BaseAddr);//������̳�ʼ��	
	if((fp=fopen(FilePath,"r"))!=NULL){//���ش����ļ�
		fread(BaseAddr,sizeof(char),DiskSize,fp);
		printf("\n�����Ѽ��أ�������%s��\n\n",FilePath);
	}
	else{
		printf("��ӭʹ�ñ��ļ�����ϵͳ��\t���ڳ�ʼ��...\n");
		format();
		printf("��ʼ������ɣ�������ʹ�ã�ף��ʹ����죺\n\n");
	}
	while(1)
	{
		cout<<currentPath;
		cin>>cmd;
		if(cmd=="format"){
			format();
		}
		else if(cmd=="mkdir"){
			cin>>command;
			mkdir(command);
		}
		else if(cmd=="rmdir"){
			cin>>command;
			rmdir(command);
		}
		else if(cmd=="ls"){
			listshow();
		}
		else if(cmd=="cd"){
			cin>>command;
			changePath(command);
		}
		else if(cmd=="create"){
			cin>>command;
			create(command);
		}

		else if(cmd=="write"){
			cin>>command;
			write(command);
		}
		else if(cmd=="read"){
			cin>>command;
			read(command);
		}
		else if(cmd=="rm"){
			cin>>command;
			destroy(command);
		}
		else if(cmd=="open"){
			cin>>command;
			open(command);
		}
		else if(cmd=="close"){
			cin>>command;
			close(command);
		}
		else if(cmd=="exit"){
			exit();
			break;
		}
		else
			cout<<"ָ����Ч,����������:"<<endl;
	}
	Menu_Exit();
	return 1;
}
