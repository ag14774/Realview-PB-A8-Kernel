#include "fd.h"


void init_fd_system(fd_allocator_t* fd_allocator){
    fd_allocator->free_fd_ptr = -1;
}

int get_free_fd(fd_allocator_t* fd_allocator){
    int fd;
    if(fd_allocator->free_fd_ptr>=0){
        fd = fd_allocator->free_fd[fd_allocator->free_fd_ptr--];
        return fd;
    }
    fd = fd_allocator->next_fd++;
    return fd;
}

void declare_free_fd(fd_allocator_t* fd_allocator, int fd){
    fd_allocator->free_fd[++fd_allocator->free_fd_ptr] = fd;
}

