#ifndef UTIL_H
#define UTIL_H

#define PATHLEN 128
#define LISTEN_Q 1024
#define BUFLEN 8192
#define DELIM "="


#define CONF_OK 0
#define CONF_ERR -1


#define Min(a,b) ((a) < (b) ? (a):(b))

typedef struct _conf_t{
	char root[PATHLEN];
	int port;
	int thread_num;
}conf_t;

int read_conf(char* filename,conf_t* conf);

void handle_for_sigpipe();

int socket_bind_listen(int fd);

int make_socket_nonblocking(int fd);

void accept_connection(int listen_fd,int epoll_fd,char* path);


#endif
