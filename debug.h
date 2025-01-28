#ifndef _DEBUG_H

#define _DEBUG_H 1

extern void print_debug( int dbg, const char* fmt, ...);

#define DEBUG 1

#ifdef DEBUG
  #define DEBUG_MSG( dbg, fmt, ...) print_debug( dbg, fmt, ##__VA_ARGS__)
#else
  #define DEBUG_MSG( dbg, fmt, args...) ((void) 0)
#endif

#endif
