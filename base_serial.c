#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>

#include <string.h>

#include <errno.h>

#include "debug.h"
#include "base_serial.h"

extern int g_read_loop;

string_t *add(string_t *string, void const *const data, size_t const size)
{
    size_t const oldsize = (string) ? string->size : (size_t)0;
    string_t    *newstr;

    newstr = realloc(string, sizeof(string_t) + oldsize + size + 1);
    if (!newstr) {
        if (string) {
            string->size = 0;
            free(string);
        }
        errno = ENOMEM;
        return NULL;
    }

    if (size)
        memcpy(&(newstr->data[oldsize]), data, size);

    newstr->data[oldsize + size] = 0;
    newstr->size = oldsize + size;

    return newstr;
}

/* Open and configure device. Return -1 if an error occurs.
*/
int init_port(char const *const device , int dbg )
{
    int             descriptor, result;
    int             lup_cnt;
    struct termios  settings;

    DEBUG_MSG ( dbg, ">> En %s , device = %s  \n" , __FUNCTION__ , device  );

    if (!device || !*device) {
        printf( "<< LV %s NO deice !! dev = %s \n" , __FUNCTION__ , device );
        errno = EINVAL;
        return -1;
    }

    lup_cnt = 0;
    if ( dbg ) printf( "  lup_cnt = 0 , %s L%d \n" , __FUNCTION__ , __LINE__ );

    do {
        descriptor = open(device, O_RDWR | O_NOCTTY);
        lup_cnt++;
        if ( lup_cnt > 1000 ) printf( " looping on O_NOCTTY !! , cnt = %d\n" , lup_cnt );
    } while (descriptor == -1 && errno == EINTR);
    if (descriptor == -1) {
        printf( "<< LV %s rc = %d \n" , __FUNCTION__ , -1 );
        return -1;
    }

    if ( dbg ) printf( "  %s L%d \n" , __FUNCTION__ , __LINE__ );

    do {
        result = fcntl(descriptor, F_SETFL, O_NONBLOCK);
    } while (result == -1 && errno == EINTR);
    if (result == -1) {
        int const error = errno;
        do {
            result = close(descriptor);
        } while (result == -1 && errno == EINTR);
        errno = error;
        printf( "<< Lv %s , device = %s  Couldn't set NON-BLOCK , L%d \n" , __FUNCTION__ , device  , __LINE__ );
        return -1;
    }

    if ( dbg ) printf( "  %s L%d \n" , __FUNCTION__ , __LINE__ );

    do {
        result = tcgetattr(descriptor, &settings);
    } while (result == -1 && errno == EINTR);
    if (result == -1) {
        int const error = errno;
        do {
            result = close(descriptor);
        } while (result == -1 && errno == EINTR);
        errno = error;
        printf( "<< Lv %s , device = %s  , L%d \n" , __FUNCTION__ , device  , __LINE__ );
        return -1;
    }

    // set RAW mode . see man page 'Raw Mode'
    settings.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                           | INLCR | IGNCR | ICRNL | IXON);
    settings.c_oflag &= ~OPOST;
    settings.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    settings.c_cflag &= ~(CSIZE | PARENB);

    settings.c_cflag |= CS8;
    settings.c_cflag |= CREAD | CLOCAL ; // set after it would no long er open port after a number of serial run tests

    cfsetispeed(&settings, B115200);
    cfsetospeed(&settings, B115200);

    if ( dbg ) printf( "  %s L%d \n" , __FUNCTION__ , __LINE__ );

    do {
        result = tcsetattr(descriptor, TCSANOW, &settings);
    } while (result == -1 && errno == EINTR);
    if (result == -1) {
        int const error = errno;
        do {
            result = close(descriptor);
        } while (result == -1 && errno == EINTR);
        errno = error;
        printf( "<< Lv %s , device = %s  , L%d \n" , __FUNCTION__ , device  , __LINE__ );
        return -1;
    }

    /* Note: the above call may succeed even if only some of
     *       the flags were supported. We could tcgetattr()
     *       into a temporary struct, and compare fields
     *       (masked by the significant modes) to make sure.
    */

    if ( dbg ) printf( "<< Lv %s , device = %s  , L%d \n" , __FUNCTION__ , device  , __LINE__ );
    return descriptor;
}

