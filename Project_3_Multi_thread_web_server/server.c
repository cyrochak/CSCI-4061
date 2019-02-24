/* CSci4061 Fall 2018 Project 3
* login: chanx393 (login used to submit)
* date: 12/12/18
* name: Kin Nathan Chan, Cyro Chun Fai Chak, Isaac Aruldas (for partner(s))
* id: 5330106- chanx393, 5312343 - chakx011, 5139488 - aruld002 */
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include "util.h"
#include <stdbool.h>

#define MAX_THREADS 100
#define MAX_QUEUE_LEN 100
#define MAX_CE 100
#define INVALID -1
#define BUFF_SIZE 1024

/*
THE CODE STRUCTURE GIVEN BELOW IS JUST A SUGESSTION. FEEL FREE TO MODIFY AS NEEDED
*/

// structs:
typedef struct request_queue {
  int fd;
  char request[1024];
  int status; // 0 means not finished, 1 means finished
} request_t;

typedef struct cache_entry {
  int len;
  char request[1024];
  char *content;
  long time;
  int status; // 0 means empty, 1 means full
} cache_entry_t;

/* ************************ Global Variables ***********************************/
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condv_dispatch = PTHREAD_COND_INITIALIZER;
int queue_length;
int cache_size;
int num_workers;
int num_active_workers;
int current_requests = 0;
cache_entry_t* cache_entries;
request_t *request_queue;
pthread_t *worker_threads;
long total_time = 0;

/* ************************ Functions ***********************************/
long getCurrentTimeInMicro();
void * worker(void *arg);
/* ************************ Dynamic Pool Code ***********************************/
// Extra Credit: This function implements the policy to change the worker thread pool dynamically
// depending on the number of requests

/*  When the thread for monitoring the dynamic pool size is set up at the first
time, it will wait for 500000 microseconds (0.5 second) to let the worker
threads and the dispatch threads to finish setting up. Then, it will keep trying
to hold the lock which is shared with other threads. Once, it gets the lock, it
will check the usage by use the number of current requests to divide the number
of active workers. Then, we want to maintain the usage at 50% in order to handle
any accident requests by using the number of current requests to divide 0.5. If
the usage is lower than 10% or higher than 90%, the thread will either kill the
worker threads (at least one worker thread will stay alive at anytime), or fire
up more threads by the number of active workers minus the threads size goal or
the threads size goal minus the number of active workers.
*/
void * dynamic_pool_size_update(void *arg) {
  usleep(500000);
  while(1) {
    // Run at regular intervals
    // Increase / decrease dynamically based on your policy
    if(pthread_mutex_trylock(&lock) == 0) {
      float current_use_rate = current_requests/num_active_workers;
      int threads_goal = current_requests/0.5;
      FILE *log_file = fopen("../webserver_log", "a");
      if (current_use_rate < 0.1 && num_active_workers > 1) {
        int needed_remove = num_active_workers - threads_goal;
        if (needed_remove == num_active_workers) {
          needed_remove = num_active_workers - 1;
        }
        if (needed_remove > 0) {
          for (int k = 1; k <= needed_remove; k++) {
            pthread_cancel(worker_threads[num_active_workers-k]);
          }
          for (int i = 1; i <= needed_remove; i++) {
            pthread_join(worker_threads[num_active_workers-i], NULL);
          }
          if (log_file != NULL) {
            fprintf(stdout, "Deleted %d worker threads because the server load is below 10%%.\n",
            needed_remove);
            fprintf(log_file, "Deleted %d worker threads because the server load is below 10%%.\n",
            needed_remove);
          } else {
            perror("Couldn't create a log file.");
          }
          if (threads_goal == 0) {
            num_active_workers = 1;
          } else {
            num_active_workers = threads_goal;
          }
        }
      }
      else if (current_use_rate > 0.9 && num_active_workers < num_workers) {
        int thread_id = num_active_workers;
        int threads_needed = threads_goal - num_active_workers;
        int order[threads_needed];
        for (int i = 0; i < threads_needed; i++) {
          order[num_active_workers] = num_active_workers;
          pthread_create(&worker_threads[num_active_workers], NULL, &worker, (void *)&order[num_active_workers]);
          num_active_workers++;
        }
        if (log_file != NULL) {
          fprintf(stdout, "Created %d worker threads because the server load is above 90%%.\n",
          threads_needed);
          fprintf(log_file, "Created %d worker threads because the server load is above 90%%.\n",
          threads_needed);
        } else {
          perror("Couldn't create a log file.");
        }
      }
      fclose(log_file);
      pthread_mutex_unlock(&lock);
      pthread_cond_broadcast(&condv_dispatch);

    }
  }
}
/**********************************************************************************/

