#include "scheduler.h"

#define NORM_THRES 0x80000000

pcb_t* current = NULL;
pcb_t* blocking[MAX_BLOCKING];
queue_t q;
hashtable_t ht;
hashtable_t* ht_ptr;
user_stack us;
input_buff ib;
input_buff* ib_ptr;

void unblock_by_index(uint32_t index){
    pcb_t* blocked = blocking[index];
    blocking[index] = NULL;
    schedule(blocked->pid);
}

void unblock_by_pid(pid_t pid){
    for(int i=0;i<MAX_BLOCKING;i++){
        if(blocking[i]->pid == pid){
            unblock_by_index(i);
            return;
        }
    }
}

void unblock_random(){ //It might need to add check for WAITING
    for(int i=0;i<MAX_BLOCKING;i++){
        if(blocking[i]!=NULL){
            unblock_by_index(i);
            return;
        }
    }
}

void scheduler(ctx_t* ctx){ //signal normalisation if normalisation_threshold exceeded
    if(current){
        update_ctx(current, ctx);
        update_runtime(current);
        insert(&q, current);
        normalise_vruntimes();
    }
    current = NULL;
    while(!current){
        current = extract_min(&q);
        if(!current){
            if(ib.pid){
                unblock_by_pid(ib.pid);
            }
            else {
                unblock_random();
            }
        }
    }
    current->proc_state = RUNNING;
    restore_ctx(ctx,current);
}

void initialise_scheduler(uint32_t init_stack){ //init all data structures here
    q.alloc_size = MAX_QUEUE;
    q.processes = 0;
    q.max_pid = 0;
    q.min_vruntime = 0;
    us.len = 0;
    us.tos = init_stack;
    flush_buff(&ib);
    ht_ptr = &ht;
    ib_ptr = &ib;
}

int schedule(pid_t pid){ //returns -1 if unsuccessful
    pcb_t* pcb = find_pid_ht(&ht, pid);
    if(!pcb)
        return -1; //pid does not exist in hashtable
    if(pcb->vruntime == 0)
        pcb->vruntime = q.min_vruntime;
    insert(&q, pcb);
    return 0;
}

int deschedule(pid_t pid){ //returns -1 if unsuccessful
    pcb_t* pcb = find_pid_ht(&ht, pid);
    if(!pcb)
        return -1; //pid does not exist in hashtable
    int error = 0;
    error = dequeue(&q, pcb);
    return error;
}

pid_t init_pcb(uint32_t pc){
    pid_t pid = ++q.max_pid;
    uint32_t cpsr = 0x50;
    uint8_t priority = MIN_PRIORITY;
    uint32_t vruntime = 0; //this should be set when 'schedule' is called
    uint32_t sp = get_next_stack(&us);
    stack_info si;
    si.tos = sp;
    si.size = 0x1000;
    int error = 0;
    while(error = insert_ht(&ht, pid, cpsr, pc, sp, si, priority, vruntime) == -1){
        pid = ++q.max_pid; //conflict and bucket was full. increase pid.
    }
    if(error == -2)
        return -2; //-2 out of space!
    return pid;
}

pid_t init_pcb_by_pointer(pcb_t* pcb){
    if(!pcb)
        return 0;
    stack_info si;
    pid_t pid = ++q.max_pid;
    si.tos = replicate_stack(&us, pcb->stack.tos);
    si.size = pcb->stack.size;
    int error = 0;
    while(error = insert_ht_by_pointer(&ht, pid, si, pcb) == -1){
        pid = ++q.max_pid;
    }
    if(error == -2)
        return -2; //-2 out of space
    return pid;
}

int destroy_pid(pid_t pid){
    pcb_t* pcb = find_pid_ht(&ht, pid);
    if(!pcb)
        return -1;
    declare_free(&us, pcb->stack.tos);
    int error = delete_ht(&ht, pid);
    return error;
}

void normalise_vruntimes(){
    int i;
    if(q.min_vruntime<NORM_THRES)
        return;
    for(i=0;i<q.processes;i++){
        q.pcbs[i]->vruntime = q.pcbs[i]->vruntime - q.min_vruntime;
    }
    q.min_vruntime = 0;
}
