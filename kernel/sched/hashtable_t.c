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
    pcb->keyindex = add_key(&ht->keyset,pid);
    
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
    pcb->keyindex = add_key(&ht->keyset,pid);
    
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
    remove_key(&ht->keyset,(pidkey_t*)pcb->keyindex);
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

pidkey_t* add_key(keyset_t* keyset, pid_t pid){
    pidkey_t* key = malloc(sizeof(pidkey_t));
    if(!key)
        return NULL;
    key->pid = pid;
    if(keyset->head == NULL && keyset->tail == NULL){
        keyset->head = key;
        keyset->tail = key;
        key->prev = NULL;
        key->next = NULL;
    }
    else{
        keyset->tail->next = key;
        key->prev = keyset->tail;
        keyset->tail = key;
        key->next = NULL;
    }  
}

void remove_key(keyset_t* keyset, pidkey_t* key){
    if(key==keyset->head)
        keyset->head = key->next;
    if(key==keyset->tail)
        keyset->tail = key->prev;
    if(key->prev)
        key->prev->next = key->next;
    if(key->next)
        key->next->prev = key->prev;
    free(key);
}
