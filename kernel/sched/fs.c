#include "fs.h"

uint32_t inode_size;
uint32_t sb_size;
uint32_t block_num;
uint32_t block_len;
uint32_t disk_size;
uint32_t bl_bitmap_size;
uint32_t bl_bitmap_start;
uint32_t inode_start;
uint32_t max_file_size;
uint32_t data_start;

cache_ht cache;

uint32_t cache_hash(uint32_t block_addr){
    return ((uint32_t)(282563095*block_addr+29634029) % 993683819) % CACHE_SIZE ; //best so far
}

cache_entry* find_cache_entry(uint32_t block_addr){
    uint32_t index = cache_hash(block_addr);
    int j=0;
    for(j=0;j<CACHE_BUCKET;j++){
        if(cache.entries[index][j].active && cache.entries[index][j].block_addr == block_addr){
            return &cache.entries[index][j];
        }
    }
    return NULL;
}

uint32_t cache_block(uint32_t block_addr, uint8_t force){
    cache_entry* centry = find_cache_entry(block_addr);
    if(centry!=NULL)
        return 0;//already cached
    uint32_t index = cache_hash(block_addr);
    int j=0;
    while(cache.entries[index][j].active && j<CACHE_BUCKET){
        j = j + 1;
    }
    if(j == CACHE_BUCKET){
        if(force){
            //empty bucket
            for(j=0;j<CACHE_BUCKET;j++){
                empty_cache_block(cache.entries[index][j].block_addr, 1);
            }
            j=0;
        }
        else{
            return -1;//cache full
        }
    }
    centry = &cache.entries[index][j];
    disk_rd(block_addr, centry->data, block_len);
    centry->block_addr = block_addr;
    centry->active = 1;
    if(cache.head == NULL && cache.tail == NULL){
        cache.head = centry;
        cache.tail = centry;
        centry->prev = NULL;
        centry->next = NULL;
    }
    else{
        cache.tail->next = centry;
        centry->prev = cache.tail;
        cache.tail = centry;
        centry->next = NULL;
    }  
    return 0;
}

void empty_cache_block(uint32_t block_addr, int write2disk){
    cache_entry* centry = find_cache_entry(block_addr);
    if(!centry)
        return;
    if(write2disk)
        disk_wr(centry->block_addr,centry->data,block_len);
    centry->active = 0;
    if(centry == cache.head)
        cache.head = centry->next;
    if(centry == cache.tail)
        cache.tail = centry->prev;
    if(centry->prev)
        centry->prev->next = centry->next;
    if(centry->next)
        centry->next->prev = centry->prev;
}

void empty_cache(){
    while(cache.head)
        empty_cache_block(cache.head->block_addr, 1);
}

void disk_wr_byte(uint32_t addr, uint8_t b){
    uint32_t block_addr   = addr/block_len;
    uint32_t block_offset = addr%block_len;
    cache_block(block_addr,1);
    cache_entry* centry = find_cache_entry(block_addr);
    centry->data[block_offset] = b;
}

uint8_t disk_rd_byte(uint32_t addr){
    uint32_t block_addr   = addr/block_len;
    uint32_t block_offset = addr%block_len;
    cache_block(block_addr,1);
    cache_entry* centry = find_cache_entry(block_addr);
    return centry->data[block_offset];
}

void set_block_bit(uint32_t block_addr){
    uint32_t byte_addr = block_addr/8 + bl_bitmap_start*block_len;
    uint8_t bit_offset = block_addr%8;
    uint8_t temp = disk_rd_byte(byte_addr);
    temp |= 1<<bit_offset;
    disk_wr_byte(byte_addr, temp);
}

void clear_block_bit(uint32_t block_addr){
    uint32_t byte_addr = block_addr/8 + bl_bitmap_start*block_len;
    uint8_t bit_offset = block_addr%8;
    uint8_t temp = disk_rd_byte(byte_addr);
    temp &= ~(1<<bit_offset);
    disk_wr_byte(byte_addr, temp);
}

int get_global_entry(global_table* t, int globalID){
    if(globalID>=0){
        if(!t->entries[globalID].active){
            t->entries[globalID].active = 1;
            t->count++;
        }
        return globalID;
    }
    for(int i=0;i<MAXGLOBAL;i++){
        if(!t->entries[i].active){
            t->entries[i].active = 1;
            t->count++;
            return i;
        }
    }
    return -1;
}

