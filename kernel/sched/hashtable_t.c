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
    add_child(ht, p, pid); //(ht,parent,childpid)
    pcb->stack       = si;
    pcb->ctx.sp      = p->ctx.sp - p->stack.tos + si.tos; //correct stack pointer
    pcb->vruntime    = 0;
    pcb->queue_index = -1;
    pcb->proc_state  = CREATED;
    ht->processes    = ht->processes + 1;
    return 0;
}


int delete_ht(hashtable_t* ht, pid_t pid) { //returns -1 if unsuccessful
    pcb_t* pcb = find_pid_ht(ht, pid);      //returns -2 if scheduled
    if(!pcb)
        return -1;
    if(pcb->queue_index != -1)
        return -2;
    pcb_t* parent = find_pid_ht(ht, pcb->ph.parent);
    remove_child(ht, parent, pid); //pid is the pid of child. all of its children will set their parent to 1, and parent will remove pid form its children
    memset(pcb, 0, sizeof(pcb_t) ); //setting pid to 0 might be faster
    ht->processes = ht->processes - 1;
    return 0;
}
pcb_t* find_pid_ht(hashtable_t* ht, pid_t pid) { //return null if not found
    uint32_t index = hash_fun(pid);
    uint32_t j = 0;
    for(j=0;j<HT_BUCKET;j++){
        if(ht->pcbs[index][j].pid == pid)
            return &ht->pcbs[index][j];
    }
    return NULL;
}

//THIS IS NOT NICE. MOVE TO PCB_T_H AND USE A BETTER DATA STRUCTURE
int add_child(hashtable_t* ht, pcb_t* parent, pid_t child){
    pcb_t* child_pcb = find_pid_ht(ht, child);
    child_pcb->ph.parent = parent->pid;
    for(int i = 0;i<MAX_CHILD;i++){
        if(parent->ph.children[i] == 0){
            parent->ph.children[i] = child;
            return 0;
        }
    }
    return -1;
}

int remove_child(hashtable_t* ht, pcb_t* parent, pid_t child){
    pcb_t* child_pcb = find_pid_ht(ht, child);
    pcb_t* childschild;
    for(int i = 0;i<MAX_CHILD;i++){
        if(child_pcb->ph.children[i] != 0)
        childschild = find_pid_ht(ht, child_pcb->ph.children[i]);
        childschild->ph.parent = 1; //make everyone's pid = 1
    }
    for(int i = 0;i<MAX_CHILD;i++){
        if(parent->ph.children[i] == child){
            parent->ph.children[i] = 0;
            return 0;
        }
    }
    return -1;
}

