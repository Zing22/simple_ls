#include    <stdio.h>
#include    <stdlib.h>
#include    <unistd.h>
#include    <dirent.h>
#include    <string.h>
#include    <sys/param.h>
#include    <sys/stat.h>
#include    <grp.h>
#include    <pwd.h>
#include    <sys/types.h>
#include    <stddef.h>
#include    <time.h>
#include    <locale.h>
#include    <sys/ioctl.h>
#include    <termios.h>
#include    <wchar.h>

#define     INITNUMBER 4

//	各个数据的宽度
unsigned int w_nlink,w_uid,w_gid,w_size,w_time,w_name;
//	是否为长输出
unsigned int isLong = 0;
//	块总占用数
unsigned long total = 0;

struct Filelist
{
	char *name;
	char permission[11];
	int filelength;
	int nlink;
	char *uid,*gid;
	char mtime[128];
	char *linkTo;
	int column;
};
struct Filelist **files = NULL;	//用于储存数据的结构体组的指针files
int maxNum = INITNUMBER;		//当前能存储的最大文件数量

//结构体排序所用参数函数
int mycmp(const void *a,const void *b)
{
	return strcasecmp((*(struct Filelist**)a)->name,(*(struct Filelist**)b)->name);
}

wchar_t temp[256];
int nameColumn(char * const name)
{
	setlocale(LC_ALL,"");
	if( mbstowcs(temp,name,256*sizeof(wchar_t))!=(size_t)-1 )
		return wcswidth(temp,sizeof(wchar_t)*256);
	printf("mbstowcs error!\n");
	return -1;
}

struct stat buf;
struct passwd *data1 = NULL;
struct group *data2 = NULL;
struct tm *mtm = NULL;
unsigned int tempLen=0;
ssize_t sz=0;
//读取文件属性数据
int readAll(char *filename,int filenum,struct Filelist** files)
{
	//stat函数获取文件属性
	lstat(filename,&buf);

	//size of file
	files[filenum]->filelength = buf.st_size;
	tempLen = snprintf(NULL,0,"%i",files[filenum]->filelength);
	if(tempLen > w_size) w_size = tempLen;
	//permission of file
	memset(files[filenum]->permission,'-',11*sizeof(char));
	if(S_ISCHR(buf.st_mode))
		files[filenum]->permission[0]='c';
	else if(S_ISBLK(buf.st_mode))
		files[filenum]->permission[0]='b';
	else if(S_ISLNK(buf.st_mode))
		files[filenum]->permission[0]='l';
	else if(S_ISSOCK(buf.st_mode))
		files[filenum]->permission[0]='s';
	else if(S_ISDIR(buf.st_mode))
		files[filenum]->permission[0]='d';
	else if(S_ISREG(buf.st_mode))
		files[filenum]->permission[0]='-';
	else
		files[filenum]->permission[0]='p';
		
	if(buf.st_mode & S_IRUSR) files[filenum]->permission[1] = 'r';
	if(buf.st_mode & S_IWUSR) files[filenum]->permission[2] = 'w';
	if(buf.st_mode & S_IXUSR) files[filenum]->permission[3] = 'x';
	if(buf.st_mode & S_IRGRP) files[filenum]->permission[4] = 'r';
	if(buf.st_mode & S_IWGRP) files[filenum]->permission[5] = 'w';
	if(buf.st_mode & S_IXGRP) files[filenum]->permission[6] = 'x';
	if(buf.st_mode & S_IROTH) files[filenum]->permission[7] = 'r';
	if(buf.st_mode & S_IWOTH) files[filenum]->permission[8] = 'w';
	if(buf.st_mode & S_IXOTH) files[filenum]->permission[9] = 'x';
	if(buf.st_mode & S_ISUID) files[filenum]->permission[3] = 's';
	if(buf.st_mode & S_ISGID) files[filenum]->permission[6] = 's';
	if(buf.st_mode & S_ISVTX) files[filenum]->permission[9] = 't';
	files[filenum]->permission[10] = '\0';
	//nlink of file
	files[filenum]->nlink = buf.st_nlink;
	tempLen = snprintf(NULL,0,"%i",files[filenum]->nlink);
	if(tempLen > w_nlink) w_nlink = tempLen;
	//UID & GID of file
	data1 = getpwuid(buf.st_uid);
	data2 = getgrgid(buf.st_gid);
	files[filenum]->uid = data1->pw_name;
	files[filenum]->gid = data2->gr_name;
	if(strlen(files[filenum]->uid) > w_uid) w_uid = strlen(files[filenum]->uid);
	if(strlen(files[filenum]->gid) > w_gid) w_gid = strlen(files[filenum]->gid);
	//mtime of file
	mtm = localtime(&buf.st_mtime);
	setlocale(LC_TIME,"");
	strftime(files[filenum]->mtime,128,"%b %e %R",mtm);
	if(strlen(files[filenum]->mtime) > w_time) w_time = strlen(files[filenum]->mtime);
	//readlink
	if( files[filenum]->permission[0]=='l' )
	{
		files[filenum]->linkTo = (char*)malloc(buf.st_size+1);
		sz = readlink(filename,files[filenum]->linkTo,buf.st_size+1);
		files[filenum]->linkTo[sz] = '\0';
	}
	//total
	total += buf.st_blocks;
	return 0;
}