void setGlobalEntry(global_table* t, int globalID, i_node inode, char name[], file_type type){
    if(!t->entries[globalID].active)
        return;
    t->entries[globalID].globalID = globalID;
    t->entries[globalID].refcount = 0;
    t->entries[globalID].inode    = inode;
    if(name)
        memcpy(t->entries[globalID].name,name,20);
    else
        t->entries[globalID].name[0] = '\0';
    t->entries[globalID].type = type;
    t->entries[globalID].wqwrite.head = 0;
    t->entries[globalID].wqwrite.tail = 0;
    t->entries[globalID].wqwrite.len  = 0;
    t->entries[globalID].wqread.head  = 0;
    t->entries[globalID].wqread.tail  = 0;
    t->entries[globalID].wqread.len   = 0;
}

void enqueue_wq(global_table* t, int globalID, int queue, int pid){
    if(!t->entries[globalID].active)
        return;
    wait_queue* wq;
    if(queue == 0)
        wq = &t->entries[globalID].wqwrite;
    else
        wq = &t->entries[globalID].wqread;
    if(wq->len == PIPE_WQ)
        return;
    wq->pids[wq->tail] = pid;
    wq->tail = (wq->tail + 1) % PIPE_WQ;
    wq->len++;
}

int dequeue_wq(global_table* t, int globalID, int queue){ //return 0 if empty;
    if(!t->entries[globalID].active)
        return 0;
    wait_queue* wq;
    if(queue == 0)
        wq = &t->entries[globalID].wqwrite;
    else
        wq = &t->entries[globalID].wqread;
    if(wq->len == 0)
        return 0;
    int pid = wq->pids[wq->head];
    wq->head = (wq->head + 1) % PIPE_WQ;
    wq->len--;
    return pid;
}

void remove_wq(global_table* t, int globalID, int queue, int pid){
    if(!t->entries[globalID].active)
        return;
    if(queue < 0)
        return;
    wait_queue* wq;
    if(queue == 0)
        wq = &t->entries[globalID].wqwrite;
    else
        wq = &t->entries[globalID].wqread;
    if(wq->len == 0)
        return;
    int len = wq->len;
    int temp = -1;
    int i=0;
    while(temp!=pid && i<len){
        i++;
        temp = dequeue_wq(t, globalID, queue);
        if(temp!=pid)
            enqueue_wq(t, globalID, queue, temp);
    }
}

int close_global_entry(global_table* t, int globalID){ //close pipe or file first
    if(t->entries[globalID].refcount>0)
        return -1;
    if(t->entries[globalID].wqwrite.len>0 || t->entries[globalID].wqread.len>0)
        return -1;
    if(!t->entries[globalID].active)
        return -1;
    t->entries[globalID].active = 0;
    t->entries[globalID].inode  = 0;
    t->entries[globalID].name[0]='\0';
    t->count--;
    return 0;
}

void reset_pipe(pipes_t* pipes, i_node inode){
    pipes->p[inode].buff[0] = '\0';
    pipes->p[inode].readptr = 0;
    pipes->p[inode].writeptr= 0;
    pipes->p[inode].len = 0;

}

i_node get_pipe(pipes_t* pipes,i_node inode){
    if(inode>=0){
        if(!pipes->p[inode].active){
            pipes->p[inode].active = 1;
            pipes->count++;
        }
        return inode;
    }
    for(int i=0;i<MAXPIPES;i++){
        if(!pipes->p[i].active){
            pipes->p[i].active = 1;
            pipes->count++;
            return i;
        }
    }
    return -1;
}

void setPipe(pipes_t* pipes, i_node inode, int globalID){
    reset_pipe(pipes, inode);
    pipes->p[inode].globalID = globalID;
}

void send_pipe(pipes_t* pipes, i_node inode, char n){
    if(!pipes->p[inode].active)
        return;
    //if(isPipeFull(pipes, inode))
    //    return 1;
    pipe_t* p = &pipes->p[inode];
    p->buff[p->writeptr] = n;
    p->writeptr = (p->writeptr + 1) % PIPESIZE;
    p->len++;
}

char consume_pipe(pipes_t* pipes, i_node inode){
    if(!pipes->p[inode].active)
        return 0;
    //if(isPipeEmpty(pipes, inode))
    //    return 1;
    pipe_t* p = &pipes->p[inode];
    char n = p->buff[p->readptr];
    p->readptr = (p->readptr + 1) % PIPESIZE;
    p->len--;
    return n;
}

