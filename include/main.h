#ifndef MAIN_LOOP_H
#define MAIN_LOOP_H

#include "helpers/helpers_common.h"

// #define ENABLE_DEBUG_INFO

#define NORMAL_EXIT      (0)
#define INVALID_EXIT     (-1)

#define THREAD_COUNT     (2)
#define MAIN_BUFFER_SIZE (128)

#define MAX_SEC_WAIT     (30)

void * backup_loop(void * argument);
void * main_loop(void * argument);
int main(int argc, char * argv[]);

#endif // MAIN_LOOP_H