/* Set char by char mode, noecho */
void set_terminal_raw( void ){
    struct  termios ttystate;
    tcgetattr( 0, &ttystate);               /* read current setting */
    ttystate.c_lflag          &= ~ICANON;   /* no buffering     */
    ttystate.c_lflag          &= ~ECHO;     /* no echo either   */
    ttystate.c_cc[VMIN]        =  1;        /* get 1 char at a time */
    tcsetattr( 0 , TCSANOW, &ttystate);     /* install settings */
}

/*
  pass in 0 to save current mode
  pass in 1 to restore prev mode
*/
void tty_mode( int operation ) {
    static struct termios original_mode;
    if ( operation == 0 )
        tcgetattr( 0, &original_mode );
    else
        tcsetattr( 0, TCSANOW, &original_mode );
  return;
}

int write_port(void const *const data, size_t const size, int port_descriptor )
{
    char const       *p = (char const *)data;
    char const *const q = (char const *)data + size;
    ssize_t           n;

    while (p < q) {
        do {
            n = write( port_descriptor, p, (size_t)(q - p));
        } while (n == (ssize_t)-1 && errno == EINTR);
        if (n == (ssize_t)-1 && errno == EWOULDBLOCK) {
            /* Sleep for a millisecond, then retry. */
            usleep(1000);
            continue;
        }

        if (n == (ssize_t)-1)
            return errno;
        else
        if (n < (ssize_t)1)
            return EIO;

        p += (size_t)n;
    }

    if (p != q)
        return EIO;

    return 0;
}

/* Return number of bytes actually read.
 * If 0, errno will contain the reason.
*/
size_t read_port(void *const data, size_t const size , int port_descriptor , int dbg )
{
    ssize_t           r;

    do {
        r = read(port_descriptor, data, size);
    } while (r == (ssize_t)-1 && errno == EINTR);
    if (r > (ssize_t)0)
        return (size_t)r;

    if (r == (ssize_t)-1)
        return (size_t)0;

    if (r == (ssize_t)0) {
        errno = 0;
        return (size_t)0;
    }

    /* r < -1, should never happen. */
    errno = EIO;
    return (size_t)0;
}

void *reader( int port_descriptor ) {
    char     buffer[512];
    size_t   result;
    char     rxd[128];
    char    *endp, *endpp;

    pstring_t rx_result = NULL;
    int no_rx = 0;
    int dbg   = 1;

    DEBUG_MSG( dbg, "  >> E %s L%d \n" , __FUNCTION__ , __LINE__  );

    while ( g_read_loop ) {
        result = read_port(buffer, sizeof(buffer) , port_descriptor , dbg );

        if (result) {
          rx_result =  add( rx_result, buffer , result) ;
          DEBUG_MSG( dbg, "    rx> '%s' (%d characters).\n", buffer , result );
          endpp = rx_result->data+(rx_result->size-2);
          endp  = rx_result->data+(rx_result->size-1);
          //printf( "rxd endp=%x %c\n" , *endp, *endp );
          if ( *endp == '\n' || *endp == '\r' ) {
            DEBUG_MSG( dbg, "    rxd endpp=%x endp=%x CR=%x LF=%x \n" , *endpp, *endp , '\n' , '\r' );
            DEBUG_MSG( dbg, "    rxd ->%s<-\n" , rx_result->data );
            rx_result->size = 0;
            free ( rx_result );
            rx_result = NULL;
          }
        }

        if (errno == EINTR || errno == EWOULDBLOCK) {
            /* Sleep for a millisec, then retry */
            usleep(1000);
            continue;
        }
      //usleep(100000);  // go back to this ??
      sleep( 0.1 ); // increase or decrease according to how many chars in buffer you want to accumulate
    }

    DEBUG_MSG( dbg, "  << Lv %s L%d \n" , __FUNCTION__ , __LINE__  );

    /* Never reached. */
    return NULL;
}

