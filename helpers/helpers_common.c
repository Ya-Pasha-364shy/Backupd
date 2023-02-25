#include "../include/helpers/helpers_common.h"

#define COPY_COMMAND "/bin/cp"

static unsigned int keep_running = 1;
static unsigned long int watched_items;

void signal_handler(int signum)
{
    logger("SIGINT signal handled, the program is terminated!");
    keep_running = 0;
}

static void handle_event(queue_entry_t event, thread_argument_t * arg);
static void backup_fs_make_backup(char * path_to_file, char * filename);

int helpers_get_keep_running()
{
    return keep_running;
}

/* ===================== */
/* HANDLE INOTIFY CALLS */

int close_inotify_fd(int fd)
{
    int r;

    if ((r = close(fd)) < 0)
    {
        fprintf(stderr, "impossible to close inotify fd\n");
        return HELPERS_INVALID_EXIT;
    }

    watched_items = 0;
    return r;
}

int open_inotify_fd(void)
{
    int fd;
    fd = inotify_init();

    if (fd < 0)
    {
        fprintf(stderr, "inotify_init failed !\n");
        return HELPERS_INVALID_EXIT;
    }
    return fd;
}

int watch_dir(int fd, const char * dirname, unsigned long mask)
{
    int wd;
    char message_buffer[HELPERS_AVER_BUFFER_SIZE] = {0};
    wd = inotify_add_watch(fd, dirname, mask);
    if (wd < 0)
    {
        sprintf(message_buffer, "Cannot add watch for \"%s\" with event mask %lX", dirname, mask);
        return HELPERS_INVALID_EXIT;
    }
    else
    {
        watched_items++;
        int offset = sprintf(message_buffer, "Watching %s WD=%d", dirname, wd);
        sprintf(message_buffer+offset, "; Watching = %ld items", watched_items);
    }
    if (!logger(message_buffer))
    {
        fprintf(stderr, "start-log-messages was be writed into log-file !\n");
    }
    return wd;
}

static int event_check(int fd)
{
    int rv = 0;
    struct timeval tv;
    tv.tv_sec = 5; tv.tv_usec = 0;
    fd_set rfds;

    while (keep_running)
    {
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);

        rv = select(FD_SETSIZE, &rfds, NULL, NULL, &tv);
        if (rv == 0)
        {
            continue;
        }
        else if (rv < 0)
        {
            fprintf(stderr, "Error: select is returned wrong answer\n");
        }
        return rv;
    }
    return HELPERS_NORMAL_EXIT;
}

static int read_events(queue_t q, int fd)
{
    int count = 0;
    size_t event_size, q_event_size;
    ssize_t r, buffer_i;
    struct inotify_event * pevent;
    queue_entry_t event;
    char buffer[HELPERS_MAX_BUFFER_SIZE];

    r = read(fd, buffer, HELPERS_MAX_BUFFER_SIZE);
    if (r <= 0)
    {
        return HELPERS_INVALID_EXIT;
    }

    buffer_i = 0;
    while (buffer_i < r)
    {
        /* Parse events and queue them */
        pevent       = (struct inotify_event *)&buffer[buffer_i];
        event_size   = offsetof(struct inotify_event, name) + pevent->len;
        q_event_size = offsetof(struct queue_entry, inot_ev.name) + pevent->len;

        event = (queue_entry_t)malloc(q_event_size);
        memmove(&(event->inot_ev), pevent, event_size);
        queue_enqueue(event, q);

        buffer_i += event_size;
        count++;
    }
    return count;
}

void process_handle_events(queue_t q, thread_argument_t * arg)
{
    queue_entry_t event;
    while (keep_running && !queue_empty(q))
    {
        event = queue_dequeue(q);
        handle_event(event, arg);
        free(event);
    }
    return;
}

int process_inotify_events(queue_t q, int fd, thread_argument_t * arg)
{
    int r;
    while (keep_running && (watched_items > 0))
    {
        if (event_check(fd) > 0)
        {
            if ((r = read_events(q, fd)) < 0)
            {
                break;
            }
            else
            {
                process_handle_events(q, arg);
            }
        }
    }
    return HELPERS_NORMAL_EXIT;
}