int fileNum = 0;
size_t length;
int readFile(const char *path)
{
	DIR	*dir = NULL;
	struct dirent *ptr = NULL;
	char *temName = NULL;
	dir = opendir(path);
	maxNum = INITNUMBER;
	fileNum=0;
	while((ptr=readdir(dir)) != NULL)
	{
		if(ptr->d_name[0] == '.')
			continue;

		//length = realLenth(ptr->d_name,strlen(ptr->d_name));	//ptr->d_name 的大小
		length = strlen(ptr->d_name);

		if( length > w_name )					//更新最大文件名长度
			w_name = length;

		if( fileNum == maxNum )				//使结构体组指针内存空间翻倍
		{
			maxNum *= 2;
			files = (struct Filelist**)realloc(files,sizeof(struct Filelist*)*maxNum);
		}
		files[fileNum]=(struct Filelist*)malloc(sizeof(struct Filelist));	//为每一个结构体开辟内存空间
		files[fileNum]->name = (char*)malloc((length+1)*sizeof(char));		//为每个文件名C-sting开辟内存空间
		memset(files[fileNum]->name,'\0',(length+1)*sizeof(char));
		strncpy(files[fileNum]->name,ptr->d_name,length);
		files[fileNum]->column = nameColumn(ptr->d_name);
		if(isLong)
		{
			if(strcmp(path,".")==0)
				readAll(files[fileNum]->name,fileNum,files);			//读写其他文件属性数据
			else
			{
				temName = (char *)malloc(sizeof(char)*(strlen(files[fileNum]->name)+strlen(path)+3));
				memset(temName,'\0',(strlen(files[fileNum]->name)+strlen(path)+3));
				strcpy(temName,path);
				strcat(temName,"/");
				strcat(temName,files[fileNum]->name);
				readAll(temName,fileNum,files);
				free(temName);
			}	
		}
		++fileNum;
	}

	qsort(files,fileNum,sizeof(files[0]),mycmp);	//按文件名升序排序

	closedir(dir);
	return 0;
}

//	长输出
int long_printf(struct Filelist** list,int num)
{
	printf("total %ld\n",total/2);
	int i;
	for( i = 0 ; i < num ; ++i )				//输出并且free内存空间
	{
		printf("%s %*i %*s %*s %*i %*s %-s",
				list[i]->permission, w_nlink, 
				list[i]->nlink, w_uid, list[i]->uid,
				w_gid, list[i]->gid, w_size, 
				list[i]->filelength, w_time,
				list[i]->mtime,
				list[i]->name);
		if( list[i]->permission[0]=='l' )
		{
			printf(" -> %s\n",list[i]->linkTo);
			free(list[i]->linkTo);
		}
		else
		  printf("\n");
		free(list[i]->name);
		free(list[i]);
	}
	return 0;
}

