#ifndef _THREADPOOL_H
#define _THREADPOOL_H 

typedef struct threadpool_t threadpool_t;
#include<pthread.h>


/*
 *
 *desc: create a thread pool 
 */ 
threadpool_t* threadpool_create(int min_thr_num,int max_thr_num,int queue_max_size);




/*
 *desc: add a new task to the queue of a thread pool 
 *
 */ 
int threadpool_add(threadpool_t* pool,void*(function)(void* arg),void* arg);


/*
 *desc: destory a thread pool 
 *
 */ 
int threadpool_destory(threadpool_t *pool);


/*
 *desc: work thread  
 *
 */ 
void* threadpool_thread(void* threadpool);


/*
 *desc: manager thread 
 *
 *
 */ 
void* adjust_thread(void* threadpool);



int is_thread_alive(pthread_t tid);


int threadpool_free(threadpool_t* pool);
#endif



