#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <sys/resource.h>
#include <sys/wait.h>

#define APPEND O_APPEND
#define CLOEXEC O_CLOEXEC
#define CREAT O_CREAT
#define DIRECTORY O_DIRECTORY
#define DSYNC O_DSYNC
#define EXCL O_EXCL
#define NOFOLLOW O_NOFOLLOW
#define NONBLOCK O_NONBLOCK
#define RSYNC O_RSYNC
#define SYNC O_SYNC
#define TRUNC O_TRUNC

void sighandler(int signum)
{
	fprintf(stderr, "Sighandler: Signal %d caught.\n", signum);
   	exit(signum);
}

int max(int num1, int num2)
{
	return (num1 > num2 ? num1 : num2);
}

void print_message(char c, char ** argv, int optind)
{
	if(c == 'f')
		printf("%s\n", argv[optind-1]);

	if(c == 'o')
		printf("%s %s\n", argv[optind - 2], optarg);

	fflush(stdout);
}

struct commandargs
{
	pid_t pid;
	char * argstring;
};

struct time
{
	struct timeval utime;
	struct timeval stime;
};


/* Subtract the ‘struct timeval’ values X and Y,
   storing the result in RESULT.
   Return 1 if the difference is negative, otherwise 0. */

int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y)
{
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

void start_timer(int kind, struct time * startTime)
{
	struct rusage usage;
	int time = getrusage(kind, &usage);
	startTime->utime = usage.ru_utime;
	startTime->stime = usage.ru_stime;
}

void end_timer(int kind, struct time * startTime, char * str)
{
	struct rusage usage;
	int num = getrusage(kind, &usage);
	struct time endTime;
	struct time totalTime;
	endTime.utime = usage.ru_utime;
	endTime.stime = usage.ru_stime;

	timeval_subtract(&totalTime.utime, &endTime.utime, &startTime->utime);
	timeval_subtract(&totalTime.stime, &endTime.stime, &startTime->stime);


	printf("%s: User time: %ld.%ds | System time: %ld.%lds \n", 
				str, 
				totalTime.utime.tv_sec, totalTime.utime.tv_usec,
				totalTime.stime.tv_sec, totalTime.stime.tv_usec);
	fflush(stdout);
}

int main(int argc, char ** argv)
{
	//declare file descriptor array and possible flags
	int filearr_size = 10;
	int * filelist = (int*) malloc(10 * sizeof(int));
	int filepos = 0, verbose = 0, exitnum = 0;
	
	//intialize process array for when running commands
	int pro_size = 10, pro_pos = 0; 
	pid_t * pro_arr = (pid_t *) malloc(10 * sizeof(pid_t));
	int c = 0, option_index = 0, fileflags = 0;
	int pipefile[2];
	char * str = NULL;
	int cmdarray_size = 10;

	int terminated = 0;
	int signum = 0;

	int commandargs_size = 0;
	struct commandargs * commandstruct = malloc(100*sizeof(struct commandargs));

	int profile = 0;
	struct time * childStartTime = malloc(sizeof(struct time));
	struct time * startTime = malloc(sizeof(struct time));

	struct time * shelltime = malloc(sizeof(struct time));
	struct time * childshelltime = malloc(sizeof(struct time));

	//long_options class to read through different options
	static struct option long_options[] = {
		//File flags
		{"append", no_argument, 0, APPEND},
		{"cloexec", no_argument, 0, CLOEXEC},
		{"creat", no_argument, 0, CREAT},
		{"directory", no_argument, 0, DIRECTORY},
		{"dsync", no_argument, 0, DSYNC},
		{"excl", no_argument, 0, EXCL},
		{"nofollow", no_argument, 0, NOFOLLOW},
		{"nonblock", no_argument, 0, NONBLOCK},
		{"rync", no_argument, 0, RSYNC},
		{"sync", no_argument, 0, SYNC},
		{"trunc", no_argument, 0, TRUNC},

		//Open Files
		{"rdonly", required_argument, 0, 'r'},
		{"rdwr", required_argument, 0, 'R'},
		{"wronly", required_argument, 0, 'w'},
		{"pipe", no_argument, 0, 'p'},

		//Subcommand options
		{"command", required_argument, 0, 'c'},
		{"wait", no_argument, 0, 'W'},

		//Misc options
		{"close", required_argument, 0, 'C'},
		{"verbose", no_argument, 0, 'v'},
		{"profile", no_argument, 0, 'P'},
		{"abort", no_argument, 0, 'a'},
		{"catch", required_argument, 0, 'b'},
		{"ignore", required_argument, 0, 'i'},
		{"default", required_argument, 0, 'd'},
		{"pause", no_argument, 0, 'e'},

		{0,0,0,0}
	};

	//check to make sure both process array and file descriptor array are malloc'ed correctly
	if(!pro_arr || !filelist)
	{
		fprintf(stderr, "Error: Could not allocate dynamic memory. \n");
		exit(1);
	}

	//main while loop to parse through each option
	while((c = getopt_long(argc, argv, "", long_options, &option_index)) != -1) 
	{
		switch (c) 
		{
			case APPEND:    case CLOEXEC:   case CREAT:     case DIRECTORY:
            case DSYNC:     case EXCL:      case NOFOLLOW:  case NONBLOCK:
            case SYNC:      case TRUNC:
                if(profile)
               		start_timer(RUSAGE_SELF, startTime);
                if(verbose)
               		print_message('f', argv, optind);
                fileflags |= c;
                if(profile)
               		end_timer(RUSAGE_SELF, startTime, (char*) argv[optind-1]);
                break;
				
			case 'r': //rdonly
			{              	
				if(verbose)
					print_message('o', argv, optind);

				if(profile)
               		start_timer(RUSAGE_SELF, startTime);
				
				//open file in file description array using O_RDONLY flag
				filelist[filepos] = open(optarg, fileflags | O_RDONLY, S_IRUSR | S_IWUSR | S_IROTH);

				if(filelist[filepos]  == -1) //check for read error, and set flags
				{
					fprintf(stderr, "RDONLY Error: %s, Unable to open file %s. \n", strerror(errno), optarg);
					exitnum = 1;
				}

				filepos++;
				fileflags = 0;
				if(profile)
               		end_timer(RUSAGE_SELF, startTime, (char*) argv[optind-2]);
				break;
			}

			case 'R': //RDWR
			{
				if(verbose)
					print_message('o', argv, optind);
				if(profile)
               		start_timer(RUSAGE_SELF, startTime);
				//open file in file description array using O_RDWR flag
				filelist[filepos] = open(optarg, fileflags | O_RDWR, S_IRUSR | S_IWUSR | S_IROTH);

				if(filelist[filepos]  == -1) //check for read error, and set flags
				{
					fprintf(stderr, "RDWR Error: %s, Unable to open file %s. \n", strerror(errno), optarg);
					exitnum = 1;
				}

				filepos++;
				fileflags = 0;

				if(profile)
               		end_timer(RUSAGE_SELF, startTime, (char*) argv[optind-2]);
				break;
			}

			case 'w': //wronly, same process as above
			{
				if(verbose)
					print_message('o', argv, optind);

				if(profile)
               		start_timer(RUSAGE_SELF, startTime);
				filelist[filepos] = open(optarg, fileflags | O_WRONLY, S_IRUSR | S_IWUSR | S_IROTH);

				if(filelist[filepos]  == -1)
				{
					fprintf(stderr, "WRONLY Error: %s Unable to open file %s. \n", strerror(errno), optarg);
					exitnum = 1;
				}

				filepos++;
				fileflags = 0;
				if(profile)
               		end_timer(RUSAGE_SELF, startTime, (char*) argv[optind-2]);
				break;
			}

			case 'p': 
			{
				if(verbose)
					print_message('f', argv, optind);

				if(profile)
               		start_timer(RUSAGE_SELF, startTime);
				if(pipe(pipefile) == 0)
				{
					filelist[filepos] = pipefile[0]; //read
					filepos++;
					filelist[filepos] = pipefile[1]; //write
					filepos ++;
				}
				else
				{
					fprintf(stderr, "Pipe Error: %s Unable to open file %s. \n", strerror(errno), optarg);
					exitnum = 1;
				}

				if(profile)
               		end_timer(RUSAGE_SELF, startTime, (char*) argv[optind-1]);
				break;
			}

			case 'c': //command
			{
				int start = optind;
				int cur = start;
				int end;

				int input = atoi(argv[start-1]); //set input fd to one before optind
				int output = atoi(argv[start]); //set output fd to where optind is located
				int error = atoi(argv[start+1]); //set error fd to one after optind
				char * cmd = argv[start+2]; 	//starting command after fd

				if(profile)
               		start_timer(RUSAGE_SELF, startTime);
				
				//check for any file descriptor errors
				if(input < 0 || output < 0 || error < 0 || input >= filepos || output >= filepos || error >= filepos)
				{
					fprintf(stderr, "Error: Invalid I/O or Error file descriptor \n");
					exit(1);		
				}

				while(cur < argc)
				{
					//parse through option until next "--" option
					if(argv[cur][0] == '-' && argv[cur][1] == '-')
					{
						end = cur - 1;
						break;
					}
					cur++;
				}

				//if current index is at end, set end to one before.
				if(cur == argc)
					end = argc - 1;

				int optlen = end - start - 2; //create local variable for length of option
				char ** optarray = malloc((optlen+1) * sizeof(char*));

				if(optarray) //check to see if array is malloc'ed correctly
				{
					//set first part of array to the function
					optarray[0] = cmd;
					int i = 1;
					for(; i <= optlen; i++) //insert options into optarray
					{
						optarray[i] = argv[start + 2 + i];
					}
					optarray[i] = NULL; //set end of array to NULL				

					if(verbose)
					{
						printf("%s %d %d %d", argv[optind-2], input, output, error);
						int inc = 0;
						while(inc <= optlen)
						{
							printf(" %s", optarray[inc]);
							fflush(stdout);
							inc++;
						}
						printf("\n");
						fflush(stdout);
					}


					int inc = 1;
					int index = 0;
					int cmdstring_size = 40;
					char* cmdstring = malloc(cmdstring_size*sizeof(char));
					while(index < strlen(cmd))
					{
						cmdstring[index] = cmd[index];
						index++;
					}
					cmdstring[index] = ' ';
					index ++;

					while(inc <= optlen)
					{
						for(int j = 0; j < strlen(argv[start + 2 + inc]); j ++)
						{
							if(cmdstring_size - index < 15)
							{
								cmdarray_size += 40;
								cmdstring = realloc(cmdstring, cmdstring_size*sizeof(char));
							}

							cmdstring[index] = argv[start + 2 + inc][j];
							index++;
						}
						cmdstring[index] = ' ';
						index++;
						inc ++;
					}	

					//check for closed files
					if(filelist[input] == -1 || filelist[output] == -1 || filelist[error] == -1)
					{
						fprintf(stderr, "Error: File is unable to be opened.\n");
						exit(1);
					}


					if(cmd[0] != '-' && cmd != NULL)
					{	
						pro_arr[pro_pos] = fork(); //fork processes in the process array to run

						if(pro_arr[pro_pos] >= 0) //check to make sure the fork was sucessful
						{
							if(pro_arr[pro_pos] == 0) // if the process is a child
							{
								//duplicate each file descriptor
								dup2(filelist[input], 0);
								dup2(filelist[output], 1);
								dup2(filelist[error], 2);

								int j = 0;

								while(j < filepos)
								{
									close(filelist[j]); //close original file descriptors
									j++;
								}

								//execvp runs the function, check to see ran correctly							
	  							if (execvp(optarray[0], optarray) == -1)
								{
									fprintf(stderr, "Error %s: Could not execute command. \n", strerror(errno));
									exitnum = 1;
								}
							}
							commandstruct[commandargs_size].argstring = cmdstring;
							commandstruct[commandargs_size].pid = pro_arr[pro_pos];
							commandargs_size++;
						}
						else //else return an error
						{
							fprintf(stderr, "Error: Could not create child process. \n");
							exitnum = 1;;
						}

					}
					else //else return an error
					{
						fprintf(stderr, "Error: Invalid Command. \n");
						exitnum = 1;;
					}
				}
				else
				{
					fprintf(stderr, "Error: Could not allocate memory. \n");
					exitnum = 1;
				}
				pro_pos++; //increase process array index each time goes through

				if(profile)
               		end_timer(RUSAGE_SELF, startTime, (char*) argv[optind-2]);
				break;
			}

			case 'W': //wait
			{
				if(verbose)
					print_message('f', argv, optind);


				if(profile)
				{
               		start_timer(RUSAGE_SELF, startTime);
            		start_timer(RUSAGE_CHILDREN, startTime);
				}

				while(terminated < pro_pos)
				{
					int wstatus;
					int cstatus = 0;
					pid_t finprocess = waitpid(-1, &wstatus, 0);

					if(WIFEXITED(wstatus))
					{
						cstatus = WEXITSTATUS(wstatus);
						printf("exit %d ", cstatus);
					}

					if(WIFSIGNALED(wstatus))
					{
						cstatus = WTERMSIG(wstatus);
						printf("signal %d ", cstatus);
						signum = max(signum, cstatus);
					}

					exitnum = max(cstatus, exitnum);
					int i = 0;
					for(; i < commandargs_size; i ++)
					{
						if(commandstruct[i].pid == finprocess)
						{
							printf("%s\n", commandstruct[i].argstring);
							fflush(stdout);
							break;
						}
					}
					terminated ++;
				}

				// free(commandstruct);
				commandargs_size = 0;
				fflush(stdout);
				commandstruct = malloc(100*sizeof(int));
				
				if(profile)
				{
               		end_timer(RUSAGE_SELF, startTime, (char*) argv[optind-1]);
               		end_timer(RUSAGE_CHILDREN, startTime, "Children Processes");

				}

				break;
			}

			case 'C': //close
			{
				if(verbose)
					print_message('f', argv, optind);

				if(profile)
    				start_timer(RUSAGE_SELF, startTime);

				int index = atoi(optarg);
				if(close(filelist[index]) == -1)
				{
					fprintf(stderr, "Error: Could not close file correctly. \n");
					exitnum = 1;
				}
				filelist[index] = -1;

				if(profile)
   					end_timer(RUSAGE_SELF, startTime, (char*) argv[optind-2]);
				break;
			}

			case 'v': //verbose
			{
				if(profile)
    				start_timer(RUSAGE_SELF, startTime);
				verbose = 1;
				if(profile)
   					end_timer(RUSAGE_SELF, startTime, (char*) argv[optind-1]);
				break;
			}

			case 'P': //profile
			{
				if(verbose)
					print_message('f', argv, optind);
				profile = 1;
				break;
			}

			case 'a': //abort
			{
				if(verbose)
					print_message('f', argv, optind);
				if(profile)
    				start_timer(RUSAGE_SELF, startTime);
				*str = 'l';
				if(profile)
   					end_timer(RUSAGE_SELF, startTime, (char*) argv[optind-1]);
				break;
			}

			case 'b': //catch
			{
				if(verbose)
					print_message('o', argv, optind);
				if(profile)
    				start_timer(RUSAGE_SELF, startTime);
				signal(atoi(optarg), sighandler);
				if(profile)
   					end_timer(RUSAGE_SELF, startTime, (char*) argv[optind-2]);
				break;
			}

			case 'i': //ignore
			{
				if(verbose)
					print_message('o', argv, optind);
				if(profile)
    				start_timer(RUSAGE_SELF, startTime);
				signal(atoi(optarg), SIG_IGN);
				if(profile)
   					end_timer(RUSAGE_SELF, startTime, (char*) argv[optind-2]);
				break;
			}

			case 'd': //default
			{
				if(verbose)
					print_message('o', argv, optind);
				if(profile)
    				start_timer(RUSAGE_SELF, startTime);
				signal(atoi(optarg), SIG_DFL);
				if(profile)
   					end_timer(RUSAGE_SELF, startTime, (char*) argv[optind-2]);
				break;
			}

			case 'e': //pause
			{
				if(verbose)
					print_message('f', argv, optind);
				if(profile)
    				start_timer(RUSAGE_SELF, startTime);
				pause();
				if(profile)
   					end_timer(RUSAGE_SELF, startTime, (char*) argv[optind-1]);
				break;
			}

			default:
				break;
		}

		if(filearr_size == filepos) //check for max size of file descriptor array
		{
			filearr_size += 10;
			filelist = realloc(filelist, filearr_size*sizeof(int));
		}

		if(pro_size == pro_pos) //check for max size of process array
		{
			pro_size += 10;
			pro_arr = realloc(pro_arr, pro_size*sizeof(int));
		}

		for(int l = optind; l < argc; l ++) //increment index of optind to next '--'
		{
			if(argv[optind][0] == '-' && argv[optind][1] == '-')
				break;
			optind++;
		}
	}

	if(signum != 0)
	{
		signal(signum, SIG_DFL);
		raise(signum);
	}

	exit(exitnum);
}