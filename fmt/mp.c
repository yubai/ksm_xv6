6450 // Multiprocessor support
6451 // Search memory for MP description structures.
6452 // http://developer.intel.com/design/pentium/datashts/24201606.pdf
6453 
6454 #include "types.h"
6455 #include "defs.h"
6456 #include "param.h"
6457 #include "memlayout.h"
6458 #include "mp.h"
6459 #include "x86.h"
6460 #include "mmu.h"
6461 #include "proc.h"
6462 
6463 struct cpu cpus[NCPU];
6464 static struct cpu *bcpu;
6465 int ismp;
6466 int ncpu;
6467 uchar ioapicid;
6468 
6469 int
6470 mpbcpu(void)
6471 {
6472   return bcpu-cpus;
6473 }
6474 
6475 static uchar
6476 sum(uchar *addr, int len)
6477 {
6478   int i, sum;
6479 
6480   sum = 0;
6481   for(i=0; i<len; i++)
6482     sum += addr[i];
6483   return sum;
6484 }
6485 
6486 // Look for an MP structure in the len bytes at addr.
6487 static struct mp*
6488 mpsearch1(uint a, int len)
6489 {
6490   uchar *e, *p, *addr;
6491 
6492   addr = p2v(a);
6493   e = addr+len;
6494   for(p = addr; p < e; p += sizeof(struct mp))
6495     if(memcmp(p, "_MP_", 4) == 0 && sum(p, sizeof(struct mp)) == 0)
6496       return (struct mp*)p;
6497   return 0;
6498 }
6499 
6500 // Search for the MP Floating Pointer Structure, which according to the
6501 // spec is in one of the following three locations:
6502 // 1) in the first KB of the EBDA;
6503 // 2) in the last KB of system base memory;
6504 // 3) in the BIOS ROM between 0xE0000 and 0xFFFFF.
6505 static struct mp*
6506 mpsearch(void)
6507 {
6508   uchar *bda;
6509   uint p;
6510   struct mp *mp;
6511 
6512   bda = (uchar *) P2V(0x400);
6513   if((p = ((bda[0x0F]<<8)| bda[0x0E]) << 4)){
6514     if((mp = mpsearch1(p, 1024)))
6515       return mp;
6516   } else {
6517     p = ((bda[0x14]<<8)|bda[0x13])*1024;
6518     if((mp = mpsearch1(p-1024, 1024)))
6519       return mp;
6520   }
6521   return mpsearch1(0xF0000, 0x10000);
6522 }
6523 
6524 // Search for an MP configuration table.  For now,
6525 // don't accept the default configurations (physaddr == 0).
6526 // Check for correct signature, calculate the checksum and,
6527 // if correct, check the version.
6528 // To do: check extended table checksum.
6529 static struct mpconf*
6530 mpconfig(struct mp **pmp)
6531 {
6532   struct mpconf *conf;
6533   struct mp *mp;
6534 
6535   if((mp = mpsearch()) == 0 || mp->physaddr == 0)
6536     return 0;
6537   conf = (struct mpconf*) p2v((uint) mp->physaddr);
6538   if(memcmp(conf, "PCMP", 4) != 0)
6539     return 0;
6540   if(conf->version != 1 && conf->version != 4)
6541     return 0;
6542   if(sum((uchar*)conf, conf->length) != 0)
6543     return 0;
6544   *pmp = mp;
6545   return conf;
6546 }
6547 
6548 
6549 
6550 void
6551 mpinit(void)
6552 {
6553   uchar *p, *e;
6554   struct mp *mp;
6555   struct mpconf *conf;
6556   struct mpproc *proc;
6557   struct mpioapic *ioapic;
6558 
6559   bcpu = &cpus[0];
6560   if((conf = mpconfig(&mp)) == 0)
6561     return;
6562   ismp = 1;
6563   lapic = (uint*)conf->lapicaddr;
6564   for(p=(uchar*)(conf+1), e=(uchar*)conf+conf->length; p<e; ){
6565     switch(*p){
6566     case MPPROC:
6567       proc = (struct mpproc*)p;
6568       if(ncpu != proc->apicid){
6569         cprintf("mpinit: ncpu=%d apicid=%d\n", ncpu, proc->apicid);
6570         ismp = 0;
6571       }
6572       if(proc->flags & MPBOOT)
6573         bcpu = &cpus[ncpu];
6574       cpus[ncpu].id = ncpu;
6575       ncpu++;
6576       p += sizeof(struct mpproc);
6577       continue;
6578     case MPIOAPIC:
6579       ioapic = (struct mpioapic*)p;
6580       ioapicid = ioapic->apicno;
6581       p += sizeof(struct mpioapic);
6582       continue;
6583     case MPBUS:
6584     case MPIOINTR:
6585     case MPLINTR:
6586       p += 8;
6587       continue;
6588     default:
6589       cprintf("mpinit: unknown config type %x\n", *p);
6590       ismp = 0;
6591     }
6592   }
6593   if(!ismp){
6594     // Didn't like what we found; fall back to no MP.
6595     ncpu = 1;
6596     lapic = 0;
6597     ioapicid = 0;
6598     return;
6599   }
6600   if(mp->imcrp){
6601     // Bochs doesn't support IMCR, so this doesn't run on Bochs.
6602     // But it would on real hardware.
6603     outb(0x22, 0x70);   // Select IMCR
6604     outb(0x23, inb(0x23) | 1);  // Mask external interrupts.
6605   }
6606 }
6607 
6608 
6609 
6610 
6611 
6612 
6613 
6614 
6615 
6616 
6617 
6618 
6619 
6620 
6621 
6622 
6623 
6624 
6625 
6626 
6627 
6628 
6629 
6630 
6631 
6632 
6633 
6634 
6635 
6636 
6637 
6638 
6639 
6640 
6641 
6642 
6643 
6644 
6645 
6646 
6647 
6648 
6649 
