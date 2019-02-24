/*CSci4061 F2018 Assignment 2
* login: chanx393 (login used to submit)
* date: 11/10/18
* name: Kin Nathan Chan, Cyro Chun Fai Chak, Isaac Aruldas (for partner(s))
* id: 5330106- chanx393, 5312343 - chakx011, 5139488 - aruld002 */
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "comm.h"
#include "util.h"
// #include <time.h>

/* -----------Functions that implement server functionality -------------------------*/
int all_space(char s[]);
/*
* Returns the empty slot on success, or -1 on failure
*/
int find_empty_slot(USER * user_list) {
  // iterate through the user_list and check m_status to see if any slot is EMPTY
  // return the index of the empty slot
  int i = 0;
  for(i=0;i<MAX_USER;i++) {
    if(user_list[i].m_status == SLOT_EMPTY) {
      return i;
    }
  }
  return -1;
}

/*
* list the existing users on the server shell
*/
int list_users(int idx, USER * user_list)
{
  // iterate through the user list
  // if you find any slot which is not empty, print that m_user_id
  // if every slot is empty, print "<no users>""
  // If the function is called by the server (that is, idx is -1), then printf the list
  // If the function is called by the user, then send the list to the user using write() and passing m_fd_to_user
  // return 0 on success
  int i, flag = 0;
  char buf[1024] = {}, *s = NULL;

  /* construct a list of user names */
  s = buf;
  strncpy(s, "---connecetd user list---\n", strlen("---connecetd user list---\n"));
  s += strlen("---connecetd user list---\n");
  for (i = 0; i < MAX_USER; i++) {
    if (user_list[i].m_status == SLOT_EMPTY)
    continue;
    flag = 1;
    strncpy(s, user_list[i].m_user_id, strlen(user_list[i].m_user_id));
    s = s + strlen(user_list[i].m_user_id);
    strncpy(s, "\n", 1);
    s++;
  }
  if (flag == 0) {
    strcpy(buf, "<no users>\n");
  } else {
    s--;
    strncpy(s, "\n", 1);
    // strncpy(s, "\0", 1);
  }

  if(idx < 0) {
    printf("%s",buf);
    printf("\n");
  } else {
    /* write to the given pipe fd */
    if (write(user_list[idx].m_fd_to_user, buf, strlen(buf) + 1) < 0)
    printf("writing to server shell");
  }

  return 0;
}

/*
* add a new user
*/
int add_user(int idx, USER * user_list, int pid, char * user_id, int pipe_to_child, int pipe_to_parent)
{
  // populate the user_list structure with the arguments passed to this function
  user_list[idx].m_pid = pid;
  strcpy(user_list[idx].m_user_id, user_id);
  user_list[idx].m_fd_to_user = pipe_to_child;
  user_list[idx].m_fd_to_server = pipe_to_parent;
  user_list[idx].m_status = SLOT_FULL;
  return idx; // return the index of user added
}

/*
* Kill a user
*/
void kill_user(int idx, USER * user_list) {
  if(kill(user_list[idx].m_pid, SIGKILL)==-1) // kill a user (specified by idx) by using the systemcall kill()
  {
    printf("Failed to send a signal to the child with pid number: %d\n", user_list[idx].m_pid);
  }
  int status;
  if(waitpid(user_list[idx].m_pid, &status, 0)==-1) // then call waitpid on the user
  {
    printf("Failed to wait the child with pid number: %d\n", user_list[idx].m_pid);
  }
}

/*
* Perform cleanup actions after the used has been killed
*/
void cleanup_user(int idx, USER * user_list)
{
  printf("User :%s disconnected.\n", user_list[idx].m_user_id);
  user_list[idx].m_pid = -1; // m_pid should be set back to -1
  memset(user_list[idx].m_user_id, '\0', MAX_USER_ID); // m_user_id should be set to zero, using memset()
  if(fcntl(user_list[idx].m_fd_to_user, F_GETFD) != -1)
  {
    if(close(user_list[idx].m_fd_to_user)==-1) // close all the fd
    {
      printf("Failed to Close fd\n");
    }
  }
  if(fcntl(user_list[idx].m_fd_to_server, F_GETFD) != -1)
  {
    if(close(user_list[idx].m_fd_to_server)==-1) // close all the fd
    {
      printf("Failed to Close fd\n");
    }
  }
  user_list[idx].m_fd_to_user = -1; // set the value of all fd back to -1
  user_list[idx].m_fd_to_server = -1; // set the value of all fd back to -1
  user_list[idx].m_status = SLOT_EMPTY; // set the status back to empty
}

