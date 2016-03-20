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
    child_t* newchild = malloc(sizeof(child_t));
    if(!newchild)
        return;
    newchild->caddr = child;
    newchild->next = NULL;
    if(parentph->children.head == NULL){
        parentph->children.head = newchild;
    }
    else {
        parentph->children.tail->next = newchild;
    }
    parentph->children.tail = newchild;
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
    childph->parent = NULL;
    child_t* temp = parentph->children.head;
    child_t* prev = NULL;
    while(temp){
        if(temp->caddr == child){
            if(parentph->children.head == temp){
                parentph->children.head = temp->next;
            }
            if(parentph->children.tail == temp){
                parentph->children.tail = prev;
            }
            if(prev){
                prev->next = temp->next;
            }
            free(temp);
            return;
        }
        prev = temp;
        temp = temp->next;
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
    child_t* temp = parentph->children.head;
    while(temp){
        child_t* temp2 = temp;
        temp = temp->next;
        temp2->caddr->ph.parent = NULL;
        free(temp2);
    }
    parentph->children.head = NULL;
    parentph->children.tail = NULL;
}

int isChildOf(pcb_t* parent, pcb_t* child){
    if(child->ph.parent == parent)
        return 1;
    return 0;
}


fcb_entry* find_fcb_entry(pcb_t* pcb, int fd){
    return &pcb->fcb.entries[fd];
}

fcb_entry* find_fcb_empty(pcb_t* pcb){
    int fd = get_free_fd(&pcb->fcb.alloc);
    fcb_entry* entry = &pcb->fcb.entries[fd];
    entry->local_fd = fd;
    //entry->global_fd = -1;
    entry->offset = 0;
    return entry;
}

void destroy_fcb(pcb_t* pcb, int fd){
    fcb_entry* entry = find_fcb_entry(pcb, fd);
    declare_free_fd(&pcb->fcb.alloc, fd);
    entry->local_fd = -1;
    entry->global_fd = -1;
    entry->offset = 0;
}

fcb_entry* create_fcb(pcb_t* pcb, int global_fd){
    fcb_entry* entry = find_fcb_empty(pcb);
    entry->global_fd = global_fd;
    return entry;
}

