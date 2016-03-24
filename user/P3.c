#include "P3.h"

void P3() {
  char c;
  while(1){
    int r = read_line(&c, 1);
    printF("%c",c);
  }
  return;
}

void (*entry_P3)() = &P3;
