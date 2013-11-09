8550 // Boot loader.
8551 //
8552 // Part of the boot sector, along with bootasm.S, which calls bootmain().
8553 // bootasm.S has put the processor into protected 32-bit mode.
8554 // bootmain() loads an ELF kernel image from the disk starting at
8555 // sector 1 and then jumps to the kernel entry routine.
8556 
8557 #include "types.h"
8558 #include "elf.h"
8559 #include "x86.h"
8560 #include "memlayout.h"
8561 
8562 #define SECTSIZE  512
8563 
8564 void readseg(uchar*, uint, uint);
8565 
8566 void
8567 bootmain(void)
8568 {
8569   struct elfhdr *elf;
8570   struct proghdr *ph, *eph;
8571   void (*entry)(void);
8572   uchar* pa;
8573 
8574   elf = (struct elfhdr*)0x10000;  // scratch space
8575 
8576   // Read 1st page off disk
8577   readseg((uchar*)elf, 4096, 0);
8578 
8579   // Is this an ELF executable?
8580   if(elf->magic != ELF_MAGIC)
8581     return;  // let bootasm.S handle error
8582 
8583   // Load each program segment (ignores ph flags).
8584   ph = (struct proghdr*)((uchar*)elf + elf->phoff);
8585   eph = ph + elf->phnum;
8586   for(; ph < eph; ph++){
8587     pa = (uchar*)ph->paddr;
8588     readseg(pa, ph->filesz, ph->off);
8589     if(ph->memsz > ph->filesz)
8590       stosb(pa + ph->filesz, 0, ph->memsz - ph->filesz);
8591   }
8592 
8593   // Call the entry point from the ELF header.
8594   // Does not return!
8595   entry = (void(*)(void))(elf->entry);
8596   entry();
8597 }
8598 
8599 
8600 void
8601 waitdisk(void)
8602 {
8603   // Wait for disk ready.
8604   while((inb(0x1F7) & 0xC0) != 0x40)
8605     ;
8606 }
8607 
8608 // Read a single sector at offset into dst.
8609 void
8610 readsect(void *dst, uint offset)
8611 {
8612   // Issue command.
8613   waitdisk();
8614   outb(0x1F2, 1);   // count = 1
8615   outb(0x1F3, offset);
8616   outb(0x1F4, offset >> 8);
8617   outb(0x1F5, offset >> 16);
8618   outb(0x1F6, (offset >> 24) | 0xE0);
8619   outb(0x1F7, 0x20);  // cmd 0x20 - read sectors
8620 
8621   // Read data.
8622   waitdisk();
8623   insl(0x1F0, dst, SECTSIZE/4);
8624 }
8625 
8626 // Read 'count' bytes at 'offset' from kernel into physical address 'pa'.
8627 // Might copy more than asked.
8628 void
8629 readseg(uchar* pa, uint count, uint offset)
8630 {
8631   uchar* epa;
8632 
8633   epa = pa + count;
8634 
8635   // Round down to sector boundary.
8636   pa -= offset % SECTSIZE;
8637 
8638   // Translate from bytes to sectors; kernel starts at sector 1.
8639   offset = (offset / SECTSIZE) + 1;
8640 
8641   // If this is too slow, we could read lots of sectors at a time.
8642   // We'd write more to memory than asked, but it doesn't matter --
8643   // we load in increasing order.
8644   for(; pa < epa; pa += SECTSIZE, offset++)
8645     readsect(pa, offset);
8646 }
8647 
8648 
8649 
