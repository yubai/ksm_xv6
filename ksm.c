/***
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Author:
 *     2012 Bai Yu - zjuyubai@gmail.com
 */

#include "types.h"
#include "defs.h"
#include "spinlock.h"
#include "ksm.h"

#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

#define NAME_MAX 32     // maximum length of file name
#define NRKSM    32     // maximum number of shared memories
#define NRPG     1024   // maximum number of pages available for allocating

struct ksm_pg_t {
	void* addr;
	int next;
};

struct {
	struct spinlock lock;
	int first_free;
	struct ksm_pg_t pg[NRPG];
}ksmpgtable;

struct ksminfo_global_t {
	uint total_shrg_nr;    // Total number of existing shared regions
	uint total_shpg_nr;    // Total number of existing shared pages
}ksminfo_global;

// KSM handler type
struct ksm_t {
	char ksm_name[NAME_MAX+1];  // KSM name
	struct ksminfo_t ksminfo;   // KSM info structure
	int first_pg_id;            // Id for the first allocated page
	int id;                     // myself
	int next_free;              // next free ksm handler
};

struct {
	struct spinlock lock;
	int first_free;
	struct ksm_t ksm[NRKSM];
} ksmtable;

void
ksminit(void)
{
	int i = 0;
	
	initlock(&ksmtable.lock, "ksmtable");
	acquire(&ksmtable.lock);
	
	struct ksm_t* p;
	ksmtable.first_free = 0;
	for (p = ksmtable.ksm; p < &ksmtable.ksm[NRKSM]; p++) {
		memset(p->ksm_name, 0, NAME_MAX + 1);
		memset((char*)&(p->ksminfo), 0, sizeof(struct ksminfo_t));
		p->id = i;
		p->next_free = ++i;
		p->first_pg_id = -1;
	}
	ksmtable.ksm[NRKSM-1].next_free = -1;
	
	ksminfo_global.total_shrg_nr = 0;
	ksminfo_global.total_shpg_nr = 0;
	
	release(&ksmtable.lock);
	
	initlock(&ksmpgtable.lock, "ksmpgtable");
	acquire(&ksmpgtable.lock);
	struct ksm_pg_t *pg;
	for (i = 0; i < NRPG; i++) {
		pg = &ksmpgtable.pg[i];
		pg->addr = 0;
		pg->next = i+1;
	}
	ksmpgtable.pg[i-1].next = -1;
	ksmpgtable.first_free = 0;
	release(&ksmpgtable.lock);
}

int
ksmget(char* name, uint size)
{
	struct ksm_t *p;
	char *mem;
	uint a, pgsz;
	
	acquire(&ksmtable.lock);
	acquire(&ksmpgtable.lock);
	
	for (p = ksmtable.ksm; p < &ksmtable.ksm[NRKSM]; p++) {
		if (strncmp(p->ksm_name, name, NAME_MAX) == 0)
			goto found;
	}
	
	if (ksmtable.first_free == -1)
		return -1;
	
	p = &ksmtable.ksm[ksmtable.first_free];
	ksmtable.first_free = p->next_free;
	p->ksminfo.cpid = proc->pid;
	
found:
	p->ksminfo.ksmsz = size;
	pgsz = PGROUNDUP(p->ksminfo.ksmsz);
	
	for (a = 0; a < pgsz; a += PGSIZE) {
		mem = kalloc();
		int pgid = ksmpgtable.first_free;
		if (mem == 0 || pgid == -1) {
			cprintf("No enough space for %d size of shared memory!\n", size);
			// Clean up the allocated pages here
			if (mem != 0)
				kfree(mem);
			int current;
			current = p->first_pg_id;
			struct ksm_pg_t *ksmpg;
			while (current != -1) {
				ksmpg = &ksmpgtable.pg[current];
				kfree(ksmpg->addr);
				current = ksmpg->next;
			}
			release(&ksmpgtable.lock);
			release(&ksmtable.lock);
			return -1;
		}
		memset(mem, 0, PGSIZE);
		
		ksminfo_global.total_shpg_nr++;
		
		// Inserting into the list of pages of ksm_t
		struct ksm_pg_t *pg = &ksmpgtable.pg[pgid];
		ksmpgtable.first_free = pg->next;
		pg->addr = mem;
		pg->next = p->first_pg_id;
		p->first_pg_id = pgid;
	}
	
	ksminfo_global.total_shrg_nr++;
	
	int id = proc->ksmhdtable.first_free;
	proc->ksmhdtable.ksm[id].ksmhd = p->id + 1;
	proc->ksmhdtable.first_free = proc->ksmhdtable.ksm[id].next;
	proc->ksmhdtable.ksm[id].next = proc->ksmhdtable.first_id;
	proc->ksmhdtable.first_id = id;
	
	release(&ksmpgtable.lock);
	release(&ksmtable.lock);
	
	return p->id+1;
}

