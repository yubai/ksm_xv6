6650 // The local APIC manages internal (non-I/O) interrupts.
6651 // See Chapter 8 & Appendix C of Intel processor manual volume 3.
6652 
6653 #include "types.h"
6654 #include "defs.h"
6655 #include "memlayout.h"
6656 #include "traps.h"
6657 #include "mmu.h"
6658 #include "x86.h"
6659 
6660 // Local APIC registers, divided by 4 for use as uint[] indices.
6661 #define ID      (0x0020/4)   // ID
6662 #define VER     (0x0030/4)   // Version
6663 #define TPR     (0x0080/4)   // Task Priority
6664 #define EOI     (0x00B0/4)   // EOI
6665 #define SVR     (0x00F0/4)   // Spurious Interrupt Vector
6666   #define ENABLE     0x00000100   // Unit Enable
6667 #define ESR     (0x0280/4)   // Error Status
6668 #define ICRLO   (0x0300/4)   // Interrupt Command
6669   #define INIT       0x00000500   // INIT/RESET
6670   #define STARTUP    0x00000600   // Startup IPI
6671   #define DELIVS     0x00001000   // Delivery status
6672   #define ASSERT     0x00004000   // Assert interrupt (vs deassert)
6673   #define DEASSERT   0x00000000
6674   #define LEVEL      0x00008000   // Level triggered
6675   #define BCAST      0x00080000   // Send to all APICs, including self.
6676   #define BUSY       0x00001000
6677   #define FIXED      0x00000000
6678 #define ICRHI   (0x0310/4)   // Interrupt Command [63:32]
6679 #define TIMER   (0x0320/4)   // Local Vector Table 0 (TIMER)
6680   #define X1         0x0000000B   // divide counts by 1
6681   #define PERIODIC   0x00020000   // Periodic
6682 #define PCINT   (0x0340/4)   // Performance Counter LVT
6683 #define LINT0   (0x0350/4)   // Local Vector Table 1 (LINT0)
6684 #define LINT1   (0x0360/4)   // Local Vector Table 2 (LINT1)
6685 #define ERROR   (0x0370/4)   // Local Vector Table 3 (ERROR)
6686   #define MASKED     0x00010000   // Interrupt masked
6687 #define TICR    (0x0380/4)   // Timer Initial Count
6688 #define TCCR    (0x0390/4)   // Timer Current Count
6689 #define TDCR    (0x03E0/4)   // Timer Divide Configuration
6690 
6691 volatile uint *lapic;  // Initialized in mp.c
6692 
6693 static void
6694 lapicw(int index, int value)
6695 {
6696   lapic[index] = value;
6697   lapic[ID];  // wait for write to finish, by reading
6698 }
6699 
6700 void
6701 lapicinit(void)
6702 {
6703   if(!lapic)
6704     return;
6705 
6706   // Enable local APIC; set spurious interrupt vector.
6707   lapicw(SVR, ENABLE | (T_IRQ0 + IRQ_SPURIOUS));
6708 
6709   // The timer repeatedly counts down at bus frequency
6710   // from lapic[TICR] and then issues an interrupt.
6711   // If xv6 cared more about precise timekeeping,
6712   // TICR would be calibrated using an external time source.
6713   lapicw(TDCR, X1);
6714   lapicw(TIMER, PERIODIC | (T_IRQ0 + IRQ_TIMER));
6715   lapicw(TICR, 10000000);
6716 
6717   // Disable logical interrupt lines.
6718   lapicw(LINT0, MASKED);
6719   lapicw(LINT1, MASKED);
6720 
6721   // Disable performance counter overflow interrupts
6722   // on machines that provide that interrupt entry.
6723   if(((lapic[VER]>>16) & 0xFF) >= 4)
6724     lapicw(PCINT, MASKED);
6725 
6726   // Map error interrupt to IRQ_ERROR.
6727   lapicw(ERROR, T_IRQ0 + IRQ_ERROR);
6728 
6729   // Clear error status register (requires back-to-back writes).
6730   lapicw(ESR, 0);
6731   lapicw(ESR, 0);
6732 
6733   // Ack any outstanding interrupts.
6734   lapicw(EOI, 0);
6735 
6736   // Send an Init Level De-Assert to synchronise arbitration ID's.
6737   lapicw(ICRHI, 0);
6738   lapicw(ICRLO, BCAST | INIT | LEVEL);
6739   while(lapic[ICRLO] & DELIVS)
6740     ;
6741 
6742   // Enable interrupts on the APIC (but not on the processor).
6743   lapicw(TPR, 0);
6744 }
6745 
6746 
6747 
6748 
6749 
6750 int
6751 cpunum(void)
6752 {
6753   // Cannot call cpu when interrupts are enabled:
6754   // result not guaranteed to last long enough to be used!
6755   // Would prefer to panic but even printing is chancy here:
6756   // almost everything, including cprintf and panic, calls cpu,
6757   // often indirectly through acquire and release.
6758   if(readeflags()&FL_IF){
6759     static int n;
6760     if(n++ == 0)
6761       cprintf("cpu called from %x with interrupts enabled\n",
6762         __builtin_return_address(0));
6763   }
6764 
6765   if(lapic)
6766     return lapic[ID]>>24;
6767   return 0;
6768 }
6769 
6770 // Acknowledge interrupt.
6771 void
6772 lapiceoi(void)
6773 {
6774   if(lapic)
6775     lapicw(EOI, 0);
6776 }
6777 
6778 // Spin for a given number of microseconds.
6779 // On real hardware would want to tune this dynamically.
6780 void
6781 microdelay(int us)
6782 {
6783 }
6784 
6785 #define IO_RTC  0x70
6786 
6787 // Start additional processor running entry code at addr.
6788 // See Appendix B of MultiProcessor Specification.
6789 void
6790 lapicstartap(uchar apicid, uint addr)
6791 {
6792   int i;
6793   ushort *wrv;
6794 
6795   // "The BSP must initialize CMOS shutdown code to 0AH
6796   // and the warm reset vector (DWORD based at 40:67) to point at
6797   // the AP startup code prior to the [universal startup algorithm]."
6798   outb(IO_RTC, 0xF);  // offset 0xF is shutdown code
6799   outb(IO_RTC+1, 0x0A);
6800   wrv = (ushort*)P2V((0x40<<4 | 0x67));  // Warm reset vector
6801   wrv[0] = 0;
6802   wrv[1] = addr >> 4;
6803 
6804   // "Universal startup algorithm."
6805   // Send INIT (level-triggered) interrupt to reset other CPU.
6806   lapicw(ICRHI, apicid<<24);
6807   lapicw(ICRLO, INIT | LEVEL | ASSERT);
6808   microdelay(200);
6809   lapicw(ICRLO, INIT | LEVEL);
6810   microdelay(100);    // should be 10ms, but too slow in Bochs!
6811 
6812   // Send startup IPI (twice!) to enter code.
6813   // Regular hardware is supposed to only accept a STARTUP
6814   // when it is in the halted state due to an INIT.  So the second
6815   // should be ignored, but it is part of the official Intel algorithm.
6816   // Bochs complains about the second one.  Too bad for Bochs.
6817   for(i = 0; i < 2; i++){
6818     lapicw(ICRHI, apicid<<24);
6819     lapicw(ICRLO, STARTUP | (addr>>12));
6820     microdelay(200);
6821   }
6822 }
6823 
6824 
6825 
6826 
6827 
6828 
6829 
6830 
6831 
6832 
6833 
6834 
6835 
6836 
6837 
6838 
6839 
6840 
6841 
6842 
6843 
6844 
6845 
6846 
6847 
6848 
6849 
