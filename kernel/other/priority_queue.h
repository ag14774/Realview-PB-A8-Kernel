#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include "pcb_t.h"
#define MAX_QUEUE 50

typedef struct queue_t{
    pcb_t *pcbs[MAX_QUEUE];
    int alloc_size; //This could be used to keep track of size when dynamically allocated.
    int processes;
    pid_t max_pid;
    uint32_t min_vruntime;
} queue_t;

void insert(queue_t* q, pcb_t* p); 
pcb_t* extract_min(queue_t* q);    //return NULL if q is empty
int dequeue(queue_t* q, pcb_t* p); //return -1 on error

#endif
