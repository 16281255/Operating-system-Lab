/************************************************************************/
/* 最后修改：黄东   16281255  安全1601                                  */
/* 联系方式：16281255@bjtu.edu.cn                                       */
/************************************************************************/

#include <stdio.h>
#include <memory.h>  
#include <string>   //"exit"语句使用
#include <string.h>
#include <iostream>
#include <windows.h>
#include <time.h>
using namespace std;

#define GENERAL 1 
#define DIRECTORY 2
#define Zero 0
//1：普通文件；2：目录文件；0：空文件


/************************************************************************/
/* FCB存储                                                              */
/************************************************************************/
struct FCB
{
	char fname[16]; //文件名
	char type;  // 0：空文件  1：普通文件 2：目录文件   
	int size;    //文件大小
	int fatherBlockNum;    //当前的父目录盘块号
	int currentBlockNum;    //当前的盘块
	char Dir_Time[24];//文件夹时间
	char File_Time[24];//文件时间
	void initialize() //清空函数
	{
		strcpy(fname,"\0");
		type = Zero;
		size =0;
		fatherBlockNum = currentBlockNum = 0;
	}
}; 

/************************************************************************/
/* 文件系统初始化参数设定                                               */
/************************************************************************/
const char* FilePath = "C:\\huangdong";//初始文件目录
const int BlockSize = 512;       //盘块大小
const int OPEN_MAX = 5;          //能打开最多的文件数
const int BlockCount = 128;   //盘块数
const int DiskSize = BlockSize * BlockCount;    //磁盘大小
const int BlockFcbCount = BlockSize/sizeof(FCB);//目录文件的最多FCB数
int OpenFileCount = 0; // 统计当前打开文件数目

struct OPENLIST      //用户文件打开表
{
	int files;      //当前打开文件数
	FCB f[OPEN_MAX];    //FCB拷贝
	OPENLIST()
	{
		files=0;
		for(int i=0;i<OPEN_MAX;i++){
			f[i].fatherBlockNum = -1;//为分配打开
			f[i].type=GENERAL;
		}
	}
};

/************************************************************************/
/* 目录文件结构                                                         */
/************************************************************************/
struct dirFile
{
	struct FCB fcb[BlockFcbCount];
	void init(int _FatherBlockNum,int _CurrentBlockNum,char *name)
		//父块号，当前块号，目录名
	{
		strcpy(fcb[0].fname,name); //本身的FCB
		fcb[0].fatherBlockNum=_FatherBlockNum;
		fcb[0].currentBlockNum=_CurrentBlockNum;
		fcb[0].type=DIRECTORY;     //标记目录文件
		for(int i=1;i<BlockFcbCount;i++){
			fcb[i].fatherBlockNum=_CurrentBlockNum; //标记为子项
			fcb[i].type=Zero;    // 标记为空白项
		}
	}
};

/************************************************************************/
/* 根目录初始化设置                                                     */
/************************************************************************/
struct DISK
{
	int FAT1[BlockCount];     //FAT1
	int FAT2[BlockCount];     //FAT2
	struct dirFile root;    //根目录
	char data[BlockCount-3][BlockSize];
	void format(){
		memset(FAT1,0,BlockCount);     //FAT1
		memset(FAT2,0,BlockCount);     //FAT2
		FAT1[0]=FAT1[1]=FAT1[2]=-2; //0,1,2盘块号依次代表FAT1,FAT2,根目录区
		FAT2[0]=FAT2[1]=FAT2[2]=-2;  //FAT作备份
		root.init(2,2,"C:\\");//根目录区
		memset(data,0,sizeof(data));//数据区
	}
};

FILE *fp;      //磁盘文件地址
char * BaseAddr;    //虚拟磁盘空间基地址
string currentPath="C:\\huangdong\\";   //当前路径
int current=2;    //当前目录的盘块号
string cmd;      //输入指令
struct DISK *osPoint;    //磁盘操作系统指针
char command[16];    //文件名标识
struct OPENLIST* openlist; //用户文件列表指针

/************************************************************************/
/* 指令集函数                                                           */
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
/* 文件系统初始化/格式化                                                */
/************************************************************************/
int format()
{
	current = 2;
	currentPath="C:\\";   //当前路径
	osPoint->format();//打开文件列表初始化
	delete openlist;
	openlist=new OPENLIST;
	fp = fopen(FilePath,"w+");
	fwrite(BaseAddr,sizeof(char),DiskSize,fp);
	fclose(fp);
	printf("成功：格式化完成！\n");
	return 1;
}

