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
#define cnum 1

char *filetable[maxsize];
int fnum=0;
int flag=0;

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
	char buf[maxsize];
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

void cmd_put(int fd,char *filename){
	char buf[filesize];
	FILE *fp=fopen(filename,"w");
	while(1){
		memset(buf,0,sizeof(buf));
		int bytes=read(fd,buf,sizeof(buf));
		int err=fwrite(buf,sizeof(char),bytes,fp);
		if(bytes<sizeof(buf))
			break;
	}
	fclose(fp);
	filetable[fnum++]=strdup(filename);
}

void cmd_list(int fd){
	for (int i = 0; i < fnum; i++){
		char buf[filesize];
        	memset(buf,0,sizeof(buf));
		strcpy(buf,filetable[i]);
		strcat(buf,"\n");
		write(fd,buf,sizeof(buf));
	}
}

void cmd_off(int fd){
	close(fd);
	for (int i = 0; i < fnum; ++i){
		remove(filetable[i]);
		free(filetable[i]);
	}
	fnum=0;
	flag=0;
}


void handle(int fd,char *buf){
	char *cmd,*token[2];
	cmd=strtok(buf," \n");

	if(!strncmp(cmd,"GET",3)){
		token[0]=strtok(NULL," ");
		token[1]=strtok(NULL," \n");
		cmd_get(fd,token[0]);
	}
	else if(!strncmp(cmd,"PUT",3)){
		token[0]=strtok(NULL," ");
		token[1]=strtok(NULL," \n");
		cmd_put(fd,token[1]);
	}
	else if(!strncmp(cmd,"LIST",4)){
		cmd_list(fd);
	}
	
}

int main(int argc,char *argv[]){
	if(argc!=2)
		exit(0);

	char buf[maxsize];
	int listenfd=socket(AF_INET,SOCK_STREAM,0);
	struct sockaddr_in server,client;
	bzero(&server,sizeof(server));
	server.sin_port=htons(atoi(argv[1]));
	server.sin_addr.s_addr=htonl(INADDR_ANY);
	server.sin_family=AF_INET;

	int err=bind(listenfd,(struct sockaddr*) &server,sizeof(server));
	if(err==-1)
		printf("bind error\n");
	err=listen(listenfd,cnum);
	if(err==-1)
                printf("listen error\n");
	for(;;){
		int csize=sizeof(client);
		int clientfd;
		if(!flag){
			bzero(&client,sizeof(client));
			clientfd=accept(listenfd,(struct sockaddr*) &client,&csize);
			flag++;
		}
		//char *buf=calloc(maxsize,sizeof(char));
		memset(buf,0,sizeof(buf));
		int off=readline(clientfd,buf);
		if(!off){
			cmd_off(clientfd);
			continue;
		}
		printf("%s",buf);
		handle(clientfd,buf);
		//free(buf);
		

	}

}
