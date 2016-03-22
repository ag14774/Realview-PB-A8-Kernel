#ifndef __KERNEL_H
#define __KERNEL_H

#include <stddef.h>
#include <stdint.h>

#include   "GIC.h"
#include "PL011.h"
#include "SP804.h"

#include "interrupt.h"
#include "sched/scheduler.h"

// Include functionality from newlib, the embedded standard C library.

#include <string.h>

// Include definitions relating to the 2 user programs.

#include "shell.h"
#include "P0.h"
#include "P1.h"
#include "P2.h"
#include "genPrimes.h"

#define SIGCONT 0xA0
#define SIGTERM 0xA1


typedef struct{
    const char *name;
    uint32_t entry;
}entry_info_t;

extern uint32_t tos_irq;

#endif
