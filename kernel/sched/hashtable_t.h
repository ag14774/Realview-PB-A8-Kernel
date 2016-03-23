#ifndef HASHTABLE_T_H
#define HASHTABLE_T_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include "pcb_t.h"

#define HT_SIZE 50
#define HT_BUCKET 4
#define MAX_PROC  50

typedef struct {
    pcb_t* head;
    pcb_t* tail;
} keyset_t;

typedef struct {
    pcb_t pcbs[HT_SIZE][HT_BUCKET];
    int processes;
    keyset_t keyset;
} hashtable_t;

//returns -1 if bucket is full, -2 if above MAX_PROC
int insert_ht(hashtable_t* ht, pid_t pid, uint32_t cpsr, uint32_t pc,
               uint32_t sp, stack_info si, uint8_t priority, uint32_t vruntime);

int insert_ht_by_pointer(hashtable_t* ht, pid_t pid, stack_info si, pcb_t* p); //returns -1 if bucket is full, -2 if above MAX_PROC

//returns -1 if not found, -2 if in queue
int delete_ht(hashtable_t* ht, pid_t pid);

//return null if not found
pcb_t* find_pid_ht(hashtable_t* ht, pid_t pid);

void add_key(keyset_t* keyset, pcb_t* p);
void remove_key(keyset_t* keyset, pcb_t* p);

#endif
