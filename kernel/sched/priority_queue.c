#include "priority_queue.h"

/**
 * The priority queue is just an array of pointers
 * to PCBs that are stored in another array
 **/

void insert(queue_t* q, pcb_t* p){
    if(q->alloc_size <= q->processes){
        //handle reallocation here
    }
    int i = q->processes;//next empty position
    int j = (i-1)/2;//get parent
    while(i>0 && q->pcbs[j]->vruntime > p->vruntime){
        q->pcbs[i] = q->pcbs[j];
        q->pcbs[i]->queue_index = i;
        i = j;
        j = (i-1)/2;
    }
    q->pcbs[i] = p;
    p->queue_index = i; //where is p stored in the queue? (for fast removal)
    p->proc_state = READY;
    q->processes = q->processes + 1;
    q->min_vruntime = q->pcbs[0]->vruntime; //update min_vruntime
}

int dequeue(queue_t* q, pcb_t* p){
    int i = p->queue_index;
    if(i==-1)
        return -1;
    int j = (i-1)/2;
    while(i>0){
        q->pcbs[i] = q->pcbs[j];
        q->pcbs[i]->queue_index = i;
        i = j;
        j = (i-1)/2;
    }
    q->pcbs[i] = p;
    p->queue_index = i;
    pcb_t* res = extract_min(q);
    if(p == res)
        return 0;
    return -1;
}

pcb_t* extract_min(queue_t* q){
    if(!q->processes)
        return NULL;
    pcb_t *pcb = q->pcbs[0];
    q->processes = q->processes - 1;

    q->pcbs[0] = q->pcbs[q->processes]; //move last element to root
                                        //new last element is at q->len - 1
    q->pcbs[0]->queue_index = 0;

    int i = 0; //where to put the last element that was moved
    while(1){
        int jRight= i*2 + 2;
        int jSmallest = jRight;
        if(jSmallest >= q->processes
         || q->pcbs[jSmallest]->vruntime >= q->pcbs[jSmallest-1]->vruntime){
            jSmallest = jSmallest - 1; //Left
        }
        if(jSmallest >= q->processes){
            jSmallest = i;
        }
        if(i == jSmallest){
            break;
        }
        q->pcbs[i] = q->pcbs[jSmallest];
        q->pcbs[i]->queue_index = i;
        i = jSmallest;
    }
    q->pcbs[i] = q->pcbs[q->processes];
    q->pcbs[i]->queue_index = i;
    if(q->processes != 0)
        q->min_vruntime = q->pcbs[0]->vruntime; //update min_vruntime
    else
        q->min_vruntime = 0;
    pcb->queue_index = -1;
    pcb->proc_state = WAITING; //this will change to RUNNING if proc moves to *current
    return pcb;
}