/************************************************************************/
/* 创建文件夹                                                           */
/*判断是否有重名，增加该子目录项分配子目录盘块，并且初始化修改fat表     */
/************************************************************************/
int  mkdir(char *sonfname)
{
	int i,temp,iFAT;
	time_t dirTime;
	dirTime = time(NULL);
	char DirTime[24];
	strcpy(DirTime,ctime(&dirTime));
	DirTime[24]=' ';

	struct dirFile *dir;     //当前目录的指针
	if(current == 2) // 根目录 
		dir=&(osPoint->root);
	else
		dir=(struct dirFile *)(osPoint->data [current-3]);
	for(i = 1;i<BlockFcbCount;i++)//同名文件夹检查
	{
		if(dir->fcb[i].type==DIRECTORY && strcmp(dir->fcb[i].fname,sonfname)==0 )
		{
			printf("失败：该文件夹已存在，请选择打开或者重新命名！\n");
			return 0;
		}
	}
	for(i = 1;i < BlockFcbCount; i++){//查找空白fcb序号
		if(dir->fcb[i].type==Zero)
			break;
	}
	if(i == BlockFcbCount){
		printf("失败：此目录已满，请选择新的目录下创建!\n");
		return 0;
	}
	temp = i;
	for(i = 3;i < BlockCount;i++)
	{
		if(osPoint->FAT1[i] == 0)
			break;
	}
	if(i == BlockCount){
		printf("失败：磁盘已满，请清理无用文件后再创建！\n");
		return 0;
	}

	iFAT=i;
	//分配磁盘
	osPoint->FAT1[iFAT]=osPoint->FAT2[iFAT] = 2;   //2表示分配给下级目录文件
	//填写该分派新的盘块的参数
	strcpy(dir->fcb[temp].fname,sonfname);
	dir->fcb[temp].type=DIRECTORY;
	dir->fcb[temp].fatherBlockNum=current;
	dir->fcb[temp].currentBlockNum=iFAT;
	strcpy(dir->fcb[temp].Dir_Time,DirTime);
	//初始化子目录文件盘块
	dir=(struct dirFile*)(osPoint->data [iFAT-3]);   //定位到子目录盘块号
	dir->init (current,iFAT,sonfname);//iFAT是要分配的块号，这里的current用来指要分配的块的父块号
	printf("成功：文件夹已创建！\n");
	return 1;
}
/************************************************************************/
/* 删除文件夹                                                           */
/************************************************************************/
int rmdir(char *sonfname)
{

	int i,temp,j;//确保当前目录下有该文件,并记录下该FCB下标
	struct dirFile *dir;     //当前目录的指针
	if(current==2)
		dir=&(osPoint->root);
	else
		dir=(struct dirFile *)(osPoint->data [current-3]);

	for(i=1;i<BlockFcbCount;i++){     //查找该目录文件
		if(dir->fcb[i].type==DIRECTORY && strcmp(dir->fcb[i].fname,sonfname)==0){
			break;
		}
	}

	temp=i;

	if(i==BlockFcbCount){
		printf("失败：未找到该文件夹！\n");
		return 0;
	}

	j = dir->fcb[temp].currentBlockNum;
	struct dirFile *sonDir;     //当前子目录的指针
	sonDir=(struct dirFile *)(osPoint->data [ j - 3]);

	for(i=1;i<BlockFcbCount;i++) { //查找子目录是否为空目录
		if(sonDir->fcb[i].type!=Zero)
		{
			printf("警告：非空文件夹，请检查文件后再删除！\n");
			return 0;
		}
	}
	/*开始删除子目录操作*/
	osPoint->FAT1[j] = osPoint->FAT2[j]=0;     //fat清空
	char *p=osPoint->data[j-3];      //格式化子目录
	memset(p,0,BlockSize);
	dir->fcb[temp].initialize();          //回收子目录占据目录项
    printf("成功：文件夹已被清理！\n");
	return 1;
}

