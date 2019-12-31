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
#include <sys/time.h>

#define maxsize 1016
#define datasize 1000
#define tagsize 10
#define rtt_ratio 0.75
#define rtt_init 100000
#define millsec 1000000
#define ratio_interval 2

int main(int argc,char *argv[]){
	if(argc!=4)
		exit(1);
	char *ip;
	if(!strcmp(argv[2],"localhost"))
		ip="127.0.0.1";
	else
		ip=argv[2];

	int sendfd=socket(AF_INET,SOCK_DGRAM,0);
	struct sockaddr_in info;
	bzero(&info,sizeof(info));
	info.sin_addr.s_addr=inet_addr(ip);
	info.sin_port=htons(atoi(argv[3]));
	info.sin_family=AF_INET;

	if(connect(sendfd,(struct sockaddr*) &info,sizeof(info))<0)
		printf("connect err\n");

	char *filename=argv[1];
	FILE *fp=fopen(filename,"rb");
	fpos_t position;
	fgetpos(fp,&position);

	unsigned char buf[tagsize+datasize];
	char recv_ack[tagsize];
	char *buf_after_tag=buf+tagsize;
	int recv_bytes;
	int tag_n=0;

	struct timeval start_t,end_t;
	unsigned long new_rtt,old_rtt=rtt_init,rtt=rtt_init;


	fd_set allset,rset;
	FD_ZERO(&allset);
	FD_ZERO(&rset);
	FD_SET(sendfd,&allset);

	struct timeval tv;

	while(1){
		memset(recv_ack,0,sizeof(recv_ack));
		memset(buf,0,sizeof(buf));
		sprintf(buf,"%d",tag_n);
		
		fsetpos(fp,&position);
		int data_bytes=fread(buf_after_tag,sizeof(char),datasize,fp);
		int send_bytes=tagsize+data_bytes;
		if(data_bytes==0)
			break;

		gettimeofday(&start_t,NULL);
		int sent_bytes=send(sendfd,buf,send_bytes,0);
		
		if(sent_bytes<=0) 
			printf("send err\n");
		else if(sent_bytes<send_bytes){
			printf("Not sending all\n");
			fsetpos(fp,&position);
			fread(buf,sizeof(char),sent_bytes,fp);
		} 

		tv.tv_sec=rtt/millsec;
		tv.tv_usec=(rtt%millsec)*ratio_interval;

		rset=allset;
		select(sendfd+1,&rset,NULL,NULL,&tv);
		if(FD_ISSET(sendfd,&rset)){
			recv_bytes=recv(sendfd,recv_ack,sizeof(recv_ack),0);
			printf("tag=%d ack=%d rtt=%ld\n",tag_n,atoi(recv_ack),rtt);
			if(atoi(recv_ack)==tag_n){ //with correct tag
				fgetpos(fp,&position); //update indicator of input stream
				gettimeofday(&end_t,NULL);
				new_rtt=millsec*(end_t.tv_sec-start_t.tv_sec)+end_t.tv_usec-start_t.tv_usec;
				rtt=rtt_ratio*old_rtt+(1-rtt_ratio)*new_rtt;
				old_rtt=rtt;
				tag_n++;
				if(data_bytes<datasize && sent_bytes==send_bytes) //finish read file
					break;
			}
			
		}
		else{
			printf("timeout\n");
			rtt*=ratio_interval;
		}
	}

	memset(buf,0,sizeof(buf));
    sprintf(buf,"%d",-1);

	while(1){ //inform receiver to close
		
		if(send(sendfd,buf,tagsize,0)<=0){
			printf("receiver off\n");	
			break;
		}

		tv.tv_sec=rtt/millsec;
        tv.tv_usec=(rtt%millsec)*ratio_interval;

		rset=allset;
		select(sendfd+1,&rset,NULL,NULL,&tv);
		if(FD_ISSET(sendfd,&rset)){
			memset(recv_ack,0,sizeof(recv_ack));
			if((recv_bytes=recv(sendfd,recv_ack,sizeof(recv_ack),0))<=0)
				break;
			if(atoi(recv_ack)==-1){
				break;
			}
		}
		//else rtt*=ratio_interval;
	}

	fclose(fp);
	exit(0);
}
