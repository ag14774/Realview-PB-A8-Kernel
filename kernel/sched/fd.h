#ifndef FD_H
#define FD_H

#define MAX_FILES 20

typedef enum {STDIO, PROC, DISK} fd_type;
typedef enum {NON_BLOCKING, BLOCKING} block_t;

typedef enum {READ, WRITE, BOTH} read_write_t;

typedef struct{
    int global_fd;
    fd_type type;
    block_t block;
    char filename[20];
    void* addr;
    int waiting[10];//replace with max priority queue
    int current;
} file_table_entry;

typedef struct{
    int local_fd;
    int global_fd;
    int offset;
} fcb_entry;

typedef struct{
    int next_fd;
    int free_fd[MAX_FILES];
    int free_fd_ptr;
} fd_allocator_t;

typedef struct{
    fcb_entry entries[MAX_FILES];
    fd_allocator_t alloc;
} fcb_t;

int get_free_fd(fd_allocator_t* fd_allocator);
void declare_free_fd(fd_allocator_t* fd_allocator, int fd);
void init_fd_system(fd_allocator_t* fd_allocator);

#endif