/************************************************************************/
/* 创建文件                                                             */
/************************************************************************/
int create(char *name){
	time_t fileTime;
	fileTime = time(NULL);
	char FileTime[24];
	strcpy(FileTime,ctime(&fileTime));
	FileTime[24]=' ';

	int i,iFAT;//temp,
	int emptyNum = 0,isFound = 0;        //空闲目录项个数
	struct dirFile *dir;     //当前目录的指针
	if(current==2)
		dir=&(osPoint->root);
	else
		dir=(struct dirFile *)(osPoint->data [current-3]);
	for(i=1;i<BlockFcbCount;i++)//查看目录是否已满//为了避免同名的文本文件
	{
		if(dir->fcb[i].type == Zero && isFound == 0)	{
			emptyNum = i;
			isFound = 1;
		}
		else if(dir->fcb[i].type==GENERAL && strcmp(dir->fcb[i].fname,name)==0 ){
			printf("失败：此文件已存在！\n");
			return 0;
		}
	}

	if(emptyNum == 0){
		printf("失败：文件目录已满！\n");
		return 0;
	}

	for(i = 3;i<BlockCount;i++)//查找FAT表寻找空白区，用来分配磁盘块号j
	{
		if(osPoint->FAT1[i]==0)
			break;
	}
	if(i==BlockCount){
		printf("失败：磁盘已满！\n");
		return 0;
	}
	iFAT=i;
	//分配磁盘块
	osPoint->FAT1[iFAT] = osPoint->FAT2[iFAT] = 1;
	//填写该分派新的盘块的参数
	strcpy(dir->fcb[emptyNum].fname,name);
	dir->fcb[emptyNum].type=GENERAL;
	dir->fcb[emptyNum].fatherBlockNum=current;
	dir->fcb[emptyNum].currentBlockNum=iFAT;
	dir->fcb[emptyNum].size =0;
	strcpy(dir->fcb[emptyNum].File_Time,FileTime);
	char* p = osPoint->data[iFAT -3];
	memset(p,4,BlockSize);
	printf("成功：创建文件完成！\n");
	return 1;
}

/************************************************************************/
/* 文件查询                                                             */
/************************************************************************/
int listshow()
{
	int i,DirCount=0,FileCount=0;
	//搜索当前目录
	struct dirFile *dir;     //当前目录的指针
	if(current==2)
		dir=&(osPoint->root);
	else
		dir=(struct dirFile *)(osPoint->data [current-3]);

	for(i=1;i<BlockFcbCount;i++)
	{
		if(dir->fcb[i].type==GENERAL){   //查找普通文件
			FileCount++;
		}
		if(dir->fcb[i].type==DIRECTORY){   //查找目录文件
			DirCount++;
		}
	}

	if (FileCount+DirCount == 0)
	{
		printf("此文件目录为空！\n");
		return 1;
	} 
	else
	{
		FileCount=0;
		DirCount=0;
		printf("\n文件名称\t文件类型\t拥有者\t\t创建时间\n");
	}

	for(i=1;i<BlockFcbCount;i++)
	{
		if(dir->fcb[i].type==GENERAL){   //查找普通文件
			FileCount++;
			printf("%s\t\t文本文件\thuangdong\t",dir->fcb[i].fname);
			for (int m=0;m<24;m++)
			{
				printf("%c",dir->fcb[i].File_Time[m]);
			}
			printf("\n");
		}
		if(dir->fcb[i].type==DIRECTORY){   //查找目录文件
			DirCount++;
			printf("%s\t\t文件夹\t\thuangdong\t%s\n",dir->fcb[i].fname,dir->fcb[i].Dir_Time);
		}
	}
	printf("\n成功： 文件数目：%d ；文件夹数目：%d\n\n",FileCount,DirCount);
	return 1;
}

/************************************************************************/
/*删除文件                                                              */
/************************************************************************/
int destroy(char *name)
{
	int i,temp,j;
	//确保当前目录下有该文件,并且记录下它的FCB下标
	struct dirFile *dir;     //当前目录的指针
	if(current == 2)
		dir=&(osPoint->root);
	else
		dir=(struct dirFile *)(osPoint->data [current-3]);
	for(i=1;i < BlockFcbCount;i++) //查找该文件
	{
		if(dir->fcb[i].type==GENERAL && strcmp(dir->fcb[i].fname,name)==0){
		break;
		}
	}

	if(i == BlockFcbCount){
		printf("失败：该文件不存在！\n");
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
				printf("失败：该文件未找到！\n");
				return 0;
			}
		}
	}

	if(k!=OPEN_MAX){
		close(name);
	}

	//从打开列表中删除
	temp=i;
	j = dir->fcb [temp].currentBlockNum ;    //查找盘块号j
	osPoint->FAT1[j]=osPoint->FAT2[j]=0;     //fat1,fat2表标记为空白
	char *p=osPoint->data[j - 3];
	memset(p,0,BlockSize); //清除原文本文件的内容
	dir->fcb[temp].initialize(); //type=0;     //标记该目录项为空文件
	printf("成功：该文件已删除！\n");
	return 1;
}

