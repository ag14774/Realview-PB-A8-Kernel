#include "grep.h"

#define ASIZE 127

int bmBc[ASIZE];

void preBmBc(char *x, int m) {
   int i;
 
   for (i = 0; i < ASIZE; ++i)
      bmBc[i] = m;
   for (i = 0; i < m - 1; ++i)
      bmBc[x[i]] = m - i - 1;
}

void HORSPOOL(char *x, int m, char *y, int n) {
   int j;
   char c;

   /* Searching */
   j = 0;
   while (j <= n - m) {
      c = y[j + m - 1];
      if (x[m - 1] == c && memcmp(x, y + j, m - 1) == 0)
         printF("%s\n",y);
      j += bmBc[c];
   }
}

void grep(int argc, char** argv) {
  char c[100];
  preBmBc(argv[1],strlen(argv[1]));
  while(1){
    int r = -1;
    c[0] = '\0';
    do{
        r++;
        read_line(&c[r], 1);
    }while(c[r]!='\n');
    HORSPOOL(argv[1], strlen(argv[1]),c,r);
  }
  return;
}

void (*entry_grep)() = &grep;
