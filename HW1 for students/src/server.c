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
#include <time.h>

#define clientnum 10
int maxsize=1024;
int clientset[clientnum];
char *username[clientnum];
struct sockaddr_in clientinfo[clientnum];
time_t timer[clientnum];

char *pfx="[Server]";

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


int writen(int fd,char *buf,size_t num){
        ssize_t res;
        size_t n=1;
	char *ptr=buf;
	while(strncmp(ptr,"\n",1)){
		n++;
		ptr++;
	}
        ptr=buf;
        while(n>0){
				printf("write msg:%s",ptr);
                if((res=write(fd,ptr,n))<=0){
                        if(errno==EINTR)
                                res=0;
                        else{
								printf("write error\n");
                                return -1;
                        }
                }
                ptr+=res;
                n-=res;
        }
       	return num;
}

void handleon(int idx){
	char msg[maxsize];
	for(int i=0;i<clientnum;i++){
		memset(msg,0,sizeof(msg));
		if(i==idx){
			char *port=malloc(maxsize);
			sprintf(port,"%d",ntohs(clientinfo[idx].sin_port));
			char *ip=malloc(maxsize);
			inet_ntop(AF_INET,&clientinfo[idx].sin_addr,ip,maxsize);
			strcat(msg,pfx);
			strcat(msg," Hello, anonymous! From: ");
			strcat(msg,ip);
			strcat(msg,":");
			strcat(msg,port);
			strcat(msg,"\n");
			writen(clientset[i],msg,sizeof(msg));
		}
		else if(clientset[i]!=-1){
			strcat(msg,pfx);	
			strcat(msg," Someone is coming!\n");
			writen(clientset[i],msg,sizeof(msg));
		}

	}
}
	
void handleoff(int idx){
	int err;
	char msg[maxsize];
	memset(msg,0,sizeof(msg));
	strcat(msg,pfx);
	strcat(msg," ");
	strcat(msg,username[idx]);	
	strcat(msg," is offline.\n");
    for(int i=0;i<clientnum;i++){
		if(clientset[i]!=-1)
			writen(clientset[i],msg,sizeof(msg));
	}
}

int checkunique(char *name,int idx){
	for(int i=0;i<clientnum;i++)
		if(!strcmp(name,username[i])&&i!=idx)
			return 1;
	return 0;
}

int checkrule(char *name){
	if(strlen(name)>12||strlen(name)<2)
		return 1;
	int idx=0;
	while(((name[idx]>=65&&name[idx]<=90)||(name[idx]>=97&&name[idx]<=122))&&idx<strlen(name))
		idx++;
	
	return idx!=strlen(name);
}

int namematch(char *name){
	for(int i=0;i<clientnum;i++)
        if(!strcmp(name,username[i]))
            return i;
    return -1;
}


