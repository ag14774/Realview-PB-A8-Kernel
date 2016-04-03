#include "P0.h"

int is_prime( uint32_t x ) {
  if ( !( x & 1 ) || ( x < 2 ) ) {
    return ( x == 2 );
  }

  for( uint32_t d = 3; ( d * d ) <= x ; d += 2 ) {
    if( !( x % d ) ) {
      return 0;
    }
  }

  return 1;
}

void P0() {
  int x = 0;
  char s[20];
  while( 1 ) {
    //file1
    int r = creat("/file1");
    printF("File created: %d\n",r);
    int fd = open("/file1",READ_WRITE);
    char* x = "tHIS SENTENCE IS MORE THAN 16 BYTES. YES IT IS";
    write(fd, x, 47);
    lseek(fd, 0, SEEK_SET);
    char c = 'T';
    write(fd, &c, 1);
    lseek(fd, 0, SEEK_SET);
    read(fd, x, 4);
    close(fd);
    //file2
    creat("/file2");
    fd = open("/file2",READ_WRITE);
    write(fd, x, 47);
    lseek(fd, 0, SEEK_SET);
    close(fd);
    //clearing and rewriting to file 1
    fd = open("/file1",READ_WRITE|CLEAR_FILE);
    write(fd, x, 4);
    close(fd);
    //unlink test
    unlink("/file2");
    for( uint32_t x = ( 1 << 8 ); x < ( 1 << 24 ); x++ ) {
      int r = is_prime( x ); printF( "is_prime( %d ) = %d\n", x, r );
    }
  }

  return;
}

//void P0() {
//  int x = 0;
//  char s[20];
//  int test = 0;
//  int chars = read_line(s,20);
//  printF("Characters read: %d\n", chars);
//  printF("Input text: %s\n",s);
//  int pid = fork();
//  if(pid!=0)
//    test = waitpid(pid);
//  if(pid==0){
//    for( uint32_t x = ( 1 << 8 ); x < ( 1 << 24 ); x++ ) {
//      int r = is_prime( x ); printF( "is_prime( %d ) = %d\n", x, r );
//    }
//  }
//  printF("result: %d\n",test);
//
//  return;
//}

void (*entry_P0)() = &P0;