/*
* Kills the user and performs cleanup
*/
void kick_user(int idx, USER * user_list) {
  kill_user(idx, user_list); // should kill_user()
  cleanup_user(idx, user_list); // then perform cleanup_user()
}

/*
* broadcast message to all users
*/
int broadcast_msg(USER * user_list, char *buf, char * sender)
{
  if(sender == NULL)
  {
    char temp[1024];
    sprintf(temp, "admin-notice:%s", buf);
    for(int i=0; i < MAX_USER ; i++) //iterate over the user_list and if a slot is full, and the user is not the sender itself,
    {
      if(user_list[i].m_status != SLOT_EMPTY) {
        if(write(user_list[i].m_fd_to_user, temp, strlen(temp)) ==-1) //then send the message to that user
        {
          char error_message[1024];
          sprintf(error_message, "Error on Sending the Message to %s user with the fd %d\n", user_list[i].m_user_id, user_list[i].m_fd_to_user);
          printf("%s", error_message);
          return -1;
        }
      }
    }
    memset(temp, '\0', 1024);
  }
  else {
    char temp[1024];
    sprintf(temp, "%s:%s", sender, buf);
    // printf("IN THE wrtie part\n" );
    for(int i=0; i < MAX_USER; i++) //iterate over the user_list and if a slot is full, and the user is not the sender itself,
    {
      if(user_list[i].m_status != SLOT_EMPTY) {
        if(strcmp(user_list[i].m_user_id, sender) != 0)
        {
          if(write(user_list[i].m_fd_to_user, temp, strlen(temp)) ==-1) //then send the message to that user
          {
            char error_message[1024];
            sprintf(error_message, "Error on Sending the Message to %s user with the fd %d\n", user_list[i].m_user_id, user_list[i].m_fd_to_user);
            printf("%s", error_message);
            return -1;
          }
        }
      }
    }
    memset(temp, '\0', 1024);
  }

  return 0; //return zero on success
}

/*
* Cleanup user chat boxes
*/
void cleanup_users(USER * user_list)
{
  // go over the user list and check for any empty slots
  int i;
  // int count = 0;
  for(i=0; i < MAX_USER ; i++)
  {
    if(user_list[i].m_status != SLOT_EMPTY)
    {
      int temp_pid = user_list[i].m_pid;
      int status;
      kick_user(i, user_list);
    }
  }
}

/*
* find user index for given user name
*/
int find_user_index(USER * user_list, char * user_id)
{
  // go over the  user list to return the index of the user which matches the argument user_id
  // return -1 if not found
  int i, user_idx = -1;

  if (user_id == NULL) {
    fprintf(stderr, "NULL name passed.\n");
    return user_idx;
  }
  for (i=0;i<MAX_USER;i++) {
    if (user_list[i].m_status == SLOT_EMPTY)
    continue;
    if (strcmp(user_list[i].m_user_id, user_id) == 0) {
      return i;
    }
  }

  return -1;
}

/*
* given a command's input buffer, extract name
*/
int extract_name(char * buf, char * user_name)
{
  char inbuf[1024];
  char * tokens[16];
  strcpy(inbuf, buf);

  int token_cnt = parse_line(inbuf, tokens, " ");

  if(token_cnt >= 2) {
    strcpy(user_name, tokens[1]);
    return 0;
  }

  return -1;
}

int extract_text(char *buf, char * text)
{
  char inbuf[1024];
  char * tokens[16];
  char * s = NULL;
  strcpy(inbuf, buf);

  int token_cnt = parse_line(inbuf, tokens, " ");

  if(token_cnt >= 3) {
    //Find " "
    s = strchr(buf, ' ');
    s = strchr(s+1, ' ');

    strcpy(text, s+1);
    return 0;
  }

  return -1;
}