/************************************************************************/
/* 进入文件夹                                                           */
/************************************************************************/
int changePath(char *sonfname)
{
	struct dirFile *dir;     //当前目录的指针
	if(current==2)
		dir=&(osPoint->root);
	else
		dir=(struct dirFile *)(osPoint->data [current-3]);
	//回到父目录
	if(strcmp(sonfname,"..")==0){
		if(current==2){
			printf("警告：已经在目的文件夹！\n");
			return 0;
		}
		current = dir->fcb[0].fatherBlockNum ;
		currentPath = currentPath.substr(0,currentPath.size() - strlen(dir->fcb[0].fname )-1);
		return 1;
	}
	//进入子目录
	//确保当前目录下有该目录,并且记录下它的FCB下标
	int i,temp;	
	for(i = 1; i < BlockFcbCount; i++){     //查找该文件
		if(dir->fcb[i].type==DIRECTORY&&strcmp(dir->fcb[i].fname,sonfname)==0){
			temp=i;
			break;
		}
	}

	if(i==BlockFcbCount){
		printf("失败：该目录不存在！\n");
		return 0;
	}

	//修改当前文件信息
	current=dir->fcb [temp].currentBlockNum ;
	currentPath = currentPath+dir->fcb [temp].fname +"\\";
	printf("成功：已到目的文件夹！\n");
	return 1;
}

/************************************************************************/
/* 系统退出函数                                                         */
/************************************************************************/
int exit(){//保存到磁盘上C:\huangdong
	//将所有文件都关闭
	fp=fopen(FilePath,"w+");
	fwrite(BaseAddr,sizeof(char),DiskSize,fp);
	fclose(fp);
	//释放内存上的虚拟磁盘
	free(osPoint);
	//释放用户打开文件表
	delete openlist;
	printf("成功：文件已经保存，系统已退出，欢迎下次使用！\n\n");
	return 1;
}
/************************************************************************/
/* 写文件                                                               */
/************************************************************************/
int write(char *name)
{
	int i;
	char *startPoint,*endPoint;
	//在打开文件列表中查找 file(还需要考虑同名不同目录文件的情况!!!)
    for(i=0;i<OPEN_MAX;i++){
		if(strcmp(openlist->f [i].fname,name)==0 ){
			if(openlist->f[i].fatherBlockNum ==current){
				break;
			}
			else{
				printf("失败：文件未找到！\n");
				return 0;
			}
		}
	}

	if(i==OPEN_MAX){
		printf("警告：文件未打开！\n");
		return 0;
	}
	int active=i;
	int fileStartNum = openlist->f[active].currentBlockNum - 3 ;
	startPoint = osPoint->data[fileStartNum];
	endPoint = osPoint->data[fileStartNum + 1];
	printf("成功：请输入内容，结束后Ctrl+D完成！\n");
	char input;
	while(((input=getchar())!=4))	{
		if(startPoint < endPoint-1)	{
			*startPoint++ = input;
		}
		else{
			printf("警告：文件内容已满！");
			*startPoint++ = 4;
			break;
		}
	}
	return 1;
}

/************************************************************************/
/* 读取文件                                                             */
/************************************************************************/
int  read(char *file)
{
	int i,fileStartNum;
	char *startPoint,*endPoint;
	//struct dirFile *dir;
	//在打开文件列表中查找 file(还需要考虑同名不同目录文件的情况!!!)
	for(i=0;i<OPEN_MAX;i++){
		if(strcmp(openlist->f [i].fname,file)==0 ){
			if(openlist->f[i].fatherBlockNum ==current){
				break;
			}
			else{
				printf("失败：该文件未打开！\n");
				return 0;
			}
		}
	}

	if(i==OPEN_MAX){
		printf("失败：该文件未打开！\n");
		return 0;
	}
	int active=i;//计算文件物理地址
	fileStartNum = openlist->f[active].currentBlockNum - 3 ;
	startPoint = osPoint->data[fileStartNum];
	endPoint = osPoint->data[fileStartNum + 1];
	printf("成功：文件已打开，内容为:  \n");
	while((*startPoint)!=4&& (startPoint < endPoint)){
		putchar(*startPoint++);
	}
	printf("\n");
	return 1;
}

