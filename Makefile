CC=gcc

test_threadpool: threadpool.o main.o
	$(CC) -o test_threadpool main.o threadpool.o  -lpthread
main.o: main.c threadpool.c threadpool.h
	$(CC) -c  threadpool.c main.c
threadpool.o: threadpool.c threadpool.h
	$(CC) -c  threadpool.c	

clean:
	rm -f *.o test_threadpool
