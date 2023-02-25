#ifndef HELPERS_COMMON_H
#define HELPERS_COMMON_H

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/inotify.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <signal.h>
#include <dirent.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h>
#include <fts.h>
#include <pthread.h>
#include "event_queue.h"
#include "helpers_hash_table.h"
#include "helpers_logger.h"

#define HELPERS_NORMAL_EXIT         (0)
#define HELPERS_INVALID_EXIT        (-1)
#define HELPERS_AVER_BUFFER_SIZE    (2048)
#define HELPERS_MAX_BUFFER_SIZE     (16384)

#define HELPERS_10_SEC              10000
#define HELPERS_MAX_SEC_WAIT        (15)
#define HELPERS_BUF_SIZE            (1024)

#define HELPERS_FILE_STROKE_MAX_LEN (168)

#define PATH_TO_DIR_DEFINE          "path_to_dir"
#define PATH_TO_BACKUP_DEFINE       "path_to_backup"

typedef struct thread_argument_s
{
    char *            path_to_dir;
    hash_table_t *    hash_table;
    pthread_mutex_t * mutex;
} thread_argument_t;

// for ignore and handle SIGINT signal
void signal_handler(int signum);

int helpers_get_keep_running(void);

// USE ONLY WITH PTHREADS !
void * pthread_on_dir_run(void * argument);
/*
 * TODO:
 * arrange functions in different submodules
 * for encapsulation
*/

/* ======================= */
/*  INOTIFY EVENT HANDLING */

// close inotify instance by fd
int close_inotify_fd(int fd);

// for create an inotify instance and open
// the file descriptor
int open_inotify_fd(void);

// handle events in queue 
void process_handle_events(queue_t q, thread_argument_t * arg);

// add watcher of all events becouse mask is IN_ALL_EVENTS
// for this case
int watch_dir(int fd, const char * dirname, unsigned long mask);

// waits some event to happen,
// then processes the queue before returning
// to wait for more events
int process_inotify_events(queue_t q, int fd, thread_argument_t * arg);

/* ======================= */
/*  BACKUP EVENT HANDLING  */

char * backup_get_path_to_backup();
void backup_set_path_to_backup(char * set);
int backup_fs_iteration_main(thread_argument_t * arg);

/* ======================= */
/*     CONFIG PARSER      */

#define PARSER_DELIMETER ':'

int parser_get_index_by_param(char * stroke, char param);
char * parser_read_conf();

#endif // HELPERS_COMMON_H