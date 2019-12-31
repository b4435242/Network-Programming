#include "utils.hpp"

using namespace std;

int addr_len;
int Recv_Restore=0,Send_Restore=0;
struct sockaddr_in server;
struct timeval tv;
fd_set rset,allset,wset,wset_back;
char recvline[MAX_SIZE],sendline[MAX_SIZE],*ptr;
vector<char*> workload;
bool set_name=false;
string pid_info;

int main(int argc,char *argv[]){
	char *username;
	bzero(&server,sizeof(server));
	server.sin_family=AF_INET;
	if(argc==4){
		server.sin_addr.s_addr=inet_addr(argv[1]);
		server.sin_port=htons(atoi(argv[2]));
		username=argv[3];
	}
	else if(argc==2){
		server.sin_addr.s_addr=inet_addr(DEFAULT_IP);
		server.sin_port=htons(DEFAULT_PORT);
		username=argv[1];
	}
	else 
		exit(1);

	int sockfd=socket(AF_INET,SOCK_STREAM,0);
	
	//int flags = fcntl(sockfd, F_GETFL, 0);
	//fcntl(sockfd, F_SETFL, flags|O_NONBLOCK);

	int connect_err=connect(sockfd,(struct sockaddr*) &server,sizeof(server));
	if(connect_err<0)
		if(errno!=EINPROGRESS)
			fprintf(stderr, "connect error\n");
	else if(connect_err==0);

	FD_ZERO(&allset);
	FD_ZERO(&wset_back);
	FD_SET(0,&allset);
	FD_SET(sockfd,&allset);
	FD_SET(sockfd,&wset_back);

	tv.tv_sec = 0; 
	tv.tv_usec = 100;

	pid_info="Pid: "+to_string(getpid());

	while(1){
		rset=allset;
		wset=wset_back;
		select(sockfd+1,&rset,&wset,NULL,&tv);

		if(FD_ISSET(0,&rset)){
			memset(sendline,0,sizeof(sendline));
			memset(recvline,0,sizeof(recvline));
			ptr=fgets(recvline,MAX_SIZE,stdin);
			if(ptr==NULL)
                continue;
            if(!strncmp(recvline,"exit",4)){
                close(sockfd);
                exit(0);
			}
			else if(!strncmp(recvline,"put",3)){
				if(recvline[strlen(recvline)-1]=='\n')
					recvline[strlen(recvline)-1]=0;
				ptr=recvline+4;
				int filesize=get_file_size(ptr);
				sprintf(sendline,"%s %d\n",recvline,filesize); //put filename filesize
				char *work=strdup(sendline);
				workload.push_back(work);
			}
			else if(!strncmp(recvline,"sleep",5)){
				printf("%s The client starts to sleep.\n",pid_info.c_str());
				ptr=recvline+6;
				for(int i=1;i<=atoi(ptr);i++){
					cout<<pid_info<<" ";
					printf("sleep %d\n", i);
					sleep(1);
				}
				printf("%s Client wakes up.\n",pid_info.c_str());
			}
			
					
		}



		if(FD_ISSET(sockfd,&rset)){
			memset(sendline,0,sizeof(sendline));
			memset(recvline,0,sizeof(recvline));

			int readerr=readline(sockfd,recvline);
			if(readerr<0)
				fprintf(stderr,"read error\n");
			else if(readerr==0){
				fprintf(stderr,"Server offline\n");
				exit(1);
			}
			else{			

				if(!strncmp(recvline,"put",3)){
					char* ptr=recvline+4;
					char* filename=strtok(ptr," ");
					char* filename_back=strdup(filename);
					char* filesize_str=strtok(NULL," \n");
					int filesize=atoi(filesize_str);
					int total_sent=0;
					progress_bar(Init_Download,filename_back,0,0,pid_info);
					FILE *fp=fopen(filename_back,"ab");
					while(total_sent<filesize){
						memset(recvline,0,sizeof(recvline));
						int recv_byte=read(sockfd,recvline,MAX_SIZE);
						if(recv_byte<0)
							if(errno==EINTR)
								continue;
							//else if(errno==EWOULDBLOCK){
								//Recv_Restore+=total_sent;
								//break;
							//}
								

						fwrite(recvline,sizeof(char),recv_byte,fp);
						total_sent+=recv_byte;
						progress_bar(Process,filename_back,filesize,total_sent,pid_info);
					}
					progress_bar(End_Download,filename_back,0,0,pid_info);
					fclose(fp);
				}

			
				else{
					cout<<pid_info<<" ";
					printf("%s",recvline);

				}
			}

		}


		if(FD_ISSET(sockfd,&wset)){
			memset(recvline,0,sizeof(recvline));
			memset(sendline,0,sizeof(sendline));
			if(!set_name){
				sprintf(sendline,"name %s\n",username);
				writen(sockfd,sendline,strlen(sendline));
				set_name=true;
			}

			if(workload.size()>0){
				memset(sendline,0,sizeof(sendline));
				char *work=*workload.begin();
				writen(sockfd,work,strlen(work)); //put filename & filesize
				work+=4; //skip put & space
				char *filename=strtok(work," ");
				string filename_string(filename);
				char* filename_back=strdup(filename);
				int filesize=get_file_size(filename_string);
				int total_sent=0;
				//put file
				progress_bar(Init_Upload,filename_back,0,0,pid_info);
				FILE *fp=fopen(filename_back,"rb");
				while(total_sent<filesize){
					memset(sendline,0,sizeof(sendline));
					int read_byte=fread(sendline,sizeof(char),MAX_SIZE,fp);
					int write_byte=writen(sockfd,sendline,read_byte);
					if(write_byte<0)
						fprintf(stderr,"write error\n");
					total_sent+=read_byte;
					progress_bar(Process,filename_back,filesize,total_sent,pid_info);
				}
				progress_bar(End_Upload,filename_back,0,0,pid_info);
				fclose(fp);


				delete *workload.begin(); //free memory
				workload.erase(workload.begin());
			}




		}


	}


	return 0;
}

