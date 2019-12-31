#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <arpa/inet.h>
#include <signal.h>

#define clientn 10
#define maxsize 1016
#define datasize 1000
#define tagsize 10

int main(int argc,char *argv[]){
	if(argc!=3)
		exit(1);

	char *filename=argv[1];

	int fd=socket(AF_INET,SOCK_DGRAM,0);
	struct sockaddr_in info;
	struct sockaddr sender;
	bzero(&info,sizeof(info));
	bzero(&sender,sizeof(sender));
	info.sin_addr.s_addr=htonl(INADDR_ANY);
	info.sin_port=htons(atoi(argv[2]));
	info.sin_family=AF_INET;

	if(bind(fd,(struct sockaddr*) &info,sizeof(info))<0)
		printf("bind err\n");

	unsigned char buf[maxsize];
	char ack[tagsize],tag[tagsize];
	char *buf_after_tag=buf+tagsize;
	int tag_n=0;
	int sender_len=sizeof(sender);

	FILE *fp=fopen(filename,"wb");

	while(1){
		memset(buf,0,sizeof(buf));
		memset(tag,0,sizeof(tag));
		memset(ack,0,sizeof(ack));
		int recv_bytes=recvfrom(fd,buf,maxsize,0,&sender,&sender_len);
		strncpy(tag,buf,tagsize);
		int recv_tag=atoi(tag);
	 printf("recv_tag=%d tag_n=%d\n ",recv_tag,tag_n);
		if(recv_tag<tag_n && recv_tag>=0){ //ack lost
			sprintf(ack,"%d",recv_tag);
			sendto(fd,ack,tagsize,0,&sender,sender_len);
		}
		else if(recv_tag==tag_n){

			fwrite(buf_after_tag,sizeof(char),datasize,fp);
			sprintf(ack,"%d",tag_n++);
			sendto(fd,ack,tagsize,0,&sender,sender_len);
		}
		else if(recv_tag==-1){ //sender have finished
			sprintf(ack,"%d",-1);
			sendto(fd,ack,tagsize,0,&sender,sender_len);
			break;
		}
	}
	fclose(fp);
	exit(0);
}
