/*CSci4061 F2018 Assignment 2
* login: chanx393 (login used to submit)
* date: 11/10/18
* name: Kin Nathan Chan, Cyro Chun Fai Chak, Isaac Aruldas (for partner(s))
* id: 5330106- chanx393, 5312343 - chakx011, 5139488 - aruld002 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include "comm.h"
#include "util.h"
// #include <time.h>
int all_space(char s[]);
/* -------------------------Main function for the client ----------------------*/
void main(int argc, char * argv[]) {

	int pipe_user_reading_from_server[2], pipe_user_writing_to_server[2];

	// You will need to get user name as a parameter, argv[1].

	if(connect_to_server("5330106", argv[1], pipe_user_reading_from_server, pipe_user_writing_to_server) == -1) {
		exit(-1);
	}

	/* -------------- YOUR CODE STARTS HERE -----------------------------------*/
	// poll pipe retrieved and print it to sdiout
	if(close(pipe_user_reading_from_server[1])==-1)
	{
		printf("Failed to Close fd\n");
	}
	if(close(pipe_user_writing_to_server[0])==-1)
	{
		printf("Failed to Close fd\n");
	}
	printf("pipe to read: %d\n", pipe_user_reading_from_server[0]);
	printf("pipe to write: %d\n", pipe_user_writing_to_server[1]);
	if(fcntl(0, F_SETFL, fcntl(0, F_GETFL)| O_NONBLOCK)==-1)
  {
    printf("Failed to set the fd flag!\n");
  }
	if(fcntl(pipe_user_writing_to_server[1], F_SETFL, fcntl(pipe_user_writing_to_server[1], F_GETFL)| O_NONBLOCK)==-1)
  {
    printf("Failed to set the fd flag!\n");
  }
	if(fcntl(pipe_user_reading_from_server[0], F_SETFL, fcntl(pipe_user_reading_from_server[0], F_GETFL)| O_NONBLOCK)==-1)
  {
    printf("Failed to set the fd flag!\n");
  }
	print_prompt(argv[1]);
	char buf_write[1024];
	char buf_read[1024];
	memset(buf_write,'\0',1024);
	memset(buf_read,'\0',1024);
	ssize_t read_size_STDIN;
	ssize_t read_size_PIPE;
	while(1)
	{
		// Poll stdin (input from the terminal) and send it to server (child process) via pipe
		usleep(1000);
		// nanosleep(0,999999998);
		read_size_STDIN= read(0, buf_write, 1024);
		if(read_size_STDIN> 0 && all_space(buf_write) == 1)
		{
			buf_write[strlen(buf_write)-1] = '\0';
			if(strlen(buf_write) > 0)
			{
				char *n =NULL;
				enum command_type SERVER_Comm = get_command_type(buf_write);
        switch (SERVER_Comm) {
					case 3:
					*n = 1;
					break;
					case 4:
					printf("Exiting Now.\n");
					if(close(pipe_user_writing_to_server[1])==-1)
					{
						printf("Failed to Close fd\n");
					}
					if(close(pipe_user_reading_from_server[0])==-1)
					{
						printf("Failed to Close fd\n");
					}
					exit(0);
					break;
					default:
					if(write(pipe_user_writing_to_server[1], buf_write, read_size_STDIN)==-1)
					{
						printf("failed to send message to pipe\n");
					}
				}
				memset(buf_write,'\0',1024);
			}
			print_prompt(argv[1]);
		}
		else if(read_size_STDIN> 0 && all_space(buf_write) == 0)
		{
			if(write(pipe_user_writing_to_server[1], buf_write, read_size_STDIN)==-1)
			{
				printf("failed to send message to pipe\n");
			}
			print_prompt(argv[1]);
		}
		else if(read_size_STDIN> 0 && all_space(buf_write) == 2)
		{
			print_prompt(argv[1]);
		}
		read_size_PIPE = read(pipe_user_reading_from_server[0], buf_read, 1024);
		if(read_size_PIPE> 0)
		{
			if(buf_read[strlen(buf_read)-1]=='\n')
			{
				fprintf(stdout, "\n%s", buf_read );
				print_prompt(argv[1]);
			}else {
				fprintf(stdout, "\n%s\n", buf_read );
				print_prompt(argv[1]);
			}
			memset(buf_read,'\0',1024);
		}
		if(read_size_STDIN ==0 || read_size_PIPE ==0 )
		{
			printf("The server process seems dead\n");
			if(close(pipe_user_writing_to_server[1])==-1)
			{
				printf("Failed to Close fd\n");
			}
			if(close(pipe_user_reading_from_server[0])==-1)
			{
				printf("Failed to Close fd\n");
			}
			exit(0);
		}
	}
	/* -------------- YOUR CODE ENDS HERE -----------------------------------*/
}

/*--------------------------End of main for the client --------------------------*/

/* -------------------------all_space function for the client ----------------------*/
int all_space(char s[]) {
	int len = strlen(s);
	if(s[0] !='\n')
	{
		for(int i=0; i<len; i++)
		{
			if(s[i] != '\n')
			{
				if(s[i] != ' ')
				{
					return 1; //  means there is something in the string
				}
			}
			else
			{
				return 0;
			}
		}
	}
	else {
		return 2;
	}
}
/*--------------------------End of all_space for the client --------------------------*/