void handle(int idx,int fd,char *line,struct sockaddr_in info){
	char *token,*cmd,*msg[10],sendline[maxsize];
	char *delim=" \n";
	int idxmsg=0;
 	token=strtok(line,delim);
	cmd=strdup(token);
	/*while(token!=NULL){
		token=strtok(NULL,delim);
		if(token!=NULL)
			msg[idxmsg++]=strdup(token);
	}*/
	printf("0");
	if(!strncmp(cmd,"who",3)){
		for(int i=0;i<clientnum;i++){
			if(clientset[i]==-1)
				continue;
			memset(sendline,0,sizeof(sendline));
			char *port=malloc(maxsize);
 	        sprintf(port,"%d",ntohs(clientinfo[i].sin_port));
    	    char *ip=malloc(maxsize);
        	inet_ntop(AF_INET,&clientinfo[i].sin_addr,ip,maxsize);
			
			strcat(sendline,pfx);
			strcat(sendline," ");
			strcat(sendline,username[i]);
			strcat(sendline," ");
			strcat(sendline,ip);
			strcat(sendline,":");
			strcat(sendline,port);
			strcat(sendline," ");
			char *diffstr=malloc(maxsize);
			time_t cutime;
			time(&cutime);
			double diff=difftime(cutime,timer[i]);
			sprintf(diffstr,"%.0f",diff);
			strcat(sendline,diffstr);
			strcat(sendline,"s ");
			if(i==idx)
				strcat(sendline," ->me");
			strcat(sendline,"\n");
			writen(clientset[idx],sendline,sizeof(sendline));
		}
	}
	else if(!strncmp(cmd,"name",4)){
		token=strtok(NULL,delim);
        if(token!=NULL)
            msg[idxmsg++]=strdup(token);

		if(!strncmp(msg[0],"anonymous",9)){
			memset(sendline,0,sizeof(sendline));
			strcat(sendline,pfx);
			strcat(sendline," ERROR: Username cannot be anonymous.\n");
			writen(clientset[idx],sendline,sizeof(sendline));
		}
		else if(checkunique(msg[0],idx)){
			memset(sendline,0,sizeof(sendline));
            strcat(sendline,pfx);
			strcat(sendline," ERROR: ");
			strcat(sendline,msg[0]);
			strcat(sendline," has been used by others.\n"); 
			writen(clientset[idx],sendline,sizeof(sendline));
		}
		else if(checkrule(msg[0])){
			memset(sendline,0,sizeof(sendline));
            strcat(sendline,pfx);
			strcat(sendline," ERROR: Username can only consists of 2~12 English letters.\n");
			writen(clientset[idx],sendline,sizeof(sendline));
		}
		else{
			memset(sendline,0,sizeof(sendline));
            strcat(sendline,pfx);
			strcat(sendline," You're now known as ");
			strcat(sendline,msg[0]);
			strcat(sendline,".\n");
			writen(clientset[idx],sendline,sizeof(sendline));
			memset(sendline,0,sizeof(sendline));
            strcat(sendline,pfx);
            strcat(sendline," ");
   	        strcat(sendline,username[idx]);
            strcat(sendline," is now known as ");
            strcat(sendline,msg[0]);
			strcat(sendline,".\n");	
			for(int i=0;i<clientnum;i++){
				printf("%d",i);
				if(i==idx||clientset[i]==-1)
					continue;
				writen(clientset[i],sendline,sizeof(sendline));
			}		
			strcpy(username[idx],msg[0]);	
		}
	}
	else if(!strncmp(cmd,"tell",4)){
		token=strtok(NULL," ");
        if(token!=NULL)
            msg[idxmsg++]=strdup(token);
		token=strtok(NULL,"\n");
        if(token!=NULL)
            msg[idxmsg++]=strdup(token);

		if(!strncmp(username[idx],"anonymous",9)){
			memset(sendline,0,sizeof(sendline));
            strcat(sendline,pfx);
			strcat(sendline," ERROR: You are anonymous.\n");
			writen(clientset[idx],sendline,sizeof(sendline));
		}						
		else if(!strncmp(msg[0],"anonymous",9)){
			memset(sendline,0,sizeof(sendline));
            strcat(sendline,pfx);    
            strcat(sendline," ERROR: The client to which you sent is anonymous.\n");
			writen(clientset[idx],sendline,sizeof(sendline)); 
		}
		else if(namematch(msg[0])==-1){					
			memset(sendline,0,sizeof(sendline));
            strcat(sendline,pfx);
            strcat(sendline," ERROR: The receiver doesn't exist.\n");
            writen(clientset[idx],sendline,sizeof(sendline));
		}
		else{
			memset(sendline,0,sizeof(sendline));
            strcat(sendline,pfx);
			strcat(sendline," SUCCESS: Your message has been sent.\n");
			writen(clientset[idx],sendline,sizeof(sendline));

			memset(sendline,0,sizeof(sendline));
            strcat(sendline,pfx);
			strcat(sendline," ");
			strcat(sendline,username[idx]);
			strcat(sendline," tell you ");
			strcat(sendline,msg[1]);
			strcat(sendline,"\n");
			int receiveridx=namematch(msg[0]);
			writen(clientset[receiveridx],sendline,sizeof(sendline));
		}
	}
	else if(!strncmp(cmd,"yell",4)){
		token=strtok(NULL,"\n");
        if(token!=NULL)
            msg[idxmsg++]=strdup(token);

		memset(sendline,0,sizeof(sendline));
        strcat(sendline,pfx);
        strcat(sendline," ");
        strcat(sendline,username[idx]);			
		strcat(sendline," yell ");
		strcat(sendline,msg[0]);
		strcat(sendline,"\n");
		for(int i=0;i<clientnum;i++)
			if(clientset[i]!=-1)	
				writen(clientset[i],sendline,sizeof(sendline));						
	}
	else{
		memset(sendline,0,sizeof(sendline));
        strcat(sendline,pfx);
        strcat(sendline," ERROR: Error command.\n");
		writen(clientset[idx],sendline,sizeof(sendline)); 
	}
	
}

