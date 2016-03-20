#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "priority_queue.h"
#include "hashtable_t.h"
#include "free_stack.h"
#include "input_buffer.h"

typedef struct node{
    struct node* next;
    pcb_t* addr;
} node;

typedef struct{
    node* head;
    node* tail;
} list_t;

void  scheduler(ctx_t* ctx);
int   schedule(pid_t pid); //returns -1 if unsuccessful
int   deschedule(pid_t pid); //returns -1 if unsuccessful
void  initialise_scheduler(uint32_t init_stack);
void  normalise_vruntimes();
pid_t init_pcb(uint32_t pc); //return -2 if out of space!
pid_t init_pcb_by_pointer(pcb_t* pcb);
int   destroy_pid(pid_t pid);

void  unblock_by_pid(pid_t pid);
void  unblock_head();
int   block_pid(pid_t pid);

#endif
