/*
 * =====================================================================================
 *
 *       Filename:  eratosthenesARRAY.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  20/11/14 14:53:00
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Mr. Andreas Georgiou (ag14774), a.georgiou.2014@bristol.ac.uk
 *   Organization:  University of Bristol
 *
 * =====================================================================================
 */
//MUCH MUCH FASTER ALGORITHM BECAUSE IT DOES NOT CHECK EVERY NUMBER . WASTES MEMORY. SLOW PRINTING BECAUSE IT DOES NOT ACTUALLY REMOVE THE ELEMENTS FROM THE ARRAY.
#include "genPrimes.h"

typedef unsigned int uint;
void SpawnArray(uint a, uint array[]){
    for(uint i=2;i<=a;i++){
        array[i-2]=i;
    }
}

void printArray(uint *a,uint num){
    int buf[3];
    int c=0;
    for (uint i=0;i<(num-1);i++){
        if(a[i]!=0){
            buf[c] = a[i];
            c++;
            if(c==3){
                printF("%d %d %d\n",buf[0], buf[1], buf[2]);
                c=0;
            }
        }
    }
    switch(c){
    case 1:
        printF("%d \n",buf[0]);
        break;
    case 2:
        printF("%d %d\n",buf[0], buf[1]);
        break;
    case 3:
        printF("%d %d %d\n",buf[0], buf[1], buf[2]);
        break;
    default:
        printF("\n");
        break;
    }
}

void del(uint *a,uint num){
    int check=1;
    uint step=2;
    uint i;
    uint lastpos=0;
    while(check){
        check=0;
        for(i=lastpos;i<(num-1);i=i+step){
            if((a[i]!=step)&&(a[i]!=0)){
                a[i]=0;
                check=1;
            }
        }
        for(i=lastpos+1;i<(num-1);i++){
            if(a[i]!=0){
                lastpos=i;
                printF("Multiples of %d removed!\n",step);
                step=a[i];
                break;
            }
        }
    }
}

int s2int(char* s){
    int acc = 0;
    for(int i=0;s[i]!='\0';i++){
        acc = 10*acc + s[i] - '0';
    }
    return acc;
}

int main(int argc, char** argv){
    uint n;
    n = s2int(argv[1]);
    uint array[200];
    SpawnArray(n, array);
    del(array,n);
    printArray(array,n);
    return 0;
}

int (*entry_genPrimes)(int argc, char** argv) = &main;
