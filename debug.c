#include <stdio.h>
#include <stdlib.h>

#include <stdarg.h>

#include "debug.h"



void print_debug( int dbg, const char* fmt, ...) {
  va_list args;
  if( dbg ) {
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
    fflush(stderr);
  }
}

