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

//*******FIX RUNTIMES. UPDATE RUNTIMES IS NOT CALLED IN KERNEL.C*******
//*******MAKE SCHEDULER CHECK IF PROCESS IS BLOCKED BEFORE INSERT******

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
  if(pid == 1 || temp->ph.parent < 1) //Do not exit the shell
    return;
  deschedule(pid);
  pid_t parent_id = temp->ph.parent;
  pcb_t* parent = find_pid_ht(ht_ptr,parent_id);
  if(parent){
    if(parent->proc_state == WAITING){ //if parent is waiting, let it know that we are exiting
      parent->ctx.gpr[0] = pid;
      unblock_by_pid(parent_id);
    }
    __ioctl(pid, parent_id);
  }
  if(destroy){
    destroy_pid(pid);
  }
  else {
    temp->proc_state = WAITING;
  }
  scheduler( ctx );
}

void process_char_stdout(uint8_t c){
  if( (c == BACKSPACE || c == DELETE) && !ib_ptr->ready ){
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

  TIMER0->Timer1Load     = 0x00040000; // select period = 2^18 ticks ~= 0.25 sec
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
   * - the PC and SP values matche the entry point and top of stack. 
   */

  initialise_scheduler((uint32_t)&tos_irq);
  pid_t temp;
  temp = init_pcb((uint32_t)entry_P0);
  schedule(temp);
  temp = init_pcb((uint32_t)entry_P1);
  schedule(temp);
  temp = init_pcb((uint32_t)entry_P2);
  schedule(temp);

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
      int ready = process_char(ib_ptr,c);
      if(c == CTRLC){ //exit the foreground process. have an internal _exit() call
        __exit(ctx, 1, ib_ptr->pid);
      }
      else if(c == CTRLZ){
        __exit(ctx, 0, ib_ptr->pid);
      }
      else{
        process_char_stdout(c);
      }
      if(ready){
        unblock_by_pid(ib_ptr->pid);
        scheduler( ctx );//not sure if this should be here
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
      PL011_puth(UART0, pid);
      PL011_putc(UART0, ':');
      PL011_putc(UART0, ' ');

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
      if(ib_ptr->pid == current->pid && n <= ib_ptr->n){
        
      }
      if(ib_ptr->pid != current->pid || !ib_ptr->ready){
        __ioctl(0, current->pid);
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
    default   : { // unknown
      break;
    }
  }

  return;
}


