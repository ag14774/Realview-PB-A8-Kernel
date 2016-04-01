#ifndef __FS_H
#define __FS_H

#include <stdint.h>
#include <string.h>

#include "disk.h"

#define MAXPIPES  50
#define MAXGLOBAL 50
#define PIPESIZE  500
#define PIPE_WQ   15

#define MAX_DIRECT_BLOCKS 15
#define MAX_BLOCK_SIZE 128
#define CACHE_SIZE 128
#define CACHE_BUCKET 5

#define ACTIVE_INODE_HT 50
#define ACTIVE_INODE_BUCKET 5

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
    uint8_t active;
    i_node inode;
    inode_t disk_inode;
} inode_entry;

typedef struct{
    inode_entry entries[ACTIVE_INODE_HT][ACTIVE_INODE_BUCKET];
} active_inode_ht;

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
    i_node inode;
    char buff[PIPESIZE];
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

typedef struct cache_entry{
    uint8_t active;
    uint32_t block_addr;
    uint8_t data[MAX_BLOCK_SIZE];
    struct cache_entry* next;
    struct cache_entry* prev;
} cache_entry;

typedef struct{
    cache_entry entries[CACHE_SIZE][CACHE_BUCKET];
    cache_entry* head;
    cache_entry* tail;
} cache_ht;

int get_global_entry(global_table* t, int globalID); //if globalID negative, then return the first that is free
void setGlobalEntry(global_table* t, int globalID, i_node inode, char name[], file_type type);
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
void create_dir(i_node parent, const char* name);

inode_entry* find_active_inode(i_node inode);

#endif
