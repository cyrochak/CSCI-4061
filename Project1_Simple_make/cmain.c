/* CSci4061 F2018 Assignment 1
* login: cselabs login name (login used to submit)
* date: mm/dd/yy
* name: full name1, full name2, full name3 (for partner(s))
* id: id for first name, id for second name, id for third name */

// This is the main file for the code
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
//#include  "util.c"
#include "util.h"

/*-------------------------------------------------------HELPER FUNCTIONS PROTOTYPES---------------------------------*/
void show_error_message(char * ExecName);
//Write your functions prototypes here
void show_targets(target_t targets[], int nTargetCount);
/*-------------------------------------------------------END OF HELPER FUNCTIONS PROTOTYPES--------------------------*/


/*-------------------------------------------------------HELPER FUNCTIONS--------------------------------------------*/

//This is the function for writing an error to the stream
//It prints the same message in all the cases
void show_error_message(char * ExecName)
{
	fprintf(stderr, "Usage: %s [options] [target] : only single target is allowed.\n", ExecName);
	fprintf(stderr, "-f FILE\t\tRead FILE as a makefile.\n");
	fprintf(stderr, "-h\t\tPrint this message and exit.\n");
	exit(0);
}

//Write your functions here
void rebuilt(char *TargetName, target_t targets[], int nTargetCount)
{
	int TargetIndex = find_target(TargetName, targets, nTargetCount);
		printf(" The Target Name is:%s\n", TargetName );
	if (TargetIndex!= -1){

		for (int i =0; i< targets[TargetIndex].DependencyCount; i++)
		{
			char* name = targets[TargetIndex].DependencyNames[i];
			rebuilt(name, targets, nTargetCount);
			//t = compare_modification_time(TargetName,name);
			// printf(" The Target Name is:%s\n", TargetName );

		}
		for (int t = 0; t < targets[TargetIndex].DependencyCount; t++)
		{
			int check = compare_modification_time(targets[TargetIndex].DependencyNames[t], targets[TargetIndex].TargetName);

			if(check==-1 || check==1)
			{
				int wstatus;
				char **argv;
				char *newstring[1023];
				//strcpy(newstring, targets[TargetIndex].Command);

				pid_t childpid;

				childpid = fork();
				if (childpid == 0) {
					printf("We got in the fork!\n");
					printf("Command is: %s\n", targets[TargetIndex].Command);
					if(targets[TargetIndex].Command == 0){
						printf("The Command is: %s\n", targets[TargetIndex].Command);
						perror("Compile failed!");
						exit(-1);
					}
					int count = parse_into_tokens(targets[TargetIndex].Command, newstring, " ");

					execvp(newstring[0], newstring);
					perror("execvp");
					//printf("something is wrong\n");

					exit(3);

				}
				else if (childpid > 0) {
					wait(&wstatus);
					if (WEXITSTATUS(wstatus) != 0)
					{
						printf("child exited with error code=%d\n", WEXITSTATUS(wstatus));
						exit(-1);
					}
				}
				else {
					perror("Fork Failed\n");
				}
			}
		}

}}
//Phase1: Warmup phase for parsing the structure here. Do it as per the PDF (Writeup)
void show_targets(target_t targets[], int nTargetCount)
{
	//Write your warmup code here
	int index, index_DependencyNames;

	char DependencyName[ARG_MAX];

	for(index = 0; index < nTargetCount; index ++)
	{
		printf("TargetName: %s\nDependencyCount: %d\n", targets[index].TargetName, targets[index].DependencyCount);
		DependencyName[0] = 0;
		for(index_DependencyNames = 0; index_DependencyNames < targets[index].DependencyCount;index_DependencyNames++)
		{
			if(DependencyName[0] == '\0')
			{
				sprintf(DependencyName,"%s", targets[index].DependencyNames[index_DependencyNames]);
			}
			else if(targets[index+1].DependencyNames[index_DependencyNames] == NULL)
			{
				sprintf(DependencyName,"%s", targets[index].DependencyNames[index_DependencyNames]);
			}
			else
			{
				strcat(DependencyName, ", ");
				strcat(DependencyName, targets[index].DependencyNames[index_DependencyNames]);
			}
		}
		printf("DependencyNames: %s\nCommand: %s\n",DependencyName, targets[index].Command);
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
  //show_targets(targets, nTargetCount);
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
	for (int i =0; i< nTargetCount; i++){
		targets[i].Status = 2;
	}
  rebuilt(TargetName, targets, nTargetCount);


  /*End of your code*/
  //End of Phase2------------------------------------------------------------------------------------------------------

  return 0;
}
/*-------------------------------------------------------END OF MAIN PROGRAM------------------------------------------*/
