#include "../include/helpers/helpers_logger.h"

static char path_to_log[LOGGER_PATH_TO_LOG_LEN] = {0};

// TODO: 
//  * add mnemonics for logs
//  * add func for getting real time

void logger_set_path_to_log(char * set)
{
    if (NULL == set) return;
    strncpy(path_to_log, set, strlen(set));
}

char * logger_get_path_to_log()
{
    return path_to_log;
}

int logger(char * message)
{
    if (!message) return LOGGER_INVALID_EXIT;
    FILE * fptr;
    
    if ((fptr = fopen(path_to_log, "a")) == NULL)
    {
        fprintf(stderr, "Error: unable to open log file!\n");
        return LOGGER_INVALID_EXIT;
    }
    fprintf(fptr, "%s\n", message);

    fclose(fptr);
    return LOGGER_NORMAL_EXIT;
}