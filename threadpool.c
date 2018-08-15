#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include "threadpool.h"


#define DEFAULT_TIME 10
#define MIN_WAIT_TASK_NUM 10
#define DEFAULT_THREAD_VARY 10
#define true 1
#define false 0


/*
 *desc: thread task struct 
 */ 
typedef struct {
	void* (*function)(void *); //callback function 
	void* arg;  //argument of callback function 
}threadpool_task_t;


struct threadpool_t{
	pthread_mutex_t lock;  //lock the whole struct 
	pthread_mutex_t thread_counter; //lock the "busy_thr_num"
	pthread_cond_t queue_not_full; //when task queue is full,server thread wait for this cond 
	pthread_cond_t queue_not_empty;//when task queue not empty,inform the thread of wait task 

	pthread_t* threads; //an array save thread id 
	pthread_t adjust_tid; //manager thread id 
	threadpool_task_t* task_queue; //task queue 

	int min_thr_num;  //argument of threadpool 
	int max_thr_num;
	int live_thr_num;
	int busy_thr_num;
	int wait_exit_thr_num;

	int queue_front; //argument of task queue 
	int queue_rear;
	int queue_size;
	int queue_max_size;

	int showdowm;
};



threadpool_t* threadpool_create(int min_thr_num,int max_thr_num,int queue_max_size){

	int i=0;
	threadpool_t* pool=NULL;

	//use do-while statement implement goto statement
	do{
		if((pool=(threadpool_t*)malloc(sizeof(threadpool_t)))==NULL){
			printf("func threadpool_create error: malloc pool fail\n");
			break;
		}

		//initialize the threadpool parameter
		pool->min_thr_num=min_thr_num;
		pool->max_thr_num=max_thr_num;
		pool->live_thr_num=min_thr_num;
		pool->busy_thr_num=0;
		pool->wait_exit_thr_num=0;

		pool->queue_front=0;
		pool->queue_rear=0;
		pool->queue_size=0;
		pool->queue_max_size=queue_max_size;

		pool->showdowm=false;


		//allocate space for work threads array
		pool->threads=(pthread_t*)malloc(sizeof(pthread_t)*max_thr_num);
		if(pool->threads==NULL){
			printf("func threadpool_create error: malloc pool->pthreads fail\n");
			break;
		}
		memset(pool->threads,0,sizeof(pthread_t)*max_thr_num);


		//allocate the space for task queue 
		pool->task_queue=(threadpool_task_t*)malloc(sizeof(threadpool_task_t)*queue_max_size);
		if(pool->task_queue==NULL){
			printf("func threadpool_create error: malloc pool->task_queue fail\n");
			break;
		}


		//initialize the mutex and cond 
		if(pthread_mutex_init(&(pool->lock),NULL)!=0){
			printf("func threadpool_create error: initialize the mutex and cond fail\n");
			break;
		}

	}while(0);
	return pool;
}


int threadpool_add(threadpool_t* pool,void*(function)(void* arg),void* arg);
int threadpool_destory(threadpool_t *pool){
	threadpool_t* tmp=NULL;
	tmp=pool;
	return 0;
}
