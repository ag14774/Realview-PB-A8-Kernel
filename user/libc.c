#include "libc.h"

void yield() {
  asm volatile( "svc #0     \n"  );
}

int fork() {
  int r;

  asm volatile( "svc #2     \n"
                "mov %0, r0 \n"
              : "=r" (r)      );

  return r;
}

void exit(int status) {
  asm volatile( "mov r0, %0 \n"
                "svc #3     \n"
              :
              : "r" (status)
              : "r0" );
}

int waitpid(int pid) {
  int r;

  asm volatile( "mov r0, %1 \n"
                "svc #5     \n"
                "mov %0, r0 \n"
              : "=r" (r)
              : "r" (pid)
              : "r0" );

  return r;
}

int write( int fd, void* x, size_t n ) {
  int r;

  asm volatile( "mov r3, #0 \n"
                "mov r0, %1 \n"
                "mov r1, %2 \n"
                "mov r2, %3 \n"
                "svc #1     \n"
                "mov %0, r3 \n" 
              : "=r" (r) 
              : "r" (fd), "r" (x), "r" (n) 
              : "r0", "r1", "r2", "r3" );

  return r;
}

int read( int fd, void* x, size_t n ) {
  int r;

  asm volatile( "mov r3, #0 \n"
                "mov r0, %1 \n"
                "mov r1, %2 \n"
                "mov r2, %3 \n"
                "svc #4     \n"
                "mov %0, r3 \n"
              : "=r" (r)
              : "r" (fd), "r" (x), "r" (n)
              : "r0", "r1", "r2", "r3" );
  
  return r;
}

int procs(void* x){
  int r;
  
  asm volatile( "mov r0, %1 \n"
                "svc #7     \n"
                "mov %0, r0 \n"
              : "=r" (r)
              : "r" (x)
              : "r0");
  
  return r;

}

int procstat(int p){
  int r;
  asm volatile( "mov r0, %1 \n"
                "svc #8     \n"
                "mov %0, r0 \n"
              : "=r" (r)
              : "r" (p)
              : "r0" );
  return r;
}

int exec(const char *path, char *const argv[]){
  int r;

  asm volatile( "mov r0, %1 \n"
                "mov r1, %2 \n"
                "svc #6     \n"
                "mov %0, r0 \n"
              : "=r" (r)
              : "r" (path), "r" (argv)
              : "r0", "r1" );

  return r;
}

void kill(int pid, int signal){
  asm volatile( "mov r0, %0 \n"
                "mov r1, %1 \n"
                "svc #9     \n"
              :  
              : "r" (pid), "r" (signal)
              : "r0", "r1" );
}

void nice(int pid, int priority){
    asm volatile( "mov r0, %0 \n"
                  "mov r1, %1 \n"
                  "svc #10    \n"
                : /* No outputs. */
                : "r" (pid), "r" (priority)
                : "r0", "r1" );
}

int getpipe(){
    int r;

    asm volatile( "svc #11    \n"
                  "mov %0, r0 \n"
                : "=r" (r)
                : 
                : "r0" );

    return r;
}

int fcntl(int fd, int flags){
    int r;

    asm volatile( "mov r0, %1 \n"
                  "mov r1, %2 \n"
                  "svc #12    \n"
                  "mov %0, r0 \n"
                : "=r" (r)
                : "r" (fd), "r" (flags)
                : "r0", "r1" );
    
    return r;
}

int redir(int from, int to){
    int r;

    asm volatile( "mov r0, %1 \n"
                  "mov r1, %2 \n"
                  "svc #13    \n"
                  "mov %0, r0 \n"
                : "=r" (r)
                : "r" (from), "r" (to)
                : "r0", "r1" );

    return r;
}

void close(int fd){
    asm volatile( "mov r0, %0 \n"
                  "svc #14    \n"
                :
                : "r" (fd)
                : "r0" );
}

int getppid(int pid){
    int r;

    asm volatile( "mov r0, %1 \n"
                  "svc #15    \n"
                  "mov %0, r0 \n"
                : "=r" (r)
                : "r" (pid)
                : "r0" );

    return r;
}

int creat(char* path){
    int r;
    asm volatile( "mov r0, %1 \n"
                  "svc #16    \n"
                  "mov %0, r0 \n"
                : "=r" (r)
                : "r" (path)
                : "r0" );
    return r;
}

int open(char* path, int flags){
    int r;
    asm volatile( "mov r0, %1 \n"
                  "mov r1, %2 \n"
                  "svc #17    \n"
                  "mov %0, r0 \n"
                : "=r" (r)
                : "r" (path), "r" (flags)
                : "r0", "r1" );
    return r;
}

int unlink(char* path){
    int r;
    asm volatile( "mov r0, %1 \n"
                  "svc #18    \n"
                  "mov %0, r0 \n"
                : "=r" (r)
                : "r" (path)
                : "r0" );
    return r;
}

int lseek(int fd, int offset, int whence){
    int r;
    asm volatile( "mov r0, %1 \n"
                  "mov r1, %2 \n"
                  "mov r2, %3 \n"
                  "svc #19    \n"
                  "mov %0, r0 \n"
                : "=r" (r)
                : "r" (fd), "r" (offset), "r" (whence)
                : "r0", "r1", "r2" );
    return r;
}

int read_line(char b[], size_t array_size){
  int chars = 0;
  if(array_size > 500)
    array_size = 500;
  chars = read(0, b, array_size);
  if(chars == 500){
    b[chars-1] = '\0';
  }
  else if(b[chars] != '\0'){
    b[chars] = '\0';
  }
  
  return chars;
}

/**
 * A custom printf that only supports %d and %s
 */
int printF(const char* f, ...){
    va_list args;      //Define a list of variables
    int done, num;     //'num' is used to temporarily store integer arguments
    char buffer[500];  //Final message stored here
    char intbuff[15];  //String version of integer argument
    char *s;           //Used as a pointer to intbuff
    size_t len;
    size_t templen;
    uint8_t paramFound;

    //memset(buffer,0,50);
    //memset(intbuff,0,50);
    buffer[0]='\0';

    va_start(args, f);
    len = strlen(f);
    int x = 0;
    for(int i = 0;i<len;i++){
        paramFound = 0;
        if(f[i]=='%'){
            if(i<len-1){
                if(f[i+1]=='d'){
                    paramFound=1;
                    num = va_arg(args,int);
                    s = itoa(num,intbuff);
                    templen = strlen(s);
                    strcat(buffer,s);
                    i++;
                    x = x + templen;
                }
                else if(f[i+1]=='s'){
                    paramFound=1;
                    s = va_arg( args, char*);
                    templen = strlen(s);
                    strcat(buffer,s);
                    i++;
                    x = x + templen;
                }
                else if(f[i+1]=='c'){
                    paramFound=1;
                    num = va_arg(args, int);
                    buffer[x++] = num;
                    i++;
                }
            }
        }
        if(!paramFound){
            buffer[x++] = f[i];
            buffer[x] = '\0';
        }
    }
    buffer[x]='\0';
    len = strlen(buffer);
    done = write(1, buffer, len);
    va_end(args);
    return done;
}

//This is from stackoverflow
char* itoa(int i, char b[]){
    char const digit[] = "0123456789";
    char* p = b;
    if(i<0){
        *p++ = '-';
        i *= -1;
    }
    int shifter = i;
    do{ //Move to where representation ends
        ++p;
        shifter = shifter/10;
    }while(shifter);
    *p = '\0';
    do{ //Move back, inserting digits as u go
        *--p = digit[i%10];
        i = i/10;
    }while(i);
    return b;
}
