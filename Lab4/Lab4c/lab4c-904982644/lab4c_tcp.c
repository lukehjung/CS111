// NAME: Luke Jung
// EMAIL: lukejung@ucla.edu
// ID: 904982644

#define _POSIX_C_SOURCE 200809L
#define h_addr h_addr_list[0]
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
#include <netdb.h>
#include <netinet/in.h>
#include <ctype.h>
#include <sys/socket.h>

#define A0 1
#define SERVER 1
#define SCALE 100000.0


FILE* log_fd = 0;
int scale = 'F';
int period = 1;
int paused = 0;
int log_flag = 0;

mraa_aio_context temp_sensor;

time_t next = 0;
struct tm *now;

// server vars
int port = -1;
struct sockaddr_in serv_address;
struct hostent * server;
char * host = "";
char * id = "";
int sock;

void printer(const char * string, int server)
{
	if (server)
		dprintf(sock, "%s\n", string);
	fprintf(stderr,  "%s\n", string);
	
	fprintf(log_fd, "%s\n", string);
	fflush(log_fd);

}

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

void m_shutdown()
{
	struct timeval ck;
	char buffer[200];
	gettimeofday(&ck, 0);
	now = localtime(&ck.tv_sec);

	sprintf(buffer, "%02d:%02d:%02d SHUTDOWN\n", now->tm_hour, now->tm_min, now->tm_sec);
	printer(buffer, SERVER);
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
	while(*string == '\t' || *string == ' ')
	{
		string ++;
	}

	if(!strcmp(string, "START"))
	{
		paused = 0;
		printer(string, 0);
	}
	else if (!strcmp(string, "STOP")) 
	{
		paused = 1;
		printer(string, 0);
	}
	else if(!strcmp(string, "OFF"))
	{
		printer(string, 0);
		m_shutdown();
	}
	else if (!strcmp(string, "SCALE=F")) 
	{
		scale = 'F';
		printer(string, 0);
	}
	else if (!strcmp(string, "SCALE=C")) 
	{
		scale = 'C';		
		printer(string, 0);
	}
	else if(strlen(string) > 4 && string[0] == 'L' && string[1] == 'O' && string[2] == 'G')
	{
		printer(string, 0);

	}
	else if(string[0] == 'P' && string[1] == 'E' && string[2] == 'R')
	{
		int m_period = checkPeriod(string);

		if(m_period == 0)
		{
			fprintf(stderr, "Error: Invalid Period.\n");
			exit(1);
		}

		period = m_period;
		printer(string, 0);
	}
	else
	{
		fprintf(stderr, "Error: Invalid command %s.\n", string);
		exit(1);
	}
}

void get_server_input()
{
	char * input = (char *) malloc(sizeof(char) * 1024);
	char * copy = input; // create a copy of the input string to parse

	// ret is the number of bytes that read caught
	int ret = read(sock, input, 256);

	if (ret > 0) 
		input[ret] = 0; // where text for buffer ends, place a 0
	

	while (copy < &input[ret]) // while the copy's address is before the end of the input
	{
		// create incr string to find each the end of each word
		char* incr = copy;
		while (incr < &input[ret] && *incr != '\n') 
		{
			incr++;
		}
		*incr = 0;
		process(copy); // process null string, then each new word that comes up
		copy = &incr[1];
	}
}

void connect_to_server()
{
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0)
	{
		fprintf(stderr, "Error: Client could not create socket.\n");
		exit(1);
	}

	server = gethostbyname(host);

	if(server == NULL)
	{
		fprintf(stderr, "Error: Host not found.\n");
	}

	unsigned size = sizeof(serv_address);
	memset((void *) &serv_address, 0, size);
	serv_address.sin_family = AF_INET;

	memcpy((char *) &serv_address.sin_addr.s_addr, (char *) server->h_addr, server->h_length);
	serv_address.sin_port = htons(port);

	int num = connect(sock, (struct sockaddr *) &serv_address, size);
	if ( num < 0 )
	{
		fprintf(stderr, "Error: Client could not connect to server.\n");
		exit(1);
	}
}

int main(int argc, char ** argv)
{
	static struct option long_options[] = 
	{
		{"scale", required_argument, 0, 's'},
		{"period", required_argument, 0, 'p'},
		{"log", required_argument, 0, 'l'},
		{"host", required_argument, 0, 'h'},
		{"id", required_argument, 0, 'i'},
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
				log_fd = fopen(optarg, "w+");
				if(log_fd == NULL)
				{	
					fprintf(stderr, "Error: Could not open log file.\n");
					exit(1);
				}
				log_flag = 1;
				break;
			}

			case 'h':
			{
				host = optarg;
				if (strlen(host) == 0)
				{
					fprintf(stderr, "Error: Host Argument is empty.\n");
					exit(1);
				}
				break;
			}

			case 'i':
			{
				id = optarg;
				if (strlen(id) != 9)
				{
					fprintf(stderr, "Error: ID is not 9 digits.\n");
					exit(1);
				}
				break;
			}
			default:
				fprintf(stderr, "Error: Bogus argument.\n");
				exit(1);

		}
	}

	if (optind < argc)
	{
		port = atoi(argv[optind]);
		if (port < 1)
		{
			fprintf(stderr, "Error: Port is Invalid.\n");
			exit(1);
		}
	}

	connect_to_server();
	
	char buf[20];
	sprintf(buf, "ID=%s", id);
	printer(buf, SERVER);

	// initialize senosrs on beaglebone
	temp_sensor = mraa_aio_init(A0);

	struct pollfd pollfds[1];

	pollfds[0].fd = sock;
	pollfds[0].events = POLLIN | POLLHUP | POLLERR;

	struct timeval ck;
	char buffer[100];

	while(1)
	{
		gettimeofday(&ck, 0);
		
		// if current ck is greater or equal than last ck second plus period...
		if(ck.tv_sec >= next && !paused)
		{
			double temp = measure_temp();

			now = localtime(&(ck.tv_sec));
			sprintf(buffer, "%02d:%02d:%02d %.1f", now->tm_hour, now->tm_min, now->tm_sec, temp);
			printer(buffer, SERVER);

			next = ck.tv_sec + period;
		}

		int poll_ret = poll(pollfds, 1, 0);
		if(poll_ret < 0)
		{
			fprintf(stderr, "Error: Polling error.");
			exit(1);
		}

		if(poll_ret)
		{
			get_server_input();
		}
	}

	mraa_aio_close(temp_sensor);
	return 0;
}