int main(int argc,char *argv[]){
	if(argc!=2)
		exit(1);
	
	int sockfd=0;
	int port=atoi(argv[1]);
	int listenfd=socket(AF_INET,SOCK_STREAM,0);
	struct sockaddr_in info;
	bzero(&info,sizeof(info));
	info.sin_family=AF_INET;
	//inet_pton(AF_INET,"127.0.0.1",&info.sin_addr);
	info.sin_addr.s_addr=htonl(INADDR_ANY);
	info.sin_port=htons(port);
	int err=bind(listenfd,(struct sockaddr*) &info,sizeof(info));
	if(err){
		printf("bind error\n");
        	exit(1);}
	err=listen(listenfd,clientnum);
	if(err){
		printf("listen error\n");
        	exit(1);}

	fd_set rset,allset;
	FD_ZERO(&rset);
	FD_ZERO(&allset);
	FD_SET(listenfd,&allset);
	memset(clientset,-1,sizeof(clientset));
	for(int i=0;i<clientnum;i++)
		username[i]=strdup("anonymous");
	int nready;
	char recvline[maxsize];
	bzero(&clientinfo,sizeof(clientinfo));
	char *anonymous=strdup("anonymous");
	int maxfd=listenfd+1;
	for(;;){
		rset=allset;
		nready=select(maxfd,&rset,NULL,NULL,NULL);
		if(FD_ISSET(listenfd,&rset)){
			struct sockaddr_in testsock;
            bzero(&testsock,sizeof(testsock));
			socklen_t testlen=sizeof(testsock);

			int addrlen=sizeof(struct sockaddr);
			int index=0;
			while(index<clientnum&&clientset[index]!=-1)
                index++;
			int clientfd=accept(listenfd,(struct sockaddr*) &clientinfo[index],&addrlen);
			if(clientfd==-1)
				printf("accept error\n");
			if(clientfd+1>maxfd)
				maxfd=clientfd+1;
    		getpeername(clientfd,(struct sockaddr*) &testsock,&testlen);
    		printf("ip:%s port:%d\n",inet_ntoa(testsock.sin_addr),ntohs(testsock.sin_port));
			clientset[index]=clientfd;
			time(&timer[index]);
			FD_SET(clientfd,&allset);
			handleon(index);
			if(--nready<=0)
				continue;
		}
		for(int i=0;i<clientnum;i++){
			int fd=clientset[i];
			if(fd!=-1&&FD_ISSET(fd,&rset)){
				//readable | offline
				memset(recvline,0,sizeof(recvline));
				int n=readline(fd,recvline,maxsize);
				printf("receive msg:%s",recvline);
				if(n>0)
					handle(i,fd,recvline,clientinfo[i]);
				else if(!n){
					FD_CLR(fd,&allset);
					clientset[i]=-1;
					handleoff(i);
					bzero(&clientinfo[i],sizeof(struct sockaddr_in));
					strcpy(username[i],anonymous);
					close(fd);
				}
					
			}
		}					
	} 
	
	



	
}
