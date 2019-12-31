#pragma once
#include "utils.hpp"
using namespace std;

extern int get_file_size(string filename){
    FILE *p_file = NULL;
    p_file = fopen(filename.c_str(),"rb");
    fseek(p_file,0,SEEK_END);
    int size = ftell(p_file);
    fclose(p_file);
    return size;
}

void progress_bar(int status,char* filename,int total,int sent,string pid_info){
	if(status==Init_Download || status==Init_Upload){
		if(status==Init_Download)
			printf("%s [Download] %s Start!\n",pid_info.c_str(),filename);
		else
			printf("%s [Upload] %s Start!\n",pid_info.c_str(),filename);
	}
	else if(status==Process){
		static int print_tag=0;
		int number_of_tag=floor((double(sent)/total) *20);
		
		printf("\r%s Progress : [",pid_info.c_str());
		for(int i=0;i<number_of_tag;i++)
			printf("#");
		for(int i=0;i<20-number_of_tag;i++)
			printf(" ");
		printf("]");
	}
	else if(status==End_Download){
		printf("\n%s [Download] %s Finish!\n",pid_info.c_str(),filename);
	}
	else if(status==End_Upload){
		printf("\n%s [Upload] %s Finish!\n",pid_info.c_str(),filename);
	}

}

int readline(int fd,char *buf){
	int nread=0;
	char *ptr=buf;
	while(1){
		int flag=read(fd,ptr,1);
		if(flag==-1){
			if(errno==EINTR)
				continue;
			else if(errno==EWOULDBLOCK)
				return -1;
			else{
				return -1;
			}
		}
		else if(flag==0){
			return 0;
		}	
		else if(flag==1){
			nread++;
			if(!strncmp(ptr,"\n",1))
				return nread;
			
			ptr++;
		}
	}
}

ssize_t writen(int fd,char *buf,size_t n){
    ssize_t res;
    char *ptr=buf;
	int total=0;
    while(n>0){
		res=write(fd,ptr,n);	
        if(res<=0){
            if(errno==EINTR || errno==EWOULDBLOCK)
                res=0;
            else    
                return -1;
        }
		               
        ptr+=res;       
        n-=res;
		total+=res;
    }       
    return res;
}

int readn(int fd,void *buffer,int length)
{
int bytes_left;
int bytes_read;
char *ptr;
  
bytes_left=length;
while(bytes_left>0)
{
    bytes_read=read(fd,ptr,bytes_read);
    if(bytes_read<0)
    {
      if(errno==EINTR)
         bytes_read=0;
      else
         return(-1);
    }
    else if(bytes_read==0)
        break;
     bytes_left-=bytes_read;
     ptr +=bytes_read;
}
return(length-bytes_left);
}
