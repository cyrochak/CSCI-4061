/* CSci4061 F2018 Assignment 1
* login: chanx393 (login used to submit)
* date: 10/04/18
* name: Kin Nathan Chan, Cyro Chun Fai Chak, Isaac Aruldas (for partner(s))
* id: 5330106- chanx393, 5312343 - chakx011, 5139488 - aruld002 */

// This is the main file for the code
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "util.h"

/*-------------------------------------------------------HELPER FUNCTIONS PROTOTYPES---------------------------------*/
void show_error_message(char * ExecName);
//Write your functions prototypes here
void show_targets(target_t targets[], int nTargetCount);

void rebuilt(char *root, char *TargetName, target_t targets[], int nTargetCount);
void do_clean(char *TargetName, target_t targets[], int nTargetCount);
void forking(int TargetIndex, target_t targets[]);
/*-------------------------------------------------------END OF HELPER FUNCTIONS PROTOTYPES--------------------------*/

/*-------------------------------------------------------HELPER FUNCTIONS--------------------------------------------*/

//This is the function for writing an error to the stream
//It prints the same message in all the cases
void show_error_message(char * ExecName)
{
	fprintf(stderr, "Usage: %s [options] [target] : only single target is allowed. ", ExecName);
	fprintf(stderr, "-f FILE\t\tRead FILE as a makefile. ");
	fprintf(stderr, "-h\t\tPrint this message and exit. ");
	exit(0);
}

//Write your functions here
/*----------------------------------forking function--------------------------*/
/*The forking function will take two inputs. The first input is the target index
 of targets that we want to execute. The second input is the whole target
 structure. Then, the forking function creates the parent process and the child
 process by calling fork(). The child process will execute the command of
 the target by using the TargetIndex and execvp(). The parent process with
 id > 0 will wait for it's child process to be terminated, and once the child
 process is successfully terminated which means it executed the command
 successfully, then the parent process will mark the target's status to be 3.
 Please see README for details.
*/
void forking(int TargetIndex, target_t targets[])
{
	int wstatus;
	char **argv;
	char *newstring[1023];
	//strcpy(newstring, targets[TargetIndex].Command);

	pid_t childpid;

	childpid = fork();
	if (childpid == 0)
	{
		printf("%s\n", targets[TargetIndex].Command);
		int count = parse_into_tokens(targets[TargetIndex].Command, newstring, " ");

		execvp(newstring[0], newstring);
		perror("execvp is wrong!");
		exit(3);
	}
	else if (childpid > 0)
	{
		wait(&wstatus);
		targets[TargetIndex].Status = 3; // 3 means the target's command is executed
		if (WEXITSTATUS(wstatus) != 0)
		{
			printf("child exited with error code=%d\n", WEXITSTATUS(wstatus));
			exit(-1);
		}
	}
	else
	{
		perror("Fork Failed\n");
	}
}

/*----------------------------------rebuilt function--------------------------*/
/*The rebuilt function will take four inputs. The first input is the root name
  which is either a specific target name or a default targets[0].TargetName.
	The second input is target name which means the name of current recursion. The
	third and fourth inputs are the targets structure and the target count.
	The function would checks the dependency of every target, then if the target
	fulfils the conditions, the function will pass it to forking function,
	otherwise, print a error massage. Please see README for details.
*/
void rebuilt(char *root, char *TargetName, target_t targets[], int nTargetCount)
{
	int TargetIndex = find_target(TargetName, targets, nTargetCount);
	if (TargetIndex!= -1)
	{
		for (int i =0; i< targets[TargetIndex].DependencyCount; i++)
		{
			char* name = targets[TargetIndex].DependencyNames[i];
			rebuilt(root, name, targets, nTargetCount);
		}
		if(targets[TargetIndex].DependencyCount != 0)
		{
			for (int t = 0; t < targets[TargetIndex].DependencyCount; t++)
			{
				int check =
					compare_modification_time(targets[TargetIndex].DependencyNames[t],
					targets[TargetIndex].TargetName);

				switch(check)
				{
					case -1 :
					{
						int depend_name_index =
							find_target(targets[TargetIndex].DependencyNames[t], targets,
								nTargetCount);

						if(depend_name_index == -1)
						{
							if(does_file_exist(targets[TargetIndex].DependencyNames[t])==-1)
							{
								char error_massage[1024];
								sprintf(error_massage,
									"./make4061: *** No rule to make target '%s'.  Stop.",
										targets[TargetIndex].DependencyNames[t]);
								printf("%s\n", error_massage);
								exit(0);
							}
						}

					}
					//Fall through
					case 1 :
					{
						if(targets[TargetIndex].Status!= 3)
						{
							forking(TargetIndex, targets);
						}
						break;
					}

					case 0:
					case 2:
					{
						if(strcmp(TargetName, root)==0 && targets[TargetIndex].Status != 3)
						{
							if((targets[TargetIndex].DependencyCount-1) ==t)
							{
								char up_to_date[1024];
								sprintf(up_to_date, "make4061: '%s' is up to date.",root );
								printf("%s\n", up_to_date);
								exit(0);
							}

						}
						else
						{

						}
					}
				}
			}
		}
		else
		{
			if(targets[TargetIndex].Status!= 3)
			{
				forking(TargetIndex, targets);
			}
		}
	}
	else if(TargetIndex == -1 && does_file_exist(TargetName) == -1 )
	{
		char error_massage[1024];
		sprintf(error_massage,
			"./make4061: *** No rule to make target '%s'.  Stop.", TargetName);
		printf("%s\n", error_massage);
		exit(0);
	}

}
// }
//Phase1: Warmup phase for parsing the structure here. Do it as per the PDF (Writeup)
/*----------------------------------rebuilt function--------------------------*/
/*The show_targets function will take two inputs. The first input is the
	targets structure taht is imported by the default function. The second input
	is the total target count in the targets structure. First, the show_targets
	function will simply loop through the whole structure(array), and get every
	target name, dependency count, dependency name, and the command. Finally, it
	will print all the information out.
*/
void show_targets(target_t targets[], int nTargetCount)
{
	//Write your warmup code here
	int i, row;
	char DepNames[ARG_MAX];

	for(i = 0; i < nTargetCount; i ++)
	{
		printf("TargetName: %s\nDependencyCount: %d\n", targets[i].TargetName,
			targets[i].DependencyCount);
		DepNames[0] = 0;
		for(row = 0; row < targets[i].DependencyCount;row++)
		{
			if(DepNames[0] == '\0')
			{
				sprintf(DepNames,"%s", targets[i].DependencyNames[row]);
			}
			else if(targets[i+1].DependencyNames[row] == NULL)
			{
				sprintf(DepNames,"%s", targets[i].DependencyNames[row]);
			}
			else
			{
				strcat(DepNames, ", ");
				strcat(DepNames, targets[i].DependencyNames[row]);
			}
		}
		printf("DependencyNames: %s\nCommand: %s\n",DepNames,
			targets[i].Command);
	}
}

