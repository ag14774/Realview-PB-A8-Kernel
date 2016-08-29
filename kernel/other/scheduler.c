#include "scheduler.h"

#define NORM_THRES 0x80000000

pcb_t* current = NULL;
queue_t q;
hashtable_t ht;
user_stack us;
input_buff ib;
pipes_t pipes;
global_table filetable;

hashtable_t* ht_ptr;
input_buff* ib_ptr;
pipes_t* pipes_ptr;
global_table* filetable_ptr;

void unblock_by_pid(pid_t pid){
    pcb_t* p = find_pid_ht(&ht, pid);
    if(p->proc_state == BLOCKED){
        int operation  = p->block_info.operation;
        int globalID   = p->fdtable.fd[p->block_info.fd].globalID;
        if(operation>=0 && globalID>=0){
            file_type type = filetable_ptr->entries[globalID].type;
            switch(type){
                case stdio: //nothing to do here
                    break;
                case pipe:
                    remove_wq(filetable_ptr, globalID, operation, pid);
                    break;
                case file: //nohing to do here
                    break;
                default:
                    break;
            }
        }
        p->proc_state = WAITING;
    }
    if(p->queue_index!=-1){
        p->proc_state = READY;
        return;
    }
    schedule(p->pid);
}

//this function will never be called
void unblock_first(){ //It might need to add check for WAITING
    keyset_t* keys = &ht.keyset;
    pcb_t* temp = keys->head;
    while(temp){
        if(temp->proc_state == WAITING || temp->proc_state == BLOCKED){
            unblock_by_pid(temp->pid);
            return;
        }
        temp = temp->next;
    }
}

int block_pid(pid_t pid){
    pcb_t* p = find_pid_ht(&ht, pid);
    if(p){
        deschedule(pid);
        if(pid == current->pid)
            current = NULL;
        return 0;
    }
    return -1;
}

void scheduler(ctx_t* ctx){ //signal normalisation if normalisation_threshold exceeded
    if(current){
        update_ctx(current, ctx);
        if(current->proc_state == BLOCKED || current->proc_state == WAITING)
            block_pid(current->pid);
        else{
            update_runtime(current);
            insert(&q, current);
        }
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
                unblock_first();
            }
        }
        else if(current->proc_state == BLOCKED || current->proc_state == WAITING){
            block_pid(current->pid);
            current = NULL;
        }
    }
    current->proc_state = RUNNING;
    if(!current->ph.parent)
        add_child(find_pid_ht(ht_ptr, 1), current);
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
    ht.keyset.head = NULL;
    ht.keyset.tail = NULL;
    ht_ptr = &ht;
    ib_ptr = &ib;
    pipes_ptr = &pipes;
    filetable_ptr = &filetable;
}

int schedule(pid_t pid){ //returns -1 if unsuccessful
    pcb_t* pcb = find_pid_ht(&ht, pid);
    if(!pcb || pcb->queue_index!=-1)
        return -1; //pid does not exist in hashtable or already scheduled
    if(pcb->vruntime < q.min_vruntime) //check if ten times more than the minimum
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
    //pcb_t* p = find_pid_ht(&ht, pid);
    //int stdinfd = getFileDes(p);
    //int globalfd = get_global_entry(&filetable);
    //filetable->entries[globalfd].refcount++;
    //setFileDes(pcb_t* pcb, int fd, int globalID, int flags, int rwpointer)
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
    if(pcb->queue_index != -1) //if scheduled, deschedule it.
        deschedule(pid);
    if(current->pid == pid)
        current = NULL;
    declare_free(&us, pcb->stack.tos);
    int error = delete_ht(&ht, pid);
    return error;
}

void normalise_vruntimes(){
    int i;
    if(q.min_vruntime<NORM_THRES)
        return;
    pcb_t* temp = ht.keyset.head;
    while(temp){
        temp->vruntime = temp->vruntime - q.min_vruntime;
        if(temp->vruntime<0)
            temp->vruntime = 0;
        temp = temp->next;
    }
    q.min_vruntime = 0;
}