//	短输出
int simple_print(struct Filelist** list,int num)
{
	int maxColumn=0;
	struct winsize size;
	ioctl(STDIN_FILENO,TIOCGWINSZ,&size);
	maxColumn = size.ws_col;
	int colNum = maxColumn / (w_name+2);
	int rowNum = (num+colNum-1) / colNum;
	int r,c;
	int temp;
	for( r=0 ; r<rowNum ; ++r )
	{
		for( c=0 ; c<colNum ; ++c )
		{
			temp = c*rowNum+r;
			if(temp<num)
			{
				printf("%-s",list[temp]->name);
				printf("%*s",w_name-list[temp]->column+2,"");
				free(list[temp]->name);
				free(list[temp]);
			}
			else
				printf("%-*s",w_name," ");
		}
		printf("\n");
	}
	return 0;
}



int main(int argc,char *argv[])
{
	int i,fiNum=0,dirNum=0;
	char **fiList = (char**)malloc(argc*sizeof(char*));
	char **dirList = (char**)malloc(argc*sizeof(char*));
	struct stat info;
	int len=0;
	for( i=1 ; i<argc ; ++i )
	{
		if(!isLong)
		{
			if(strcmp(argv[i],"-l")==0)
			{
				isLong=1;
				continue;
			}
		}
		if(stat(argv[i],&info)==0)
		{
			if(S_ISDIR(info.st_mode))
			{
				len = strlen(argv[i]);
				dirList[dirNum] = (char *)malloc(sizeof(char)*(len+1));
				memset(dirList[dirNum],'\0',sizeof(char)*(len+1));
				strncpy(dirList[dirNum],argv[i],len+1);
				dirNum++;
			}
			else
			{
				len = strlen(argv[i]);
				fiList[fiNum] = (char *)malloc(sizeof(char)*(len+1));
				memset(fiList[fiNum],'\0',sizeof(char)*(len+1));
				strncpy(fiList[fiNum],argv[i],len+1);
				fiNum++;
			}
		}
		else
			if(strcmp(argv[i],"-l")!=0)
				printf("\"%s\" is an invalid parameter!\n",argv[i]);
	}
	
	if( fiNum+dirNum==0 )
	{
		files = (struct Filelist **)malloc(sizeof(struct Filelist*)*INITNUMBER);//初始化结构体组指针的内存空间
		readFile(".");
		if(isLong)
		{
			long_printf(files,fileNum);
		}
		else simple_print(files,fileNum);
		free(files);							//释放结构体组指针所占的内存空间
	}
	else
	{
		unsigned int temLen;
		if( fiNum )
		{
			struct Filelist **temList = (struct Filelist **)malloc(sizeof(struct Filelist*)*fiNum);//初始化结构体组指针的内存空间
			for( i=0 ; i<fiNum ; ++i )
			{
				temLen = strlen(fiList[i]);
				if(w_name<temLen) w_name = temLen;
				temList[i] = (struct Filelist *)malloc(sizeof(struct Filelist));
				temList[i]->name = (char *)malloc(sizeof(char)*(temLen+1));
				memset(temList[i]->name,'\0',(temLen+1)*sizeof(char));
				strncpy(temList[i]->name,fiList[i],temLen);
				temList[i]->column = nameColumn(temList[i]->name);
				free(fiList[i]);
				readAll(temList[i]->name,i,temList);
			}
			
			qsort(temList,fiNum,sizeof(temList[0]),mycmp);	//按文件名升序排序

			if(isLong)
				long_printf(temList,fiNum);
			else
				simple_print(temList,fiNum);
			
			free(temList);
			w_name=0;
		}
		if( dirNum )
		{
			for( i=0 ; i<dirNum ; ++i )
			{
				files = (struct Filelist **)malloc(sizeof(struct Filelist*)*INITNUMBER);//初始化结构体组指针的内存空间
				readFile(dirList[i]);
				if(fiNum+dirNum>1)
					printf("%s:\n",dirList[i]);
				if(isLong)
					long_printf(files,fileNum);
				else
					simple_print(files,fileNum);
				free(files);
				free(dirList[i]);
				w_nlink=w_uid=w_gid=w_size=w_time=w_name=total=0;
			}
		}
	}

	free(fiList);
	free(dirList);
	return 0;
}