int isPipeEmpty(pipes_t* pipes, i_node inode){
    if(!pipes->p[inode].active)
        return -1;
    if(pipes->p[inode].len == 0)
        return 1;
    return 0;
}

int isPipeFull(pipes_t* pipes, i_node inode){
    if(!pipes->p[inode].active)
        return -1;
    if(pipes->p[inode].len == PIPESIZE)
        return 1;
    return 0;
}

void close_pipe(pipes_t* pipes, i_node inode){
    if(!pipes->p[inode].active)
        return;
    reset_pipe(pipes, inode);
    pipes->p[inode].active = 0;
    pipes->count--;
}

int load_sb(){
    char magic[4];
    inode_size      = 0;
    sb_size         = 0;
    block_num       = 0;
    block_len       = 0;
    disk_size       = 0;
    bl_bitmap_size  = 0;
    bl_bitmap_start = 0;
    inode_start     = 0;
    max_file_size   = 0;
    data_start      = 0;
    char sbtemp[MAX_BLOCK_SIZE];
    inode_size   = sizeof(inode_t);
    sb_size      = sizeof(superblock_t);
    block_num    = disk_get_block_num();
    block_len    = disk_get_block_len();
    disk_rd(0,(uint8_t*)sbtemp,block_len);
    magic[0] = sbtemp[0];
    magic[1] = sbtemp[1];
    magic[2] = sbtemp[2];
    magic[3] = sbtemp[3];
    if(!(magic[0]=='A' && magic[1]=='N' && magic[2]=='D' && magic[3]=='R'))
        return 1;
        
    disk_size    = block_len * block_num;
    bl_bitmap_size  = block_num/8 + 1;
    
    superblock_t* sb = (superblock_t*)&sbtemp;
    bl_bitmap_start = sb->block_bitmap_start;
    inode_start = sb->inode_start;
    max_file_size = block_len * MAX_DIRECT_BLOCKS;
    data_start = sb->data_start;
    return 0;
}

void format_disk(){
    inode_size      = 0;
    sb_size         = 0;
    block_num       = 0;
    block_len       = 0;
    disk_size       = 0;
    bl_bitmap_size  = 0;
    bl_bitmap_start = 0;
    inode_start     = 0;
    max_file_size   = 0;
    data_start      = 0;
    superblock_t sb;
    inode_size   = sizeof(inode_t);
    sb_size      = sizeof(superblock_t);
    block_num    = disk_get_block_num();
    block_len    = disk_get_block_len();
    disk_size    = block_len * block_num;

    bl_bitmap_size  = block_num/8 + 1; //in bytes
    bl_bitmap_start = sb_size;
    while(bl_bitmap_start % block_len != 0) bl_bitmap_start++;
    bl_bitmap_start = bl_bitmap_start/block_len; //convert to block address;

    inode_start = bl_bitmap_start*block_len + bl_bitmap_size;
    while(inode_start % block_len !=0) inode_start++;
    inode_start = inode_start/block_len; //convert to block address;

    max_file_size  = block_len * MAX_DIRECT_BLOCKS;
    uint32_t max_files      = (disk_size - inode_start*block_len)/(inode_size+max_file_size); //IS THIS CORRECT?

    data_start = inode_start*block_len + max_files*inode_size;
    while(data_start % block_len !=0) data_start++;
    data_start = data_start/block_len;

    sb.magic[0] = 'A';
    sb.magic[1] = 'N';
    sb.magic[2] = 'D';
    sb.magic[3] = 'R';
    sb.block_bitmap_start = bl_bitmap_start;
    sb.inode_start        = inode_start;
    sb.data_start         = data_start;

    //write superblock
    uint8_t* sb_ptr = (uint8_t*)&sb;
    for(int i=0;i<sb_size;i++){
        disk_wr_byte(i,sb_ptr[i]);
    }

    for(int i=0;i<data_start;i++){
        clear_block_bit(i);
    }
    
    for(int i=data_start;i<block_num;i++){
        set_block_bit(i);
    }

    inode_t inode;
    inode.free = 0;
    inode.size = 0;
    for(int i=0;i<MAX_DIRECT_BLOCKS;i++)
        inode.blocks[i] = 0;

    uint8_t* inode_ptr = (uint8_t*)&inode.free;
    for(int i=0;i<max_files;i++){
        for(int j=0;j<sizeof(uint16_t);j++){
            disk_wr_byte(block_len*inode_start+i*inode_size+j,inode_ptr[j]);
        }
    }
    empty_cache();
}
