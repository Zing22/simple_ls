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

//各个数据的宽度
unsigned int w_nlink,w_uid,w_gid,w_size,w_time,w_name;
//是否为长输出
unsigned int isLong = 0;
//块总占用数
unsigned long total = 0;
//是否显示隐藏文件
unsigned int f_printAll = 0;
//是否反向排序
unsigned int isReverse = 0;
//储存文件数量
int fileNum = 0;

struct Filelist
{
	char *name;
	int isLink;
	char *linkTo;
	char mtime[128];
	unsigned int column;
	struct stat* buf;
	char *path;
};
//用于储存数据的结构体组的指针files
struct Filelist **files = NULL;
//当前能存储的最大文件数量
int maxNum = INITNUMBER;

//处理参数
int dealArgv(int ,char *[]);
//读取文件夹的文件
int readFile(const char *);
//结构体排序所用参数函数
int mycmp(const void *,const void *);
//文件夹名字排序
int dir_cmp(const void *,const void *);
//计算名字宽度
int name_width(char * const );
//计算长输出的各个宽度
int width_for_long(struct Filelist **,int );
//输出权限
int print_per(struct Filelist *);
//输出nlink
int print_nlink(struct Filelist *);
//输出UId和GId
int print_Ids(struct Filelist *);
//输出文件大小
int print_size(struct Filelist *);
//输出mtime
int print_mtime(struct Filelist *);
//输出名字
int print_name(struct Filelist *);
//输出软链接目标文件
int print_link_to(struct Filelist *);
//长输出
int long_print(struct Filelist **,int );
//短输出
int short_print(struct Filelist **,int );

//主函数
int main(int argc,char *argv[])
{
	setlocale(LC_ALL,"");
	dealArgv(argc,argv);
	return 0;
}

int dealArgv(int argc,char *argv[])
{
	int i,fiNum=0,dirNum=0;
	char **fiList = (char**)malloc(argc*sizeof(char*));
	char **dirList = (char**)malloc(argc*sizeof(char*));
	struct stat info;
	int len=0;
	for( i=1 ; i<argc ; ++i )
	{
		if(strcmp(argv[i],"-l")==0)
		{
			isLong = 1;
			continue;
		}
		if(strcmp(argv[i],"-r")==0)
		{
			isReverse = 1;
			continue;
		}
		if( strcmp(argv[i],"-a")==0 )
		{
			f_printAll = 1;
			continue;
		}
		if(stat(argv[i],&info)==0)
		{
			if(S_ISDIR(info.st_mode))
			{
				len = strlen(argv[i]);
				dirList[dirNum] = (char *)malloc(sizeof(char)*(len+1));
				dirList[dirNum][len] = '\0';
				strncpy(dirList[dirNum],argv[i],len+1);
				dirNum++;
			}
			else
			{
				len = strlen(argv[i]);
				fiList[fiNum] = (char *)malloc(sizeof(char)*(len+1));
				fiList[fiNum][len] = '\0';
				strncpy(fiList[fiNum],argv[i],len+1);
				fiNum++;
			}
		}
		else
			printf("\"%s\" No such file or directory!\n",argv[i]);
	}
	
	if( fiNum+dirNum==0 )
	{
		//初始化结构体组指针的内存空间
		files = (struct Filelist **)malloc(sizeof(struct Filelist*)*INITNUMBER);
		readFile(".");
		if(isLong)
		{
			printf("total:%lu\n",total);
			long_print(files,fileNum);
		}
		else short_print(files,fileNum);
		//释放结构体组指针所占的内存空间
		free(files);
	}
	else
	{
		unsigned int temLen;
		if( fiNum )
		{
			//初始化结构体组指针的内存空间
			struct Filelist **temList = (struct Filelist **)malloc(sizeof(struct Filelist*)*fiNum);
			for( i=0 ; i<fiNum ; ++i )
			{
				temLen = strlen(fiList[i]);
				temList[i] = (struct Filelist *)malloc(sizeof(struct Filelist));
				temList[i]->name = (char *)malloc(sizeof(char)*(temLen+1));
				temList[i]->name[temLen] = '\0';
				strncpy(temList[i]->name,fiList[i],temLen);
				temList[i]->column = name_width(temList[i]->name);
				//更新最大文件名长度
				if( temList[i]->column > w_name )
					w_name = temList[i]->column;
				if( isLong )
				{
					temList[i]->buf = (struct stat*)malloc(sizeof(struct stat));
					temList[i]->path = (char*)malloc(sizeof(char)*(temLen+1));
					temList[i]->path[temLen] = '\0';
					lstat(temList[i]->name,temList[i]->buf);
					strncpy(temList[i]->path,fiList[i],temLen);
				}
				//释放内存
				free(fiList[i]);
			}
			//文件夹名字排序
			qsort(temList,fiNum,sizeof(temList[0]),mycmp);
			//输出
			if(isLong)
			{
				width_for_long(temList,fiNum);
				long_print(temList,fiNum);
			}
			else
				short_print(temList,fiNum);
			free(temList);
			//初始化
			w_nlink=w_uid=w_gid=w_size=w_time=w_name=total=0;
			printf("\n");
		}
		if( dirNum )
		{
			qsort(dirList,dirNum,sizeof(dirList[0]),dir_cmp);
			for( i=0 ; i<dirNum ; ++i )
			{
				//初始化结构体组指针的内存空间
				files = (struct Filelist **)malloc(sizeof(struct Filelist*)*INITNUMBER);
				//读取数据
				readFile(dirList[i]);
				//输出
				if(fiNum+dirNum>1)
					printf("%s:\n",dirList[i]);
				if(isLong)
				{
					printf("total:%lu\n",total);
					long_print(files,fileNum);
				}
				else
					short_print(files,fileNum);
				free(files);
				free(dirList[i]);
				//初始化
				w_nlink=w_uid=w_gid=w_size=w_time=w_name=total=0;
				printf("\n");
			}
		}
	}

	free(fiList);
	free(dirList);
	return 0;
}

