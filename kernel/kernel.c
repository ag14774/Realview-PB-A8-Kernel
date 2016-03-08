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

void process_char_stdout(uint8_t c){
  if(c == BACKSPACE || c == DELETE){
    PL011_putc(UART0, c);
    PL011_putc(UART0, ' ');
    PL011_putc(UART0, c);
  }
  else if(c == NEWLINE || c == RETURN){
    PL011_putc(UART0, '\n');
  }
  else if(c>=0x20 && c<=0x7E){
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
      //process_char(&ib,c);
      if(c == CTRLC){ //THIS SHOULD EXIT THE FOREGROUND PROCESS
        deschedule(current->pid);
        destroy_pid(current->pid);
        current = NULL;
        scheduler( ctx );
      }
      else if(c == CTRLZ){
      
      }
      else{
        process_char_stdout(c);
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
      deschedule(current->pid);
      destroy_pid(current->pid);
      current = NULL;
      scheduler( ctx );
      break;
    }
    case 0x04 : { //read(fd, x, n)
      //if current in background
      //  block and remove from queue
      //if current in foreground
      //if(!ib.ready){
        //go back 4 bytes in lr
        //deschedule
      //}
      //else{
        int   fd = ( int   )( ctx->gpr[ 0 ] );
        char*  x = ( char* )( ctx->gpr[ 1 ] );
        int    n = ( int   )( ctx->gpr[ 2 ] );
        int i = 0;
        //for(i=0;i<n;i++){
        //  x[i] = ib.buff[i];
        //  if(i == ib.n-1){
        //    i = ib.n;
        //    break;
        //  }
        //}
        ctx->gpr[0] = i;
      //}
      break;
    }
    default   : { // unknown
      break;
    }
  }

  return;
}


