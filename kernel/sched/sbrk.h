#ifndef SBRK_H
#define SBRK_H

#include <sys/types.h>
#include <stdint.h>
extern uint32_t boh;

caddr_t _sbrk( int incr );

#endif
