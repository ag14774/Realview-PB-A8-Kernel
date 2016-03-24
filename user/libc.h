#ifndef __LIBC_H
#define __LIBC_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#define SIGCONT 0xA0
#define SIGTERM 0xA1

#define READ_ONLY 0x01
#define WRITE_ONLY 0x02
#define READ_WRITE 0x03
//you can bitwise OR those together
#define CLOSE_ON_EXEC 0x10
#define KEEP_ON_EXEC  0x00

// cooperatively yield control of processor, i.e., invoke the scheduler
void yield();

// write n bytes from x to the file descriptor fd
int write( int fd, void* x, size_t n );

//read max n bytes from fd to x. returns number of actual bytes read.
int read( int fd, void* x, size_t n );

int fork();

void exit();

int exec(const char *path, char *const argv[]);

//Blocks until a child or syscall forces the program to continue
//pid is the next process that is going to be in the foreground
//(only if the calling process is in the foreground)
//if pid is the same as the calling process, then the process
//blocks until a syscall unblocks it
//return child pid or 0 if interrupted
//in some sense, it is a mix of waitpid() and ioctl()
int waitpid(int pid);

char* itoa(int i, char b[]);

int printF(const char* f,...);

int read_line(char b[], size_t array_size);

int procs(void* x);

int procstat(int p);

void kill(int pid, int signal);

void nice(int pid, int priority);

int getpipe();

int fcntl(int fd, int flags);

int redir(int from, int to);

void close(int fd);

#endif
