#ifndef PCB_T_H
#define PCB_T_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define MAX_PRIORITY 1
#define MIN_PRIORITY 20
#define MAX_CHILD    10

/* The kernel source code is made simpler by three type definitions:
 *
 * - a type that captures each component of an execution context (i.e.,
 *   processor state) in a compatible order wrt. the low-level handler
 *   preservation and restoration prologue and epilogue,
 * - a type that captures a process identifier, which is basically an
 *   integer, and
 * - a type that captures a process PCB.
 **/

//CREATED: When a new process is spawned
//READY  : Process is in priority queue
//RUNNING: Currently running process
//BLOCKED: Waiting for use input(usually unblocked by read)
//WAITING: Waiting for child to finish
typedef enum {NONE, CREATED, READY, RUNNING, BLOCKED, WAITING} proc_state_t;

typedef struct ctx_t { //DO NOT MODIFY THIS
  uint32_t cpsr, pc, gpr[ 13 ], sp, lr;
} ctx_t;

typedef int pid_t;

typedef struct proc_hierarchy {
  pid_t parent;
  pid_t children[MAX_CHILD];
} proc_hierarchy;

typedef struct stack_info {
  uint32_t tos;
  uint32_t size;
} stack_info;

typedef struct pcb_t {
  pid_t pid;
  ctx_t ctx;
  proc_hierarchy ph;
  stack_info stack;
  proc_state_t proc_state;
  uint8_t priority; //1(highest)-20
  uint32_t vruntime; //vruntime = vruntime + priority
  uint32_t queue_index;
  //maybe add block_index too?
} pcb_t;

void update_runtime(pcb_t *pcb);
void setPriority(pcb_t *pcb, uint8_t priority);
void update_ctx(pcb_t *pcb, ctx_t *ctx);
void restore_ctx(ctx_t *ctx, pcb_t *pcb);

#endif
