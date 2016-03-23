#include "fs.h"

int get_global_entry(global_table* t, int globalID){
    if(globalID>=0){
        if(!t->entries[globalID].active){
            t->entries[globalID].active = 1;
            t->count++;
        }
        return globalID;
    }
    for(int i=0;i<MAXGLOBAL;i++){
        if(!t->entries[i].active){
            t->entries[i].active = 1;
            t->count++;
            return i;
        }
    }
    return -1;
}

void setGlobalEntry(global_table* t, int globalID, i_node inode, char name[], file_type type){
    if(!t->entries[globalID].active)
        return;
    t->entries[globalID].globalID = globalID;
    t->entries[globalID].refcount = 0;
    t->entries[globalID].inode    = inode;
    if(name)
        memcpy(t->entries[globalID].name,name,20);
    else
        t->entries[globalID].name[0] = '\0';
    t->entries[globalID].type = type;
    t->entries[globalID].wqwrite.head = 0;
    t->entries[globalID].wqwrite.tail = 0;
    t->entries[globalID].wqwrite.len  = 0;
    t->entries[globalID].wqread.head  = 0;
    t->entries[globalID].wqread.tail  = 0;
    t->entries[globalID].wqread.len   = 0;
}

void enqueue_wq(global_table* t, int globalID, int queue, int pid){
    if(!t->entries[globalID].active)
        return;
    wait_queue* wq;
    if(queue == 0)
        wq = &t->entries[globalID].wqwrite;
    else
        wq = &t->entries[globalID].wqread;
    if(wq->len == PIPE_WQ)
        return;
    wq->pids[wq->tail] = pid;
    wq->tail = (wq->tail + 1) % PIPE_WQ;
    wq->len++;
}

int dequeue_wq(global_table* t, int queue, int globalID){ //return 0 if empty;
    if(!t->entries[globalID].active)
        return 0;
    wait_queue* wq;
    if(queue == 0)
        wq = &t->entries[globalID].wqwrite;
    else
        wq = &t->entries[globalID].wqread;
    if(wq->len == 0)
        return 0;
    int pid = wq->pids[wq->head];
    wq->head = (wq->head + 1) % PIPE_WQ;
    wq->len--;
    return pid;
}

int close_global_entry(global_table* t, int globalID){ //close pipe or file first
    if(t->entries[globalID].refcount>0)
        return -1;
    if(t->entries[globalID].wqwrite.len>0 || t->entries[globalID].wqread.len>0)
        return -1;
    if(!t->entries[globalID].active)
        return -1;
    t->entries[globalID].active = 0;
    t->entries[globalID].inode  = 0;
    t->entries[globalID].name[0]='\0';
    t->count--;
    return 0;
}

void reset_pipe(pipes_t* pipes, i_node inode){
    pipes->p[inode].buff[0] = '\0';
    pipes->p[inode].readptr = 0;
    pipes->p[inode].writeptr= 0;
    pipes->p[inode].len = 0;

}

i_node get_pipe(pipes_t* pipes,i_node inode){
    if(inode>=0){
        if(!pipes->p[inode].active){
            pipes->p[inode].active = 1;
            pipes->count++;
        }
        return inode;
    }
    for(int i=0;i<MAXPIPES;i++){
        if(!pipes->p[i].active){
            pipes->p[i].active = 1;
            pipes->count++;
            return i;
        }
    }
    return -1;
}

void setPipe(pipes_t* pipes, i_node inode, int globalID){
    reset_pipe(pipes, inode);
    pipes->p[inode].globalID = globalID;
}

void send_pipe(pipes_t* pipes, i_node inode, int n){
    if(!pipes->p[inode].active)
        return;
    //if(isPipeFull(pipes, inode))
    //    return 1;
    pipe_t* p = &pipes->p[inode];
    p->buff[p->writeptr] = n;
    p->writeptr = (p->writeptr + 1) % PIPESIZE;
    p->len++;
}

int consume_pipe(pipes_t* pipes, i_node inode){
    if(!pipes->p[inode].active)
        return 0;
    //if(isPipeEmpty(pipes, inode))
    //    return 1;
    pipe_t* p = &pipes->p[inode];
    int n = p->buff[p->readptr];
    p->readptr = (p->readptr + 1) % PIPESIZE;
    p->len--;
    return n;
}

int isPipeEmpty(pipes_t* pipes, i_node inode){
    if(!pipes->p[inode].active)
        return -1;
    if(pipes->p[inode].len == 0)
        return 1;
    return 0;
}

int isPipeFull(pipes_t* pipes, i_node inode){
    if(!pipes->p[inode].active)
        return -1;
    if(pipes->p[inode].len == PIPESIZE)
        return 1;
    return 0;
}

void close_pipe(pipes_t* pipes, i_node inode){
    if(!pipes->p[inode].active)
        return;
    reset_pipe(pipes, inode);
    pipes->p[inode].active = 0;
    pipes->count--;
}

