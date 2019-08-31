/*
NAME: Luke Jung
EMAIL: lukejung@ucla.edu
UID: 904982644
*/

#include "SortedList.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>

#define THREADS 't'
#define ITERATIONS 'i'
#define SYNC 'x'
#define YIELD 'y'
#define NONE 'n'
#define MUTEX 'm'
#define SPINLOCK 's'
#define LISTS 'l'

#define ID_YIELD 0x03
#define IL_YIELD 0x05
#define DL_YIELD 0x06
#define IDL_YIELD 0x07

SortedList_t * lists;
SortedListElement_t * elements;

int iterations = 1;
pthread_mutex_t lock;
char options = NONE;
int threads;
int spinlock;
int numlists =  1;
long long * threadTimes;

int opt_yield = 0;
int element_len = 0;

void seg_fault()
{
	fprintf(stderr, "Error: Seg fault. \n");
	exit(2);
}

//Hash fucnction from http://www.partow.net/programming/hashfunctions/#top
unsigned long hash(const char *str)
{
   unsigned int b    = 378551;
   unsigned int a    = 63689;
   unsigned int hash = 0;
   unsigned int i    = 0;

   for (i = 0; i < 10; ++str, ++i)
   {
      hash = hash * a + (*str);
      a = a * b;
   }

   return hash % numlists;
}

