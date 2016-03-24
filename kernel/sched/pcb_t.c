#include "pcb_t.h"

void update_runtime(pcb_t *pcb){
    uint8_t priority = pcb->priority;
    uint32_t vruntime = pcb->vruntime;
    vruntime = vruntime + priority;
    pcb->vruntime = vruntime;
}

void setPriority(pcb_t *pcb, uint8_t priority){
    if(priority<MAX_PRIORITY)
        priority = MAX_PRIORITY;
    if(priority>MIN_PRIORITY)
        priority = MIN_PRIORITY;
    pcb->priority = priority;
}

void update_ctx(pcb_t *pcb, ctx_t *ctx){
    memcpy(&pcb->ctx, ctx, sizeof(ctx_t));
}

void restore_ctx(ctx_t *ctx, pcb_t *pcb){
    memcpy(ctx, &pcb->ctx, sizeof(ctx_t));
}

void add_child(pcb_t* parent, pcb_t* child){
    proc_hierarchy* parentph = &parent->ph;
    proc_hierarchy* childph = &child->ph;
    childph->parent = parent;
    childph->sibling = NULL;
    if(parentph->children.head == NULL){
        parentph->children.head = child;
    }
    else {
        parentph->children.tail->ph.sibling = child;
    }
    parentph->children.tail = child;
}

/*
*         *
*        / \ -> cuts this
*       *   *-> child 
*      / \ / \
*     *  * *  *
*/
void remove_child(pcb_t* child){
    pcb_t* parent = child->ph.parent;
    if(!parent)
        return;
    proc_hierarchy* parentph = &parent->ph;
    proc_hierarchy* childph  = &child->ph;
    childph->parent  = NULL;
    pcb_t* temp = parentph->children.head;
    pcb_t* prev = NULL;
    while(temp){
        if(temp == child){
            if(parentph->children.head == temp){
                parentph->children.head = temp->ph.sibling;
            }
            if(parentph->children.tail == temp){
                parentph->children.tail = prev;
            }
            if(prev){
                prev->ph.sibling = temp->ph.sibling;
            }
            temp->ph.sibling = NULL;
            return;
        }
        prev = temp;
        temp = temp->ph.sibling;
    }
}

/*
*         *
*        / \
*       *   *  -> parent
*      / \ / \ -> cuts these
*     *  * *  *
*/
void remove_children(pcb_t* parent){
    proc_hierarchy* parentph = &parent->ph;
    pcb_t* temp = parentph->children.head;
    while(temp){
        pcb_t* temp2 = temp;
        temp = temp->ph.sibling;
        temp2->ph.parent = NULL;
        temp2->ph.sibling = NULL;
    }
    parentph->children.head = NULL;
    parentph->children.tail = NULL;
}

int isChildOf(pcb_t* parent, pcb_t* child){
    if(child->ph.parent == parent)
        return 1;
    return 0;
}


int  getFileDes(pcb_t* pcb, int fd){ //returns filedes
    if(fd>=0){
        if(!pcb->fdtable.fd[fd].active){
            pcb->fdtable.fd[fd].active = 1;
            pcb->fdtable.count++;
        }
        return fd;
    }
    for(int i=0;i<MAXFD;i++){
        if(!pcb->fdtable.fd[i].active){
            pcb->fdtable.fd[i].active = 1;
            pcb->fdtable.count++;
            return i;
        }
    }
    return -1;
}

void setFileDes(pcb_t* pcb, int fd, int globalID, int flags, int rwpointer){
    if(!pcb->fdtable.fd[fd].active)
        return;
    pcb->fdtable.fd[fd].fd = fd;
    pcb->fdtable.fd[fd].globalID = globalID;
    pcb->fdtable.fd[fd].flags = flags;
    pcb->fdtable.fd[fd].rwpointer = rwpointer;
}

int  closeFileDes(pcb_t* pcb, int fd){ //returns globalID to be used to decrease globalID count.
    pcb->fdtable.fd[fd].active = 0;
    pcb->fdtable.count--;
    return pcb->fdtable.fd[fd].globalID;
}

