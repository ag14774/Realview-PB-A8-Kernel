#ifndef FREE_STACK_H
#define FREE_STACK_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef struct {
    uint32_t tos;
    uint32_t free_stacks[50]; //stacks of processes that have exited
    uint8_t  len;
} user_stack;

uint32_t peek_stack(user_stack* us); //returns the address of the next free stack
uint32_t get_next_stack(user_stack* us); //allocates and returns the top of the next free stack
uint32_t replicate_stack(user_stack* us, uint32_t src_addr); //copies stack from src_addr to a new stack and returns address of new stack
void declare_free(user_stack* us, uint32_t stack_pointer); //mark a stack as free and add it to free_stacks

#endif