void * pthread_on_dir_run(void * argument)
{
    int inotify_fd = open_inotify_fd();
	thread_argument_t * arg = (thread_argument_t *)argument;

	if (inotify_fd > 0)
	{
		queue_t q = queue_create();

		int wd = watch_dir(inotify_fd, arg->path_to_dir, IN_ALL_EVENTS & ~(IN_CLOSE | IN_OPEN | IN_ACCESS));
		if (wd > 0)
		{
			process_inotify_events(q, inotify_fd, arg);
		}

		close_inotify_fd(inotify_fd);
		queue_destroy(q);
	}

	pthread_exit(NULL);
}

static void handle_event(queue_entry_t event, thread_argument_t * arg)
{
    unsigned int rc_mutex = 0, total_sleep = 0, sleep_now = 100;
    bool need_to_backup = false;
    bool is_dir = event->inot_ev.mask & IN_ISDIR;
    /* If the event was associated with a filename, we will store it here */
    char * cur_event_filename = NULL;
    char * cur_event_file_or_dir = NULL;
    /* This is the watch descriptor the event occurred on */
    int cur_event_wd = event->inot_ev.wd;
    int cur_event_cookie = event->inot_ev.cookie;
    long unsigned int flags;

    if (event->inot_ev.len)
    {
        cur_event_filename = event->inot_ev.name;
    }
    if (is_dir)
    {
        // only for "dir" flow
        cur_event_file_or_dir = "Dir";
    }
    else
    {
        if (*cur_event_filename == '.')
        {
            return;
        }
        // only for "file" flow
        cur_event_file_or_dir = "File";
    }
    flags = event->inot_ev.mask &
            ~(IN_ALL_EVENTS | IN_UNMOUNT | IN_Q_OVERFLOW | IN_IGNORED);

    char message_buffer[HELPERS_AVER_BUFFER_SIZE] = {0};
    /* Perform event dependent handler routines */
    /* The mask is the magic that tells us what file operation occurred */
    switch (event->inot_ev.mask & (IN_ALL_EVENTS | IN_Q_OVERFLOW | IN_IGNORED))
    {
        case IN_CREATE:
        {
            sprintf(message_buffer, "CREATE: %s \"%s\", need to backup", cur_event_file_or_dir, cur_event_filename);
            need_to_backup = true;
            break;
        }
        case IN_DELETE:
        {
            sprintf(message_buffer, "DELETE: %s \"%s\" need to backup", cur_event_file_or_dir, cur_event_filename);
            need_to_backup = true;
            break;
        }
        case IN_CLOSE_WRITE:
        {
            sprintf(message_buffer, "CLOSE_WRITE: %s \"%s\"", cur_event_file_or_dir, cur_event_filename);
            break;
        }
        case IN_CLOSE_NOWRITE:
        {
            sprintf(message_buffer, "IN_CLOSE_NOWRITE: %s \"%s\"", cur_event_file_or_dir, cur_event_filename);
            break;
        }
        case IN_MODIFY:
        {
            sprintf(message_buffer, "MODIFY: %s \"%s\" need to backup", cur_event_file_or_dir, cur_event_filename);
            need_to_backup = true;
            break;
        }
        case IN_MOVED_FROM:
        {
            sprintf(message_buffer, "MOVED_FROM: %s \"%s\" Cookie=%d", cur_event_file_or_dir, cur_event_filename,
                    cur_event_cookie);
            break;
        }
        // WRITE YOUR CASE EVENTS HERE !
        /*
         * Watch was removed explicitly by inotify_rm_watch or automatically
         * because file was deleted, or file system was unmounted. 
        */
        case IN_IGNORED:
        {
            watched_items--;
            fprintf(stderr, "IGNORED: WD #%d\n", cur_event_wd);
            fprintf(stderr, "Watching = %ld items\n", watched_items);
            if (watched_items <= 0)
            {
                return;
            }
            break;
        }
        case IN_Q_OVERFLOW:
        {
            sprintf(message_buffer, "%s", "Warning: AN OVERFLOW EVENT OCCURRED!");
            return;
        }
        default:
        {
            // need for handle CTRL+S signal to saving file
            sprintf(message_buffer, "UNKNOWN EVENT \"%X\" OCCURRED for file \"%s\", need to backup",
                    event->inot_ev.mask, cur_event_filename);
            need_to_backup = true;
            break;
        }
    }
    if (!logger(message_buffer))
    {
        fprintf(stderr, "Log-message was be writed in %s successfully !\n", logger_get_path_to_log());
    }
    while (keep_running)
    {
        if (0 == (rc_mutex = pthread_mutex_trylock(arg->mutex)))
        {
            break;
        }
        else if (EBUSY == rc_mutex)
        {
            usleep(sleep_now);
            total_sleep += sleep_now;
            if (0 == total_sleep % HELPERS_10_SEC)
            {
                fprintf(stderr, "Error: mutex trylock is timeout !\n");
                return;
            }
            continue;
        }
        else
        {
            fprintf(stderr, "Error: mutex trylock is fail !\n");
            return;
        }
    }
    if (need_to_backup && !is_dir)
    { 
        char full_path[HELPERS_BUF_SIZE] = {0};
        
        if (1 == strlen(strrchr(arg->path_to_dir, '/')))
        {
            sprintf(full_path, "%s%s", arg->path_to_dir, cur_event_filename);
        }
        else
        {
            sprintf(full_path, "%s/%s", arg->path_to_dir, cur_event_filename);
        }
        backup_fs_make_backup(full_path, cur_event_filename);
    }
    pthread_mutex_unlock(arg->mutex);

    /* If any flags were set other than IN_ISDIR, report the flags */
    if (flags & (~IN_ISDIR))
    {
        flags = event->inot_ev.mask;
        fprintf(stderr, "Flags = %lX\n", flags);
    }

    return;
}

