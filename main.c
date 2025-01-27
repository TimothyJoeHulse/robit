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

#include "base_serial.h"

int main(int argc, char *argv[])
{
  char      *message = "ATZ\n";
  int port_descriptor = 0;
  int dbg = 0;

  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s /dev/ttyS0 [ message ]\n", argv[0]);
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


  return 0;
}
