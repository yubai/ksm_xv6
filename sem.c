#include "types.h"
#include "defs.h"
#include "spinlock.h"

// semaphore
#define NAME_MAX 32     // maximum length of file name
#define NRSEM    32     // maximum number of semaphores

typedef uint semhd_id;

struct semaphore_t {
  int value;             // semaphore value
  int counter;             // semaphore current value
  int name;
  semhd_id id;                // myself
  semhd_id next_free;         // next free sem handler
};

struct {
  struct spinlock lock;
  semhd_id first_free;
  struct semaphore_t sem[NRSEM];
} semtable;

void
seminit(void)
{
  int i = 0;

  initlock(&semtable.lock, "sem");
  acquire(&semtable.lock);

  struct semaphore_t* p;
  semtable.first_free = 0;
  for (p = semtable.sem; p < &semtable.sem[NRSEM]; p++) {
    p->name = -1;
    p->id = i;
    p->next_free = ++i;
  }
  semtable.sem[NRSEM-1].next_free = -1;

  release(&semtable.lock);
}

int sem_get(uint name, int value)
{
  struct semaphore_t *p;

  acquire(&semtable.lock);

  for (p = semtable.sem; p < &semtable.sem[NRSEM]; p++) {
//    if (strncmp(p->name, name, NAME_MAX) == 0)
    if (p->name == name)    
      goto found;
  }

  if (semtable.first_free == -1)
    return -1;

  p->name = name;
  p = &semtable.sem[semtable.first_free];
  semtable.first_free = p->next_free;

found:
  p->counter = p->value = value;
  release(&semtable.lock);

  return p->id + 1;
}

int sem_signal(int hd)
{
  acquire(&semtable.lock);

  struct semaphore_t* p;
  p = &semtable.sem[hd - 1];
  p->counter++;
  wakeup(p);
  
  release(&semtable.lock);

  return 0;
}

int sem_wait(int hd)
{
  if (hd < 0 || hd > NRSEM)
    return -1;
  acquire(&semtable.lock);
  
  struct semaphore_t* p;
  p = &semtable.sem[hd - 1];
  while (p->counter <= 0) {
    sleep(p, &semtable.lock);
  }
  p->counter--;
  
  release(&semtable.lock);
  
  return 0;
}

int sem_delete(int hd)
{
  if (hd < 0 || hd > NRSEM)
    return -1;

  acquire(&semtable.lock);
  
  struct semaphore_t* p;
  p = &semtable.sem[hd - 1];

  p->name = -1;
  p->next_free = semtable.first_free;
  semtable.first_free = p->id;
  
  release(&semtable.lock);
  
  return 0;
}
