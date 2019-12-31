#pragma once
#include "Manager.hpp"

using namespace std;

Client::Client(){}
Client::~Client(){}

Client::Client(int sendfd,SA *addr){
	this->conn_fd=sendfd;
	this->addr=addr;
}

Manger::Manger(){
	
}
Manger::~Manger(){}


void Manger::Init_fd_set(int listenfd){
	FD_ZERO(&rset_back);
	FD_ZERO(&wset_back);
	FD_SET(listenfd,&rset_back);

	maxfd=listenfd;
}

void Manger::parse_msg(int sendfd,char* raw_msg){
	char *ptr;
	if(!strncmp(raw_msg,"name",4)){
		map<int,Client*>::iterator it;
		it=unset_client.find(sendfd);
		if(it==unset_client.end()) //msg is username
			print_err("Can't find client in unset_client");

		ptr=raw_msg+5;
		char *username=strtok(ptr," \n");
		string username_in_string(username);
		set_client(sendfd,username_in_string,it->second);
		unset_client.erase(it);

		char buf[MAX_SIZE];
		memset(buf,0,sizeof(buf));
		sprintf(buf,"Welcome to the dropbox-like server: %s\n",username);
		printf("%s",buf);
		writen(sendfd,buf,strlen(buf));	

	}
	else if(!strncmp(raw_msg,"put",3)){
		ptr=raw_msg+4;
		char *filename=strtok(ptr," ");
		char *filesize=strtok(NULL," \n");
		string filename_in_string(filename);
		get_file(sendfd,filename_in_string,atoi(filesize));
	}

}

void Manger::add_client(int sendfd,Client* client){
	unset_client[sendfd]=client;
	set_name[sendfd]=false;
	
	Recv_Restore_Record[sendfd]=0;
	Send_Restore_Record[sendfd]=0;

	fd_table.push_back(sendfd);
	FD_SET(sendfd,&rset_back);
	FD_SET(sendfd,&wset_back);
	if(sendfd>maxfd)
		maxfd=sendfd;
}