/* ************************************ Cache Code ********************************/

// Function to check whether the given request is present in cache
/* The getCacheIndex will find the cache that matches the request and return
the content. Also, in order to implement LRU caching for addIntoCache() function,
when it finds the request in the cache, it will update the add_time by calling
getCurrentTimeInMicro().
*/
int getCacheIndex(char *request){
  /// return the index if the request is present in the cache
  for (int i = 0; i < cache_size; i++) {
    if(cache_entries[i].status == 1) {
      if(strcmp(request, cache_entries[i].request) == 0) {
        long add_time = getCurrentTimeInMicro();
        cache_entries[i].time = add_time;
        return i;
      }
    }
  }
  return -1;
}

// Function to add the request and its file content into the cache
/* The addIntoCache is implemented as LRU caching scheme. When we add the
request to the cache, we first record the time using getCurrentTimeInMicro().
Then, we will look up the cache to see if there is any free spots that we can
place the request. While we are looking up the free spots, we record the least
access cache by comparing the add time with the time of each item in the cache.
At the end, if it couldn't find a free spot, then it will replace the least
access item that we reocrd eaeiler time. Also, the LRU caching scheme is
implemented in the getCacheIndex() function.
*/
void addIntoCache(char *mybuf, char *memory , int memory_size){
  // It should add the request at an index according to the cache replacement policy
  // Make sure to allocate/free memeory when adding or replacing cache entries
  long add_time = getCurrentTimeInMicro();
  long max = add_time;
  int max_index;
  for (int i = 0; i < cache_size; i++) {
    if (cache_entries[i].status == 0) {
      cache_entries[i].len = memory_size;
      memset(cache_entries[i].request, '\0', 1024);
      sprintf(cache_entries[i].request, "%s", mybuf);
      cache_entries[i].content = memory;
      cache_entries[i].time = add_time;
      cache_entries[i].status = 1;
      break;
    } else if ((add_time - cache_entries[i].time) > (add_time - max )) {
      max = cache_entries[i].time;
      max_index = i;
    }

    if (i == (cache_size -1)) {
      cache_entries[max_index].len = memory_size;
      memset(cache_entries[max_index].request, '\0', 1024);
      sprintf(cache_entries[max_index].request, "%s", mybuf);
      free(cache_entries[max_index].content);
      cache_entries[max_index].content = memory;
      cache_entries[max_index].time = add_time;
      cache_entries[max_index].status = 1;
    }
  }
}

// clear the memory allocated to the cache
void deleteCache(){
  // De-allocate/free the cache memory
  for (int i = 0; i < cache_size; i++) {
    free(cache_entries[i].content);
  }
}

// Function to initialize the cache
void initCache(){
  // Allocating memory and initializing the cache array
  cache_entries = (cache_entry_t *)malloc(sizeof(cache_entry_t) * cache_size);
  request_queue = (request_t *)malloc(sizeof(request_t)* queue_length);
  for (int i = 0; i < cache_size; i++) {
    cache_entries[i].status = 0;
  }

  for (int j = 0; j < queue_length; j++) {
    request_queue[j].status = 1;
  }
}

// Function to get the content type from the request
char* getContentType(char * mybuf) {
  // Should return the content type based on the file type in the request
  // (See Section 5 in Project description for more details)
  char * type;
  if((type = strstr(mybuf, ".html")) != NULL) {
    return "text/html";
  } else if ((type = strstr(mybuf, ".jpg")) != NULL) {
    return "image/jpeg";
  } else if ((type = strstr(mybuf, ".gif")) != NULL) {
    return "image/gif";
  } else {
    return "text/plain";
  }
}

// Function to open and read the file from the disk into the memory
// Add necessary arguments as needed
int readFromDisk(char *request_path, char *memory, long file_size) {
  // Open and read the contents of file given the request
  char target_path[1024];
  sprintf(target_path, ".%s", request_path);
  FILE *request_file = fopen(target_path, "r");
  if(request_file != NULL) {
    fread(memory, file_size, 1, request_file);
    fclose(request_file);
    return 0;
  }
  return -1;
}

