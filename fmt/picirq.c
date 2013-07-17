6950 // Intel 8259A programmable interrupt controllers.
6951 
6952 #include "types.h"
6953 #include "x86.h"
6954 #include "traps.h"
6955 
6956 // I/O Addresses of the two programmable interrupt controllers
6957 #define IO_PIC1         0x20    // Master (IRQs 0-7)
6958 #define IO_PIC2         0xA0    // Slave (IRQs 8-15)
6959 
6960 #define IRQ_SLAVE       2       // IRQ at which slave connects to master
6961 
6962 // Current IRQ mask.
6963 // Initial IRQ mask has interrupt 2 enabled (for slave 8259A).
6964 static ushort irqmask = 0xFFFF & ~(1<<IRQ_SLAVE);
6965 
6966 static void
6967 picsetmask(ushort mask)
6968 {
6969   irqmask = mask;
6970   outb(IO_PIC1+1, mask);
6971   outb(IO_PIC2+1, mask >> 8);
6972 }
6973 
6974 void
6975 picenable(int irq)
6976 {
6977   picsetmask(irqmask & ~(1<<irq));
6978 }
6979 
6980 // Initialize the 8259A interrupt controllers.
6981 void
6982 picinit(void)
6983 {
6984   // mask all interrupts
6985   outb(IO_PIC1+1, 0xFF);
6986   outb(IO_PIC2+1, 0xFF);
6987 
6988   // Set up master (8259A-1)
6989 
6990   // ICW1:  0001g0hi
6991   //    g:  0 = edge triggering, 1 = level triggering
6992   //    h:  0 = cascaded PICs, 1 = master only
6993   //    i:  0 = no ICW4, 1 = ICW4 required
6994   outb(IO_PIC1, 0x11);
6995 
6996   // ICW2:  Vector offset
6997   outb(IO_PIC1+1, T_IRQ0);
6998 
6999 
7000   // ICW3:  (master PIC) bit mask of IR lines connected to slaves
7001   //        (slave PIC) 3-bit # of slave's connection to master
7002   outb(IO_PIC1+1, 1<<IRQ_SLAVE);
7003 
7004   // ICW4:  000nbmap
7005   //    n:  1 = special fully nested mode
7006   //    b:  1 = buffered mode
7007   //    m:  0 = slave PIC, 1 = master PIC
7008   //      (ignored when b is 0, as the master/slave role
7009   //      can be hardwired).
7010   //    a:  1 = Automatic EOI mode
7011   //    p:  0 = MCS-80/85 mode, 1 = intel x86 mode
7012   outb(IO_PIC1+1, 0x3);
7013 
7014   // Set up slave (8259A-2)
7015   outb(IO_PIC2, 0x11);                  // ICW1
7016   outb(IO_PIC2+1, T_IRQ0 + 8);      // ICW2
7017   outb(IO_PIC2+1, IRQ_SLAVE);           // ICW3
7018   // NB Automatic EOI mode doesn't tend to work on the slave.
7019   // Linux source code says it's "to be investigated".
7020   outb(IO_PIC2+1, 0x3);                 // ICW4
7021 
7022   // OCW3:  0ef01prs
7023   //   ef:  0x = NOP, 10 = clear specific mask, 11 = set specific mask
7024   //    p:  0 = no polling, 1 = polling mode
7025   //   rs:  0x = NOP, 10 = read IRR, 11 = read ISR
7026   outb(IO_PIC1, 0x68);             // clear specific mask
7027   outb(IO_PIC1, 0x0a);             // read IRR by default
7028 
7029   outb(IO_PIC2, 0x68);             // OCW3
7030   outb(IO_PIC2, 0x0a);             // OCW3
7031 
7032   if(irqmask != 0xFFFF)
7033     picsetmask(irqmask);
7034 }
7035 
7036 
7037 
7038 
7039 
7040 
7041 
7042 
7043 
7044 
7045 
7046 
7047 
7048 
7049 
