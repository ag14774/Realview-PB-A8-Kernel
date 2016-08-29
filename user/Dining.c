#include "Dining.h"

#define PHILOSOPHERS 5

#define EATREQUEST   0
#define LEFTRELEASE  1
#define RIGHTRELEASE 2
#define EATACCEPT    3
#define EATREJECT    4

int is_prime2( uint32_t x ) {
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

void delay(){
    for( uint32_t x = ( 1 << 8 ); x < ( 1 << 15 ); x++ ) {
      int r = is_prime2( x );
    }
}

void Philosopher( int fd, int i ) {
  char data;
  while(1){
    printF("Philosopher %d:Thinking\n",i);
    delay();
    data = EATREQUEST;
    write(fd, &data, 1);
    sync(fd);
    read(fd, &data, 1);
    if(data == EATACCEPT){
        printF("Philosopher %d:Eating\n",i);
        delay();
        data = LEFTRELEASE;
        write(fd, &data, 1);
        sync(fd);
        data = RIGHTRELEASE;
        write(fd, &data, 1);
        sync(fd);
    }
  }
}

void Dining() {
  int chopsticks[PHILOSOPHERS];
  int phils[PHILOSOPHERS];
  for(int i=0;i<PHILOSOPHERS;i++){
    phils[i] = getpipe();
    chopsticks[i] = 0;
    int pid = fork();
    if(pid == 0){
        for(int j=0;j<i;j++){
            close(phils[j]);
        }
        Philosopher(phils[i], i);
    }
  }
  
  while(1){
    int i = select(phils,PHILOSOPHERS);
    char data;
    read(phils[i], &data, 1);
    if(data == EATREQUEST){
        if(chopsticks[i] == 0 && chopsticks[(i+1)%PHILOSOPHERS] == 0){
            chopsticks[i] = 1;
            chopsticks[(i+1)%PHILOSOPHERS] = 1;
            data = EATACCEPT;
        }
        else{
            data = EATREJECT;
        }
        write(phils[i], &data, 1); 
    }
    else if(data == LEFTRELEASE){
        chopsticks[i] = 0;
    }
    else if(data == RIGHTRELEASE){
        chopsticks[(i+1)%PHILOSOPHERS] = 0;
    }
  } 

  return;
}

void (*entry_Dining)() = &Dining;
