#ifndef EPOLL_H
#define EPOLL_H

#include<sys/epoll.h>
#include "threadpool.h"
#include "http.h"

#define MAX_EVENTS 1024


int Epoll_create();

int Epoll_add(int epoll_fd,int fd,http_request_t* request,int events);

int Epoll_mod(int epoll_fd,int fd,http_request_t* request,int events);

int Epoll_del(int epoll_fd,int fd,http_request_t* request,int events);

int Epoll_wait(int epoll_fd,struct epoll_event* events,int max_events,int timeout);

void handle_events(int epoll_fd,int listen_fd,struct epoll_event* events,
		int events_num,char* path,threadpool_t* tp);


#endif