/**********************************************************************************/

/* ************************************ Utilities ********************************/

// This function returns the current time in microseconds
long getCurrentTimeInMicro() {
  struct timeval curr_time;
  gettimeofday(&curr_time, NULL);
  return curr_time.tv_sec * 1000000 + curr_time.tv_usec;
}

/**********************************************************************************/

// Function to receive the request from the client and add to the queue
void * dispatch(void *arg) {
  while (1) {

    // Accept client connection
    int connection_fd;
    if((connection_fd = accept_connection()) > INVALID) {

      // Get request from the client
      char filename[BUFF_SIZE];
      if(get_request(connection_fd, filename) == 0) {

        // Add the request into the queue
        int i;
        pthread_mutex_lock(&lock);
        for(i = 0; i < queue_length; i++) {
          if(request_queue[i].status == 1) {
            request_queue[i].fd = connection_fd;
            memset(request_queue[i].request, '\0', 1024);
            sprintf(request_queue[i].request, "%s", filename);
            request_queue[i].status = 0;
            current_requests++;
            break;
          }
          if(i == queue_length - 1) {
            pthread_cond_wait(&condv_dispatch, &lock);
            i = -1;
          }
        }
        pthread_mutex_unlock(&lock);
      }
    }
  }
}

/**********************************************************************************/

// Function to retrieve the request from the queue, process it and then return a result to the client
void * worker(void *arg) {
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  int thread_id = *(int *) arg;
  int handle_count = 0;
  while (1) {

    // Get the request from the queue
    int index;
    for(index = 0; index < queue_length; index++) {
      if(pthread_mutex_trylock(&lock) == 0) {
        if(request_queue[index].status == 0) {
          char *memory;
          handle_count++;
          long file_size;
          char target_path[1025];

          // Start recording time
          long start = getCurrentTimeInMicro();

          // Get the data from the disk or the cache
          int cache_index = getCacheIndex(request_queue[index].request);
          char not_found_error[1024];
          if (cache_index == -1) {
            sprintf(target_path, ".%s", request_queue[index].request);
            FILE *request_file = fopen(target_path, "r");
            if(request_file != NULL) {
              fseek(request_file, 0, SEEK_END);
              file_size = ftell(request_file);
              rewind(request_file);
              memory = (char *)malloc((file_size+1)*sizeof(char));
              fclose(request_file);
              readFromDisk(request_queue[index].request, memory, file_size);
              addIntoCache(request_queue[index].request, memory, file_size);
            } else {
              sprintf(not_found_error, "%s", request_queue[index].request);
            }
          } else {
            memory = cache_entries[cache_index].content;
            file_size = cache_entries[cache_index].len;
          }
          // Stop recording the time
          long end = getCurrentTimeInMicro();
          long used_time = end-start;
          // total_time += used_time;
          // printf("Total time would be :%ld ms\n", total_time);

          // Log the request into the file and terminal
          FILE *log_file = fopen("../webserver_log", "a");
          if (log_file != NULL) {
            char result[1024];
            if(strlen(not_found_error) > 0) {
              fprintf(stdout, "[%d][%d][%d][%s][Requested file not found.][%ld ms][MISS]\n",
              thread_id, handle_count, request_queue[index].fd, not_found_error, used_time);
              fprintf(log_file, "[%d][%d][%d][%s][Requested file not found.][%ld ms][MISS]\n",
              thread_id, handle_count, request_queue[index].fd, not_found_error,used_time);
            } else if (cache_index == -1) {
              fprintf(stdout, "[%d][%d][%d][%s][%ld][%ld ms][MISS]\n",
              thread_id, handle_count, request_queue[index].fd, request_queue[index].request, file_size, used_time);
              fprintf(log_file, "[%d][%d][%d][%s][%ld][%ld ms][MISS]\n",
              thread_id, handle_count, request_queue[index].fd, request_queue[index].request, file_size, used_time);
            } else if (cache_index > -1){
              fprintf(stdout, "[%d][%d][%d][%s][%ld][%ld ms][HIT]\n",
              thread_id, handle_count, request_queue[index].fd, request_queue[index].request, file_size, used_time);
              fprintf(log_file, "[%d][%d][%d][%s][%ld][%ld ms][HIT]\n",
              thread_id, handle_count, request_queue[index].fd, request_queue[index].request, file_size, used_time);
            }
            fclose(log_file);
          } else {
            perror("Couldn't create a log file.");
          }

          // return the result
          if(file_size > 0) {
            char * type = getContentType(request_queue[index].request);
            if(return_result(request_queue[index].fd, type, memory, file_size) != 0) {
              perror("Failed to return the file");
            }
          } else if (strlen(not_found_error) > 0) {
            return_error(request_queue[index].fd, request_queue[index].request);
            memset(not_found_error, '\0', 1024);
          }
          request_queue[index].status = 1;
          current_requests --;
        }
        pthread_mutex_unlock(&lock);
        pthread_cond_broadcast(&condv_dispatch);
        fflush(stdout);
      }
    }

  }
}

