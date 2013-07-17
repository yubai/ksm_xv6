// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"

#define BYTESIZE           8
#define WORDSIZE           32
#define BYTEROUNDUP(sz)    (((sz)+BYTESIZE-1) & ~(BYTESIZE-1))
#define BYTEROUNDDOWN(sz)  (((sz)) & ~(BYTESIZE-1))
#define BYTESHIFT          3

#define WORDROUNDUP(sz)    (((sz)+WORDSIZE-1) & ~(WORDSIZE-1))

void initbitmap(void *vstart, void *vend);
extern char end[]; // first address after kernel loaded from ELF file

struct bitmap {
    unsigned int nb;      /* Number of bitmap size */
    void* start;
    char bitmaps[];       /* Bitmap array  */
};

struct {
    struct spinlock lock;
    int use_lock;
    struct bitmap* bmap;
} kmem;

// Initialization happens in two phases.
// 1. main() calls kinit1() while still using entrypgdir to place just
// the pages mapped by entrypgdir on free list.
// 2. main() calls kinit2() with the rest of the physical pages
// after installing a full page table that maps them on all cores.
void
kinit1(void *vstart, void *vend)
{
  initlock(&kmem.lock, "kmem");
  kmem.use_lock = 0;

  initbitmap(vstart, vend);
}

void
kinit2(void *vstart, void *vend)
{
    initbitmap(vstart, vend);
    kmem.use_lock = 1;
}

void
initbitmap(void *vstart, void *vend)
{
    kmem.bmap = vstart;

    char *begin, *end;
    begin = (char*)PGROUNDUP((uint)vstart);
    end = (char*)PGROUNDDOWN((uint)vend);

    kmem.bmap->nb = ((unsigned int)end - (unsigned int)begin) >> PGSHIFT;

    unsigned int bytes = BYTEROUNDUP(kmem.bmap->nb) >> BYTESHIFT;
    unsigned int sz = bytes + sizeof(unsigned int) + sizeof(void*);

    while (((unsigned int)begin - (unsigned int)vstart) < sz) {
        begin += PGSIZE;
        kmem.bmap->nb--;
    }

    bytes = BYTEROUNDUP(kmem.bmap->nb) >> BYTESHIFT;
    kmem.bmap->start = begin;

    memset(kmem.bmap->bitmaps, 0, bytes);
}

//PAGEBREAK: 21
// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(char *v)
{
  if((uint)v % PGSIZE || v < end || v2p(v) >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(v, 1, PGSIZE);

  if(kmem.use_lock)
    acquire(&kmem.lock);

  unsigned int bit = (void*)v - (void*)kmem.bmap->start;
  unsigned int byte = BYTEROUNDDOWN(bit) >> BYTESHIFT;
  bit -= (byte << BYTESHIFT);

  unsigned int mask = BYTESIZE - bit - 1;

  *(kmem.bmap->bitmaps + byte)  &= (char)(~(1 << mask));

  if(kmem.use_lock)
    release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char*
kalloc(void)
{
  char *start;
  int i, j;
  char* byte;

  if(kmem.use_lock)
    acquire(&kmem.lock);

  for (i = 0; i < kmem.bmap->nb;) {
    for (j = BYTESIZE - 1; j >= 0; j--, i++) {
      byte = kmem.bmap->bitmaps + BYTEROUNDDOWN(i);
      if ((*byte & (1 << j)) == 0) {
        *byte |= 1 << j;
        start = (char*)((unsigned int)kmem.bmap->start
                        + (unsigned int)(i << PGSHIFT));
        goto __UNLOCK;
      }
    }
  }

__UNLOCK:
  if(kmem.use_lock)
    release(&kmem.lock);
  return start;
}