static struct ksmhd_t *
ksmhd_find(int hd) {
	int current;
	current = proc->ksmhdtable.first_id;
	struct ksmhd_t *ksmhd;
	while (current != -1) {
		ksmhd = &proc->ksmhdtable.ksm[current];
		if (ksmhd->ksmhd == hd) {
			return ksmhd;
		}
		current = ksmhd->next;
	}
	return 0;
}

int
ksmattach(int hd, int flag)
{
	if (hd < 0 || hd >= NRKSM)
		return -1;
	
	acquire(&ksmtable.lock);
	struct ksm_t *p = &ksmtable.ksm[hd-1];
	
	uint pgsz;
	void* addr;
	int perm;
	
	switch (flag) {
		case KSM_READ:
			perm = PTE_U;
			break;
		case KSM_RDWR:
			perm = PTE_W | PTE_U;
			break;
		default:
			cprintf ( "Unknown flag of ksmattach!\n" );
			return -1;
	}
	
	pgsz = PGROUNDUP(p->ksminfo.ksmsz);
	
	struct ksmhd_t *ksmhd = ksmhd_find(hd);
	if (ksmhd == 0)
		return -1;
	
	addr = (void*)PGROUNDUP(proc->sz);
	ksmhd->addr = addr;        // Starting virtual address
	
	proc->sz += pgsz;
	
	struct ksm_pg_t *ksmpg;
	int current = p->first_pg_id;
	while (current != -1) {
		ksmpg = &ksmpgtable.pg[current];
		mappages(proc->pgdir, addr, PGSIZE, v2p(ksmpg->addr), perm);
		addr += PGSIZE;
		current = ksmpg->next;
	}
	
	// inc the attached number of processes
	p->ksminfo.attached_nr++;
	
	// update the latest attach time
	acquire(&tickslock);
	p->ksminfo.atime = ticks;
	release(&tickslock);
	
	release(&ksmtable.lock);
	
	return (int)ksmhd->addr;
}

int
ksmdetach(int hd)
{
	if (hd < 0 || hd >= NRKSM)
		return -1;
	
	acquire(&ksmtable.lock);
	struct ksm_t *p = &ksmtable.ksm[hd-1];
	void* addr;
	
	struct ksmhd_t *ksmhd = ksmhd_find(hd);
	if (ksmhd == 0)
		return -1;
	addr = ksmhd->addr;
	
	struct ksm_pg_t *ksmpg;
	int current = p->first_pg_id;
	while (current != -1) {
		ksmpg = &ksmpgtable.pg[current];
		unmappages(proc->pgdir, addr, PGSIZE, v2p(ksmpg->addr));
		addr += PGSIZE;
		current = ksmpg->next;
	}
	
	// decrease the attached number of processes
	p->ksminfo.attached_nr--;
	
	// update the latest attach time
	acquire(&tickslock);
	p->ksminfo.dtime = ticks;
	release(&tickslock);
	
	release(&ksmtable.lock);
	
	return 0;
}

int
ksminfo(int hd, struct ksminfo_t* info)
{
	struct ksm_t *p;
	
	if (hd < 0 || hd >= NRKSM)
		return -1;
	
	if (hd > 0) {
		p = &ksmtable.ksm[hd-1];
		memmove(info, &p->ksminfo, sizeof(struct ksminfo_t));
	}
	info->total_shrg_nr = ksminfo_global.total_shrg_nr;
	info->total_shpg_nr = ksminfo_global.total_shpg_nr;
	
	return 0;
}

int
ksmdelete(int hd)
{
	if (hd < 0 || hd >= NRKSM)
		return -1;
	
	struct ksm_t *p;
	uint pgnr;
	
	acquire(&ksmtable.lock);
	
	p = &ksmtable.ksm[hd-1];
	pgnr = PGROUNDUP(p->ksminfo.ksmsz) >> PGSHIFT;
	
	// Clean up the allocated pages here
	struct ksm_pg_t *ksmpg;
	int current = p->first_pg_id;
	while (current != -1) {
		ksmpg = &ksmpgtable.pg[current];
		kfree(ksmpg->addr);
		current = ksmpg->next;
		ksminfo_global.total_shpg_nr--;
	}
	p->first_pg_id = -1;
	
	struct ksmhd_t *ksmhd = ksmhd_find(hd);
	if (ksmhd == 0)
		return -1;
	struct ksmhd_t *prev = &proc->ksmhdtable.ksm[ksmhd->prev];
	struct ksmhd_t *next = &proc->ksmhdtable.ksm[ksmhd->next];
	prev->next = next->id;
	next->prev = prev->id;
	
	ksmhd->next = proc->ksmhdtable.first_free;
	proc->ksmhdtable.first_free = ksmhd->id;
	
	ksminfo_global.total_shpg_nr -= pgnr;
	ksminfo_global.total_shrg_nr--;
	
	p->next_free = ksmtable.first_free;
	ksmtable.first_free = p->id;
    
	release(&ksmtable.lock);
	
	return 0;
}
