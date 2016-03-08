#include "pcb_t.h"

void update_runtime(pcb_t *pcb){
    uint8_t priority = pcb->priority;
    uint32_t vruntime = pcb->vruntime;
    vruntime = vruntime + priority;
    pcb->vruntime = vruntime;
}

void setPriority(pcb_t *pcb, uint8_t priority){
    if(priority<MAX_PRIORITY)
        priority = MAX_PRIORITY;
    if(priority>MIN_PRIORITY)
        priority = MIN_PRIORITY;
    pcb->priority = priority;
}

void update_ctx(pcb_t *pcb, ctx_t *ctx){
    memcpy(&pcb->ctx, ctx, sizeof(ctx_t));
}

void restore_ctx(ctx_t *ctx, pcb_t *pcb){
    memcpy(ctx, &pcb->ctx, sizeof(ctx_t));
}

