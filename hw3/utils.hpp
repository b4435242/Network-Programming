#pragma once

#include <bits/stdc++.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <arpa/inet.h>

#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT 6636
#define MAX_SIZE 1024

#define MODE (S_IRWXU | S_IRWXG | S_IRWXO)  

typedef struct sockaddr SA;
//typedef std::vector<Client*> USER;
using namespace std;

enum Progress_option{
	Init_Download=0,
	Init_Upload,
	Process,
	End_Download,
	End_Upload
};

int get_file_size(string filename);

void progress_bar(int status,char* filename,int total,int sent,string pid_info);

int readline(int fd,char *buf);

ssize_t writen(int fd,char *buf,size_t n);

int readn(int fd,void *buffer,int length);
  