/* ===================== */
/*  HANDLE BACKUP CALLS  */

static char path_to_backup[HELPERS_AVER_BUFFER_SIZE];

char * backup_get_path_to_backup()
{
    return path_to_backup;
}

void backup_set_path_to_backup(char * set)
{
    if (NULL == set) return;
    strncpy(path_to_backup, set, strlen(set));
}

static void backup_fs_make_backup(char * path_to_file, char * filename)
{
    char command[HELPERS_BUF_SIZE] = {0}, backup_path[HELPERS_AVER_BUFFER_SIZE+HELPERS_BUF_SIZE+1] = {0};
    sprintf(backup_path, "%s/%s", path_to_backup, filename);
    sprintf(command, "%s %s %s", COPY_COMMAND, path_to_file, backup_path);

    if (-1 == system(command))
    {
        fprintf(stderr, "Cannot execute the command!\n");
    }
    return;
}

static int backup_check_fs_and_run(thread_argument_t * arg)
{
    const int sleep_now = 1;
    int fts_options  = FTS_COMFOLLOW | FTS_LOGICAL | FTS_NOCHDIR;
    FTS    * ftsp    = NULL;
    FTSENT * p       = NULL,
           * chp     = NULL;
    char * const paths[] = {(char * const)arg->path_to_dir, NULL};
    // 1000 dirs - max recursive depth
    hash_table_t * ht_dirs = hash_table_create(HASH_TABLE_MAX);

    int free_and_exit(int rc)
    {
        hash_table_free(ht_dirs);
        return rc;
    }

    if (NULL == (ftsp = fts_open(paths, fts_options, NULL)))
    {
        printf("Can't open file for backup !");
        return free_and_exit(HELPERS_INVALID_EXIT);
    }

    // checking if there are files in a directory
    if (NULL == (chp = fts_children(ftsp, 0)))
    {
        return free_and_exit(HELPERS_NORMAL_EXIT);
    }
    while (keep_running && (NULL != (p = fts_read(ftsp))))
    {
        switch (p->fts_info)
        {
            case FTS_F:
            {
                // for checking that file was be "backuped"
                // file in all subdirectories of our "root" (in arg->path_to_dir)
                hash_table_insert_item(arg->hash_table, p->fts_path);
                break;
            }
            case FTS_D:
            {
                // for start new pthreads on dirs
                hash_table_insert_item(ht_dirs, p->fts_path);
                break;
            }
            default:
            {
                break;
            }
        }
    }
    fts_close(ftsp);

    int rc = HELPERS_NORMAL_EXIT;
    int thread_counter = 0;
    int dir_flow_pthreads = ht_dirs->count; 
    pthread_t threads[dir_flow_pthreads];
    thread_argument_t * argument = (thread_argument_t *)malloc(sizeof(*arg));
    memset(argument, 0, sizeof(*arg));
    // copy mutex pointer
    argument->mutex = arg->mutex;

    for (int i = 0; i < ht_dirs->size; i++)
    {
        if (NULL != hash_table_search_item(ht_dirs, i, NULL))
        {
            argument->path_to_dir = ht_dirs->table[i]->value;
            rc = pthread_create(&threads[thread_counter], NULL, pthread_on_dir_run, argument);
            if (0 != rc)
		    {
                printf("<%s> Error for create thread id #%lu; return code: %d", __func__, threads[thread_counter], rc);
                free(argument);
                return free_and_exit(HELPERS_INVALID_EXIT);
            }
            ++thread_counter;
        }
    }
	for (int i = 0; i < thread_counter; i++)
	{
		while (keep_running) { sleep(sleep_now); continue; }

		int total_sleep = 0;
		while (0 != pthread_tryjoin_np(threads[i], NULL))
		{
			total_sleep += sleep_now;
			sleep(sleep_now);

			if (!(total_sleep % HELPERS_MAX_SEC_WAIT))
			{
				fprintf(stderr, "\n!!! timeout for waiting #%lu thread, exiting !!!\n", threads[i]);
				rc = HELPERS_INVALID_EXIT;
                break;
			}
			continue;
		}
		if (rc != 0)
		{
			fprintf(stderr, "Error for join thread id #%lu; return code: %d\n", threads[i], rc);
            break;
		}
        else
        {
            rc = HELPERS_NORMAL_EXIT;
        }
	}
    return free_and_exit(HELPERS_NORMAL_EXIT);
}

