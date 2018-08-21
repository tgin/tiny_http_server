#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include "util.h"
#include "http_request.h"
#include "epoll.h"

int read_conf(char* filename,conf_t* conf){
	FILE* fp=fopen(filename,"r");
	if(!fd){
		return CONF_ERR;
	}

	char buff[BUFLEN];
	int buff_len=BUFLEN;
	char* cur_pos=buff;
	char* delim_pos=NULL;
	int i=0,pos=0;
	int line_len=0;

	while(fgets(cur_pos,buff_len-pos,fp)){
		delim_pos=strstr(cur_pos,DELIM);
		if(!delim_pos){
			return CONF_ERR;
		}
		if(cur_pos[strlen(cur_pos)-1]=='\n'){
			cur_pos[strlen(cur_pos)-1]='\0';
		}

		if(strncmp("root",cur_pos,4)==0){
			delim_pos=delim_pos+1;
			while(*delim_pos!='#'){
				conf->root[i++]=*delim_pos;
				++delim_pos;
			}
		}

		if(strncmp("port",cur_pos,4)==0){
			conf->port=atoi(delim_pos+1);
		}

		if(strncmp("thread_num",cur_pos,9)==0){
			conf->thread_num=atoi(delim_pos+1);
		}


		line_len=strlen(cur_pos);
		cur_pos+=line_len;
	}

	fclose(fp);
	return CONF_OK;
}




void handle_for_sigpipe(){
	struct sigaction sa;
	memset(&sa,'\0',sizeof(sa));
	
	sa.sig_handler=SIG_IGN;
	sa.sa_flags=0;
	if(sigaction(SIGPIPE,&sa,NULL)){
		return ;
	}
}


int socket_bind_listen(int port){
	port=((port<=1024) || (port>=65535)) ? 8000 : port;

	int listen_fd=0;
	if((listen_fd=socket(AF_INET,SOCK_STREAM,0))==-1){
		return -1;
	}

	int opt=1;
	if(setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,(const void*)&opt,sizeof(int))==-1){
		return -1;
	}
	
	struct sockaddr_in server_addr;
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	server_addr.sin_port=htons((unsigned short)port);

	if(bind(listen_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))==-1){
		return -1;
	}


	if(listen(listen_fd,LISTEN_Q)==-1){
		return -1;
	}

	if(listen_fd==-1){
		close(listen_fd);
		return -1;
	}

	return listen_fd;
}




int make_socket_nonblocking(int fd){
	int flag=fcntl(fd,F_GETFL,0);
	if(flag==-1){
		return -1;
	}

	flag |=O_NONBLOCK;
	if(fcntl(fd,F_SETFL,flag)==-1){
		return -1;
	}
}


void accept_connection(int listen_fd,int epoll_fd,char* path){
	struct sockaddr_in client_addr;
	memset(&client_addr,0,sizeof(struct sockaddr_in));
	socklen_t client_addr_len=0;

	int accept_fd=accept(listen_fd,(struct sockaddr*)&client_addr,&client_addr_len);
	if(accept_fd==-1){
		perror("accept");
	}

	int rc=make_socket_nonblocking(accept_fd);

	http_request_t* request=(http_request_t*)malloc(sizeof(http_request_t));
	init_request_t(request,accept_fd,epoll_fd,path);

	Epoll_add(epoll_fd,accept_fd,request,(EPOLLIN | EPOLLET | EPOLLONESHOT));

	add_timer(request,TIMEOUT_DEFAULT,http_close_conn);
}




