/**********************************************************************************/

int main(int argc, char **argv) {

  // Error check on number of arguments
  if(argc != 8){
    printf("usage: %s port path num_dispatcher num_workers dynamic_flag queue_length cache_size\n", argv[0]);
    return -1;
  }

  FILE *log_file = fopen("webserver_log", "w");
  fclose(log_file);
  // Get the input args
  int port_num = atoi(argv[1]);
  // the path would be argv[2], nothing needed to do for now
  int num_dispatcher = atoi(argv[3]);
  num_workers = atoi(argv[4]);
  int dynamic_flag = atoi(argv[5]);
  queue_length = atoi(argv[6]);
  cache_size = atoi(argv[7]);
  // Perform error checks on the input arguments
  if(port_num < 1025 || port_num > 65535) {
    fprintf(stderr, "Invalid port number: : %d. Only use ports 1025 - 65535.\n", port_num);
    exit(-1);
  }
  if(num_dispatcher < 1 || num_dispatcher > MAX_THREADS) {
    fprintf(stderr, "Invalid number of dispatch threads : %d. Between 1 to 100.\n", num_dispatcher);
    exit(-1);
  }
  if(num_workers < 1 || num_workers > MAX_THREADS) {
    fprintf(stderr, "Invalid number of worker threads : %d. Between 1 to 100.\n", num_workers);
    exit(-1);
  }
  if(dynamic_flag < 0 || dynamic_flag > 1) {
    fprintf(stderr, "Invalid number of dynamic flag : %d. Either 0 or 1.\n", dynamic_flag);
    exit(-1);
  }
  if(queue_length < 1 || queue_length > MAX_QUEUE_LEN) {
    fprintf(stderr, "Invalid queue_len size : %d. Between 1 to 100.\n", queue_length);
    exit(-1);
  }
  if(cache_size < 1 || cache_size > MAX_CE) {
    fprintf(stderr, "Invalid cache_size size : %d. Between 1 to 100.\n", cache_size);
    exit(-1);
  }

  // Change the current working directory to server root directory
  if(chdir(argv[2]) == INVALID) {
    perror("Couldn't change directory to server root.");
    exit(-1);
  }
  // Start the server and initialize cache
  init(port_num);
  printf("Starting server on port %d: %d disp, %d work\n", port_num, num_dispatcher, num_workers);
  initCache();

  // Create dispatcher and worker threads
  pthread_t dispatcher_threads[num_dispatcher];
  worker_threads = (pthread_t *)malloc(sizeof(pthread_t) * num_workers);
  for(int i = 0; i < num_dispatcher; i++) {
    pthread_create(&dispatcher_threads[i], NULL, &dispatch, NULL);
    pthread_detach(dispatcher_threads[i]);
  }
  pthread_t dynamic_boss;
  if (dynamic_flag == 1) {
    num_active_workers = num_workers;
    pthread_create(&dynamic_boss, NULL, &dynamic_pool_size_update, NULL);
  }
  int j;
  int ar[num_workers];
  for(j = 0; j < num_workers; ++j) {
    ar[j] = j;
    pthread_create(&worker_threads[j], NULL, &worker, (void *)&ar[j]);
  }

  for(int k = 0; k < num_workers; k++) {
    pthread_join(worker_threads[k], NULL);
  }
  pthread_join(dynamic_boss, NULL);

  // Clean up
  //rmb to free malloc
  deleteCache();
  free(worker_threads);
  return 0;
}
