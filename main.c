#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>

#include <stdbool.h>

#include <errno.h>

#include "debug.h"
#include "base_defs.h"

#include "base_serial.h"

int g_read_loop = 0;

int main(int argc, char *argv[])
{
  pthread_t  thread_reader;
  void      *result_reader;
  int        result;

  thread_info tinfo;

  char      *message = "ATZ\n";
  int port_descriptor = 0;
  int dbg = 1;

  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s /dev/ttyAMA0 [ message ]\n", argv[0]);
    return 1;
  }

  port_descriptor = init_port( argv[1] , dbg );
  if (port_descriptor == -1) {
    char const *const error = strerror(errno);
    fprintf(stderr, "%s: %s.\n", argv[1], error);
    return 1;
  }

  if (argc > 2)
    message = argv[2];

  g_read_loop = 1;

  DEBUG_MSG ( dbg, "  set tty_mode(0) !! , %s L%d \n" , __FUNCTION__ , __LINE__ );
  tty_mode(0);                /* save current terminal mode */
  DEBUG_MSG ( dbg, "  set_terminal_raw !! , %s L%d \n" , __FUNCTION__ , __LINE__ );
  set_terminal_raw();         /* set -icanon, -echo   */

  tinfo.file_handle = port_descriptor;
  tinfo.thread_num  = 1;

  DEBUG_MSG ( dbg, "  Start serial reader thread !! , %s L%d \n" , __FUNCTION__ , __LINE__ );
    // Start reader first !
  result = pthread_create(&thread_reader, NULL, reader, &tinfo );
  if (result) {
    fprintf(stderr, "Cannot create reader thread: %s.\n", strerror(result));
    exit(1);
  }
  DEBUG_MSG ( dbg, "       serial reader thread STARTED ?? !! , %s L%d \n" , __FUNCTION__ , __LINE__ );

  result = pthread_join( tinfo.thread_id , &result_reader );
   if (result) {
     fprintf(stderr, "reader thread error: %s\n", strerror(result));
     exit(1);
   }

   fprintf(stderr, "threads have completed.\n");
   fflush(stderr);

   printf("Writer returned status %ld (%s).\n",
            (long)result_reader, (result_reader == (void *)0L) ? "Success" : "Error");

  return 0;
}
