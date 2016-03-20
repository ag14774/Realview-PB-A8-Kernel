#include "kernel.h"

/* Since we *know* there will be 2 processes, stemming from the 2 user 
 * programs, we can 
 * 
 * - allocate a fixed-size process table (of PCBs), and use a pointer
 *   to keep track of which entry is currently executing, and
 * - employ a fixed-case of round-robin scheduling: no more processes
 *   can be created, and neither is able to complete.
 */

extern pcb_t *current;
extern hashtable_t* ht_ptr;
extern input_buff* ib_ptr;

//file_table_entry global_table[100];

entry_info_t programs[20];

uint32_t find_entry(char* pn){
    for(int i=0;i<20;i++){
        if(strcmp(pn,programs[i].name) == 0){
            return programs[i].entry;
        }
    }
    return -1;
}

void __user_exit(int r){
    asm volatile("svc #3     \n");
}

//Transfers buffer control from process 'from' to process 'to'
//only if process 'from' has the control of the buffer
void __ioctl(pid_t from, pid_t to){
  if(from == to)
      return;
  if(ib_ptr->pid == from || ib_ptr->pid == 0){
    flush_buff(ib_ptr);
    ib_ptr->pid = to;
  }
}

void __exit(ctx_t* ctx, int destroy, pid_t pid){
  pcb_t* temp = find_pid_ht(ht_ptr, pid);
  if(!temp->ph.parent)
      temp->ph.parent = find_pid_ht(ht_ptr,1);
  if(pid == 1 || temp->ph.parent->pid < 1) //Do not exit the shell
    return;
  //deschedule(pid); finally here's the bug!!!
  pcb_t* parent = temp->ph.parent;
  if(parent->proc_state == WAITING){ //if parent is waiting, let it know that we are exiting
    if(destroy)
        parent->ctx.gpr[0] = ctx->gpr[0];
    else
        parent->ctx.gpr[0] = pid;
    unblock_by_pid(parent->pid);
  }
  __ioctl(pid, parent->pid);
  if(destroy){
    destroy_pid(pid);
  }
  else {
    temp->proc_state = WAITING;
  }
  scheduler( ctx );
}

void process_char_stdout(uint8_t c){
  if( (c == BACKSPACE || c == DELETE) && !ib_ptr->ready && ib_ptr->n>0 ){
    PL011_putc(UART0, c);
    PL011_putc(UART0, ' ');
    PL011_putc(UART0, c);
  }
  else if(c == NEWLINE || c == RETURN){
    PL011_putc(UART0, '\n');
  }
  else if(c>=0x20 && c<=0x7E && !ib_ptr->ready){
    PL011_putc(UART0, c);
  }
}

void kernel_handler_rst( ctx_t* ctx              ) {
  /* Configure the mechanism for interrupt handling by
   *
   * - configuring timer st. it raises a (periodic) interrupt for each 
   *   timer tick,
   * - configuring GIC st. the selected interrupts are forwarded to the 
   *   processor via the IRQ interrupt signal, then
   * - enabling IRQ interrupts.
   */

  UART0->IMSC           |= 0x00000010; // enable UART    (Rx) interrupt
  UART0->CR              = 0x00000301; // enable UART (Tx+Rx)

  TIMER0->Timer1Load     = 0x00010000; // select period = 2^18 ticks ~= 0.25 sec
  TIMER0->Timer1Ctrl     = 0x00000002; // select 32-bit   timer
  TIMER0->Timer1Ctrl    |= 0x00000040; // select periodic timer
  TIMER0->Timer1Ctrl    |= 0x00000020; // enable          timer interrupt
  TIMER0->Timer1Ctrl    |= 0x00000080; // enable          timer

  GICC0->PMR             = 0x000000F0; // unmask all            interrupts
  GICD0->ISENABLER[ 1 ] |= 0x00000010; // enable timer          interrupt
  GICD0->ISENABLER[ 1 ] |= 0x00001000; // enable UART    (Rx) interrupt
  GICC0->CTLR            = 0x00000001; // enable GIC interface
  GICD0->CTLR            = 0x00000001; // enable GIC distributor

  /* Initialise PCBs representing processes stemming from execution of
   * the two user programs.  Note in each case that
   *    
   * - the CPSR value of 0x50 means the processor is switched into USR 
   *   mode, with IRQ interrupts enabled, and
   * - the PC and SP values match the entry point and top of stack. 
   */
    
  programs[0].name  = "P0";
  programs[0].entry = (uint32_t)entry_P0;
  programs[1].name  = "P1";
  programs[1].entry = (uint32_t)entry_P1;
  programs[2].name  = "P2";
  programs[2].entry = (uint32_t)entry_P2;

  initialise_scheduler((uint32_t)&tos_irq);
  pid_t temp;
  temp = init_pcb((uint32_t)entry_shell);
  schedule(temp);

  //temp = init_pcb((uint32_t)entry_P0);
  //schedule(temp);
  //temp = init_pcb((uint32_t)entry_P1);
  //schedule(temp);
  //temp = init_pcb((uint32_t)entry_P2);
  //schedule(temp);

  /* Once the PCBs are initialised, we (arbitrarily) select one to be
   * restored (i.e., executed) when the function then returns.
   */

  scheduler(ctx);

  irq_enable();

  return;
}