size_t length;
unsigned int temColumn;
int readFile(const char *path)
{
	DIR	*dir = NULL;
	struct dirent *ptr = NULL;
	dir = opendir(path);
	maxNum = INITNUMBER;
	fileNum=0;
	while((ptr=readdir(dir)) != NULL)
	{
		if(ptr->d_name[0] == '.' && !f_printAll)
			continue;
		//使结构体组指针内存空间翻倍
		if( fileNum == maxNum )
		{
			maxNum *= 2;
			files = (struct Filelist**)realloc(files,sizeof(struct Filelist*)*maxNum);
		}
		//开辟内存空间储存名字和路径
		length = strlen(ptr->d_name);
		files[fileNum]=(struct Filelist*)malloc(sizeof(struct Filelist));
		files[fileNum]->mtime[127] = '\0';
		files[fileNum]->name = (char*)malloc((length+1)*sizeof(char));
		files[fileNum]->name[length] = '\0';
		strncpy(files[fileNum]->name,ptr->d_name,length);
		files[fileNum]->column = name_width(ptr->d_name);
		//更新最大文件名长度
		if( files[fileNum]->column > w_name )
			w_name = files[fileNum]->column;
		//如果是长输出
		if(isLong)
		{
			files[fileNum]->buf = (struct stat*)malloc(sizeof(struct stat));
			if(strcmp(path,".")==0)
			{
				files[fileNum]->path = (char *)malloc(sizeof(char)*2);
				strcpy(files[fileNum]->path,".");
				lstat(files[fileNum]->name,files[fileNum]->buf);
			}
			else
			{
				files[fileNum]->path = (char *)malloc(sizeof(char)*(strlen(files[fileNum]->name)+strlen(path)+3));
				files[fileNum]->path[0] = '\0';
				strcpy(files[fileNum]->path,path);
				strcat(files[fileNum]->path,"/");
				strcat(files[fileNum]->path,files[fileNum]->name);
				lstat(files[fileNum]->path,files[fileNum]->buf);
			}	
		}
		++fileNum;
	}
	if(isLong)
		width_for_long(files,fileNum);
	//按文件名升序排序
	qsort(files,fileNum,sizeof(files[0]),mycmp);
	closedir(dir);
	return 0;
}
//结构体排序
int mycmp(const void *a,const void *b)
{
	if (isReverse == 0)
		return strcasecmp((*(struct Filelist**)a)->name,(*(struct Filelist**)b)->name);
	else
		return strcasecmp((*(struct Filelist**)b)->name,(*(struct Filelist**)a)->name);
}
//文件夹排序
int dir_cmp(const void *a,const void *b)
{
	if (isReverse == 0)
		return strcasecmp((char *)a,(char *)b);
	else
		return strcasecmp((char *)b,(char *)a);
}
//名字显示宽度
wchar_t temp[256];
int name_width(char * const name)
{
	if( mbstowcs(temp,name,256*sizeof(wchar_t))!=(size_t)-1 )
		return wcswidth(temp,sizeof(wchar_t)*256);
	printf("mbstowcs error!\n");
	return -1;
}
//计算长输出的各个宽度
unsigned int tempLen=0;
struct passwd *data1 = NULL;
struct group *data2 = NULL;
struct tm *mtm = NULL;
int width_for_long(struct Filelist ** list,int num)
{
	int i=0;
	for(  ; i<num ; i++ )
	{
		//n_link的宽度
		tempLen = snprintf(NULL,0,"%lu",list[i]->buf->st_nlink);
		if(tempLen > w_nlink) w_nlink = tempLen;
		//文件大小的宽度
		tempLen = snprintf(NULL,0,"%ld",list[i]->buf->st_size);
		if(tempLen > w_size) w_size = tempLen;
		//UId和GId的宽度
		data1 = getpwuid(list[i]->buf->st_uid);
		data2 = getgrgid(list[i]->buf->st_gid);
		if(strlen(data1->pw_name) > w_uid) w_uid = strlen(data1->pw_name);
		if(strlen(data2->gr_name) > w_gid) w_gid = strlen(data2->gr_name);
		//mtime宽度
		mtm = localtime(&list[i]->buf->st_mtime);
		strftime(list[i]->mtime,128,"%b %e %R",mtm);
		if(strlen(list[i]->mtime) > w_time) w_time = strlen(list[i]->mtime);
		//total
		total += list[i]->buf->st_blocks;
	}
	return 0;
}
//输出权限
int print_per(struct Filelist * file)
{
	file->isLink = 0;
	if(S_ISCHR(file->buf->st_mode))
		putchar('c');
	else if(S_ISBLK(file->buf->st_mode))
		putchar('b');
	else if(S_ISLNK(file->buf->st_mode))
	{
		file->isLink = 1;
		putchar('l');
	}
	else if(S_ISSOCK(file->buf->st_mode))
		putchar('s');
	else if(S_ISDIR(file->buf->st_mode))
		putchar('d');
	else if(S_ISREG(file->buf->st_mode))
		putchar('-');
	else
		putchar('p');

	if(file->buf->st_mode & S_IRUSR) putchar('r');
	else putchar('-');
	if(file->buf->st_mode & S_IWUSR) putchar('w');
	else putchar('-');
	if(file->buf->st_mode & S_ISUID) putchar('s');
	else if(file->buf->st_mode & S_IXUSR) putchar('x');
	else putchar('-');

	if(file->buf->st_mode & S_IRGRP) putchar('r');
	else putchar('-');
	if(file->buf->st_mode & S_IWGRP) putchar('w');
	else putchar('-');
	if(file->buf->st_mode & S_ISGID) putchar('s');
	else if(file->buf->st_mode & S_IXGRP) putchar('x');
	else putchar('-');

	if(file->buf->st_mode & S_IROTH) putchar('r');
	else putchar('-');
	if(file->buf->st_mode & S_IWOTH) putchar('w');
	else putchar('-');
	if(file->buf->st_mode & S_ISVTX) putchar('t');
	else if(file->buf->st_mode & S_IXOTH) putchar('x');
	else putchar('-');

	putchar(' ');
	return 0;
}
//输出nlink
int print_nlink(struct Filelist * file)
{
	printf("%*lu ",w_nlink,file->buf->st_nlink);
	return 0;
}
//输出UId和GId
int print_Ids(struct Filelist * file)
{
	printf("%*s %*s ",w_uid,getpwuid(file->buf->st_uid)->pw_name,
				w_gid,getgrgid(file->buf->st_gid)->gr_name);
	return 0;
}
//输出文件大小
int print_size(struct Filelist * file)
{
	printf("%*ld ",w_size,file->buf->st_size);
	return 0;
}
//输出mtime
int print_mtime(struct Filelist * file)
{
	printf("%*s ",w_time,file->mtime);
	return 0;
}
//输出名字
int print_name(struct Filelist * file)
{
	if( isLong )
		printf("%-s ",file->name);
	else
		printf("%-s%*s",file->name,w_name-(file->column)+2,"");
	free(file->name);
	return 0;
}
//输出软链接目标文件
unsigned int sz;
int print_link_to(struct Filelist * file)
{
	file->linkTo = (char*)malloc(file->buf->st_size+1);
	sz = readlink(file->path,file->linkTo,file->buf->st_size+1);
	file->linkTo[sz] = '\0';
	printf("-> %-s",file->linkTo);
	free(file->linkTo);
	return 0;
}
//长输出
int long_print(struct Filelist ** list,int num)
{
	int i=0;
	for(  ; i<num ; ++i )
	{
		print_per(list[i]);
		print_nlink(list[i]);
		print_Ids(list[i]);
		print_size(list[i]);
		print_mtime(list[i]);
		print_name(list[i]);
		if( list[i]->isLink )
			print_link_to(list[i]);
		free(list[i]->path);
		free(list[i]->buf);
		free(list[i]);
		printf("\n");
	}
	return 0;
}
//短输出
int short_print(struct Filelist ** list,int num)
{
	struct winsize size;
	ioctl(STDIN_FILENO,TIOCGWINSZ,&size);
	size.ws_col = size.ws_col;
	int colNum = size.ws_col / (w_name+2);
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
				print_name(list[temp]);
				free(list[temp]);
			}
			else
				printf("%-*s",w_name,"");
		}
		printf("\n");
	}
	return 0;
}

