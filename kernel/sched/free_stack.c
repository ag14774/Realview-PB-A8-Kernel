#include "free_stack.h"

uint32_t peek_stack(user_stack* us){
    if(us->len == 0)
        return us->tos;
    return us->free_stacks[us->len-1];
}
uint32_t get_next_stack(user_stack* us){
    if(us->len == 0){
        us->tos = us->tos + 0x1000;
        return us->tos;
    }
    us->len = us->len - 1;
    return us->free_stacks[us->len];
}

uint32_t replicate_stack(user_stack* us, uint32_t src_addr){
    uint32_t dest = get_next_stack(us);
    memcpy((void*)(long)(dest-0x1000),(void*)(long)(src_addr-0x1000),0x1000);
    return dest;
}

void declare_free(user_stack* us, uint32_t stack_pointer){
    us->free_stacks[us->len] = stack_pointer;
    us->len = us->len + 1;
}
