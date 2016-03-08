#ifndef INPUT_BUFFER_H
#define INPUT_BUFFER_H

#include <stdint.h>
#include "pcb_t.h"

#define BUFFSIZE    500
#define BACKSPACE   0x08
#define DELETE      0x7F
#define NEWLINE     0x0A
#define RETURN      0x0D
#define NULLCHAR    0x00
#define CTRLZ       0x3C //ctrl+z
#define CTRLC       0x3E //ctrl+c

typedef struct {
    int n;
    int ready;
    pid_t pid;
    char buff[BUFFSIZE];
} input_buff;

void erase_char(input_buff* ib);
int process_char(input_buff* ib, uint8_t ch);
void flush_buff(input_buff* ib);

#endif
