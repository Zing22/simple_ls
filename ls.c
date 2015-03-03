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

#define     INITNUMBER 4

//	各个数据的宽度
unsigned int w_nlink,w_uid,w_gid,w_size,w_time,w_name;
//	是否为长输出
unsigned int isLong = 1;
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
	unsigned int chinese;
};
struct Filelist **files = NULL;	//用于储存数据的结构体组的指针files
int maxNum = INITNUMBER;		//当前能存储的最大文件数量

//结构体排序所用参数函数
int mycmp(const void *a,const void *b)
{
	return strcasecmp((*(struct Filelist**)a)->name,(*(struct Filelist**)b)->name);
}

unsigned int realLenth(const char ch[],const size_t oldLength)
{
	unsigned int chinese=0;
	size_t i=0;
	for( ;i<oldLength;++i)
	{
		if( ch[i]<0 )
		{
			chinese++;
			i+=2;
		}
	}
	return chinese;
}

struct stat buf;
struct passwd *data1 = NULL;
struct group *data2 = NULL;
struct tm *mtm = NULL;
unsigned int tempLen=0;
ssize_t sz=0;
//读取文件属性数据
int readAll(char *filename,int filenum)
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
	dir = opendir(path);

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
		files[fileNum]->chinese = realLenth(ptr->d_name,length);
		if(isLong)
			readAll(files[fileNum]->name,fileNum);			//读写其他文件属性数据
		++fileNum;
	}

	qsort(files,fileNum,sizeof(files[0]),mycmp);	//按文件名升序排序

	closedir(dir);
	return 0;
}

//	长输出
int long_printf()
{
	printf("total %ld\n",total/2);
	int i;
	for( i = 0 ; i < fileNum ; ++i )				//输出并且free内存空间
	{
		printf("%s %*i %*s %*s %*i %*s %-s",
				files[i]->permission, w_nlink, 
				files[i]->nlink, w_uid, files[i]->uid,
				w_gid, files[i]->gid, w_size, 
				files[i]->filelength, w_time,
				files[i]->mtime,
				files[i]->name);
		if( files[i]->permission[0]=='l' )
		{
			printf(" -> %s\n",files[i]->linkTo);
			free(files[i]->linkTo);
		}
		else
		  printf("\n");
		free(files[i]->name);
		free(files[i]);
	}
	return 0;
}

//	短输出
int simple_print()
{
	int maxColumn=0;
	struct winsize size;
	ioctl(STDIN_FILENO,TIOCGWINSZ,&size);
	maxColumn = size.ws_col;
	int colNum = maxColumn / (w_name+2);
	int rowNum = (fileNum+colNum-1) / colNum;
	int r,c;
	int temp;
	for( r=0 ; r<rowNum ; ++r )
	{
		for( c=0 ; c<colNum ; ++c )
		{
			temp = c*rowNum+r;
			if(temp<fileNum)
			{
				printf("%-*s",w_name+files[temp]->chinese+2,files[temp]->name);
				free(files[temp]->name);
				free(files[temp]);
			}
			else
				printf("%-*s",w_name," ");
		}
		printf("\n");
	}
	return 0;
}

int main(int argc, char *argv[])
{
	files = (struct Filelist **)malloc(sizeof(struct Filelist*)*INITNUMBER);//初始化结构体组指针的内存空间
	if( argc > 1 )
	{
		if(strcmp(argv[1],"-l")==0)//这里vim提示我不要用argv[1]=="-l"...
		{
			readFile(".");					//读取数据
			long_printf();
		}
		else
			printf("Invalid parameter!\n");
	}
	else
	{
		isLong = 0;
		readFile(".");					//读取数据
		simple_print();
	}
	free(files);							//释放结构体组指针所占的内存空间
	return 0;
}
