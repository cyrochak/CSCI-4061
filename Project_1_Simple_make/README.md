# CSCI 4061: Simple Make

CSci4061 F2018 Assignment 1
login: chanx393 (login used to submit)
date: 10/04/18
name: Kin Nathan Chan  , Cyro Chun Fai Chak, Isaac Aruldas (for partner(s))
id:   5330106- chanx393, 5312343 - chakx011, 5139488 - aruld002

The Simple Make is an utility program what will build executable libraries or
program such as .o files or libraries based on the Makefile in the directory.
The simple Makefile will administrate the similar functionality of the GNU make,
such as generating the executable files or clean all the executable libraries
etc. Once the simple makefile is generated, it can be used in any program files
that can replace the functionality of GNU make.


### Individual Contribution
Cyro: Initial draft of the SimpleMake, BugFix, Phase 1 coding, README
Isaac: Intermediate draft of the SimpleMake, BugFix, Phase 1 coding, README
Nathan: Final draft of the SimpleMake, Bug Fix, Phase 1 coding, README


### Getting Started
1. _cp_ the project to your personal repo
2. If `./make4061` is in the directory, ignore this step. At the top level
    inside the directory from OS Command Prompt: `make`
3. _cp_ the `./make4061` to your own project
4. At Command Prompt: `./make4061` or you can do `./make4061 <TargetName>`


## Code Explanation
### class rebuilt
  The rebuilt function will take four parameters. The first parameter is the
  root name. If the user enters a specific target name that he/she wants to
  make. The root name will be stored the target that the user entered. If the
  user didn't specific any target name, the root name will be default as
  `targets[0].TargetName`. The second input is the target name which represents
  the name of current recursion. The third and fourth inputs are the targets
  structure and the target count respectively that are imported by the default
  function. What rebuilt function does is to process two main things. First, it
  will find the target index by the name. If the function is able to find it, we
  will go next and check if the target has dependencies. If yes, we will pass
  the dependency name to `rebuilt`, and keep iterate until we hit the end of the
  tree, and the method we use is as known as DFS.

  If the function can't find the target index, it will return -1. Next,
  we will check that if the target is existed in the directory by calling
  `does_file_exist` function. If the target doesn't exist in the directory , an
  error massage will be printed and exited the program. After performing the
  DFS and once it hits the bottom of the tree, we will check if the dependency
  count is 0, and if yes, we know it is a target without any dependency,
  then we will pass the target index and the target structure towards the
  forking function. If we find out its dependency count is not 0, we will check
  the modification time for the target and every single dependency that
  target has by calling `compare_modification_time` function.

  Then, we will use  switch statement according to the result of modification
  time. If the function returns -1, we will check if the dependency name can be
  found in the target structure as well as in the directory as a file, and if
  both return -1, we will print out an error message, and stop the program. If
  the dependency name can be found in the directory as a file, we will check if
  the target status is 3 which means the target is executed. If it does not,
  we will pass the target to forking function. If the
  `compare_modification_time` function returns 1, then we will check if it's
  status is 3. If not, it will pass it to forking function. If the
  `compare_modification_time` function returns 0 or 2, we will print out a
  up-to-date message.

### class Forking
 The forking function will take two parameters. The first one is the target
 index of targets that we want to execute. The second parameter is the whole
 target structure that is imported by the default. Then, the forking function
 creates the parent process and the child process by calling fork(). Next, the
 processes do different work based on their id. The child process will get the
 command of the target by using the TargetIndex, and execute the command by
 calling execvp(). The parent process if child(id) > 0 will wait for it's child
 process to be terminated, and once the child process is successfully terminated
 which means it executed the command successfully, then the parent process will
 mark the target's status to be 3 to let other processes know that this target
 is done, and don't need to fork again.

### class show_targets
The show_targets function will take two inputs. The first input is the
targets structure that is imported by the default function. The second input
is the total target count in the targets structure. First, the show_targets
function will simply loop through the whole structure(array), and get every
target name, dependency count, dependency name, and the command. Finally, it
will print all the information out.