/*
* send personal message
*/
void send_p2p_msg(int idx, USER * user_list, char *buf)
{

  // get the target user by name using extract_name() function --done
  // find the user id using find_user_index() --done
  // if user not found, write back to the original user "User not found", using the write()function on pipes.
  if(write(user_list[idx].m_fd_to_user, buf, strlen(buf))== -1)
  {
    printf("Error of writing the p2p message.\n");
  }
  // if the user is found then write the message that the user wants to send to that user.

  // printf("send_p2p_msg-ing to %s with the txt:%s\n", user_list[idx].m_user_id, buf);
}

//takes in the filename of the file being executed, and prints an error message stating the commands and their usage
void show_error_message(char *filename)
{
}


/*
* Populates the user list initially
*/
void init_user_list(USER * user_list) {

  //iterate over the MAX_USER
  //memset() all m_user_id to zero
  //set all fd to -1
  //set the status to be EMPTY
  int i=0;
  for(i=0;i<MAX_USER;i++) {
    user_list[i].m_pid = -1;
    memset(user_list[i].m_user_id, '\0', MAX_USER_ID);
    user_list[i].m_fd_to_user = -1;
    user_list[i].m_fd_to_server = -1;
    user_list[i].m_status = SLOT_EMPTY;
  }
}

/* ---------------------End of the functions that implementServer functionality -----------------*/


