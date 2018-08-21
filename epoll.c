#include "epoll.h"
#include<unistd.h>
#include<stdlib.h>



struct epoll_event* events;  //why global variable

int Epoll_create(int flags){
	int epoll_fd=epoll_create1(flags);
	if(epoll_fd==-1){
		return -1;
	}
	events=(struct epoll_event*)malloc(sizeof(struct epoll_event)*MAX_EVENTS);
	return epoll_fd;
}


int Epoll_add(int epoll_fd,int fd,http_request_t* request,int events){
	struct epoll_event event;
	event.data.ptr=(void*)request;
	event.events=events;

	int ret=epoll_ctl(epoll_fd,EPOLL_CTL_ADD,fd,&event);
	if(ret==-1){
		return -1;
	}
}

int Epoll_mod(int epoll_fd,int fd,http_request_t* request,int events){
	struct epoll_event event;
	event.data.ptr=(void*)request;
	event.events=events;

	int ret=epoll_ctl(epoll_fd,EPOLL_CTL_MOD,fd,&event);
	if(ret==-1){
		return -1;
	}
}

int Epoll_del(int epoll_fd,int fd,http_request_t* request,int events){
	struct epoll_event event;
	event.data.ptr=(void*)request;
	event.events=events;

	int ret=epoll_ctl(epoll_fd,EPOLL_CTL_DEL,fd,&event);
	if(ret==-1){
		return -1;
	}
}

int Epoll_wait(int epoll_fd,struct epoll_event* events,int max_events,int timeout){
	return epoll_wait(epoll_fd,events,max_events,timeout);
}



void handle_events(int epoll_fd,int listen_fd,struct epoll_event* events,
		int events_num,char* path,threadpool_t* tp){
	int i=0;
	for(i=0;i<events_num;++i){
		http_request_t* request=(http_request_t*)(events[i].data.ptr);
		int fd=request->fd;

		if(fd==listen_fd){
			accept_connection(listen_fd,epoll_fd,path);
		}
		else{
			if((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN))){
				close(fd);
				continue;
			}

			int rc=threadpool_add(tp,do_request,events[i].data.ptr);
		}
	}
}



