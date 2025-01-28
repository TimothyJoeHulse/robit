#ifndef _BASE_DEFS_H

#define _BASE_DEFS_H

#include <pthread.h>

typedef struct _thread_info {    /* Used as argument to thread_start() */
  pthread_t thread_id;        /* ID returned by pthread_create() */
  int       thread_num;       /* Application-defined thread # */
  int       file_handle;
} thread_info , *pthread_info;

// terminal colors
#define COLOR_BOLD   "\033[1;37m"
#define COLOR_BLACK  "\033[0;30m"
#define COLOR_RED    "\033[0;31m"
#define COLOR_GREEN  "\033[0;32m"
#define COLOR_ORANGE "\033[0;33m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_BLUE   "\033[0;34m"
#define COLOR_PURPLE "\033[0;35m"
#define COLOR_CYAN   "\033[0;36m"
#define COLOR_WHITE  "\033[0;37m"
#define COLOR_RESET  "\033[0m"

#endif