/* ---------------------Start of the Main function ----------------------------------------------*/
int main(int argc, char * argv[])
{
  int nbytes;
  setup_connection("5330106"); // Specifies the connection point as argument.

  USER user_list[MAX_USER];
  init_user_list(user_list);   // Initialize user list

  char buf[1024];
  if(fcntl(0, F_SETFL, fcntl(0, F_GETFL)| O_NONBLOCK)==-1)
  {
    printf("Failed to set the fd flag!\n");
  }
  print_prompt("admin");

  //
  while(1) {
    /* ------------------------YOUR CODE FOR MAIN--------------------------------*/
    // usleep(999999);
    if(usleep(1000)==-1)
    {
      printf("Failed to sleep the main process by using usleep\n");
    }
    // nanosleep(0,999999998);
    // Handling a new connection using get_connection
    // fprintf(stderr,"Entered while(1)\n" );
    int pipe_SERVER_reading_from_child[2];
    int pipe_SERVER_writing_to_child[2];
    char user_id[MAX_USER_ID];
    int pipe_child_writing_to_user[2];
    int pipe_child_reading_from_user[2];
    int error = get_connection(user_id, pipe_child_writing_to_user, pipe_child_reading_from_user);
    if(error != -1)
    {
      if(find_user_index(user_list, user_id)!=-1)
      {
        // User id: already taken.
        char error_message[1024];
        sprintf(error_message, "\nUser id: %s already taken.\n", user_id);
        printf("%s", error_message);
        if(close(pipe_child_writing_to_user[0])==-1)
        {
          printf("Failed to Close fd\n");
        }
        if(close(pipe_child_writing_to_user[1])==-1)
        {
          printf("Failed to Close fd\n");
        }
        if(close(pipe_child_reading_from_user[0])==-1)
        {
          printf("Failed to Close fd\n");
        }
        if(close(pipe_child_reading_from_user[1])==-1)
        {
          printf("Failed to Close fd\n");
        }
        print_prompt("admin");
      }
      else
      {
        int error_pipe;
        int error_pipe1;
        if((error_pipe=pipe(pipe_SERVER_reading_from_child))==-1)
        {
          printf("Failed to create a pipe for pipe_SERVER_reading_from_child for the new user: %s\n", user_id);
        }
        if((error_pipe1=pipe(pipe_SERVER_writing_to_child))==-1)
        {
          printf("Failed to create a pipe for pipe_SERVER_writing_to_child for the new user: %s\n", user_id);
        }
        if(error_pipe!=-1 && error_pipe1!=-1)
        {
          pid_t childpid;
          childpid = fork();
          if(childpid == 0) // Child process: poli users and SERVER
          {
            if(close(pipe_SERVER_reading_from_child[0])==-1)
            {
              printf("Failed to Close fd\n");
            }
            if(close(pipe_SERVER_writing_to_child[1])==-1)
            {
              printf("Failed to Close fd\n");
            }
            ssize_t read_size_C2C;
            ssize_t read_size_S2C;
            char buf_read_C2C[1024];
            char buf_read_S2C[1024];
            memset(buf_read_C2C,'\0',1024);
            memset(buf_read_S2C,'\0',1024);
            if(close(pipe_child_writing_to_user[0])==-1)
            {
              printf("Failed to Close fd\n");
            }
            if(close(pipe_child_reading_from_user[1])==-1)
            {
              printf("Failed to Close fd\n");
            }
            if(fcntl(pipe_SERVER_reading_from_child[1], F_SETFL, fcntl(pipe_SERVER_reading_from_child[1], F_GETFL)| O_NONBLOCK)==-1)
            {
              printf("Failed to set the fd flag!\n");
            }
            if(fcntl(pipe_SERVER_writing_to_child[0], F_SETFL, fcntl(pipe_SERVER_writing_to_child[0], F_GETFL)| O_NONBLOCK)==-1)
            {
              printf("Failed to set the fd flag!\n");
            }
            if(fcntl(pipe_child_writing_to_user[1], F_SETFL, fcntl(pipe_child_writing_to_user[1], F_GETFL)| O_NONBLOCK)==-1)
            {
              printf("Failed to set the fd flag!\n");
            }
            if(fcntl(pipe_child_reading_from_user[0], F_SETFL, fcntl(pipe_child_reading_from_user[0], F_GETFL)| O_NONBLOCK)==-1)
            {
              printf("Failed to set the fd flag!\n");
            }
            while(1) {
              read_size_C2C= read(pipe_child_reading_from_user[0], buf_read_C2C, 1024);
              if(read_size_C2C >0)
              {
                if(write(pipe_SERVER_reading_from_child[1], buf_read_C2C, read_size_C2C) == -1)
                {
                  printf("failed to send message to pipe to SERVER process\n");
                }
                memset(buf_read_C2C,'\0',1024);
              }
              read_size_S2C = read(pipe_SERVER_writing_to_child[0], buf_read_S2C, 1024);
              if(read_size_S2C >0)
              {
                if(start_with(buf_read_S2C, "msg : ") == 0)
                {
                  printf("%s\n", buf_read_S2C);
                  char temp[1024];
                  memset(temp,'\0',1024);
                  strncpy(temp, buf_read_S2C + 6, strlen(buf_read_S2C) - 6);
                  temp[strlen(temp)-1] = '\0';
                  if(write(pipe_child_writing_to_user[1], temp, strlen(temp)) == -1)
                  {
                    printf("failed to send message to pipe to user process\n");
                  }
                  memset(buf_read_S2C,'\0',1024);
                  memset(temp,'\0',1024);
                }
                else {
                  if(write(pipe_child_writing_to_user[1], buf_read_S2C, read_size_S2C) == -1)
                  {
                    printf("failed to send message to pipe to user process\n");
                  }
                  memset(buf_read_S2C,'\0',1024);
                }
              }
              if(read_size_C2C == 0 || read_size_S2C == 0)
              {
                // printf("Closeing\n");
                if(close(pipe_SERVER_reading_from_child[1])==-1)
                {
                  printf("Failed to Close fd\n");
                }
                if(close(pipe_SERVER_writing_to_child[0])==-1)
                {
                  printf("Failed to Close fd\n");
                }
                if(close(pipe_child_writing_to_user[1])==-1)
                {
                  printf("Failed to Close fd\n");
                }
                if(close(pipe_child_reading_from_user[0])==-1)
                {
                  printf("Failed to Close fd\n");
                }
                // kill(getpid(), SIGKILL);
                // exit(0);
              }
            }
          }
          else if(childpid > 0)// Server process: Add a new user information into an empty slot
          {
            if(close(pipe_child_writing_to_user[0])==-1)
            {
              printf("Failed to Close fd\n");
            }
            if(close(pipe_child_writing_to_user[1])==-1)
            {
              printf("Failed to Close fd\n");
            }
            if(close(pipe_child_reading_from_user[0])==-1)
            {
              printf("Failed to Close fd\n");
            }
            if(close(pipe_child_reading_from_user[1])==-1)
            {
              printf("Failed to Close fd\n");
            }

            if(close(pipe_SERVER_reading_from_child[1])==-1)
            {
              printf("Failed to Close fd\n");
            }
            if(close(pipe_SERVER_writing_to_child[0])==-1)
            {
              printf("Failed to Close fd\n");
            }
            if(fcntl(pipe_SERVER_reading_from_child[0], F_SETFL, fcntl(pipe_SERVER_reading_from_child[0], F_GETFL)| O_NONBLOCK)==-1)
            {
              printf("Failed to set the fd flag!\n");
            }
            if(fcntl(pipe_SERVER_writing_to_child[1], F_SETFL, fcntl(pipe_SERVER_writing_to_child[1], F_GETFL)| O_NONBLOCK)==-1)
            {
              printf("Failed to set the fd flag!\n");
            }
            int empty_index = find_empty_slot(user_list);
            if(empty_index!=-1)
            {
              add_user(empty_index, user_list, childpid, user_id, pipe_SERVER_writing_to_child[1], pipe_SERVER_reading_from_child[0]);
              printf("\nA new user: %s connected. slot: %d\n", user_id, empty_index);
              print_prompt("admin");
            }
            else {
              char error_message[1024];
              sprintf(error_message, "The user_list is full for %s\n", user_id);
              printf("%s", error_message);
              if(close(pipe_SERVER_reading_from_child[0])==-1)
              {
                printf("Failed to Close fd\n");
              }
              if(close(pipe_SERVER_writing_to_child[1])==-1)
              {
                printf("Failed to Close fd\n");
              }
              print_prompt("admin");
            }
          }
          else
          {
            printf("Error with fork!\n");
            if(close(pipe_SERVER_reading_from_child[0])==-1)
            {
              printf("Failed to Close fd\n");
            }
            if(close(pipe_SERVER_reading_from_child[1])==-1)
            {
              printf("Failed to Close fd\n");
            }
            if(close(pipe_SERVER_writing_to_child[0])==-1)
            {
              printf("Failed to Close fd\n");
            }
            if(close(pipe_SERVER_writing_to_child[1])==-1)
            {
              printf("Failed to Close fd\n");
            }
            if(close(pipe_child_writing_to_user[0])==-1)
            {
              printf("Failed to Close fd\n");
            }
            if(close(pipe_child_writing_to_user[1])==-1)
            {
              printf("Failed to Close fd\n");
            }
            if(close(pipe_child_reading_from_user[0])==-1)
            {
              printf("Failed to Close fd\n");
            }
            if(close(pipe_child_reading_from_user[1])==-1)
            {
              printf("Failed to Close fd\n");
            }
          }
        }
        else
        {
          if(close(pipe_child_writing_to_user[0])==-1)
          {
            printf("Failed to Close fd\n");
          }
          if(close(pipe_child_writing_to_user[1])==-1)
          {
            printf("Failed to Close fd\n");
          }
          if(close(pipe_child_reading_from_user[0])==-1)
          {
            printf("Failed to Close fd\n");
          }
          if(close(pipe_child_reading_from_user[1])==-1)
          {
            printf("Failed to Close fd\n");
          }
        }
      }
    }
    ssize_t read_size_STDIN;
    char buf_read_02S[1024];
    char buf_content[1024];
    char user_name[MAX_USER_ID];
    memset(buf_read_02S,'\0',1024);
    memset(buf_content,'\0',1024);
    memset(user_name,'\0',MAX_USER_ID);
    read_size_STDIN= read(0, buf_read_02S, 1024);
    if(read_size_STDIN >0 && all_space(buf_read_02S) == 1)
    {
      buf_read_02S[strlen(buf_read_02S)-1] = '\0';
      if(strlen(buf_read_02S)>0)
      {
        enum command_type SERVER_Comm = get_command_type(buf_read_02S);
        switch (SERVER_Comm) {
          case 0:
          list_users(-1, user_list);
          break;
          case 1:
          if(extract_name(buf_read_02S, user_name) == 0)
          {
            int idx_kick = find_user_index(user_list, user_name);
            if(idx_kick != -1)
            {
              if(close(user_list[idx_kick].m_fd_to_user)==-1)
              {
                printf("Failed to Close fd\n");
              }
              if(close(user_list[idx_kick].m_fd_to_server)==-1)
              {
                printf("Failed to Close fd\n");
              }
              kick_user(idx_kick, user_list);
            }
            else {
              char error_message[1024];
              sprintf(error_message, "Can't find the ID to kick: %s\n", user_name);
              printf("%s", error_message);
            }
          }
          else {
            char error_message[1024];
            sprintf(error_message, "Can't extract the name from the command: %s\n", buf_read_02S);
            printf("%s", error_message);
          }
          break;
          case 4:
          cleanup_users(user_list);
          exit(0);
          break;
          default:
          if(broadcast_msg(user_list, buf_read_02S , NULL) == -1)
          {
            printf("Failed to send message to all users\n");
          }
        }
        memset(buf_read_02S,'\0',1024);

      }
      print_prompt("admin");
    }
    else if(read_size_STDIN >0 && all_space(buf_read_02S) == 0)
    {
      // printf("Please DON'T just send space to other users.\n");
      if(broadcast_msg(user_list, buf_read_02S , NULL) == -1)
      {
        printf("Failed to send message to all users\n");
      }
      print_prompt("admin");
    }
    else if(read_size_STDIN >0 && all_space(buf_read_02S) == 2)
    {
      print_prompt("admin");
    }


    char buf_read_C2S[1024];
    char buf_C2S_content[1024];
    char target_name[MAX_USER_ID];
    memset(buf_read_C2S,'\0',1024);
    memset(buf_C2S_content,'\0',1024);
    memset(target_name,'\0',MAX_USER_ID);
    for(int i = 0; i < MAX_USER ; i++)
    {
      if(user_list[i].m_status != SLOT_EMPTY)
      {
        ssize_t read_size_child= read(user_list[i].m_fd_to_server, buf_read_C2S, 1024);
        if(read_size_child>0)
        {
          enum command_type SERVER_Comm = get_command_type(buf_read_C2S);
          if(SERVER_Comm == 0) {
            list_users(i, user_list);
          }
          else if(SERVER_Comm == 2) {
            if(extract_name(buf_read_C2S, target_name) == 0)
            {
              int idx_p2p = find_user_index(user_list, target_name);
              int ext_text = extract_text(buf_read_C2S, buf_C2S_content);
              if(idx_p2p == -1)
              {
                if(write(user_list[i].m_fd_to_user, "User not found", 15) == -1)
                {
                  printf("Failed to send ""User not found"" message to the sender\n");
                  print_prompt("admin");
                }
              }
              else if(ext_text != -1){
                char temp[1024];
                memset(temp,'\0',1024);
                fprintf(stdout, "msg : %s\n", buf_C2S_content);
                sprintf(temp, "msg : %s : %s",user_list[i].m_user_id, buf_C2S_content);
                printf("%s\n", temp);
                send_p2p_msg(idx_p2p, user_list, temp);
                memset(temp,'\0',1024);
                print_prompt("admin");
              }
              else {
                printf("Failed to extract the message from the command\n");
                print_prompt("admin");
              }
            }
            else {
              printf("Failed to extract the name from the command!\n");
              print_prompt("admin");
            }

          }
          else if(SERVER_Comm == 4) {
            kick_user(i, user_list);
          }
          else {
            if(broadcast_msg(user_list, buf_read_C2S, user_list[i].m_user_id) == -1)
            {
              printf("Failed to send message to all users\n");
            }
          }
          memset(buf_read_C2S,'\0',1024);

        }
        else if(read_size_child==0)
        {
          if(close(user_list[i].m_fd_to_user)==-1)
          {
            printf("Failed to Close fd\n");
          }
          if(close(user_list[i].m_fd_to_server)==-1)
          {
            printf("Failed to Close fd\n");
          }
          kick_user(i, user_list);
          print_prompt("admin");
        }
      }
      else
      {
        continue;
      }
    }
    // poll child processes and handle user commands
    // Poll stdin (input from the terminal) and handle admnistrative command

    /* ------------------------YOUR CODE FOR MAIN--------------------------------*/
  }
}

/* --------------------End of the main function ----------------------------------------*/

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
