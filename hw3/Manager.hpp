#pragma once
#include "utils.hpp"
class Client;

typedef std::vector<Client*> USER;

class Client
{
public:
	Client();
	Client(int sendfd,SA *addr);
	~Client();

	//vector<string> *file_table;  // shared by clients in the same user
	vector<string> update_table;
	//string username;
	SA* addr;
	int conn_fd;
};

class Manger
{
public:
	Manger();
	~Manger();
	
	void Init_fd_set(int listenfd);
	void del_client(int sendfd);
	void add_client(int sendfd,Client* client);
	void parse_msg(int sendfd,char* raw_msg);
	void set_client(int sendfd,string username,Client* client);
	void get_file(int sendfd,string filename,int filesize);
	bool put_file(int recvfd,string filename);
	

	void print_err(string msg);
	string get_NAME_by_FD(int sendfd);
	Client* get_CLIENT_by_FD(int sendfd);
	vector<string>* get_FILES_by_NAME(string username);
	USER* get_USER_by_NAME(string username);
	int my_write(int fd,char *buffer,int length);
	//int my_read(int fd,void *buffer,int length);

	int maxfd;
	fd_set rset_back,wset_back;
	vector<int> fd_table;

	map<int,string> user_table;
	map<string,USER*> client_table;
	map<string,vector<string>*> file_table;
	map<int,Client*> unset_client;
	map<int,Client*> setted_client;
	map<int,bool> set_name;
	map<int,int> Recv_Restore_Record;
	map<int,int> Send_Restore_Record;

};
