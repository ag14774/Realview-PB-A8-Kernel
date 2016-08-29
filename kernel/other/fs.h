#ifndef __FS_H
#define __FS_H

#include <stdint.h>
#include <string.h>

#include "disk.h"

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define MAXPIPES  50
#define MAXGLOBAL 50
#define PIPESIZE  500
#define PIPE_WQ   15

#define MAX_DIRECT_BLOCKS 63
#define MAX_BLOCK_SIZE 128
#define CACHE_SIZE 128
#define CACHE_BUCKET 5

typedef int i_node;

typedef enum {stdio, pipe, file} file_type;

typedef struct{
    char magic[4];
    uint32_t block_bitmap_start;
    uint32_t inode_start;
    uint32_t data_start;
} superblock_t;

typedef uint8_t i_used;
typedef uint8_t i_mode;
typedef uint16_t i_size;
typedef uint32_t i_block;

typedef struct{
    i_used used; //1 byte
    i_mode mode; //1 bytes //0 is directory, 1 is file
    i_size size; //2 bytes
    i_block blocks[MAX_DIRECT_BLOCKS]; //4 bytes each
}inode_t;

typedef struct{
    i_node inode;
    char name[12];
}dentry_t;

typedef struct{
    int head;
    int tail;
    int pids[PIPE_WQ];
    int len;
} wait_queue;

typedef struct{
    int pid1;
    int pid2;
    char flag; //sync sets this to 1
    char flag2;
}sync_data;

typedef struct{
    i_node inode;
    char buff[PIPESIZE];
    sync_data sync;
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
    file_type type;
    wait_queue wqwrite;
    wait_queue wqread;
    int deleteRequested; //used by unlink
    i_node parent_inode; //used by unlink
} global_entry;

typedef struct{
    int count;
    global_entry entries[MAXGLOBAL];
} global_table;

typedef struct cache_entry{
    uint8_t active;
    uint32_t block_addr;
    uint8_t data[MAX_BLOCK_SIZE];
    uint8_t dirty;
    struct cache_entry* next;
    struct cache_entry* prev;
} cache_entry;

typedef struct{
    cache_entry entries[CACHE_SIZE][CACHE_BUCKET];
    cache_entry* head;
    cache_entry* tail;
} cache_ht;


int get_global_entry(global_table* t, int globalID); //if globalID negative, then return the first that is free
void setGlobalEntry(global_table* t, int globalID, i_node inode, file_type type);
int find_globalID_by_inode(global_table* t, i_node inode, file_type type);
void enqueue_wq(global_table* t, int globalID, int queue, int pid); //queue 0 is write, read otherwise
void remove_wq(global_table* t, int globalID, int queue, int pid);
int dequeue_wq(global_table* t, int globalID, int queue); //queue 0 is write, read otherwise. return 0 if empty;
int close_global_entry(global_table* t, int globalID); //make sure that the pipe or file is closed

void reset_pipe(pipes_t* pipes, i_node inode);
i_node get_pipe(pipes_t* pipes, i_node inode); //if inode is negative, return the first that is free
void setPipe(pipes_t* pipes, i_node inode, int globalID);
void send_pipe(pipes_t* pipes, i_node inode, char n);
char consume_pipe(pipes_t* pipes, i_node inode);
int isPipeEmpty(pipes_t* pipes, i_node inode);
int isPipeFull(pipes_t* pipes, i_node inode);
void close_pipe(pipes_t* pipes, i_node inode);
void insertSyncPid(pipes_t* pipes, i_node inode, int pid);
void clearSyncPid(pipes_t* pipes, i_node inode, int pid);
int getOtherSyncPid(pipes_t* pipes, i_node inode, int pid);

void empty_cache();
void empty_cache_block(uint32_t block_addr, int write2disk);
uint32_t cache_block(uint32_t block_addr, uint8_t force);
cache_entry* find_cache_entry(uint32_t block_addr);
void disk_wr_byte(uint32_t addr, uint8_t b);
uint8_t disk_rd_byte(uint32_t addr);

int load_sb(); //0 if no format needed
void format_disk();
void set_block_bit(uint32_t block_addr);
void clear_block_bit(uint32_t block_addr);
uint8_t get_block_bit(uint32_t block_addr);
void write_inode_field(i_node inode, uint32_t field, uint32_t data);
uint32_t read_inode_field(i_node inode, uint32_t field);
i_node get_free_inode();
i_node get_free_block();
void write_file(i_node inode, uint32_t offset, uint8_t b);
uint8_t read_file(i_node inode, uint32_t offset);
void clear_file(i_node inode);
int delete_dentry(i_node parent, i_node inode);
int create_dentry(i_node parent, const char* name, int dir, i_node inode);
int getdentries(i_node parent, dentry_t* arr);
i_node find_file(i_node parent, const char* name);
int find_file_by_inode(i_node parent, i_node find, char* name);//name will hold the name of inode found
i_node parse_path(char* path, i_node start);
void clear_file_cache(i_node inode);

#endif
