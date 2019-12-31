#pragma once
#include "Manager.hpp"

using namespace std;

int listenfd;
int nready;
socklen_t addr_len;
struct sockaddr_in server;
struct timeval tv;
fd_set rset,wset;
Manger manager;

void Init_server(int &port){
	listenfd=socket(AF_INET,SOCK_STREAM,0);
	bzero(&server,sizeof(server));
	server.sin_addr.s_addr=htonl(INADDR_ANY);
	server.sin_port=htons(port);
	server.sin_family=AF_INET;

	if(bind(listenfd,(struct sockaddr*) &server,sizeof(server))<0)
		fprintf(stderr,"bind err\n");

	listen(listenfd,5);

	int flag = fcntl(listenfd, F_GETFL, 0);
	fcntl(listenfd, F_SETFL, flag|O_NONBLOCK);

	manager.Init_fd_set(listenfd);

	tv.tv_sec=0;
	tv.tv_usec=100;
}

int main(int argc,char *argv[]){
	
	if(argc!=2 && argc!=1)
		exit(1);

	int port=(argc==2?atoi(argv[1]):DEFAULT_PORT);
	Init_server(port);

	char buf[MAX_SIZE];
	
	while(1){
		rset=manager.rset_back;
		wset=manager.wset_back;
		nready=select(manager.maxfd+1,&rset,&wset,NULL,&tv);
		if(FD_ISSET(listenfd,&rset)){
			SA *new_client_addr=new SA;
			int new_fd=accept(listenfd,new_client_addr,&addr_len);
			Client *new_client=new Client(new_fd,new_client_addr);
			manager.add_client(new_fd,new_client);
		}

		for(int i=0;i<manager.fd_table.size();i++){
			int connfd=manager.fd_table[i];
			if(FD_ISSET(connfd,&rset)){

				memset(buf,0,sizeof(buf));
				int readn=readline(connfd,buf);
				if(readn<0)
					;
				else if(readn==0)
					manager.del_client(connfd);
				else
					manager.parse_msg(connfd,buf);


			}

			if(FD_ISSET(connfd,&wset) && manager.set_name[connfd]){
				string username=manager.get_NAME_by_FD(connfd);
				//vector<string>* files=get_FILES_by_NAME(username);
				Client *client=manager.get_CLIENT_by_FD(connfd);
				//for(int j=0;j<client->update_table.size();j++){
				while(client->update_table.size()>0)
					if(!manager.put_file(connfd,*(client->update_table).begin()))
						break;
				//}

			}

		}


	}

	return 0;
}

