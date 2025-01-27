#ifndef _BASE_SERIAL_H

#define _BASE_SERIAL_H

extern int    init_port(char const *const device , int dbg );
extern int    write_port(void const *const data, size_t const size, int port_descriptor );
extern size_t read_port(void *const data, size_t const size , int port_descriptor , int dbg );

#endif