void Manger::set_client(int sendfd,string username,Client* client){
		
	bool Is_NewUser=true;
	map<int,string>::iterator it;
	for(it=user_table.begin();it!=user_table.end();it++)
		if(username==it->second){
cout<<"got "<<username<<endl;
			Is_NewUser=false;
			break;
		}

	user_table[sendfd]=username;
    setted_client[sendfd]=client;
    set_name[sendfd]=true;

cout<<"in set_client"<<endl;
	
	USER *clients_of_user;
	if(Is_NewUser){
		//create new folder
		//string path="./"+username;
		if(access(username.c_str(),0)<0){
			int isCreate = mkdir(username.c_str(),S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
			if(isCreate<0)
				print_err("create dir path failed!");
		}

		//client_table
		clients_of_user=new USER;
		clients_of_user->push_back(client);
		client_table[username]=clients_of_user;

		//create files in file_table
		vector<string>* files=new vector<string>;
		file_table[username]=files;
	}
	else{
		//client_table
		clients_of_user=get_USER_by_NAME(username);
		clients_of_user->push_back(client);
		//client's update_table
		vector<string> *files=get_FILES_by_NAME(username);
		for(int i=0;i<files->size();i++)
			client->update_table.push_back((*files)[i]);

	}

}

void Manger::get_file(int sendfd,string filename,int filesize){
cout<<"in get"<<endl;	
	string username=get_NAME_by_FD(sendfd);
	string path=username+"/"+filename;
	FILE *fp=fopen(path.c_str(),"ab");
	
	bool DONE_RECV=true;
	char buf[MAX_SIZE];
	int total=Recv_Restore_Record[sendfd];
	
	while(total<filesize){
		memset(buf,0,sizeof(buf));
		int recv_flag=read(sendfd,buf,MAX_SIZE);
		if(recv_flag<0){
			Recv_Restore_Record[sendfd]=total;
			DONE_RECV=false;
			break;
		}

		fwrite(buf,sizeof(char),recv_flag,fp);
		total+=recv_flag;
	}
				
	fclose(fp);
	
	if(DONE_RECV){
		Recv_Restore_Record[sendfd]=0;

		//string username=get_NAME_by_FD(sendfd);
		//add filename to files in file_table
		vector<string>* files=get_FILES_by_NAME(username);
		files->push_back(filename);
		//add filename to update_table of clients in same User 
		USER *clients_of_user=get_USER_by_NAME(username);
		for(int i=0;i<clients_of_user->size();i++)
			if((*clients_of_user)[i]->conn_fd!=sendfd)
				(*clients_of_user)[i]->update_table.push_back(filename);
	}
cout<<"fin get"<<endl;
}

bool Manger::put_file(int recvfd,string filename){

	bool IS_Write_Success=true;
	string username=get_NAME_by_FD(recvfd);
	string path=username+"/"+filename;

	char buf[MAX_SIZE];
	memset(buf,0,sizeof(buf));
	int filesize=get_file_size(path);
	sprintf(buf,"put %s %d\n",filename.c_str(),filesize);
	int sent_flag=my_write(recvfd,buf,strlen(buf));
	if(sent_flag<0)
		return false;


	FILE *fp=fopen(path.c_str(),"rb");
	int total_sent=Send_Restore_Record[recvfd];
cout<<"put"<<endl;
	while(total_sent<filesize){
		memset(buf,0,sizeof(buf));
		fseek(fp,total_sent,SEEK_SET);
		int data_bytes=fread(buf,sizeof(char),MAX_SIZE,fp);

		if(data_bytes==0)
			break;

		sent_flag=write(recvfd,buf,data_bytes); 
		if(sent_flag<0){
//cout<<"errno="<<errno<<endl;
			if(errno==EINTR)
				continue;
			IS_Write_Success=false;
			Send_Restore_Record[recvfd]+=total_sent;
			break;
		}
		total_sent+=sent_flag;
//cout<<"sent= "<<sent_flag<<endl;
//cout<<"total= "<<total_sent<<endl;
	}
	fclose(fp);
	
	if(IS_Write_Success){
		Send_Restore_Record[recvfd]=0;
		Client *client=get_CLIENT_by_FD(recvfd);
		for(int i=0;i<client->update_table.size();i++)
			if((client->update_table)[i]==filename)
				client->update_table.erase(client->update_table.begin()+i);
		return true;
	}
	return false;

}


void Manger::del_client(int sendfd){

	if(sendfd==maxfd)
		maxfd=*max_element(fd_table.begin(),fd_table.end());
	FD_CLR(sendfd,&rset_back);
	FD_CLR(sendfd,&wset_back);
	close(sendfd);

	for(int i=0;i<fd_table.size();i++)
		if(fd_table[i]==sendfd){
			fd_table.erase(fd_table.begin()+i);
			break;
		}

	//map<int,string>::iterator it;
	//it=user_table.find(sendfd);
	//if(it==user_table.end())
	//	print_err("Can't find sockfd "+to_string(sendfd)+"in user_table when del_client"); 
	//user_table.erase(it);

	string username=get_NAME_by_FD(sendfd);
	USER *clients_of_user=get_USER_by_NAME(username);
	for(int i=0;i<clients_of_user->size();i++)
		if((*clients_of_user)[i]->conn_fd==sendfd){
			clients_of_user->erase(clients_of_user->begin()+i);
			break;
		}

	//map<int,Client*>::iterator it3;
	//it3=setted_client.find(sendfd);
	//if(it3!=setted_client.end()){
	//	setted_client.erase(it3);
	//}
	//else{
	//	it3=unset_client.find(sendfd);
	//	if(it3!=unset_client.end())
	//		unset_client.erase(it3);
	//}
		
}



void Manger::print_err(string msg){
	msg+="\n";
	fprintf(stderr, msg.c_str());
	exit(1);
}

string Manger::get_NAME_by_FD(int sendfd){
	map<int,string>::iterator it;
	it=user_table.find(sendfd);
	if(it==user_table.end())
		print_err("Can't find sockfd "+to_string(sendfd)+" in user_table when get_NAME_by_FD");
	return it->second;
}

vector<string>* Manger::get_FILES_by_NAME(string username){
	map<string,vector<string>*>::iterator it;
	it=file_table.find(username);
	if(it==file_table.end())
		print_err("Can't find User "+username+" in file_table");
	return it->second;
}

USER* Manger::get_USER_by_NAME(string username){
	map<string,USER*>::iterator it;
	it=client_table.find(username);
	if(it==client_table.end())
		print_err("Can't find User "+username+" in client_table");
	return it->second;
}

Client* Manger::get_CLIENT_by_FD(int sendfd){
	map<int,Client*>::iterator it;
	it=setted_client.find(sendfd);
	if(it==setted_client.end())
		print_err("Can't find sockfd "+to_string(sendfd)+" set_client");
	return it->second;
}


int Manger::my_write(int fd,char *buffer,int length){

	int bytes_left;
	int written_bytes;
	int total_write=0;
	char *ptr;
	ptr=buffer;
	bytes_left=length;
	while(bytes_left>0){    
	    written_bytes=write(fd,ptr,bytes_left);
	    if(written_bytes<=0){       
	        if(errno==EINTR)
	            written_bytes=0;
	        else{
	        	Send_Restore_Record[fd]+=total_write;
	            return(-1);
	        }
	    }
	    bytes_left-=written_bytes;
	    total_write+=written_bytes;
	    ptr +=written_bytes;     
	}

	return(0);
}

/*int Manger::my_read(int fd,void *buffer,int length)
{
int bytes_left;
int bytes_read;
char *ptr;
  
bytes_left=length;
while(bytes_left>0)
{
    bytes_read=read(fd,ptr,bytes_read);
    if(bytes_read<0)
    {
      if(errno==EINTR)
         bytes_read=0;
      else
         return(-1);
    }
    else if(bytes_read==0)
        break;
     bytes_left-=bytes_read;
     ptr =bytes_read;
}
return(length-bytes_left);
}*/

