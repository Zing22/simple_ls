#include    <stdio.h>
#include    <stdlib.h>
#include    <unistd.h>
#include    <dirent.h>
#include    <string.h>
#include    <malloc.h>
#include    <sys/param.h>
#include    <sys/stat.h>
#include    <grp.h>
#include    <pwd.h>
#include    <sys/types.h>
#include    <stddef.h>
#include    <time.h>

#define     MAXBUFSIZE PATH_MAX
#define     INITSIZE 2
#define     MAXFILENAME PATH_MAX 
#define		INITNUMBER 4

//	各个数据的宽度
unsigned int w_nlink,w_uid,w_gid,w_size,w_time,w_name;

struct Filelist
{
	char *name;
	char *permission;
	int filelength;
	int nlink;
	char *uid,*gid;
	char mtime[128];
};
struct Filelist **files = NULL;	//用于储存数据的结构体组的指针files
int maxNum = INITNUMBER;		//当前能存储的最大文件数量

//结构体排序所用参数函数
int mycmp(const void *a,const void *b)
{
	return strcasecmp((*(struct Filelist**)a)->name,(*(struct Filelist**)b)->name);
}

struct stat buf;
struct passwd *data1 = NULL;
struct group *data2 = NULL;
struct tm *mtm = NULL;
char nameLen[50];
unsigned int tempLen=0;
//读取文件属性数据
int readAll(const char *path,char *filename,int filenum)
{
	//获取文件路径，存入str中
	char *str = (char*)malloc((strlen(path)+strlen(filename)+2)*sizeof(char));
	memset(str,'\0',(strlen(path)+strlen(filename)+2)*sizeof(char));
	strcpy(str,path);
	strcat(str,"/");
	strcat(str,filename);
	strcat(str,"\0");
	//stat函数获取文件属性
	stat(str,&buf);

	//size of file
	files[filenum]->filelength = buf.st_size;
	tempLen = sprintf(nameLen,"%i",files[filenum]->filelength);
	if(tempLen > w_size) w_size = tempLen;
	//permission of file
	files[filenum]->permission = (char*)malloc(11*sizeof(char));
	memset(files[filenum]->permission,'-',11*sizeof(char));
	if(S_ISDIR(buf.st_mode))
		files[filenum]->permission[0] = 'd';
	if(buf.st_mode & S_IRUSR) files[filenum]->permission[1] = 'r';
	if(buf.st_mode & S_IWUSR) files[filenum]->permission[2] = 'w';
	if(buf.st_mode & S_IXUSR) files[filenum]->permission[3] = 'x';
	if(buf.st_mode & S_IRGRP) files[filenum]->permission[4] = 'r';
	if(buf.st_mode & S_IWGRP) files[filenum]->permission[5] = 'w';
	if(buf.st_mode & S_IXGRP) files[filenum]->permission[6] = 'x';
	if(buf.st_mode & S_IROTH) files[filenum]->permission[7] = 'r';
	if(buf.st_mode & S_IWOTH) files[filenum]->permission[8] = 'w';
	if(buf.st_mode & S_IXOTH) files[filenum]->permission[9] = 'x';
	files[filenum]->permission[10] = '\0';
	//nlink of file
	files[filenum]->nlink = buf.st_nlink;
	tempLen = sprintf(nameLen,"%i",files[filenum]->nlink);
	if(tempLen > w_nlink) w_nlink = tempLen;
	//UID & GID of file
	data1 = getpwuid(buf.st_uid);
	data2 = getgrgid(buf.st_gid);
	files[filenum]->uid = data1->pw_name;
	files[filenum]->gid = data2->gr_name;
	if(strlen(files[filenum]->uid) > w_uid) w_uid = strlen(files[filenum]->uid);
	if(strlen(files[filenum]->gid) > w_gid) w_gid = strlen(files[filenum]->gid);
	//mtime of file
	mtm = localtime(&buf.st_mtim.tv_sec);
	strftime(files[filenum]->mtime,128,"%m月 %e %R",mtm);
	if(strlen(files[filenum]->mtime) > w_time) w_time = strlen(files[filenum]->mtime);

	free(str);
	return 0;
}

int fileNum = 0;
int readFile(const char *path)
{
	DIR	*dir = NULL;
	struct dirent *ptr = NULL;
	dir = opendir(path);
	size_t sum = 0;				//已经存储的文件数量

	while((ptr=readdir(dir)) != NULL)
	{
		size_t length = strlen(ptr->d_name);	//ptr->d_name 的大小
		if(ptr->d_name[0] == '.')
			continue;

		if( length > w_name )					//更新最大文件名长度
			w_name = length;

		files[fileNum]=(struct Filelist*)malloc(sizeof(struct Filelist));	//为每一个结构体开辟内存空间
		files[fileNum]->name = (char*)malloc((length+1)*sizeof(char));		//为每个文件名C-sting开辟内存空间
		memset(files[fileNum]->name,'\0',(length+1)*sizeof(char));
		strncpy(files[fileNum]->name,ptr->d_name,length);
		readAll(path,files[fileNum]->name,fileNum);			//读写其他文件属性数据
		++fileNum;
		sum = fileNum * sizeof(char*);
		if( fileNum >= maxNum/2 )				//使结构体组指针内存空间翻倍
		{
			maxNum *= 2;
			files = (struct Filelist**)realloc(files,sizeof(struct Filelist*)*maxNum);
		}
	}

	qsort(files,fileNum,sizeof(files[0]),mycmp);	//按文件名升序排序

	closedir(dir);
	return 0;
}

//	长输出
int long_printf()
{
	int i;
	for( i = 0 ; i < fileNum ; ++i )				//输出并且free内存空间
	{
		printf("%s %*i %*s %*s %*i %*s %-*s\n",files[i]->permission, w_nlink, files[i]->nlink, w_uid, files[i]->uid, w_gid, files[i]->gid, w_size, files[i]->filelength, w_time, files[i]->mtime, w_name, files[i]->name);
		free(files[i]->permission);
		free(files[i]->name);
		free(files[i]);
	}
	return 0;
}

//	短输出
int simple_print()
{
	int i;
	for( i = 0 ; i < fileNum ; ++i )				//输出并且free内存空间
	{
		printf("%s\n",files[i]->name);
		free(files[i]->permission);
		free(files[i]->name);
		free(files[i]);
	}
	return 0;
}

int main(int argc, char *argv[])
{
	files = (struct Filelist **)malloc(sizeof(struct Filelist*)*INITNUMBER);//初始化结构体组指针的内存空间
	char currentPath[MAXBUFSIZE];
	getcwd(currentPath,MAXBUFSIZE);			//读取当前路径
	printf("current path : %s\n",currentPath);
	readFile(currentPath);					//读取数据
	const char para[] = "-l";
	if( argc==2 )
	{
		strcmp(argv[1],para)==0;
		long_printf();
	}
	else if( argc!=1 )
	{
		int i;
		for( i = 0 ; i < fileNum ; ++i )	//free内存空间
		{
			free(files[i]->permission);
			free(files[i]->name);
			free(files[i]);
		}
		printf("Invalid parameter!\n");
	}
	else
	{
		simple_print();
	}
	free(files);							//释放结构体组指针所占的内存空间
	exit(0);
}
