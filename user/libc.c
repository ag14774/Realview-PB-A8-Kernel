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

void exit() {
    asm volatile( "svc #3    \n");
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

  asm volatile( "mov r0, %1 \n"
                "mov r1, %2 \n"
                "mov r2, %3 \n"
                "svc #1     \n"
                "mov %0, r0 \n" 
              : "=r" (r) 
              : "r" (fd), "r" (x), "r" (n) 
              : "r0", "r1", "r2" );

  return r;
}

int read( int fd, void* x, size_t n ) {
  int r;

  asm volatile( "mov r0, %1 \n"
                "mov r1, %2 \n"
                "mov r2, %3 \n"
                "svc #4     \n"
                "mov %0, r0 \n"
              : "=r" (r)
              : "r" (fd), "r" (x), "r" (n)
              : "r0", "r1", "r2" );
  
  return r;
}

int read_line(char b[], size_t array_size){
  int chars = 0;
  if(array_size > 500)
    array_size = 500;
  chars = read(1, b, array_size);
  if(chars == array_size){
    b[chars-1] = '\0';
  }
  else if(b[chars-1] != '\0'){
    b[chars++] = '\0';
  }
  return chars;
}

/**
 * A custom printf that only supports %d
 */
int printF(const char* f, ...){
    va_list args;      //Define a list of variables
    int done, num;     //'num' is used to temporarily store integer arguments
    char buffer[50];   //Final message stored here
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
            }
        }
        if(!paramFound){
            buffer[x++] = f[i];
            buffer[x] = '\0';
        }
    }
    buffer[x]='\0';
    len = strlen(buffer);
    done = write(0, buffer, len);
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
