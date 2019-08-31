/*
NAME: Luke Jung
EMAIL: lukejung@ucla.edu
UID: 904982644
*/

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>

#define THREADS 't'
#define ITERATIONS 'i'
#define SYNC 'x'
#define YIELD 'y'
#define NONE 'n'
#define MUTEX 'm'
#define SPINLOCK 's'
#define CAS 'c'

static int yield = 0;
static long long counter = 0;
static int spinlock = 0;
static int iterations = 1;
static pthread_mutex_t lock;
static char options = NONE;

void add(long long *pointer, long long value)
{
	long long sum = *pointer + value;
	if(yield)
		sched_yield();
	*pointer = sum;
}

void add_controller(int num)
{
	switch(options)
	{
		case NONE:
		{
			for(int i = 0; i < iterations; i ++)
			{
				add(&counter, num);
			}
			break;
		}
		case MUTEX:
		{
			for(int i = 0; i < iterations; i ++)
			{
				pthread_mutex_lock(&lock);
				add(&counter, num);
				pthread_mutex_unlock(&lock);
			}
			break;
		}
		case SPINLOCK:
		{
			for(int i = 0; i < iterations; i ++)
			{
				while (__sync_lock_test_and_set(&spinlock, 1))
                    continue;
                add(&counter, num);
                __sync_lock_release(&spinlock);
			}
			break;
		}
		case CAS:
		{
			for(int i = 0; i < iterations; i ++)
			{
				int temp1, temp2;
				do
				{
					temp1 = counter;
					if(yield)
						sched_yield();
					temp2 = temp1 + num;
				} 
				while(__sync_val_compare_and_swap(&counter, temp1, temp2) != temp1);
			}
			break;
		}
		default:
			break;
	}

}

void *run_threads()
{
	add_controller(1);
	add_controller(-1);
	return NULL;
}

int main(int argc, char *argv[])
{
	struct option long_options[] = 
	{
		{"threads", required_argument, 0, THREADS},
		{"iterations", required_argument, 0, ITERATIONS},
		{"sync", required_argument, 0, SYNC},
		{"yield", no_argument, 0, YIELD},
		{0, 0, 0, 0}
	};

	int c = 0;
	char * optionstr = malloc(sizeof(char) * 20);
	sprintf(optionstr, "add");
	int threads = 1;

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
				yield = 1;
				strcat(optionstr, "-yield");
				break;
			}
			case SYNC:
			{
				options = optarg[0];
				if(options != MUTEX && options != SPINLOCK && options != CAS)
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
				fprintf(stderr, "Error: Invalid option");
				exit(1);
				break;
			}
		}
	} 

	if(options == NONE)
	{
		strcat(optionstr, "-none");
	}

	else
		sprintf(optionstr + strlen(optionstr), "-%c", options);

	pthread_t * id = (pthread_t *) malloc(sizeof(pthread_t) * threads);

	struct timespec start;
	clock_gettime(CLOCK_MONOTONIC, &start);
	int i = 0;
	for(; i < threads; i ++)
	{	
		if(pthread_create(&id[i], NULL, &run_threads, NULL))
		{
			fprintf(stderr, "Error: Creation of Threads failed. \n");
			free(id);
			exit(1);
		}
	}

	int j = 0;
	for(; j < threads; j++)
	{
		if(pthread_join(id[j], NULL))
		{
			fprintf(stderr, "Error: Joining Threads failed. \n");
			exit(1);
		}
	}

	struct timespec stop;
	clock_gettime(CLOCK_MONOTONIC, &stop);

	long long time = stop.tv_nsec - start.tv_nsec;
	time += (stop.tv_sec - start.tv_sec) * 1000000000;

	long long ops = threads * iterations * 2;
	printf("%s,%d,%d,%lld,%lld,%lld,%lld\n", optionstr, threads, iterations, ops,
           time, time / ops, counter);

	free(id);
	free(optionstr);
	if(options == MUTEX)
	{
		pthread_mutex_destroy(&lock);
	}

	if(counter != 0)
		exit(1);
	else
		exit(0);
}