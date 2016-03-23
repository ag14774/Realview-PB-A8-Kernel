#ifndef __FS_H
#define __FS_H

#include <stdint.h>
#include <string.h>

#define MAXPIPES  50
#define MAXGLOBAL 50
#define PIPESIZE  500
#define PIPE_WQ   15

typedef int i_node;

typedef enum {stdio, pipe, file} file_type;

typedef struct{
    int head;
    int tail;
    int pids[PIPE_WQ];
    int len;
} wait_queue;

typedef struct{
    i_node inode;
    int buff[PIPESIZE];
    int readptr;
    int writeptr;
    int len;
    int globalID;
    uint8_t active;
} pipe_t;

typedef struct{
    pipe_t p[MAXPIPES];
    int count;
} pipes_t;

typedef struct{
    int globalID;
    int refcount;
    uint8_t active;
    i_node inode;
    char name[20];
    file_type type;
    wait_queue wqwrite;
    wait_queue wqread;
} global_entry;

typedef struct{
    int count;
    global_entry entries[MAXGLOBAL];
} global_table;

int get_global_entry(global_table* t, int globalID); //if globalID negative, then return the first that is free
void setGlobalEntry(global_table* t, int globalID, i_node inode, char name[], file_type type);
void enqueue_wq(global_table* t, int globalID, int queue, int pid); //queue 0 is write, read otherwise
int dequeue_wq(global_table* t, int globalID, int queue); //queue 0 is write, read otherwise. return 0 if empty;
int close_global_entry(global_table* t, int globalID); //make sure that the pipe or file is closed

void reset_pipe(pipes_t* pipes, i_node inode);
i_node get_pipe(pipes_t* pipes, i_node inode); //if inode is negative, return the first that is free
void setPipe(pipes_t* pipes, i_node inode, int globalID);
void send_pipe(pipes_t* pipes, i_node inode, int n);
int consume_pipe(pipes_t* pipes, i_node inode);
int isPipeEmpty(pipes_t* pipes, i_node inode);
int isPipeFull(pipes_t* pipes, i_node inode);
void close_pipe(pipes_t* pipes, i_node inode);

#endif
