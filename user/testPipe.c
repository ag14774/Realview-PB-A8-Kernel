#include "P2.h"

void child( int fd ) {
  fork();
  int buff[20];
  while(1){
    int r = read(fd, buff, 1);
    printF("Read %d characters: %c\n", r, buff[0]);
  }
}

void testPipe() {
  int fd = getpipe();
  int pid = fork();
  printF("Filedes: %d\n",fd);
  if(pid != 0){
    char c;
    printF("I'm parent\n");
    while(1){
      //int r = read_line(&c, 1);
      int r=0;
      c='f';
      int w = write(fd, &c, 1);
      printF("In: %d | Out: %d\n", r, w);
    }
  }
  else{
    child(fd);
  }

  return;
}

void (*entry_testPipe)() = &testPipe;
