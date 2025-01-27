#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>

#include <errno.h>

/* Open and configure device. Return -1 if an error occurs.
*/
int init_port(char const *const device , int dbg )
{
    int             descriptor, result;
    int             lup_cnt;
    struct termios  settings;

    if ( dbg ) printf( ">> En %s , device = %s  \n" , __FUNCTION__ , device  );

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

