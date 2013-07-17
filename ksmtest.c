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
#include "user.h"
#include "ksm.h"

#define PGSIZE          4096    // bytes mapped by a page

// KAUST SHARED MEMORY
#define KSM_RDWR           0x00
#define KSM_READ           0x01

static void
ksminfo_dump(int hd, struct ksminfo_t *info)
{
	printf(1, "KSM information (handler=%d) dumping:\n", hd );
	printf(1, " --ksmsz = \t\t%d\n", info->ksmsz );
	printf(1, " --cpid = \t\t%d\n",  info->cpid );
	printf(1, " --mpid = \t\t%d\n",  info->mpid );
	printf(1, " --attached_nr = \t%d\n", info->attached_nr );
	printf(1, " --atime = \t\t%d\n", info->atime );
	printf(1, " --dtime = \t\t%d\n", info->dtime );
	printf(1, " --total_shrg_nr = \t%d\n", info->total_shrg_nr );
	printf(1, " --total_shpg_nr = \t%d\n", info->total_shpg_nr );
}

int
main(int argc, char *argv[])
{
	int hd, ret;
	struct ksminfo_t info;
	
	printf(1, "\n======================\n");
	printf(1, "      BASIC TEST      \n");
	printf(1, "======================\n");
	
	printf(1, "Before creating ksm, pages used num: %d\n", pgused());
	printf(1, "Now creating ksm with 1 page size.\n");
	hd = ksmget("shm1", PGSIZE);
	
	if (hd > 0) {
		printf(1, "------SUCCESS: ksmget success, handler = %d\n", hd );
	} else {
		printf(1, "------FAILED: ksmget failed!\n" );
	}
	
	printf(1, "After creating ksm, pages used num: %d\n", pgused());
	
	memset((void*)&info, 0, sizeof(struct ksminfo_t));
	ret = ksminfo(hd, &info);
	if (ret == 0) {
		printf(1, "------SUCCESS: ksminfo success!\n" );
	} else {
		printf(1, "------FAILED: ksminfo fail!\n" );
	}
	
	ksminfo_dump(hd, &info);
	
	printf(1, "\n======================\n");
	printf(1, "      TEST ATTACH     \n");
	printf(1, "======================\n");
	
	ret = ksmattach(hd, KSM_RDWR);
	
	if (ret <= 0) {
		printf(1, "ksmattach fail!\n" );
	} else {
		printf(1, "ksmattach success!\n" );
	}
	
	printf(1, "\n======================\n");
	printf(1, "      TEST WRITE      \n");
	printf(1, "======================\n");
	
	*(int*)ret = 10;
	printf(1, "writing %d\n", *(int*)ret );
	
	if (*(int*)ret == 10) {
		printf(1, "------SUCCESS: ksmattach success!\n" );
	} else {
		printf(1, "------FAILED: ksmattach fail!\n" );
	}
	
	printf(1, "\n======================\n");
	printf(1, "   TEST GLOBAL INFO   \n");
	printf(1, "======================\n");
	// Pass in handler with 0 to get global information
	memset((void*)&info, 0, sizeof(struct ksminfo_t));
	ret = ksminfo(0, &info);
	ksminfo_dump(hd, &info);
	
	if (ret == 0) {
		printf(1, "------SUCCESS: ksminfo global success!\n" );
	} else {
		printf(1, "------FAILED: ksminfo global fail!\n" );
	}
	
	
	printf(1, "\n======================\n");
	printf(1, "      TEST DETACH     \n");
	printf(1, "======================\n");
	ret = ksminfo(hd, &info);
	if (ret >0) {
		printf(1, "ksminfo success!\n" );
	} else {
		printf(1, "ksminfo fail!\n" );
	}
	int oldanr, newanr;
	oldanr = info.attached_nr;
	printf(1, "Before detaching, num of attached processes: %d\n", oldanr);
	
	ret = ksmdetach(hd);
	if (ret == 0) {
		printf(1, "ksmdetach success!\n" );
	} else {
		printf(1, "ksmdetach fail!\n" );
	}
	
	memset((void*)&info, 0, sizeof(struct ksminfo_t));
	ret = ksminfo(hd, &info);
	newanr = info.attached_nr;
	printf(1, "After detaching, num of attached processes: %d\n", newanr);
	
	if (oldanr - newanr != 1) {
		printf(1, "------FAILED: num of attached processes is not updated!\n");
	} else {
		printf(1, "------SUCCESS: num of attached processes is not updated!\n");
	}
	
	printf(1, "\n======================\n");
	printf(1, "      TEST DELETE     \n");
	printf(1, "======================\n");
	
	ret = ksmdelete(1);
	if (ret == 0) {
		printf(1, "ksmdelete success!\n" );
	} else {
		printf(1, "ksmdelete fail!\n" );
	}
	
	printf(1, "Then we check whether the slot with handler=%d is recyled again.\n", hd );
	int hdold = hd;
	hd = ksmget("shm3", PGSIZE);
	printf(1, "KSMGET for shm3: hd = %d\n", hd );
	// This test may only works for my implementation,
	// cause I insert the deleted pages at the head of the linked-list
	if (hd == hdold) {
		printf(1, "------SUCCESS: The slot with handler=%d is recyled!\n", hd );
	} else {
		printf(1, "------FAILED: The slot with handler=%d is NOT recyled!\n", hd );
	}
	
	printf(1, "\n======================\n");
	printf(1, " TEST WITH LARGE MEM  \n");
	printf(1, "======================\n");
	
	int oldpgnr, newpgnr;
	
	oldpgnr = pgused();
	printf(1, "Before getting large mem, num of page used: %d\n", oldpgnr);
	hd = ksmget("shm2", 400000000);
	
	if (hd > 0) {
		printf(1, "------FAILED: ksmget can get memory out of bound.\n");
	} else {
		printf(1, "------SUCCESS: ksmget cannot get memory out of bound.\n");
	}
	
	newpgnr = pgused();
	printf(1, "After getting large mem, num of page used: %d\n", newpgnr);
	
	if (oldpgnr != newpgnr) {
		printf(1, "------FAILED: ksmget cannot recycle pages in failed allocation.\n");
	} else {
		printf(1, "------SUCCESS: ksmget can recycle pages in failed allocation.\n");
	}
	
	exit();
}

