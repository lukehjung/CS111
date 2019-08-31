// NAME: Luke Jung
// EMAIL: lukejung@ucla.edu
// ID: 904982644

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <mraa.h>
#include <mraa/aio.h>
#define A0 1
#define GPIO_50 60
#define SCALE 100000.0


FILE* log_fd = 0;
int scale = 'F';
int period = 1;
int paused = 0;
int log_flag = 0;

mraa_aio_context temp_sensor;
mraa_gpio_context button;

char buffer[100];
time_t next = 0;
struct tm *now;

double measure_temp()
{
	int temp = mraa_aio_read(temp_sensor);
	float read = (1023.0/((float) temp) - 1.0) * SCALE;
	float celsius = 1.0/(log(read/SCALE)/4275 + 1/298.15) - 273.15; 
	float faren = (celsius * 9)/5 + 32;

	if (scale == 'C')
	{
		return celsius;
	}

	else 
		return faren;
}

void shutdown()
{
	struct timeval ck;
	char buffer[200];
	gettimeofday(&ck, 0);
	now = localtime(&ck.tv_sec);

	sprintf(buffer, "%02d:%02d:%02d SHUTDOWN\n", now->tm_hour, now->tm_min, now->tm_sec);
	fputs(buffer, stdout);

	if (log_flag)
		fputs(buffer, log_fd);

	exit(0);
}

int checkPeriod(const char * string)
{
	char periodstr[] = "PERIOD=";
	int i = 0;
	for(; i < 7; i ++)
	{
		if(string[i] != periodstr[i])
			return 0;
	}

	unsigned len = strlen(string);

	for(unsigned j = 7; j < len - 1; j ++)
	{
		if (!isdigit(string[j]))
		{
			return 0;
		}
	}

	char * num = malloc((len+1) * sizeof(int));
 	memcpy( num, &string[7], len);
	num[len] = '\0';

	return atoi(num);
}

void process(const char * string)
{
	if(!strcmp(string, "START\n"))
	{
		paused = 0;
		if (log_flag)
			fprintf(log_fd, string);
		fflush(log_fd);
	}
	else if (!strcmp(string, "STOP\n")) 
	{
		paused = 1;
		if (log_flag)
			fprintf(log_fd, string);
		fflush(log_fd);

	}
	else if(!strcmp(string, "OFF\n"))
	{
		if (log_flag)
			fprintf(log_fd, string);
		fflush(log_fd);
		shutdown();
	}
	else if (!strcmp(string, "SCALE=F\n")) 
	{
		scale = 'F';
		if (log_flag)
			fprintf(log_fd, string);
		fflush(log_fd);
	}
	else if (!strcmp(string, "SCALE=C\n")) 
	{
		scale = 'C';
		if (log_flag)
			fprintf(log_fd, string);
		fflush(log_fd);
	}
	else if(strlen(string) > 4 && string[0] == 'L' && string[1] == 'O' && string[2] == 'G')
	{
		if (log_flag)
			fprintf(log_fd, string);
		fflush(log_fd);
	}
	else if(string[0] == 'P' && string[1] == 'E' && string[2] == 'R')
	{
		int m_period = checkPeriod(string);

		if(m_period == 0)
		{
			fprintf(stderr, "Error: Invalid Period.\n");
			exit(1);
		}

		if (log_flag)
			fprintf(log_fd, string);
		fflush(log_fd);
		period = m_period;
	}
	else
	{
		fprintf(stderr, "Error: Invalid command.\n");
		exit(1);
	}


}

void run(struct pollfd *pollfds)
{
	struct timeval ck;

	while(1)
	{
		double temp = measure_temp();
		gettimeofday(&ck, 0);
		
		// if current ck is greater or equal than last ck second plus period...b
		if(ck.tv_sec >= next && !paused)
		{
			now = localtime(&(ck.tv_sec));
			sprintf(buffer, "%02d:%02d:%02d %.1f\n", now->tm_hour, now->tm_min, now->tm_sec, temp);
			fputs(buffer, stdout);

			if(log_flag)
				fputs(buffer, log_fd);
			next = ck.tv_sec + period;
		}

		int j = poll(pollfds, 1, 0);
		if(j < 0)
		{
			fprintf(stderr, "Error: Polling error.");
			exit(1);
		}

		if(pollfds[0].revents & POLLIN)
		{
			char tmp[100];
			fgets(tmp, 100, stdin);
			process(tmp);
		}

		if(mraa_gpio_read(button))
			shutdown();
	}
}

int main(int argc, char ** argv)
{
	static struct option long_options[] = 
	{
		{"scale", required_argument, 0, 's'},
		{"period", required_argument, 0, 'p'},
		{"log", required_argument, 0, 'l'},
		{0, 0, 0, 0}
	 };

	int c, option_index, num;

	while((c = getopt_long(argc, argv, "", long_options, &option_index)) != -1)
	{
		switch (c)
		{
			case 's':
			{
				if((optarg[0] != 'C' || optarg[0] != 'F') && strlen(optarg) == 1)
				{
					scale = optarg[0];
				}
				else
					fprintf(stderr, "Error: Invalid Scale.\n");
				break;
			}

			case 'p':
			{
				num = atoi(optarg);
				if(num > 0)
				{	
					period = num;
				}
				else
					fprintf(stderr, "Error: Negative Period.\n");
				break;
			}

			case 'l':
			{
				log_fd = fopen(optarg, "w");
				if(log_fd == NULL)
				{	
					fprintf(stderr, "Error: Could not open file.\n");
				}
				log_flag = 1;
				break;
			}
			default:
				fprintf(stderr, "Error: Bogus argument.\n");
				exit(1);

		}
	}

	// initialize senosrs on beaglebone
	temp_sensor = mraa_aio_init(A0);
	button = mraa_gpio_init(GPIO_50);

	// initialize button to be an input
	mraa_gpio_dir(button, MRAA_GPIO_IN);
	struct pollfd pollfds[1];

	pollfds[0].fd = STDIN_FILENO;
	pollfds[0].events = POLLIN | POLLHUP | POLLERR;

	run(pollfds);

	mraa_aio_close(temp_sensor);
	mraa_gpio_close(button);

	return 0;
}