/************************************************************************/
/* 打开文件                                                             */
/************************************************************************/
int open(char *file)
{
	int i,FcbIndex;
	//检查是否已经打开
	for(i=0;i<OPEN_MAX;i++){
		if(openlist->f[i].type ==GENERAL && strcmp(openlist->f [i].fname,file)==0 
		&&openlist->f[i].fatherBlockNum == current)
		{
			printf("提示：该文件已打开！\n");
			return 0;
		}
	}

	//确保有空的打开文件项
	if(openlist->files == OPEN_MAX){
		printf("失败：文件打开达到上限！\n");
		return 0;
	}
	//确保当前目录下有该文件,并且记录下它的FCB下标
	struct dirFile *dir;     //当前目录的指针
	if(current==2)
		dir=&(osPoint->root);
	else
		dir=(struct dirFile *)(osPoint->data [current-3]);

	for(i = 1;i< BlockFcbCount;i++){     //查找该文件
		if(dir->fcb[i].type==GENERAL && strcmp(dir->fcb[i].fname,file)==0 )	{
			FcbIndex=i;
			break;
		}
	}

	if(i==BlockFcbCount){
		printf("失败：文件未找到！\n");
		return 0;
	}
	//装载新文件进入打开文件列表
	openlist->f[OpenFileCount] = dir->fcb[FcbIndex]; //FCB拷贝
	openlist->files ++;
	printf("成功：文件已打开！\n");
	OpenFileCount++;
	return 1;
}

/************************************************************************/
/* 关闭文件                                                             */
/************************************************************************/
int close(char *file)
{
	//释放该文件所占内存
	//释放用户打开文件列表表项
	int i;
	//在打开文件列表中查找 file
	for(i=0;i<OPEN_MAX;i++){
		if((openlist->f [i].type = GENERAL)&&
			(strcmp(openlist->f [i].fname,file)==0)){
			if(openlist->f[i].fatherBlockNum == current)	{
				break;
			}
			else{
				printf("警告：该文件已打开，但未在当前目录下，无法关闭！\n");
				return 0;
			}
		}
	}

	if(i==OPEN_MAX){
		printf("失败：该文件未打开！\n");
		return 0;
	}
	int active=i;
	openlist->files --;
	openlist->f[active].initialize();
	OpenFileCount--;
	printf("成功：该文件已关闭！\n");
	return 1;
}
/************************************************************************/
/* 实验信息                                                             */
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
		"*                            文件系统                                  *\n"
		"*                       黄东 16281255 安全1601                         *\n"
		"*                            2019.6.9                                  *\n"
		"************************************************************************\n"
		"***********************%s************************\n"
		"************************************************************************\n",NOW);
}
/************************************************************************/
/* 功能介绍说明                                                         */
/************************************************************************/
void MenuShow()
{
	printf(
		"************************************************************************\n"
		"*                            操作指令集                                *\n"
		"*         文件夹_创建：mkdir <dirname>                                 *\n"
		"*         文件夹_删除：rmdir <dirname>                                 *\n"
		"*         文件夹_显示：ls <dirname>                                    *\n"
		"*         文件夹_更改：cd <dirname>                                    *\n"
		"*         文件_创建打开：create <filename>                             *\n"
		"*         文件_选择写入：write <filename>                              *\n"
		"*         文件_选择读取：read <filename>                               *\n"
		"*         文件_选择打开：open <filename>                               *\n"
		"*         文件_选择关闭：close <filename>                              *\n"
		"*         文件_选择删除：rm <filename>                                 *\n"
		"*         系统_磁盘清空：format                                        *\n"
		"*         系统_安全退出：exit                                          *\n"
		"************************************************************************\n");
}
/************************************************************************/
/* 退出目录                                                             */
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
		"*                            文件系统                                  *\n"
		"*                       黄东 16281255 安全1601                         *\n"
		"************************************************************************\n"
		"***********************%s************************\n"
		"************************************************************************\n"
		"*                          欢迎使用本系统                              *\n"
		"*                             下次再见                                 *\n"
		"************************************************************************\n",NOW);
}
/************************************************************************/
/* 主函数                                                               */
/************************************************************************/
int main()
{	
	AuthorShow();
	system("PAUSE");
	system("cls");
	MenuShow();
	system("PAUSE");
	openlist=new OPENLIST;//创建用户文件打开表
	BaseAddr=(char *)malloc(DiskSize);//申请虚拟空间并且初始化
	osPoint=(struct DISK *)(BaseAddr);//虚拟磁盘初始化	
	if((fp=fopen(FilePath,"r"))!=NULL){//加载磁盘文件
		fread(BaseAddr,sizeof(char),DiskSize,fp);
		printf("\n磁盘已加载，您处于%s：\n\n",FilePath);
	}
	else{
		printf("欢迎使用本文件管理系统！\t正在初始化...\n");
		format();
		printf("初始化已完成！现在请使用，祝您使用愉快：\n\n");
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
			cout<<"指令无效,请重新输入:"<<endl;
	}
	Menu_Exit();
	return 1;
}
