SECTIONS {
  /* assign address (per  QEMU)  */
  .       =     0x70010000; 
  /* place text segment(s)       */
  .text : { kernel/interrupt.o(.text) *(.text .rodata) } 
  /* place data segment(s)       */        
  .data : {                           *(.data        ) } 
  /* place bss  segment(s)       */        
  .bss  : {                           *(.bss         ) } 
  /* align  address (per AAPCS)  */
  .       = ALIGN(8);        
  /* allocate stack for svc mode */
  .       = . + 0x00001000;  
  tos_svc = .;
  /* allocate stack for irq mode */
  .       = . + 0x00001000;  
  tos_irq = .;
  /* allocate stack for P0       */
  .       = . + 0x00001000;  
  tos_P0  = .;
  /* allocate stack for P1       */
  .       = . + 0x00001000;  
  tos_P1  = .;
  /* allocate stack for P2       */
  .       = . + 0x00001000;
  tos_P2  = .;
}
