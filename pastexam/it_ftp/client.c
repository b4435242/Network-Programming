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

#define filesize 10000000
#define maxsize 1024

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

void cmd_get(int fd,char *filename){
	char buf[filesize];
	FILE *fp=fopen(filename,"w");
	while(1){
		memset(buf,0,sizeof(buf));
		int bytes=read(fd,buf,sizeof(buf));
		fwrite(buf,sizeof(char),bytes,fp);
		if(bytes<sizeof(buf))
			break;
	}
	fclose(fp);
	
}

void cmd_put(int fd,char *filename){
	char buf[filesize];
        memset(buf,0,sizeof(buf));
	FILE *fp=fopen(filename,"r");
	while(1){
		int bytes=fread(buf,sizeof(char),sizeof(buf),fp);
		write(fd,buf,bytes);
		if(feof(fp))
                        break;
	}
	fclose(fp);
}

void handle(int fd,char *buf){
	char *cmd,*token[2];
	char *msg=strdup(buf);
	cmd=strtok(msg," \n");
	
	if(!strncmp(cmd,"EXIT",4)){
        close(fd);
        exit(0);
    }
    else if(!strncmp(cmd,"PUT",3)){
    	token[0]=strtok(NULL," ");
    	token[1]=strtok(NULL," \n");
    	cmd_put(fd,token[0]);
    	printf("PUT %s %s succeed\n",token[0],token[1]);
    }
    else if(!strncmp(cmd,"GET",3)){
    	token[0]=strtok(NULL," ");
    	token[1]=strtok(NULL," \n");
    	cmd_get(fd,token[1]);
    	printf("GET %s %s succeed\n",token[0],token[1]);
    }
}

int main(int argc,char *argv[]){
	if(argc!=3)
		exit(0);

	struct sockaddr_in server;
	bzero(&server,sizeof(server));
	server.sin_port=htons(atoi(argv[2]));
	server.sin_addr.s_addr=inet_addr(argv[1]);
	server.sin_family=AF_INET;

	int sockfd=socket(AF_INET,SOCK_STREAM,0);
	//int slen=sizeof(server); 
	int err=connect(sockfd,(struct sockaddr*) &server,sizeof(server));
	if(!err)
		printf("connect succeed\n");
	fd_set rset,allset;
	FD_ZERO(&allset);
	FD_SET(sockfd,&allset);
	FD_SET(0,&allset);
	int maxfd=sockfd+1;

	for(;;){
		rset=allset;
		select(maxfd,&rset,NULL,NULL,NULL);

		if(FD_ISSET(0,&rset)){
			//char *buf=calloc(maxsize,sizeof(char));
			char buf[maxsize];
			memset(buf,0,sizeof(buf));
			char *ptr;
			ptr=fgets(buf,maxsize,stdin);
			printf("%s",buf);
			if(ptr==NULL)
				continue;
			write(sockfd,buf,strlen(buf));
			handle(sockfd,buf);
			struct sockaddr_in test;
			bzero(&test,sizeof(test));
			int testlen=sizeof(test);
			getpeername(sockfd,(struct sockaddr*) &test,&testlen);
			printf("port=%d\n",ntohs(test.sin_port));
	        	//free(buf);
    	}
    	if(FD_ISSET(sockfd,&rset)){
    		char buf[maxsize];
		memset(buf,0,sizeof(buf));
    		int end=readline(sockfd,buf);
		if(!end) exit(0);
    		printf("%s",buf);
    	}
	}

}
