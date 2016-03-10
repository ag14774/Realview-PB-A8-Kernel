#include "input_buffer.h"

void erase_char(input_buff* ib){
    if(ib->n == 0)
        return;
    ib->n = ib->n - 1;
    ib->buff[ib->n] = '\0';
}

void add_char(input_buff* ib, uint8_t ch){
    ib->buff[ib->n] = ch;
    ib->n = ib->n + 1;
}

int process_char(input_buff* ib, uint8_t ch){
    if(ib->ready)
        return ib->ready;
    if (ch == BACKSPACE || ch == DELETE){
        erase_char(ib);
    }
    else if (ch == NEWLINE || ch == NULLCHAR || ch == RETURN){
        (ch == RETURN) ? (ch = NEWLINE) : (ch = ch);
        ib->ready = 1;
        add_char(ib, ch);
    }
    else if (ch == CTRLC || ch == CTRLZ){
    }
    else if (ch>=0x20 && ch<=0x7E){
        add_char(ib, ch);
    }
    if (ib->n>=BUFFSIZE)
        ib->ready = 1;
    return ib->ready;
}

void flush_buff(input_buff* ib){
    ib->ready = 0;
    ib->n = 0;
    ib->i = 0;
    ib->buff[0] = '\0';
    ib->pid = 0;
}

char consume_char(input_buff* ib){
    if(ib->ready){
       char c = ib->buff[ib->i];
       ib->i = ib->i + 1;
       if(ib->i == ib->n){
           pid_t pid = ib->pid;
           flush_buff(ib);
           ib->pid = pid;
       }
       return c;
    }
    return '\0';
}
