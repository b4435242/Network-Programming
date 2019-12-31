#include <stdio.h>
#include<errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <arpa/inet.h>
int maxsize=1024;

int readline(int fd,char *buf,size_t num){
	int n=num,nread=0;
	char *ptr=buf;
	while(1){
		int flag=read(fd,ptr,1);
		if(flag==-1){
			if(errno==EINTR)
				continue;
			else{
				printf("read error\n");
				return -1;
			}
		}
		else if(flag==0){
			printf("Server offline\n");
			close(fd);
			exit(0);
		}	
		else if(flag==1){
			nread++;
			if(!strncmp(ptr,"\n",1))
				return nread;
			
			ptr++;
		}
	}
}

ssize_t writen(int fd,char *buf,size_t num){
        ssize_t res;
        size_t n=1;
    	char *ptr=buf;
    	while(strncmp(ptr,"\n",1)){
        	n++;
        	ptr++;
    	}   
        ptr=buf;
        while(n>0){
                //printf("write msg:%s",ptr);
                if((res=write(fd,ptr,n))<=0){
                        if(errno==EINTR)
                                res=0;
                        else    
                                return -1;
                }               
                ptr+=res;       
                n-=res;
        }       
     	return num;
}

int main(int argc, char *argv[]){
	if(argc!=3)
		exit(0);

	int sockfd=0;
	sockfd=socket(AF_INET,SOCK_STREAM,0);
	//if(sockfd>0)
		//printf("create a socket with fd=%d \n",sockfd);

	struct sockaddr_in info;
	bzero(&info,sizeof(info));
	info.sin_family=AF_INET;
	//inet_pton(AF_INET,argv[1],&info.sin_addr);
	//info.sin_addr.s_addr=htonl(info.sin_addr.s_addr);
	info.sin_port=htons(atoi(argv[2]));
	info.sin_addr.s_addr=inet_addr(argv[1]);
	int err=connect(sockfd,(struct sockaddr*) &info,sizeof(info));
	if(err!=0)
		printf("connect failed\n");
	int stdineof=0;
	/*struct sockaddr_in testsock;
	bzero(&testsock,sizeof(testsock));
	socklen_t testlen=sizeof(testsock);
	getsockname(sockfd,(struct sockaddr*) &testsock,&testlen);
	printf("ip:%s port:%d\n",inet_ntoa(testsock.sin_addr),ntohs(testsock.sin_port));*/
	fd_set rset,allset;
	char recvline[maxsize],sendline[maxsize],*ptr;
	FD_ZERO(&allset);
	FD_SET(0,&allset);
	FD_SET(sockfd,&allset);
	int maxfd=sockfd+1;		
	for(;;){
		rset=allset;
		select(maxfd,&rset,NULL,NULL,NULL);

		if(FD_ISSET(0,&rset)){
			memset(sendline,0,sizeof(sendline));
			ptr=fgets(sendline,maxsize,stdin);
			if(ptr==NULL)
                  	      continue;
               		if(!strncmp(sendline,"exit",4)){
                        	close(sockfd);
                        	exit(0);
			}
			writen(sockfd,sendline,sizeof(sendline));
					
		}
		if(FD_ISSET(sockfd,&rset)){
			int n;
			memset(recvline,0,sizeof(recvline));
			if((n=readline(sockfd,recvline,maxsize))>0){
				printf("%s",recvline);	
			}
		}
	
	}
	return 0;
}	
