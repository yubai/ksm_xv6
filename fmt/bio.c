4050 // Buffer cache.
4051 //
4052 // The buffer cache is a linked list of buf structures holding
4053 // cached copies of disk block contents.  Caching disk blocks
4054 // in memory reduces the number of disk reads and also provides
4055 // a synchronization point for disk blocks used by multiple processes.
4056 //
4057 // Interface:
4058 // * To get a buffer for a particular disk block, call bread.
4059 // * After changing buffer data, call bwrite to write it to disk.
4060 // * When done with the buffer, call brelse.
4061 // * Do not use the buffer after calling brelse.
4062 // * Only one process at a time can use a buffer,
4063 //     so do not keep them longer than necessary.
4064 //
4065 // The implementation uses three state flags internally:
4066 // * B_BUSY: the block has been returned from bread
4067 //     and has not been passed back to brelse.
4068 // * B_VALID: the buffer data has been read from the disk.
4069 // * B_DIRTY: the buffer data has been modified
4070 //     and needs to be written to disk.
4071 
4072 #include "types.h"
4073 #include "defs.h"
4074 #include "param.h"
4075 #include "spinlock.h"
4076 #include "buf.h"
4077 
4078 struct {
4079   struct spinlock lock;
4080   struct buf buf[NBUF];
4081 
4082   // Linked list of all buffers, through prev/next.
4083   // head.next is most recently used.
4084   struct buf head;
4085 } bcache;
4086 
4087 void
4088 binit(void)
4089 {
4090   struct buf *b;
4091 
4092   initlock(&bcache.lock, "bcache");
4093 
4094 
4095 
4096 
4097 
4098 
4099 
4100   // Create linked list of buffers
4101   bcache.head.prev = &bcache.head;
4102   bcache.head.next = &bcache.head;
4103   for(b = bcache.buf; b < bcache.buf+NBUF; b++){
4104     b->next = bcache.head.next;
4105     b->prev = &bcache.head;
4106     b->dev = -1;
4107     bcache.head.next->prev = b;
4108     bcache.head.next = b;
4109   }
4110 }
4111 
4112 // Look through buffer cache for sector on device dev.
4113 // If not found, allocate fresh block.
4114 // In either case, return B_BUSY buffer.
4115 static struct buf*
4116 bget(uint dev, uint sector)
4117 {
4118   struct buf *b;
4119 
4120   acquire(&bcache.lock);
4121 
4122  loop:
4123   // Is the sector already cached?
4124   for(b = bcache.head.next; b != &bcache.head; b = b->next){
4125     if(b->dev == dev && b->sector == sector){
4126       if(!(b->flags & B_BUSY)){
4127         b->flags |= B_BUSY;
4128         release(&bcache.lock);
4129         return b;
4130       }
4131       sleep(b, &bcache.lock);
4132       goto loop;
4133     }
4134   }
4135 
4136   // Not cached; recycle some non-busy and clean buffer.
4137   for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
4138     if((b->flags & B_BUSY) == 0 && (b->flags & B_DIRTY) == 0){
4139       b->dev = dev;
4140       b->sector = sector;
4141       b->flags = B_BUSY;
4142       release(&bcache.lock);
4143       return b;
4144     }
4145   }
4146   panic("bget: no buffers");
4147 }
4148 
4149 
4150 // Return a B_BUSY buf with the contents of the indicated disk sector.
4151 struct buf*
4152 bread(uint dev, uint sector)
4153 {
4154   struct buf *b;
4155 
4156   b = bget(dev, sector);
4157   if(!(b->flags & B_VALID))
4158     iderw(b);
4159   return b;
4160 }
4161 
4162 // Write b's contents to disk.  Must be B_BUSY.
4163 void
4164 bwrite(struct buf *b)
4165 {
4166   if((b->flags & B_BUSY) == 0)
4167     panic("bwrite");
4168   b->flags |= B_DIRTY;
4169   iderw(b);
4170 }
4171 
4172 // Release a B_BUSY buffer.
4173 // Move to the head of the MRU list.
4174 void
4175 brelse(struct buf *b)
4176 {
4177   if((b->flags & B_BUSY) == 0)
4178     panic("brelse");
4179 
4180   acquire(&bcache.lock);
4181 
4182   b->next->prev = b->prev;
4183   b->prev->next = b->next;
4184   b->next = bcache.head.next;
4185   b->prev = &bcache.head;
4186   bcache.head.next->prev = b;
4187   bcache.head.next = b;
4188 
4189   b->flags &= ~B_BUSY;
4190   wakeup(b);
4191 
4192   release(&bcache.lock);
4193 }
4194 
4195 
4196 
4197 
4198 
4199 
