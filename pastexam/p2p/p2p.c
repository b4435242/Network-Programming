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

#define maxsize 1024
#define cnum 100
int maxfd=1;
fd_set allset,rset;

struct connectioninfo{
	struct sockaddr_in info;
	int fd;
	char *name;
}connection[cnum];

int readline(int fd,char *buf){
	int nread=0,n;
	char *ptr=buf;
	while(1){
		n=read(fd,ptr,1);
		if(n<0&&errno==EINTR)
			continue;
		else if(n==0)
			return 0;
		else if(!strncmp(ptr,"\n",1))
			return ++nread;
		nread++;
		ptr++;
	}
}

void updatemaxfd(){
	for (int i=maxfd;i>0;--i){
		if(connection[i].fd!=-1)
			maxfd=i+1;
	}
}

void cmd_off(int idx){
	printf("%s has left the chat room\n", connection[idx].name);
	bzero(&connection[idx].info,sizeof(connection[idx].info));
	FD_CLR(connection[idx].fd,&allset);
	connection[idx].fd=-1;
	if(idx==maxfd-1)
		updatemaxfd();
	free(connection[idx].name);
}

void cmd_sendmsg(char *myname,char *input){
	int allsend=0;
	char *name=strtok(input," ");
	char *msg=strtok(NULL,"\n");
	char buf[maxsize];
	memset(buf,0,sizeof(buf));
	sprintf(buf,"%s SAID %s\n",myname,msg);
	printf("%s",buf);
	if(!strncmp(name,"ALL",3))
		allsend=1;
	for(int i=0;i<cnum;i++){
		if(connection[i].fd==-1)
			continue;
		if(!strncmp(name,connection[i].name,strlen(name))||allsend){
			printf("%d\n",connection[i].fd);
			write(connection[i].fd,buf,strlen(buf));}
	}
}

int main(int argc,char *argv[]){
	int nready;
	char myname[maxsize],myrawname[maxsize];
	memset(myname,0,sizeof(myname));
	strcpy(myrawname,argv[1]);
	strcpy(myname,argv[1]);
	strcat(myname,"\n");

	char buf[maxsize];

	FD_ZERO(&allset);

	for(int i=0;i<cnum;i++)
		connection[i].fd=-1;

	for (int i=3;i<argc;i++){
		int idx=i-3,err;
		connection[idx].fd=socket(AF_INET,SOCK_STREAM,0);
		bzero(&connection[idx].info,sizeof(connection[idx].info));
		connection[idx].info.sin_family=AF_INET;
		inet_aton("127.0.0.1",&connection[idx].info.sin_addr);
		connection[idx].info.sin_port=htons(atoi(argv[i]));
		if(connect(connection[idx].fd,(struct sockaddr*) &connection[idx].info,sizeof(connection[idx].info))<0)
			printf("connect err\n");
		printf("cfd=%d\n",connection[idx].fd);
		printf("ip:%s port:%d\n",inet_ntoa(connection[idx].info.sin_addr),ntohs(connection[idx].info.sin_port));
		memset(buf,0,sizeof(buf));
		if(readline(connection[idx].fd,buf)<=0)
			printf("server name err");
		printf("server is %s",buf);
		write(connection[idx].fd,myname,strlen(myname));
		connection[idx].name=strdup(buf);
		FD_SET(connection[idx].fd,&allset);
		if(connection[idx].fd>maxfd)
			maxfd=connection[idx].fd+1;

	}


	struct sockaddr_in server;
	bzero(&server,sizeof(server));
	server.sin_port=htons(atoi(argv[2]));
	server.sin_addr.s_addr=htonl(INADDR_ANY);
	server.sin_family=AF_INET;

	int listenfd=socket(AF_INET,SOCK_STREAM,0);
	if(bind(listenfd,(struct sockaddr*) &server,sizeof(server))<0)
		printf("bind error\n");
	if(listen(listenfd,cnum)<0)
        printf("listen error\n");

    FD_SET(listenfd,&allset);
    FD_SET(0,&allset);

	if(listenfd>maxfd)
		maxfd=listenfd+1;

    for(;;){
    	rset=allset;
    	nready=select(maxfd,&rset,NULL,NULL,NULL);

    	if(FD_ISSET(listenfd,&rset)){
			if(nready--<=0)
				continue;	
    		int idx=0,clen=sizeof(struct sockaddr);
    		while(connection[idx].fd!=-1)
    			idx++;
    		if((connection[idx].fd=accept(listenfd,(struct sockaddr*) &connection[idx].info,&clen))<0)
				printf("errno:%d\n",errno);
			
			printf("listenfd=%d\n",listenfd);	
			printf("clientfd=%d\n",connection[idx].fd);
			printf("ip:%s port:%d\n",inet_ntoa(connection[idx].info.sin_addr),ntohs(connection[idx].info.sin_port));

    		int bytes=write(connection[idx].fd,myname,strlen(myname));
			memset(buf,0,sizeof(buf));
    		readline(connection[idx].fd,buf);
			connection[idx].name=strdup(buf);
			printf("client is %s",buf);
			FD_SET(connection[idx].fd,&allset);
			if(connection[idx].fd>maxfd)
				maxfd=connection[idx].fd+1;

    	}
    	if(FD_ISSET(0,&rset)){
			if(nready--<=0)
				continue;
    		memset(buf,0,sizeof(buf));
    		if(fgets(buf,sizeof(buf),stdin)!=NULL)
    			cmd_sendmsg(myrawname,buf);

    	}
		for (int i=0;i<maxfd;i++){
			if(nready--<=0)
				continue;
			if(FD_ISSET(connection[i].fd,&rset)&&connection[i].fd!=-1){
				memset(buf,0,sizeof(buf));
				if(readline(connection[i].fd,buf)>0)
					printf("%s",buf);
				else
					cmd_off(i);
			}
    	
		}
    }

}