void kernel_handler_irq( ctx_t* ctx ) {
  // Step 2: read the interrupt identifier so we know the source.

  uint32_t id = GICC0->IAR;

  // Step 4: handle the interrupt, then clear (or reset) the source.
  switch(id){
    case GIC_SOURCE_TIMER0 : {
      scheduler( ctx );
      TIMER0->Timer1IntClr = 0x01;
      break;
    }
    case GIC_SOURCE_UART0 : {
      uint8_t c = PL011_getc( UART0 );
      if(c == CTRLC){ //exit the foreground process. have an internal _exit() call
        __exit(ctx, 1, ib_ptr->pid);
      }
      else if(c == CTRLZ){
        __exit(ctx, 0, ib_ptr->pid);
      }
      else{
        process_char_stdout(c);
      }
      int ready = process_char(ib_ptr,c);
      if(ready){
        unblock_by_pid(ib_ptr->pid);
        //scheduler( ctx );//not sure if this should be here
      }
      UART0->ICR = 0x10;
      break;
    }
    default : {
      break;
    }
  }

  // Step 5: write the interrupt identifier to signal we're done.
  GICC0->EOIR = id;

}

void kernel_handler_svc( ctx_t* ctx, uint32_t id ) { 
  /* Based on the identified encoded as an immediate operand in the
   * instruction, 
   *
   * - read  the arguments from preserved usr mode registers,
   * - perform whatever is appropriate for this system call,
   * - write any return value back to preserved usr mode registers.
   */

  switch( id ) {
    case 0x00 : { // yield()
      scheduler( ctx );
      break;
    }
    case 0x01 : { // write( fd, x, n )
      int   fd = ( int   )( ctx->gpr[ 0 ] );  
      char*  x = ( char* )( ctx->gpr[ 1 ] );  
      int    n = ( int   )( ctx->gpr[ 2 ] );
      pid_t pid = current->pid;
      if(pid>1){
        PL011_puth(UART0, pid);
        PL011_putc(UART0, ':');
        PL011_putc(UART0, ' ');
      }

      for( int i = 0; i < n; i++ ) {
        PL011_putc( UART0, *x++ );
      }
      
      ctx->gpr[ 0 ] = n;
      break;
    }
    case 0x02 : { //fork()
      update_ctx(current, ctx);
      pid_t child = init_pcb_by_pointer(current); //check for errors here
      ctx->gpr[0] = child;
      pcb_t* childPCB = find_pid_ht(ht_ptr, child);
      childPCB->ctx.gpr[0] = 0;
      //__ioctl(current->pid,childPCB->pid);
      schedule(child);
      break;
    }
    case 0x03 : { //exit()
      __exit(ctx, 1, current->pid);
      break;
    }
    case 0x04 : { //read(fd, x, n) //SHOULD ALSO RETURN READY WHEN N CHARACTERS ARE READ
      //when you press enter, the buffer should become ready
      //it should stay ready until i == n
      //do not accept characters while it's ready
      //once flushed accept more
      int   fd = ( int   )( ctx->gpr[ 0 ] );
      char*  x = ( char* )( ctx->gpr[ 1 ] );
      int    n = ( int   )( ctx->gpr[ 2 ] );
      if(ib_ptr->pid != current->pid || !ib_ptr->ready){
        __ioctl(0, current->pid); //if nobody has ownership then give ownership
        ctx->pc = ctx->pc - 0x4; //correct address so next time the process unblocks, it will try again
        current->proc_state = BLOCKED;
        scheduler(ctx);
        break;
      }
      int i = 0;
      for(i=0;i<n;i++){
        x[i] = consume_char(ib_ptr);
        if(!ib_ptr->ready){
          i = i + 1;
          break;
        }
      }
      ctx->gpr[0] = i;
      break;
    }
    case 0x05 : { //waitpid(int pid) if pid is not in the queue, add it.
      int pid = (int)(ctx->gpr[0]);
      if(find_pid_ht(ht_ptr,pid)==NULL)
          break;
      __ioctl(current->pid,pid);
      proc_state_t reason;
      if(current->pid == pid) //waiting for self. that means block
        reason = BLOCKED;
      else { //make this work only if pid is a child of current
        reason = WAITING;
        unblock_by_pid(pid);//if it is blocked
      }
      current->proc_state = reason;
      scheduler( ctx );
      break;
    }
    case 0x06 : { //exec("program name",[*a0,*a1,*a2,...])
      char*  pn   = (char * ) ctx->gpr[0];
      char** argv = (char **) ctx->gpr[1];
      int i = 0;

      uint32_t pc = find_entry(pn);
      if(pc==-1){
          ctx->gpr[0] = -1;
          break;
      }

      char* arg_addr[128];

      ctx->pc  = pc;
      char* sp = (char*)(long)current->stack.tos;

      while(argv[i]){
        int len = strlen(argv[i]) + 1;
        sp = sp - len;
        memcpy(sp, argv[i], len);
        arg_addr[i++] = sp;
      }
      arg_addr[i] = 0;

      //align %8
      sp = sp - 4;
      while((uint32_t)sp%8) sp = sp - 1;
      
      sp = sp - (i+1)*sizeof(char*);
      memcpy(sp, arg_addr, (i+1)*sizeof(char*));
      
      ctx->pc = pc;
      ctx->sp = (uint32_t)sp;
      ctx->lr = (uint32_t)&__user_exit;
      ctx->gpr[0] = i;
      ctx->gpr[1] = ctx->sp;
      update_ctx(current, ctx);
      break;
    }
    case 0x07 : { //int pids=procs(int* buff)
      int* buff = (int*) ctx->gpr[0];
      pidkey_t* temp = ht_ptr->keyset.head;
      int i = 0;
      while(temp){
        buff[i++] = temp->pid;
        temp = temp->next;
      }
      ctx->gpr[0] = i;
      break;
    }
    case 0x08 : { //char* procstat(int pid)
      pid_t pid = (pid_t) ctx->gpr[0];
      pcb_t* pcb = find_pid_ht(ht_ptr, pid);
      if(!pcb)
        ctx->gpr[0] = -1; //terminated
      else if(pid == ib_ptr->pid)
        ctx->gpr[0] = 0; //foreground
      else if(pcb->proc_state == RUNNING || pcb->proc_state == READY)
        ctx->gpr[0] = 1; //running
      else if(pcb->proc_state == BLOCKED)
        ctx->gpr[0] = 2; //blocked
      else
        ctx->gpr[0] = 3; //stopped temporarily
      break;
    }
    case 0x09 : { //kill(pid, signal)
      int pid    = (int) ctx->gpr[0];
      int signal = (int) ctx->gpr[1];
      switch(signal){
        case SIGCONT:
            if(find_pid_ht(ht_ptr,pid)==NULL)
                break;
            unblock_by_pid(pid);//if it is blocked
            //scheduler( ctx );
            break;
        case SIGTERM:
            __exit(ctx, 1, pid);
            break;
        default:
            break;
      }
      break;
    }
    case 0x0A : { //nice(int pid, int priority) should only be called by shell
      int pid      = (int) ctx->gpr[0];
      int priority = (int) ctx->gpr[1];
      if(current->pid != 1)
        break;
      pcb_t* p = find_pid_ht(ht_ptr,pid);
      if(!p)
        break;
      setPriority(p, priority);
      break;
    }
    default   : { // unknown
      break;
    }
  }

  return;
}