void * run_threads(void * list)
{
	struct timespec start;
	struct timespec end;

	int num = (*(int*) list);
	for(int i = num; i < element_len; i += threads)
	{
		int index = hash(elements[i].key);
		switch(options)
		{		
			case MUTEX:
			{
				clock_gettime(CLOCK_MONOTONIC, &start);
				pthread_mutex_lock(&lock);
				clock_gettime(CLOCK_MONOTONIC, &end);

				long long time = end.tv_nsec - start.tv_nsec;
				time += (end.tv_sec - start.tv_sec) * 1000000000;
				threadTimes[num] += time;

		     	SortedList_insert(&lists[index], &elements[i]);
		     	pthread_mutex_unlock(&lock);
				break; 
			}

			case SPINLOCK:
			{
				clock_gettime(CLOCK_MONOTONIC, &start);
          		while (__sync_lock_test_and_set(&spinlock, 1)) continue;
				clock_gettime(CLOCK_MONOTONIC, &end);

				long long time = end.tv_nsec - start.tv_nsec;
				time += (end.tv_sec - start.tv_sec) * 1000000000;
				threadTimes[num] += time;

				SortedList_insert(&lists[index], &elements[i]);
		     	__sync_lock_release(&spinlock);
				break; 
			}
			case NONE:
			{
				SortedList_insert(&lists[index], &elements[i]);
				break;
			}
		}
	}

	SortedListElement_t * el;
	for(int i = num; i < element_len; i += threads)
	{
		int index = hash(elements[i].key);
		switch(options)
		{		
			case MUTEX:
			{
				clock_gettime(CLOCK_MONOTONIC, &start);
				pthread_mutex_lock(&lock);
				clock_gettime(CLOCK_MONOTONIC, &end);

				long long time = end.tv_nsec - start.tv_nsec;
				time += (end.tv_sec - start.tv_sec) * 1000000000;
				threadTimes[num] += time;

		     	el = SortedList_lookup(&lists[index], elements[i].key);
		     	if(el)
		     	{
		     		SortedList_delete(el);
		     	}
		     	pthread_mutex_unlock(&lock);
				break; 
			}

			case SPINLOCK:
			{
				clock_gettime(CLOCK_MONOTONIC, &start);
          		while (__sync_lock_test_and_set(&spinlock, 1)) continue;
				clock_gettime(CLOCK_MONOTONIC, &end);

				long long time = end.tv_nsec - start.tv_nsec;
				time += (end.tv_sec - start.tv_sec) * 1000000000;
				threadTimes[num] += time;

				el = SortedList_lookup(&lists[index], elements[i].key);
		     	if(el)
		     	{
		     		SortedList_delete(el);
		     	}
		     	__sync_lock_release(&spinlock);
				break; 
			}
			case NONE:
			{
				el = SortedList_lookup(&lists[index], elements[i].key);
		     	if(el)
		     	{
		     		SortedList_delete(el);
		     	}
				break;
			}
		}
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	struct option long_options[] = 
	{
		{"threads", required_argument, 0, THREADS},
		{"iterations", required_argument, 0, ITERATIONS},
		{"sync", required_argument, 0, SYNC},
		{"yield", required_argument, 0, YIELD},
		{"lists", required_argument, 0, LISTS},
		{0, 0, 0, 0}
	};

	int c = 0;
	opt_yield = 0;
	threads = 1;
	signal(SIGSEGV, seg_fault);

	while((c = getopt_long(argc, argv, "", long_options, 0)) != -1)
	{
		switch(c)
		{
			case THREADS:
			{
				threads = atoi(optarg);
				break;
			}
			case ITERATIONS:
			{
				iterations = atoi(optarg);
				break;
			}
			case YIELD:
			{
				int len = strlen(optarg);
				for(int i = 0; i < len; i ++)
				{
					if(optarg[i] == 'd')
						opt_yield |= DELETE_YIELD;
					if(optarg[i] == 'i')
						opt_yield |= INSERT_YIELD;
					if(optarg[i] == 'l')
						opt_yield |= LOOKUP_YIELD;
				}
				break;
			}
			case SYNC:
			{
				options = optarg[0];
				if(options != MUTEX && options != SPINLOCK)
				{
					fprintf(stderr, "Error: Invalid Sync Option. \n");
					exit(1);
				}
				break;
			}
			case LISTS:
			{
				numlists = atoi(optarg);
				break;
			}
			default:
			{
				fprintf(stderr, "Error: Invalid option. \n");
				exit(1);
				break;
			}
		}
	}

	if(options == MUTEX)
	{
		if(pthread_mutex_init(&lock, NULL))
		{
			fprintf(stderr, "Error: Mutex Initialization failed. \n");
			exit(1);
		}
	} 

	element_len = threads * iterations;
	elements = malloc(element_len * sizeof(SortedListElement_t));
	if(elements == NULL)
	{
		fprintf(stderr, "Error: Could not allocate memory for list. \n");
		exit(1);
	}

	char ** keys = malloc(element_len * sizeof(char*));
	if(keys == NULL)
	{
		fprintf(stderr, "Error: Could not allocate memory for keys array. \n");
		exit(1);
	}

	for(int i = 0; i < element_len; i ++)
	{
		keys[i] = malloc(10 * sizeof(char));
		if(keys[i] == NULL)
		{
			fprintf(stderr, "Error: Could not allocate memory for keys. \n");
			exit(1);
		}
		for(int j = 0; j < 9; j ++)
		{
			keys[i][j] = rand() % 28 + 'A';
		}
		keys[i][9] = '\0';
		elements[i].key = keys[i];
	}

	lists = malloc(numlists * sizeof(SortedList_t));

	for(int i = 0; i < numlists; i ++)
	{
		lists[i].prev = &lists[i];
		lists[i].next = &lists[i];
		lists[i].key = NULL;
	}

	pthread_t * id = malloc(sizeof(pthread_t) * threads);
	int * idthreads = malloc(sizeof(int) * threads);
	threadTimes = malloc(sizeof(long long) * threads);
	for(int i = 0; i < threads; i ++)
	{
		threadTimes[i] = 0;
	}
	
	struct timespec start;
	clock_gettime(CLOCK_MONOTONIC, &start);

	for(int i = 0; i < threads; i ++)
	{	
		idthreads[i] = i;
		int n = pthread_create(&id[i], NULL, &run_threads, &idthreads[i]);
		if(n)
		{
			fprintf(stderr, "Error: Creation of Threads failed. \n");
			free(id);
			exit(1);
		}
	}
	
	for(int j = 0; j < threads; j++)
	{
		int n = pthread_join(id[j], NULL);	
		if(n)
		{
			fprintf(stderr, "Error: Joining Threads failed. \n");
			exit(1);
		}
	}

	struct timespec stop;
	clock_gettime(CLOCK_MONOTONIC, &stop);

	fprintf(stdout, "list");

	switch(opt_yield) 
	{
		case DELETE_YIELD:
			fprintf(stdout, "-d");
			break;
		case INSERT_YIELD:
			fprintf(stdout, "-i");
			break;
		case LOOKUP_YIELD:
			fprintf(stdout, "-l");
			break;
		case ID_YIELD:
			fprintf(stdout, "-id");
			break;
		case IL_YIELD:
			fprintf(stdout, "-il");
			break;
		case DL_YIELD:
			fprintf(stdout, "-dl");
			break;
		case IDL_YIELD:
			fprintf(stdout, "-idl");
			break;
		default:
			fprintf(stdout, "-none");
			break;
	}

	switch(options) 
	{
		case MUTEX:
			fprintf(stdout, "-m");
			break;
		case SPINLOCK:
			fprintf(stdout, "-s");
			break;
		default:
			fprintf(stdout, "-none");
			break;
	}

	long long time = stop.tv_nsec - start.tv_nsec;
	time += (stop.tv_sec - start.tv_sec) * 1000000000;

	long long ops = threads * iterations * 3;
	long long total = 0;
	for(int i = 0; i < threads; i ++)
	{
		total += threadTimes[i];
	}
	long long averageTime = total / threads;

	printf(",%d,%d,%d,%lld,%lld,%lld,%lld\n", threads, iterations, numlists, 
		ops, time, time / ops, averageTime);

	free(id);
	free(elements);

	if(options == MUTEX)
	{
			pthread_mutex_destroy(&lock);
	}
	exit(0);
}