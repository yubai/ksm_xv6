7600 // Intel 8253/8254/82C54 Programmable Interval Timer (PIT).
7601 // Only used on uniprocessors;
7602 // SMP machines use the local APIC timer.
7603 
7604 #include "types.h"
7605 #include "defs.h"
7606 #include "traps.h"
7607 #include "x86.h"
7608 
7609 #define IO_TIMER1       0x040           // 8253 Timer #1
7610 
7611 // Frequency of all three count-down timers;
7612 // (TIMER_FREQ/freq) is the appropriate count
7613 // to generate a frequency of freq Hz.
7614 
7615 #define TIMER_FREQ      1193182
7616 #define TIMER_DIV(x)    ((TIMER_FREQ+(x)/2)/(x))
7617 
7618 #define TIMER_MODE      (IO_TIMER1 + 3) // timer mode port
7619 #define TIMER_SEL0      0x00    // select counter 0
7620 #define TIMER_RATEGEN   0x04    // mode 2, rate generator
7621 #define TIMER_16BIT     0x30    // r/w counter 16 bits, LSB first
7622 
7623 void
7624 timerinit(void)
7625 {
7626   // Interrupt 100 times/sec.
7627   outb(TIMER_MODE, TIMER_SEL0 | TIMER_RATEGEN | TIMER_16BIT);
7628   outb(IO_TIMER1, TIMER_DIV(100) % 256);
7629   outb(IO_TIMER1, TIMER_DIV(100) / 256);
7630   picenable(IRQ_TIMER);
7631 }
7632 
7633 
7634 
7635 
7636 
7637 
7638 
7639 
7640 
7641 
7642 
7643 
7644 
7645 
7646 
7647 
7648 
7649 
