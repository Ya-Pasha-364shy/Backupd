#ifndef HELPERS_LOGGER_H
#define HELPERS_LOGGER_H

#include <stdio.h>
#include <pthread.h>
#include <string.h>

#define LOGGER_PATH_TO_LOG_LEN  (2048)
#define LOGGER_INVALID_EXIT     (1)
#define LOGGER_NORMAL_EXIT      (0)

#define PATH_TO_LOGGER_DEFINE   "path_to_log"

int logger(char * message);
char * logger_get_path_to_log();
void logger_set_path_to_log(char * path);

#endif // HELPERS_LOGGER_H