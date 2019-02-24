# CSCI 4061: Multi-ProcessChatApp

CSci4061 F2018 Assignment 2
login: chanx393 (login used to submit)
date: 11/10/18
name: Kin Nathan Chan, Cyro Chun Fai Chak, Isaac Aruldas (for partner(s))
id: 5330106- chanx393, 5312343 - chakx011, 5139488 - aruld002

In this project, we will create a multi-process chat application. There exists
a main parent process called the `server` process, and several `child` processes
which is up to a maximum of 10 user. This chat application will demostrate a
simple version of chat application, that allow client to another client publicly
or privately with a p2p function. Beside basic chat function, we have also assign
the server to have some functionality such as kick user, close server, list out
all the user and announcement.

### Individual Contribution
Cyro: Initial draft of the application, BugFix, Phase 1 coding, README
Isaac: Initial draft of the application, BugFix, Phase 1 coding, README
Nathan: Final draft of the application, Bug Fix, Phase 1 coding, README

### Getting Started
1. _cp_ the project to your personal repo
2. If `client` and `server` is in the directory, ignore this step. At the top level
    inside the directory from OS Command Prompt: `make`
3. At Command Prompt: `./server` to first create a server process, and then
   `./client <ClientName>` to create a User process.
4. After creating the first client process, you keep creating client processes with
    different client names up to 10 clients.

### Started Using
1. After create a server process and one to ten client process(s), you can do the
  following:
  A) you can enter a message in the client shell, then the message will be sent to
    other client(s).
  B) you can enter one of the commands each time :`\list`, `\p2p <ClientName> <Message>`and `\exit` to do particular work. For example, if you use `\list` command in the
  client shell, it returns a list of users that are connected to the server process.
  C) you can enter a message in the server shell, then the message will be sent to
    all client(s) as admin.
  D) you can enter one of the commands each time :`\list`, `\p2p <ClientName> <Message>`, `\exit`, and `kick <username>` to do particular work. For example, if you use `\exit` command in the server shell, it closes all the pipe, kills all the child process(s) and
  client process(s), and exit.

## Code Explanation
### Server: Main cleanup_users
First, we set up all the pipe,  between server and child, and between client to
the child. We will next check all the pipe connection
whether they are connected or not. If it is connected, we will start to check
the user id name for the user. If the user id is taken, the server will tell the client
that the id name is taken, and we will close all the pipe for reading and writing.
If there is not the same name in the server, we will open the pipe for the child to
write from the server, and the pipe for the server to read from the child. Then, we
will fork the child process. If it is in the child process, we will poll users
and the server. By doing that, we first close the pipes for the users. Then we
create an iteration to check if the message that sends from the client will be out of
memory. If it does not, we set a `\0` at the end of the message, and send it to the
pipe that will send to the user process. If there is no message need to be sent,
the server will close all the pipe and exit the child process.

If it is a parent process, which means that there will be new user information
that need to add into the empty slot. First, we will reset the pipe status, just
in case some unexpected situation. Then we will check for employ spot in the user
list. If there is a spot, we will return a message that the user is connected in
slot, where x is an empty index in the list. If else, we will return a message with
the list is full.

Once we finished forking, we will process the commands from the server,
such as list, kill, exit etc. First, we will get our command from the
`get_command_type` function to see what command is needed to process. If the command
get 0, we will list the users using the `list_users` class, the class will print
the user list by first checking the list status, and it will print out by using a
for loop to print every user information by the list users class. If the commands
return 1, we will process the kick function. By doing this, we will parse the
`extract_name` function to extract the name by given the input buffer and user id,
then we find if the target name is in the user kick, if yes we will use the `kick_user`
function to delete the user from the server. If the commands return 4, it will execute
the `cleanup_users` to kill all the users that is in the server, close all the
connection to the pipe and close the client and server process. If the commands
return 5, it will run an error handling to see if the message is empty.

The last process for server process is executing the commands that is getting
from clients. First, we set up all the buffer to read from client to service as
well as the content. Then it will run through a loop for all the user. We check
for error handling whether the slot of the user list is filled, if yes, we will
receive the command from the client. The client can do a few commands, such as
list of the users, p2p message and exit the chat. When we got a 0 from the command,
we will list user like the server, however, we will use the user index as one of
parameter to parse into the class instead of -1 to indicate as admin.

If the command receive 2, this indicate it is a p2p command, then we will extract
the target name from the buffer, and find which user does the client want to send
the message. After that, we will extract the content from the buffer that store the
message. If we fail to extract the text from content and find the user index, we
will return an error message that it fail to send the message. If it is not the
case, it will execute the `send_p2p_msg` method to write the message to the user.
If it fails, it will return the error that error of writing the p2p message.

The last command will be 4, it will exit the server. What this does is to kick
itself from the system, by using the same method to kill users as a server process.
If it fails to do so, it will print that fail to send the message to all user. This
iteration will be done and run it through all users that is existed in the user list.

### client class
In the client class, it first connects to the server that is with pipe connection
with the server, if it fails, it will exit the program. If it does not, it will
poll the pip and print the poll to sdiout, then we make sure the pipe to writing
to the server and reading from the server has a connection. Then we start the loop
for endless communication until it exit.

First, we set `usleep(1000)` to reduce CPU consumption after each iteration. Then
we check for the content that is write to users, and put `\0` at the end of each
line. If the length is larger 0, it will execute the `get_command_type` function
get commands 3, this means sending the message to other users. We also implement
an error handling to see if it fails to send the message. After it is done, we
will print the message.

Next, we will process the reading message part, if the size from reading from the
pipe is larger than 0, we will go next and check if the last character of the text
is `\n`. If it does not, we will give a next line symbol to the text. At Last, we
will check if the pipe or the server pipe does not contain any size of data, then
we return the error message and close the program.

### Assumption(s)
1. The message that is entered from both server and client shell will not be larger
  than 256(MAX_MSG) size.
2. When you just enter a empty (you only press the enter), it will not court as
  a message since there is no information.

### Error handling
1. If there is system error, such as, error of `close` system call, and error of
  `write` system call, etc, it will print out the error message, but it will not stop
  the program since it is a real-time program.
2. If a client process wants to connect to the server process with a name that is
  already registered in the USER_LIST, the server process will print out the error and
  the client will exit. Also, same thing will happen when the USER_LIST is full(10 users),
  and one new client process wants to connect to the server process.
