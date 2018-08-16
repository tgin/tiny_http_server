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

	int shotdown;
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

		pool->shotdown=false;


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
		if(pthread_mutex_init(&(pool->lock),NULL)!=0
				|| pthread_mutex_init(&(pool->thread_counter),NULL)!=0
				|| pthread_cond_init(&(pool->queue_not_empty),NULL)!=0
				|| pthread_cond_init(&(pool->queue_not_full),NULL)!=0
		  ){
			printf("func threadpool_create error: initialize the mutex and cond fail\n");
			break;
		}

		//start min_thr_num work thread 
		for(i=0;i<min_thr_num;++i){
			pthread_create(&(pool->threads[i]),NULL,threadpool_thread,(void*)pool);
			printf("start thread 0x%x...\n",(unsigned int)pool->threads[i]);
		}


		//start manager thread 
		pthread_create(&(pool->adjust_tid),NULL,adjust_thread,(void*)pool);

		return pool;
	}while(0);
	return NULL;
}














int threadpool_add(threadpool_t* pool,void*(function)(void* arg),void* arg){
	pthread_mutex_lock(&(pool->lock));


	//if the task queue is full,wait for threadpool get the task 
	while((pool->queue_size==pool->queue_max_size) && (!pool->shotdown)){
		pthread_cond_wait(&(pool->queue_not_full),&(pool->lock));
	}

	if(pool->shotdown){
		pthread_mutex_unlock(&(pool->lock));
	}

	//clean the arguement of callback function at rear of task queue 
	if(pool->task_queue[pool->queue_rear].arg != NULL){
		free(pool->task_queue[pool->queue_rear].arg);
		pool->task_queue[pool->queue_rear].arg=NULL;
	}

	//add task to the task queue 
	pool->task_queue[pool->queue_rear].function=function;
	pool->task_queue[pool->queue_rear].arg=arg;
	pool->queue_rear=(pool->queue_rear+1)%(pool->queue_max_size);
	pool->queue_size++;

	//after add a task to the task queue,task queue is't empty,wakeup a thread in threadpool 
	pthread_cond_signal(&(pool->queue_not_empty));
	pthread_mutex_unlock(&(pool->lock));

	return 0;
}












int threadpool_destory(threadpool_t *pool){
	int i=0;
	if(pool==NULL){
		return -1;
	}

	pool->shotdown=true;

	pthread_join(pool->adjust_tid,NULL);
	

	//inform every idle thread 
	for(i=0;i<pool->live_thr_num;++i){
		pthread_cond_broadcast(&(pool->queue_not_empty));
	}
	
	//???????threads[i] may be is empty?
	for(i=0;i<pool->live_thr_num;i++){
		pthread_join(pool->threads[i],NULL);
	}

	threadpool_free(pool);

	return 0;
}






int threadpool_free(threadpool_t* pool){
	if(pool==NULL)
		return -1;
	if(pool->task_queue)
		free(pool->task_queue);

	if(pool->threads){
		pthread_mutex_lock(&(pool->lock));
		pthread_mutex_destroy(&(pool->lock));

		pthread_mutex_lock(&(pool->thread_counter));
		pthread_mutex_destroy(&(pool->thread_counter));

		pthread_cond_destroy(&(pool->queue_not_empty));
		pthread_cond_destroy(&(pool->queue_not_full));
	}

	free(pool);
	pool=NULL;
	return 0;
}









void* adjust_thread(void* threadpool){
	int i=0;
	threadpool_t* pool=(threadpool_t*)threadpool;

	while(!pool->shotdown){
		//manage threadpool on time 
		sleep(DEFAULT_TIME);	

		pthread_mutex_lock(&(pool->lock));
		int queue_size=pool->queue_size;
		int live_thr_num=pool->live_thr_num;
		pthread_mutex_unlock(&(pool->lock));

		pthread_mutex_lock(&(pool->thread_counter));
		int busy_thr_num=pool->busy_thr_num;
		pthread_mutex_unlock(&(pool->thread_counter));


		//create-----------algorithm of manage thread 
		if(queue_size >= MIN_WAIT_TASK_NUM && live_thr_num < pool->max_thr_num){
			pthread_mutex_lock(&(pool->lock));
			int add=0;

			for(i=0;i<pool->max_thr_num && add<DEFAULT_THREAD_VARY && pool->live_thr_num<pool->max_thr_num;++i){
				if(pool->threads[i]==0 || !is_thread_alive(pool->threads[i])){
					pthread_create(&(pool->threads[i]),NULL,threadpool_thread,(void*)pool);	
					add++;
					pool->live_thr_num++;	
				}
			}
			pthread_mutex_unlock(&(pool->lock));
		}


		//destory--------algorithm of manage thread 
		if((busy_thr_num*2)<live_thr_num && live_thr_num>pool->min_thr_num){
			pthread_mutex_lock(&(pool->lock));
			pool->wait_exit_thr_num=DEFAULT_THREAD_VARY;
			pthread_mutex_lock(&(pool->lock));

			for(i=0;i<DEFAULT_THREAD_VARY;++i){
				pthread_cond_signal(&(pool->queue_not_empty));
			}
		}
	}
	return NULL;
}













//work thread 
void* threadpool_thread(void* threadpool){
	threadpool_t* pool = (threadpool_t*)threadpool;
	threadpool_task_t task;

	while(true){
		pthread_mutex_lock(&(pool->lock));

		// if the task queue is empty, call wait block in the queue_not_empty
		while((pool->queue_size==0) && (!pool->shotdown)){
			printf("thread 0x%x is waiting\n",(unsigned int)pthread_self());
			//at the begining,the threads of the threadpool are block here,until the server thread call pthread_cond_signal 
			pthread_cond_wait(&(pool->queue_not_empty),&(pool->lock));

			if(pool->wait_exit_thr_num>0){
				pool->wait_exit_thr_num--;

				if(pool->live_thr_num>pool->min_thr_num){
					printf("thread 0x%x is exiting\n",(unsigned int)pthread_self());
					pool->live_thr_num--;
					pthread_mutex_unlock(&(pool->lock));
					pthread_exit(NULL);
				}
			}
		}

		//if want to shotdown the threadpool,let every thread of block ahead statement execute next statement;
		if(pool->shotdown){
			pthread_mutex_unlock(&(pool->lock));
			printf("thread 0x%x is exiting\n",(unsigned int)pthread_self());
			pthread_exit(NULL);
		}

		//get task from task queue 
		task.function=pool->task_queue[pool->queue_front].function;
		task.arg=pool->task_queue[pool->queue_front].arg;
		pool->queue_front=(pool->queue_front+1)%pool->queue_max_size;
		pool->queue_size--;

		//can add new task to task queue 
		pthread_cond_broadcast(&(pool->queue_not_full));

		pthread_mutex_unlock(&(pool->lock));


		//execute the task 
		printf("thread 0x%x start working\n",(unsigned int)pthread_self());
		pthread_mutex_lock(&(pool->thread_counter));
		pool->busy_thr_num++;
		pthread_mutex_unlock(&(pool->thread_counter));

		//execute the callback function 
		(*(task.function))(task.arg);


		//task finish
		printf("thread 0x%x end working\n",(unsigned int)pthread_self());
		pthread_mutex_lock(&(pool->thread_counter));
		pool->busy_thr_num--;
		pthread_mutex_unlock(&(pool->thread_counter));
	}
	pthread_exit(NULL);
}



int is_thread_alive(pthread_t tid){
	int kill_rc=pthread_kill(tid,0);
	if(kill_rc==ESRCH){
		return false;
	}
	return true;
}













