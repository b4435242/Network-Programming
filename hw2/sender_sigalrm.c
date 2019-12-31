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
#include <time.h>
#include <sys/time.h>

#define maxsize 1016
#define datasize 1000
#define tagsize 10
#define rtt_ratio 0.75
#define rtt_init 100000
#define millsec 1000000
#define ratio_interval 2
int recv_bytes;

void sig_alrm(int signo){
	//printf("interrupt\n");
	recv_bytes=-1;
	return;
}

int double_to_int(double input)
{
    int output;
    char temp[20];
	memset(temp,0,sizeof(temp));
    sprintf(temp,"%lf", input);

    temp[19] = 0x00;

    //sscanf("%d", &output);
	output=atoi(temp);

    return output;
}

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

	
	
	unsigned char buf[maxsize];
	char recv_ack[tagsize],tag[tagsize];
	char *buf_after_tag=buf+tagsize;
	int tag_n=0,ack;
	struct timeval start_t,end_t;
	unsigned long new_rtt,old_rtt=rtt_init,rtt=rtt_init;

	//signal(SIGALRM,sig_alrm);
	struct sigaction alrm_act;
	bzero(&alrm_act,sizeof(alrm_act));
	alrm_act.sa_handler=sig_alrm;
	alrm_act.sa_flags=SA_NOMASK;
	sigaction(SIGALRM,&alrm_act,NULL);

	while(1){
		memset(buf,0,sizeof(buf));
		memset(tag,0,sizeof(tag));
		sprintf(buf,"%d",tag_n);
		
		fsetpos(fp,&position);
		int data_bytes=fread(buf_after_tag,sizeof(char),datasize,fp);
		int send_bytes=tagsize+data_bytes;
		if(data_bytes==0)
			break;	
		//timeout for writable fd (setting 1024 bytes) 
		//necessary?

		gettimeofday(&start_t,NULL);
		int sent_bytes=send(sendfd,buf,send_bytes,0);
	printf("tag=%d oldrtt=%ld ",tag_n,rtt);
		if(sent_bytes<0) 
			printf("send err\n");
		else if(sent_bytes<send_bytes){
			printf("Not sending all\n");
			fsetpos(fp,&position);
			fread(buf,sizeof(char),sent_bytes,fp);
		}
 
		memset(recv_ack,0,sizeof(recv_ack));
		//timeout RTT + queueing delay for ACK
        if(rtt>=1)
            alarm((rtt/millsec)+1); //unit s
        else
            ualarm((rtt*ratio_interval),0); //unit us

		recv_bytes=recv(sendfd,recv_ack,sizeof(recv_ack),0);
		alarm(0);
		if(recv_bytes<0){ //lost packet or ACK
			if(errno==EINTR)
				printf("timeout\n");
				//rtt*=ratio_interval;
		}
		else if(recv_bytes>0){ //recv ACK
			ack=atoi(recv_ack);
		printf("ack=%d tag_n=%d\n",ack,tag_n);
			if(ack==tag_n){ //with correct tag
				fgetpos(fp,&position); //update indicator of input stream
				gettimeofday(&end_t,NULL);
				new_rtt=(end_t.tv_sec-start_t.tv_sec)*1000000+end_t.tv_usec-start_t.tv_usec;
				rtt=rtt_ratio*old_rtt+(1-rtt_ratio)*new_rtt;
				old_rtt=rtt;
				tag_n++;
				printf("rtt=%ld new_rtt=%ld\n",rtt,new_rtt);
				if(data_bytes<datasize  && sent_bytes==send_bytes) //finish read file
					break;
				
			}
			//else ;//with incorrect tag
			
		}
		

	}

	while(1){ //inform receiver to close
		memset(recv_ack,0,sizeof(recv_ack));
		memset(buf,0,sizeof(buf)); 
		sprintf(buf,"%d",-1);
		if(send(sendfd,buf,tagsize,0)<=0){
			printf("receiver off\n");
			break;
		}

		if(rtt>=1)
            alarm((rtt/millsec)+1); //unit s
        else
            ualarm((rtt*ratio_interval),0); //unit us
	
		recv_bytes=recv(sendfd,recv_ack,sizeof(recv_ack),0);
		if(recv_bytes<0 && errno==EINTR) //lost packet or ACK
			printf("timeout\n");
		else if(recv_bytes<0){
			printf("receiver off recv errno=%d\n",errno);
			break;
		}
		else if(recv_bytes>0) //recv ACK 
			if(atoi(recv_ack)==-1)
				break;
	}

	fclose(fp);
	exit(0);

}
