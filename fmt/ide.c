3850 // Simple PIO-based (non-DMA) IDE driver code.
3851 
3852 #include "types.h"
3853 #include "defs.h"
3854 #include "param.h"
3855 #include "memlayout.h"
3856 #include "mmu.h"
3857 #include "proc.h"
3858 #include "x86.h"
3859 #include "traps.h"
3860 #include "spinlock.h"
3861 #include "buf.h"
3862 
3863 #define IDE_BSY       0x80
3864 #define IDE_DRDY      0x40
3865 #define IDE_DF        0x20
3866 #define IDE_ERR       0x01
3867 
3868 #define IDE_CMD_READ  0x20
3869 #define IDE_CMD_WRITE 0x30
3870 
3871 // idequeue points to the buf now being read/written to the disk.
3872 // idequeue->qnext points to the next buf to be processed.
3873 // You must hold idelock while manipulating queue.
3874 
3875 static struct spinlock idelock;
3876 static struct buf *idequeue;
3877 
3878 static int havedisk1;
3879 static void idestart(struct buf*);
3880 
3881 // Wait for IDE disk to become ready.
3882 static int
3883 idewait(int checkerr)
3884 {
3885   int r;
3886 
3887   while(((r = inb(0x1f7)) & (IDE_BSY|IDE_DRDY)) != IDE_DRDY)
3888     ;
3889   if(checkerr && (r & (IDE_DF|IDE_ERR)) != 0)
3890     return -1;
3891   return 0;
3892 }
3893 
3894 
3895 
3896 
3897 
3898 
3899 
3900 void
3901 ideinit(void)
3902 {
3903   int i;
3904 
3905   initlock(&idelock, "ide");
3906   picenable(IRQ_IDE);
3907   ioapicenable(IRQ_IDE, ncpu - 1);
3908   idewait(0);
3909 
3910   // Check if disk 1 is present
3911   outb(0x1f6, 0xe0 | (1<<4));
3912   for(i=0; i<1000; i++){
3913     if(inb(0x1f7) != 0){
3914       havedisk1 = 1;
3915       break;
3916     }
3917   }
3918 
3919   // Switch back to disk 0.
3920   outb(0x1f6, 0xe0 | (0<<4));
3921 }
3922 
3923 // Start the request for b.  Caller must hold idelock.
3924 static void
3925 idestart(struct buf *b)
3926 {
3927   if(b == 0)
3928     panic("idestart");
3929 
3930   idewait(0);
3931   outb(0x3f6, 0);  // generate interrupt
3932   outb(0x1f2, 1);  // number of sectors
3933   outb(0x1f3, b->sector & 0xff);
3934   outb(0x1f4, (b->sector >> 8) & 0xff);
3935   outb(0x1f5, (b->sector >> 16) & 0xff);
3936   outb(0x1f6, 0xe0 | ((b->dev&1)<<4) | ((b->sector>>24)&0x0f));
3937   if(b->flags & B_DIRTY){
3938     outb(0x1f7, IDE_CMD_WRITE);
3939     outsl(0x1f0, b->data, 512/4);
3940   } else {
3941     outb(0x1f7, IDE_CMD_READ);
3942   }
3943 }
3944 
3945 
3946 
3947 
3948 
3949 
3950 // Interrupt handler.
3951 void
3952 ideintr(void)
3953 {
3954   struct buf *b;
3955 
3956   // First queued buffer is the active request.
3957   acquire(&idelock);
3958   if((b = idequeue) == 0){
3959     release(&idelock);
3960     // cprintf("spurious IDE interrupt\n");
3961     return;
3962   }
3963   idequeue = b->qnext;
3964 
3965   // Read data if needed.
3966   if(!(b->flags & B_DIRTY) && idewait(1) >= 0)
3967     insl(0x1f0, b->data, 512/4);
3968 
3969   // Wake process waiting for this buf.
3970   b->flags |= B_VALID;
3971   b->flags &= ~B_DIRTY;
3972   wakeup(b);
3973 
3974   // Start disk on next buf in queue.
3975   if(idequeue != 0)
3976     idestart(idequeue);
3977 
3978   release(&idelock);
3979 }
3980 
3981 
3982 
3983 
3984 
3985 
3986 
3987 
3988 
3989 
3990 
3991 
3992 
3993 
3994 
3995 
3996 
3997 
3998 
3999 
4000 // Sync buf with disk.
4001 // If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
4002 // Else if B_VALID is not set, read buf from disk, set B_VALID.
4003 void
4004 iderw(struct buf *b)
4005 {
4006   struct buf **pp;
4007 
4008   if(!(b->flags & B_BUSY))
4009     panic("iderw: buf not busy");
4010   if((b->flags & (B_VALID|B_DIRTY)) == B_VALID)
4011     panic("iderw: nothing to do");
4012   if(b->dev != 0 && !havedisk1)
4013     panic("iderw: ide disk 1 not present");
4014 
4015   acquire(&idelock);  //DOC:acquire-lock
4016 
4017   // Append b to idequeue.
4018   b->qnext = 0;
4019   for(pp=&idequeue; *pp; pp=&(*pp)->qnext)  //DOC:insert-queue
4020     ;
4021   *pp = b;
4022 
4023   // Start disk if necessary.
4024   if(idequeue == b)
4025     idestart(b);
4026 
4027   // Wait for request to finish.
4028   while((b->flags & (B_VALID|B_DIRTY)) != B_VALID){
4029     sleep(b, &idelock);
4030   }
4031 
4032   release(&idelock);
4033 }
4034 
4035 
4036 
4037 
4038 
4039 
4040 
4041 
4042 
4043 
4044 
4045 
4046 
4047 
4048 
4049 
