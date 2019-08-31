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

#define ID_YIELD 0x03
#define IL_YIELD 0x05
#define DL_YIELD 0x06
#define IDL_YIELD 0x07

SortedList_t head;
SortedListElement_t * elements;

static int iterations = 1;
static pthread_mutex_t lock;
static char options = NONE;
static int threads;
static int spinlock = 0;
int opt_yield = 0;

void seg_fault()
{
	fprintf(stderr, "Error: Seg fault. \n");
	exit(2);
}

void * run_threads(void * list)
{
	SortedListElement_t * array = list;

	if (options == MUTEX)
		pthread_mutex_lock(&lock);

	else if(options == SPINLOCK)
		while (__sync_lock_test_and_set(&spinlock, 1));

	for(int i = 0; i < iterations; i ++)
	{
		SortedList_insert(&head, (SortedListElement_t *) (array + i));
	}

	long length = SortedList_length(&head);
	char * curr_key = malloc(10 * sizeof(char));
	SortedListElement_t * temp;

	if(length < iterations)
	{
		fprintf(stderr, "Error: List length shorter than iterations. \n");
		exit(2);
	}

	for(int i = 0; i < iterations; i ++)
	{
		strcpy(curr_key, (array + i)->key);
		temp = SortedList_lookup(&head, curr_key);

		if (temp == NULL)
		{
			fprintf(stderr, "Error: Couldn't find key. \n");
			exit(2);
		}

		if (SortedList_delete(temp) != 0)
		{
			fprintf(stderr, "Error: Cannot delete element correctly. \n");
			exit(2);
		}
	}

	if (options == MUTEX)
		pthread_mutex_unlock(&lock);
	if (options == SPINLOCK)
		__sync_lock_release(&spinlock);

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
				if(options == MUTEX)
				{
					if(pthread_mutex_init(&lock, NULL))
					{
						fprintf(stderr, "Error: Mutex Initialization failed. \n");
						exit(1);
					}
				}
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

	int element_len = threads * iterations;
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
		(elements + i)->key = keys[i];
	}

	pthread_t * id = (pthread_t *) malloc(sizeof(pthread_t) * threads);

	if( id == NULL )
	{
		fprintf(stderr, "Error: Could not create pthread id array. \n");
		exit(1);
	}

	struct timespec start;
	clock_gettime(CLOCK_MONOTONIC, &start);

	int i = 0;
	for(; i < threads; i ++)
	{	
		int n = pthread_create(&id[i], NULL, &run_threads, (void *) (elements + iterations * i));
		if(n)
		{
			fprintf(stderr, "Error: Creation of Threads failed. \n");
			free(id);
			exit(1);
		}
	}

	int j = 0;
	for(; j < threads; j++)
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

	if(SortedList_length(&head) != 0)
	{
		fprintf(stderr, "Error: List did not become 0. \n");
		exit(2);
	}

	fprintf(stdout, "list");

	switch(opt_yield) {
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

	switch(options) {
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
	printf(",%d,%d,%d,%lld,%lld,%lld\n", threads, iterations, 1, 
		ops, time, time / ops);


	free(id);
	free(elements);

	if(options == MUTEX)
	{
		pthread_mutex_destroy(&lock);
	}
	exit(0);
}