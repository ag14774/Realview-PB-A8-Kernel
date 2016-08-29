#include "hashtable_t.h"


uint32_t hash_fun(pid_t pid){
    return ((uint32_t)(282563095*pid+29634029) % 993683819) % HT_SIZE ; //best so far
}

int insert_ht(hashtable_t* ht, pid_t pid, uint32_t cpsr, uint32_t pc,
               uint32_t sp, stack_info si, uint8_t priority, uint32_t vruntime) {
    if(ht->processes == MAX_PROC)
        return -2;
    uint32_t index = hash_fun(pid);
    uint32_t j = 0;
    while(ht->pcbs[index][j].pid != 0 && j<HT_BUCKET){
        j = j + 1;
    }
    if(j==HT_BUCKET)
        return -1;
    pcb_t* pcb = &ht->pcbs[index][j];
    memset(pcb, 0, sizeof(pcb_t) );
    pcb->pid = pid;
    pcb->ctx.cpsr    = cpsr;
    pcb->ctx.pc      = pc;
    pcb->ctx.sp      = sp;  //consider getting this info from si
    pcb->stack       = si;
    pcb->priority    = priority;
    pcb->vruntime    = vruntime; //consider removing this and setting it to default 0
    pcb->queue_index = -1;
    pcb->proc_state  = CREATED;
    ht->processes = ht->processes + 1;
    
    //add to keyset
    add_key(&ht->keyset, pcb);
    
    return 0;
}

int insert_ht_by_pointer(hashtable_t* ht, pid_t pid, stack_info si, pcb_t* p){ //returns -1 if bucket is full, -2 if above MAX_PROC
    if(ht->processes == MAX_PROC)
        return -2;
    uint32_t index = hash_fun(pid);
    uint32_t j = 0;
    while(ht->pcbs[index][j].pid != 0 && j<HT_BUCKET){
            j = j + 1;
    }
    if(j==HT_BUCKET)
        return -1;
    pcb_t* pcb = &ht->pcbs[index][j];

    memcpy(pcb, p, sizeof(pcb_t) );
    pcb->pid = pid;
    memset(&pcb->ph,0,sizeof(proc_hierarchy));
    add_child(p, pcb); //(parent,child)
    pcb->stack       = si;
    pcb->ctx.sp      = p->ctx.sp - p->stack.tos + si.tos; //correct stack pointer
    pcb->vruntime    = 0;
    pcb->queue_index = -1;
    pcb->proc_state  = CREATED;
    ht->processes    = ht->processes + 1;
    
    //add to keyset
    add_key(&ht->keyset, pcb);
    
    return 0;
}


int delete_ht(hashtable_t* ht, pid_t pid) { //returns -1 if unsuccessful
    pcb_t* pcb = find_pid_ht(ht, pid);      //returns -2 if scheduled
    if(!pcb)
        return -1;
    if(pcb->queue_index != -1)
        return -2;
    remove_children(pcb);
    remove_child(pcb);
    remove_key(&ht->keyset, pcb);
    memset(pcb, 0, sizeof(pcb_t) ); //setting pid to 0 might be faster
    ht->processes = ht->processes - 1;
    return 0;
}
pcb_t* find_pid_ht(hashtable_t* ht, pid_t pid) { //return null if not found
    if(pid<1)
        return NULL;
    uint32_t index = hash_fun(pid);
    uint32_t j = 0;
    for(j=0;j<HT_BUCKET;j++){
        if(ht->pcbs[index][j].pid == pid)
            return &ht->pcbs[index][j];
    }
    return NULL;
}

void add_key(keyset_t* keyset, pcb_t* p){
    if(keyset->head == NULL && keyset->tail == NULL){
        keyset->head = p;
        keyset->tail = p;
        p->prev = NULL;
        p->next = NULL;
    }
    else{
        keyset->tail->next = p;
        p->prev = keyset->tail;
        keyset->tail = p;
        p->next = NULL;
    }  
}

void remove_key(keyset_t* keyset, pcb_t* p){
    if(p == keyset->head)
        keyset->head = p->next;
    if(p == keyset->tail)
        keyset->tail = p->prev;
    if(p->prev)
        p->prev->next = p->next;
    if(p->next)
        p->next->prev = p->prev;
}