int backup_fs_iteration_main(thread_argument_t * arg)
{
    fprintf(stderr, "<%s>\n", __func__);

    int rc = backup_check_fs_and_run(arg);
    if (0 == arg->hash_table->count)
    {
        fprintf(stderr, "Warning: hash table is void !\n");
        rc = HELPERS_INVALID_EXIT;
    }

    return rc;
}

/* ====================== */
/* HELPERS PARSING CONFIG */
int parser_get_index_by_param(char * stroke, char param)
{
    if (NULL == stroke)
        return -1;

    if (strlen(stroke) > HELPERS_FILE_STROKE_MAX_LEN)
        return -2;

    int res = strchr(stroke, param) - stroke;
    return res;
}

char * parser_read_conf()
{
    char cwd[HELPERS_AVER_BUFFER_SIZE] = {0};
    if (NULL == getcwd(cwd, sizeof(cwd)))
    {
        fprintf(stderr, "Error: function getcwd()\n");
        return NULL;
    }
    char * pathToConf = (char *)calloc(strlen(cwd) + strlen("/../config.conf") + 1, sizeof(char));

    strcat(pathToConf, cwd);
    strcat(pathToConf, "/../config.conf");
    FILE * cp = fopen(pathToConf, "r");
    if (NULL == cp)
    {
        fprintf(stderr, "Error for opening file!\n");
        free(pathToConf);
        return NULL;
    }

    int lines;
    while (!feof(cp))
    {
        if (fgetc(cp) == '\n')
            lines++;
    }
    lines++;
    fclose(cp);

    char * buffer = (char *)malloc(sizeof(char) * HELPERS_FILE_STROKE_MAX_LEN * lines);
    memset(buffer, 0, sizeof(buffer));
    if (NULL == (cp = fopen(pathToConf, "r")))
    {
        free(pathToConf);
        free(buffer);
        return NULL;
    }

    free(pathToConf);
    while (fread(buffer, sizeof(char), HELPERS_FILE_STROKE_MAX_LEN * lines, cp));

    return buffer;
}