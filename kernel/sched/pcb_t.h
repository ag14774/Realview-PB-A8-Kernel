#ifndef PCB_T_H
#define PCB_T_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#define MAX_PRIORITY 1
#define MIN_PRIORITY 20

#define MAXFD 15

#define READ_ONLY 0x001
#define WRITE_ONLY 0x002
#define READ_WRITE 0x003

#define CLOSE_ON_EXEC 0x010
#define KEEP_ON_EXEC  0x000

#define KEEP_ON_EXIT  0x100
#define CLOSE_ON_EXIT 0x000

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

typedef struct pcb_t pcb_t;

typedef struct ctx_t { //DO NOT MODIFY THIS
  uint32_t cpsr, pc, gpr[ 13 ], sp, lr;
} ctx_t;

typedef int pid_t;

typedef struct{
    int fd;
    int globalID;
    int flags;
    int rwpointer;
    uint8_t active;
} fd_entry;

typedef struct{
    fd_entry fd[MAXFD];
    int count;
} fdtable_t;

typedef struct {
  pcb_t* head;
  pcb_t* tail;
} children_t;

typedef struct proc_hierarchy {
  pcb_t* parent;
  children_t children;
  pcb_t* sibling;
} proc_hierarchy;

typedef struct stack_info {
  uint32_t tos;
  uint32_t size;
} stack_info;

typedef struct{
    int fd;
    int operation; //0 write, 1 read
} block_info_t;

typedef struct pcb_t {
  pid_t pid;
  ctx_t ctx;
  proc_hierarchy ph;
  stack_info stack;
  proc_state_t proc_state;
  block_info_t block_info;
  fdtable_t fdtable;
  uint8_t priority; //1(highest)-20
  uint32_t vruntime; //vruntime = vruntime + priority
  int queue_index;
  //used by hashtable
  struct pcb_t* prev;
  struct pcb_t* next;
  //*****************
} pcb_t;

void update_runtime(pcb_t *pcb);
void setPriority(pcb_t *pcb, uint8_t priority);
void update_ctx(pcb_t *pcb, ctx_t *ctx);
void restore_ctx(ctx_t *ctx, pcb_t *pcb);
void setBlockInfo(pcb_t* pcb, proc_state_t proc_state, int fd, int operation);

void add_child(pcb_t* parent, pcb_t* child);
void remove_child(pcb_t* child);
void remove_children(pcb_t* parent);
int isChildOf(pcb_t* parent, pcb_t* child);

int  getFileDes(pcb_t* pcb, int fd); //returns filedes. if fd==-1, it returns the first free fd
void setFileDes(pcb_t* pcb, int fd, int globalID, int flags, int rwpointer);
int  closeFileDes(pcb_t* pcb, int fd); //returns globalID to be used to decrease globalID count.


#endif
