#ifndef __GENPRIMES_H
#define __GENPRIMES_H

#include <stddef.h>
#include <stdint.h>

#include "libc.h"

extern int (*entry_genPrimes)(int argc, char** argv); 
extern uint32_t tos_genPrimes;

#endif
