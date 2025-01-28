#ifndef _BASE_SERIAL_H

#define _BASE_SERIAL_H

typedef struct string string_t;

typedef struct string {
    size_t  size;
    char    data[];
} string_t , * pstring_t ;

extern int    init_port(char const *const device , int dbg );

extern string_t *add(string_t *string, void const *const data, size_t const size);
extern void      set_terminal_raw( void );
extern void      tty_mode( int operation );
extern void     *reader( void *arg );

extern int       write_port(void const *const data, size_t const size, int port_descriptor );
extern size_t    read_port(void *const data, size_t const size , int port_descriptor , int dbg );


#endif
