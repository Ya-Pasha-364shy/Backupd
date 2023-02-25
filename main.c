#include "include/main.h"
#include "include/helpers/helpers_hash_table.h"

void * backup_loop(void * argument)
{
	fprintf(stderr, "<%s> thread id %lu start running on backup loop successfully...\n",
	       __func__, pthread_self());

	backup_fs_iteration_main((thread_argument_t *)argument);
	
	pthread_exit(NULL);
}

void * main_loop(void * argument)
{
	fprintf(stderr, "<%s>\n", __func__);
	return pthread_on_dir_run(argument);
}

/*
 * @brief
 * main driver
 * needed to distribute the work with POSIX threads
*/
int main(int argc, char * argv[])
{
	const int sleep_now = 1;
	void * (* func[THREAD_COUNT])(void * path_to_dir) = { backup_loop, main_loop };
	hash_table_t * HT = hash_table_create(HASH_TABLE_MAX);
	thread_argument_t * argument = malloc(sizeof(thread_argument_t));
	memset(argument, 0, sizeof(thread_argument_t));
	argument->hash_table = HT; 
	char path_to_dir_buffer[HELPERS_BUF_SIZE] = {0};

	int free_and_exit(int rc)
	{
		hash_table_free(HT);
		free(argument);
		return rc;
	}

	if (NULL == freopen("/dev/null", "r", stdin)
#ifndef ENABLE_DEBUG_INFO
	    || NULL == freopen("/dev/null", "w", stderr) 
	    || NULL == freopen("/dev/null", "w", stdout)
#endif
	)
	{
		printf("Failed to redirect I/Os to /dev/null, exiting...\n");
		return free_and_exit(INVALID_EXIT);
	}

	/* TODO: 
	 * make a func for this purpose (in helpers side)
	 * heap -> stack
	 * or
	 * change type of parsing conf file (from .yaml, for example)
	*/
	/* ==================== */
	/* Start of Serializing */
	bool flag = true;
	int before = 0, after = 0, i = 0, j = 0, counter;
	size_t size_of_line;
    char * tmp;
	char * dir;
    char before_value[HELPERS_BUF_SIZE] = {0};
	char * buffer = parser_read_conf();

	if (NULL == buffer)
	{
		fprintf(stderr, "Error: invalid buffer value");
		return free_and_exit(INVALID_EXIT);
	}
	while (flag)
	{
        for (i = before; buffer[i] != '\n'; i++)
        {
            if (buffer[i] == '\0')
            {
                flag = false;
            }
        }
        after = i;
        size_of_line = sizeof(char) * after;

        char * tmp = (char *)malloc(size_of_line);
        memset(tmp, 0, sizeof(tmp));

        counter = 0;
        for (j = before; j < after; j++)
		{
        	tmp[counter++] = buffer[j];
		}
		tmp[counter] = '\0';

        if (strlen(tmp) > HELPERS_FILE_STROKE_MAX_LEN)
        {
            fprintf(stderr, "Length of stroke in configuration file very long!\n");
			free(tmp);
			free(buffer);
            return INVALID_EXIT;
        }
        // skipping \n for normal work
        before = after + 1;
        int index = parser_get_index_by_param(tmp, PARSER_DELIMETER);
        if (0 > index)
		{
			free(tmp);
            continue;
		}

        int k;
        // in result before_value is parameter without value
        for (k = 0; k != index; k++)
        {
		    before_value[k] = tmp[k];
		}
		before_value[k] = '\0';

        // parse value (segment of data with some path)
        int local_counter = 0;
        char * slice_after_delimeter = (char *)malloc(sizeof(char) * size_of_line);
        memset(slice_after_delimeter, 0, sizeof(slice_after_delimeter));

		for (int i = index + 2; tmp[i] != '\0'; i++)
        {
            slice_after_delimeter[local_counter++] = tmp[i];
        }
		if (0 == strcmp(before_value, PATH_TO_DIR_DEFINE))
		{
			struct stat path_stat;
			stat(slice_after_delimeter, &path_stat);
			if (!S_ISDIR(path_stat.st_mode))
			{
				fprintf(stderr, "Warning: this path is not a path to directory !\n");		
				free(slice_after_delimeter);
				free(tmp);
				free(buffer);
				return free_and_exit(INVALID_EXIT);		
			}
			strncpy(path_to_dir_buffer, slice_after_delimeter, strlen(slice_after_delimeter));
		}
		if (0 == strcmp(before_value, PATH_TO_BACKUP_DEFINE))
		{
			backup_set_path_to_backup(slice_after_delimeter);
		}
		if (0 == strcmp(before_value, PATH_TO_LOGGER_DEFINE))
		{
			logger_set_path_to_log(slice_after_delimeter);
		}
		free(slice_after_delimeter);
		free(tmp);
	}
	free(buffer);
	argument->path_to_dir = path_to_dir_buffer;
	/* End of Serializing */

	if (signal(SIGINT, signal_handler) == SIG_IGN)
	{
		/* Ignore the signal SIGINT (Ctrl+C) */
		signal(SIGINT, SIG_IGN);
	}

	pthread_t threads[THREAD_COUNT];
	pthread_mutex_t mutex;
	if (0 != pthread_mutex_init(&mutex, NULL))
	{
		printf("Error: pthread_mutex_init() failed !\n");
		return free_and_exit(INVALID_EXIT);
	}
	argument->mutex = &mutex;

	int rc = 0;

	for (i = 0; i < THREAD_COUNT; i++)
	{
		rc = pthread_create(&threads[i], NULL, func[i], argument);
		if (0 != rc)
		{
			printf("<%s> Error for create thread id #%lu; return code: %d", __func__, threads[i], rc);
			pthread_mutex_destroy(&mutex);
			return free_and_exit(INVALID_EXIT);
		}
	}

	for (i = 0; i < THREAD_COUNT; i++)
	{
		while (helpers_get_keep_running()) { sleep(sleep_now); continue; }

		int total_sleep = 0;
		while (0 != pthread_tryjoin_np(threads[i], NULL))
		{
			total_sleep += sleep_now;
			sleep(sleep_now);

			if (!(total_sleep % MAX_SEC_WAIT))
			{
				printf("\n!!! timeout for waiting %lu thread, exiting !!!\n", threads[i]);
				rc = INVALID_EXIT;
				break;
			}
			continue;
		}
		if (rc != 0)
		{
			printf("Error for join thread id #%lu; return code: %d\n", threads[i], rc);
		}
	}

	pthread_mutex_destroy(&mutex);
	return free_and_exit(rc);
}