/*-------------------------------------------------------END OF HELPER FUNCTIONS-------------------------------------*/


/*-------------------------------------------------------MAIN PROGRAM------------------------------------------------*/
//Main commencement
int main(int argc, char *argv[])
{
  target_t targets[MAX_NODES];
  int nTargetCount = 0;

  /* Variables you'll want to use */
  char Makefile[64] = "Makefile";
  char TargetName[64];

  /* Declarations for getopt */
  extern int optind;
  extern char * optarg;
  int ch;
  char *format = "f:h";
  char *temp;

  //Getopt function is used to access the command line arguments. However there can be arguments which may or may not need the parameters after the command
  //Example -f <filename> needs a finename, and therefore we need to give a colon after that sort of argument
  //Ex. f: for h there won't be any argument hence we are not going to do the same for h, hence "f:h"
  while((ch = getopt(argc, argv, format)) != -1)
  {
	  switch(ch)
	  {
	  	  case 'f':
	  		  temp = strdup(optarg);
	  		  strcpy(Makefile, temp);  // here the strdup returns a string and that is later copied using the strcpy
	  		  free(temp);	//need to manually free the pointer
	  		  break;

	  	  case 'h':
	  	  default:
	  		  show_error_message(argv[0]);
	  		  exit(1);
	  }

  }

  argc -= optind;
  if(argc > 1)   //Means that we are giving more than 1 target which is not accepted
  {
	  show_error_message(argv[0]);
	  return -1;   //This line is not needed
  }

  /* Init Targets */
  memset(targets, 0, sizeof(targets));   //initialize all the nodes first, just to avoid the valgrind checks

  /* Parse graph file or die, This is the main function to perform the toplogical sort and hence populate the structure */
  if((nTargetCount = parse(Makefile, targets)) == -1)  //here the parser returns the starting address of the array of the structure. Here we gave the makefile and then it just does the parsing of the makefile and then it has created array of the nodes
	  return -1;


  //Phase1: Warmup-----------------------------------------------------------------------------------------------------
  //Parse the structure elements and print them as mentioned in the Project Writeup
  /* Comment out the following line before Phase2 */

  // show_targets(targets, nTargetCount);

  //End of Warmup------------------------------------------------------------------------------------------------------

  /*
   * Set Targetname
   * If target is not set, set it to default (first target from makefile)
   */
  if(argc == 1)
	strcpy(TargetName, argv[optind]);    // here we have the given target, this acts as a method to begin the building
  else
  	strcpy(TargetName, targets[0].TargetName);  // default part is the first target

  /*
   * Now, the file has been parsed and the targets have been named.
   * You'll now want to check all dependencies (whether they are
   * available targets or files) and then execute the target that
   * was specified on the command line, along with their dependencies,
   * etc. Else if no target is mentioned then build the first target
   * found in Makefile.
   */

  //Phase2: Begins ----------------------------------------------------------------------------------------------------
  /*Your code begins here*/
	// for (int i =0; i< nTargetCount; i++){
	// 	targets[i].Status = 2;
	// }

	// printf("%s\n", TargetName );
	char root[64];
	strcpy(root, TargetName);
	// printf("%s ", root );
  rebuilt(root, TargetName, targets, nTargetCount);


  /*End of your code*/
  //End of Phase2------------------------------------------------------------------------------------------------------

  return 0;
}
/*-------------------------------------------------------END OF MAIN PROGRAM------------------------------------------*/
