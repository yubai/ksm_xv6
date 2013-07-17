4450 // File system implementation.  Five layers:
4451 //   + Blocks: allocator for raw disk blocks.
4452 //   + Log: crash recovery for multi-step updates.
4453 //   + Files: inode allocator, reading, writing, metadata.
4454 //   + Directories: inode with special contents (list of other inodes!)
4455 //   + Names: paths like /usr/rtm/xv6/fs.c for convenient naming.
4456 //
4457 // This file contains the low-level file system manipulation
4458 // routines.  The (higher-level) system call implementations
4459 // are in sysfile.c.
4460 
4461 #include "types.h"
4462 #include "defs.h"
4463 #include "param.h"
4464 #include "stat.h"
4465 #include "mmu.h"
4466 #include "proc.h"
4467 #include "spinlock.h"
4468 #include "buf.h"
4469 #include "fs.h"
4470 #include "file.h"
4471 
4472 #define min(a, b) ((a) < (b) ? (a) : (b))
4473 static void itrunc(struct inode*);
4474 
4475 // Read the super block.
4476 void
4477 readsb(int dev, struct superblock *sb)
4478 {
4479   struct buf *bp;
4480 
4481   bp = bread(dev, 1);
4482   memmove(sb, bp->data, sizeof(*sb));
4483   brelse(bp);
4484 }
4485 
4486 // Zero a block.
4487 static void
4488 bzero(int dev, int bno)
4489 {
4490   struct buf *bp;
4491 
4492   bp = bread(dev, bno);
4493   memset(bp->data, 0, BSIZE);
4494   log_write(bp);
4495   brelse(bp);
4496 }
4497 
4498 
4499 
4500 // Blocks.
4501 
4502 // Allocate a zeroed disk block.
4503 static uint
4504 balloc(uint dev)
4505 {
4506   int b, bi, m;
4507   struct buf *bp;
4508   struct superblock sb;
4509 
4510   bp = 0;
4511   readsb(dev, &sb);
4512   for(b = 0; b < sb.size; b += BPB){
4513     bp = bread(dev, BBLOCK(b, sb.ninodes));
4514     for(bi = 0; bi < BPB && b + bi < sb.size; bi++){
4515       m = 1 << (bi % 8);
4516       if((bp->data[bi/8] & m) == 0){  // Is block free?
4517         bp->data[bi/8] |= m;  // Mark block in use.
4518         log_write(bp);
4519         brelse(bp);
4520         bzero(dev, b + bi);
4521         return b + bi;
4522       }
4523     }
4524     brelse(bp);
4525   }
4526   panic("balloc: out of blocks");
4527 }
4528 
4529 // Free a disk block.
4530 static void
4531 bfree(int dev, uint b)
4532 {
4533   struct buf *bp;
4534   struct superblock sb;
4535   int bi, m;
4536 
4537   readsb(dev, &sb);
4538   bp = bread(dev, BBLOCK(b, sb.ninodes));
4539   bi = b % BPB;
4540   m = 1 << (bi % 8);
4541   if((bp->data[bi/8] & m) == 0)
4542     panic("freeing free block");
4543   bp->data[bi/8] &= ~m;
4544   log_write(bp);
4545   brelse(bp);
4546 }
4547 
4548 
4549 
4550 // Inodes.
4551 //
4552 // An inode describes a single unnamed file.
4553 // The inode disk structure holds metadata: the file's type,
4554 // its size, the number of links referring to it, and the
4555 // list of blocks holding the file's content.
4556 //
4557 // The inodes are laid out sequentially on disk immediately after
4558 // the superblock. Each inode has a number, indicating its
4559 // position on the disk.
4560 //
4561 // The kernel keeps a cache of in-use inodes in memory
4562 // to provide a place for synchronizing access
4563 // to inodes used by multiple processes. The cached
4564 // inodes include book-keeping information that is
4565 // not stored on disk: ip->ref and ip->flags.
4566 //
4567 // An inode and its in-memory represtative go through a
4568 // sequence of states before they can be used by the
4569 // rest of the file system code.
4570 //
4571 // * Allocation: an inode is allocated if its type (on disk)
4572 //   is non-zero. ialloc() allocates, iput() frees if
4573 //   the link count has fallen to zero.
4574 //
4575 // * Referencing in cache: an entry in the inode cache
4576 //   is free if ip->ref is zero. Otherwise ip->ref tracks
4577 //   the number of in-memory pointers to the entry (open
4578 //   files and current directories). iget() to find or
4579 //   create a cache entry and increment its ref, iput()
4580 //   to decrement ref.
4581 //
4582 // * Valid: the information (type, size, &c) in an inode
4583 //   cache entry is only correct when the I_VALID bit
4584 //   is set in ip->flags. ilock() reads the inode from
4585 //   the disk and sets I_VALID, while iput() clears
4586 //   I_VALID if ip->ref has fallen to zero.
4587 //
4588 // * Locked: file system code may only examine and modify
4589 //   the information in an inode and its content if it
4590 //   has first locked the inode. The I_BUSY flag indicates
4591 //   that the inode is locked. ilock() sets I_BUSY,
4592 //   while iunlock clears it.
4593 //
4594 // Thus a typical sequence is:
4595 //   ip = iget(dev, inum)
4596 //   ilock(ip)
4597 //   ... examine and modify ip->xxx ...
4598 //   iunlock(ip)
4599 //   iput(ip)
4600 //
4601 // ilock() is separate from iget() so that system calls can
4602 // get a long-term reference to an inode (as for an open file)
4603 // and only lock it for short periods (e.g., in read()).
4604 // The separation also helps avoid deadlock and races during
4605 // pathname lookup. iget() increments ip->ref so that the inode
4606 // stays cached and pointers to it remain valid.
4607 //
4608 // Many internal file system functions expect the caller to
4609 // have locked the inodes involved; this lets callers create
4610 // multi-step atomic operations.
4611 
4612 struct {
4613   struct spinlock lock;
4614   struct inode inode[NINODE];
4615 } icache;
4616 
4617 void
4618 iinit(void)
4619 {
4620   initlock(&icache.lock, "icache");
4621 }
4622 
4623 static struct inode* iget(uint dev, uint inum);
4624 
4625 
4626 
4627 
4628 
4629 
4630 
4631 
4632 
4633 
4634 
4635 
4636 
4637 
4638 
4639 
4640 
4641 
4642 
4643 
4644 
4645 
4646 
4647 
4648 
4649 
4650 // Allocate a new inode with the given type on device dev.
4651 // A free inode has a type of zero.
4652 struct inode*
4653 ialloc(uint dev, short type)
4654 {
4655   int inum;
4656   struct buf *bp;
4657   struct dinode *dip;
4658   struct superblock sb;
4659 
4660   readsb(dev, &sb);
4661 
4662   for(inum = 1; inum < sb.ninodes; inum++){
4663     bp = bread(dev, IBLOCK(inum));
4664     dip = (struct dinode*)bp->data + inum%IPB;
4665     if(dip->type == 0){  // a free inode
4666       memset(dip, 0, sizeof(*dip));
4667       dip->type = type;
4668       log_write(bp);   // mark it allocated on the disk
4669       brelse(bp);
4670       return iget(dev, inum);
4671     }
4672     brelse(bp);
4673   }
4674   panic("ialloc: no inodes");
4675 }
4676 
4677 // Copy a modified in-memory inode to disk.
4678 void
4679 iupdate(struct inode *ip)
4680 {
4681   struct buf *bp;
4682   struct dinode *dip;
4683 
4684   bp = bread(ip->dev, IBLOCK(ip->inum));
4685   dip = (struct dinode*)bp->data + ip->inum%IPB;
4686   dip->type = ip->type;
4687   dip->major = ip->major;
4688   dip->minor = ip->minor;
4689   dip->nlink = ip->nlink;
4690   dip->size = ip->size;
4691   memmove(dip->addrs, ip->addrs, sizeof(ip->addrs));
4692   log_write(bp);
4693   brelse(bp);
4694 }
4695 
4696 
4697 
4698 
4699 
4700 // Find the inode with number inum on device dev
4701 // and return the in-memory copy. Does not lock
4702 // the inode and does not read it from disk.
4703 static struct inode*
4704 iget(uint dev, uint inum)
4705 {
4706   struct inode *ip, *empty;
4707 
4708   acquire(&icache.lock);
4709 
4710   // Is the inode already cached?
4711   empty = 0;
4712   for(ip = &icache.inode[0]; ip < &icache.inode[NINODE]; ip++){
4713     if(ip->ref > 0 && ip->dev == dev && ip->inum == inum){
4714       ip->ref++;
4715       release(&icache.lock);
4716       return ip;
4717     }
4718     if(empty == 0 && ip->ref == 0)    // Remember empty slot.
4719       empty = ip;
4720   }
4721 
4722   // Recycle an inode cache entry.
4723   if(empty == 0)
4724     panic("iget: no inodes");
4725 
4726   ip = empty;
4727   ip->dev = dev;
4728   ip->inum = inum;
4729   ip->ref = 1;
4730   ip->flags = 0;
4731   release(&icache.lock);
4732 
4733   return ip;
4734 }
4735 
4736 // Increment reference count for ip.
4737 // Returns ip to enable ip = idup(ip1) idiom.
4738 struct inode*
4739 idup(struct inode *ip)
4740 {
4741   acquire(&icache.lock);
4742   ip->ref++;
4743   release(&icache.lock);
4744   return ip;
4745 }
4746 
4747 
4748 
4749 
4750 // Lock the given inode.
4751 // Reads the inode from disk if necessary.
4752 void
4753 ilock(struct inode *ip)
4754 {
4755   struct buf *bp;
4756   struct dinode *dip;
4757 
4758   if(ip == 0 || ip->ref < 1)
4759     panic("ilock");
4760 
4761   acquire(&icache.lock);
4762   while(ip->flags & I_BUSY)
4763     sleep(ip, &icache.lock);
4764   ip->flags |= I_BUSY;
4765   release(&icache.lock);
4766 
4767   if(!(ip->flags & I_VALID)){
4768     bp = bread(ip->dev, IBLOCK(ip->inum));
4769     dip = (struct dinode*)bp->data + ip->inum%IPB;
4770     ip->type = dip->type;
4771     ip->major = dip->major;
4772     ip->minor = dip->minor;
4773     ip->nlink = dip->nlink;
4774     ip->size = dip->size;
4775     memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));
4776     brelse(bp);
4777     ip->flags |= I_VALID;
4778     if(ip->type == 0)
4779       panic("ilock: no type");
4780   }
4781 }
4782 
4783 // Unlock the given inode.
4784 void
4785 iunlock(struct inode *ip)
4786 {
4787   if(ip == 0 || !(ip->flags & I_BUSY) || ip->ref < 1)
4788     panic("iunlock");
4789 
4790   acquire(&icache.lock);
4791   ip->flags &= ~I_BUSY;
4792   wakeup(ip);
4793   release(&icache.lock);
4794 }
4795 
4796 
4797 
4798 
4799 
4800 // Drop a reference to an in-memory inode.
4801 // If that was the last reference, the inode cache entry can
4802 // be recycled.
4803 // If that was the last reference and the inode has no links
4804 // to it, free the inode (and its content) on disk.
4805 void
4806 iput(struct inode *ip)
4807 {
4808   acquire(&icache.lock);
4809   if(ip->ref == 1 && (ip->flags & I_VALID) && ip->nlink == 0){
4810     // inode has no links: truncate and free inode.
4811     if(ip->flags & I_BUSY)
4812       panic("iput busy");
4813     ip->flags |= I_BUSY;
4814     release(&icache.lock);
4815     itrunc(ip);
4816     ip->type = 0;
4817     iupdate(ip);
4818     acquire(&icache.lock);
4819     ip->flags = 0;
4820     wakeup(ip);
4821   }
4822   ip->ref--;
4823   release(&icache.lock);
4824 }
4825 
4826 // Common idiom: unlock, then put.
4827 void
4828 iunlockput(struct inode *ip)
4829 {
4830   iunlock(ip);
4831   iput(ip);
4832 }
4833 
4834 
4835 
4836 
4837 
4838 
4839 
4840 
4841 
4842 
4843 
4844 
4845 
4846 
4847 
4848 
4849 
4850 // Inode content
4851 //
4852 // The content (data) associated with each inode is stored
4853 // in blocks on the disk. The first NDIRECT block numbers
4854 // are listed in ip->addrs[].  The next NINDIRECT blocks are
4855 // listed in block ip->addrs[NDIRECT].
4856 
4857 // Return the disk block address of the nth block in inode ip.
4858 // If there is no such block, bmap allocates one.
4859 static uint
4860 bmap(struct inode *ip, uint bn)
4861 {
4862   uint addr, *a;
4863   struct buf *bp;
4864 
4865   if(bn < NDIRECT){
4866     if((addr = ip->addrs[bn]) == 0)
4867       ip->addrs[bn] = addr = balloc(ip->dev);
4868     return addr;
4869   }
4870   bn -= NDIRECT;
4871 
4872   if(bn < NINDIRECT){
4873     // Load indirect block, allocating if necessary.
4874     if((addr = ip->addrs[NDIRECT]) == 0)
4875       ip->addrs[NDIRECT] = addr = balloc(ip->dev);
4876     bp = bread(ip->dev, addr);
4877     a = (uint*)bp->data;
4878     if((addr = a[bn]) == 0){
4879       a[bn] = addr = balloc(ip->dev);
4880       log_write(bp);
4881     }
4882     brelse(bp);
4883     return addr;
4884   }
4885 
4886   panic("bmap: out of range");
4887 }
4888 
4889 
4890 
4891 
4892 
4893 
4894 
4895 
4896 
4897 
4898 
4899 
4900 // Truncate inode (discard contents).
4901 // Only called when the inode has no links
4902 // to it (no directory entries referring to it)
4903 // and has no in-memory reference to it (is
4904 // not an open file or current directory).
4905 static void
4906 itrunc(struct inode *ip)
4907 {
4908   int i, j;
4909   struct buf *bp;
4910   uint *a;
4911 
4912   for(i = 0; i < NDIRECT; i++){
4913     if(ip->addrs[i]){
4914       bfree(ip->dev, ip->addrs[i]);
4915       ip->addrs[i] = 0;
4916     }
4917   }
4918 
4919   if(ip->addrs[NDIRECT]){
4920     bp = bread(ip->dev, ip->addrs[NDIRECT]);
4921     a = (uint*)bp->data;
4922     for(j = 0; j < NINDIRECT; j++){
4923       if(a[j])
4924         bfree(ip->dev, a[j]);
4925     }
4926     brelse(bp);
4927     bfree(ip->dev, ip->addrs[NDIRECT]);
4928     ip->addrs[NDIRECT] = 0;
4929   }
4930 
4931   ip->size = 0;
4932   iupdate(ip);
4933 }
4934 
4935 // Copy stat information from inode.
4936 void
4937 stati(struct inode *ip, struct stat *st)
4938 {
4939   st->dev = ip->dev;
4940   st->ino = ip->inum;
4941   st->type = ip->type;
4942   st->nlink = ip->nlink;
4943   st->size = ip->size;
4944 }
4945 
4946 
4947 
4948 
4949 
4950 // Read data from inode.
4951 int
4952 readi(struct inode *ip, char *dst, uint off, uint n)
4953 {
4954   uint tot, m;
4955   struct buf *bp;
4956 
4957   if(ip->type == T_DEV){
4958     if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].read)
4959       return -1;
4960     return devsw[ip->major].read(ip, dst, n);
4961   }
4962 
4963   if(off > ip->size || off + n < off)
4964     return -1;
4965   if(off + n > ip->size)
4966     n = ip->size - off;
4967 
4968   for(tot=0; tot<n; tot+=m, off+=m, dst+=m){
4969     bp = bread(ip->dev, bmap(ip, off/BSIZE));
4970     m = min(n - tot, BSIZE - off%BSIZE);
4971     memmove(dst, bp->data + off%BSIZE, m);
4972     brelse(bp);
4973   }
4974   return n;
4975 }
4976 
4977 
4978 
4979 
4980 
4981 
4982 
4983 
4984 
4985 
4986 
4987 
4988 
4989 
4990 
4991 
4992 
4993 
4994 
4995 
4996 
4997 
4998 
4999 
5000 // Write data to inode.
5001 int
5002 writei(struct inode *ip, char *src, uint off, uint n)
5003 {
5004   uint tot, m;
5005   struct buf *bp;
5006 
5007   if(ip->type == T_DEV){
5008     if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].write)
5009       return -1;
5010     return devsw[ip->major].write(ip, src, n);
5011   }
5012 
5013   if(off > ip->size || off + n < off)
5014     return -1;
5015   if(off + n > MAXFILE*BSIZE)
5016     return -1;
5017 
5018   for(tot=0; tot<n; tot+=m, off+=m, src+=m){
5019     bp = bread(ip->dev, bmap(ip, off/BSIZE));
5020     m = min(n - tot, BSIZE - off%BSIZE);
5021     memmove(bp->data + off%BSIZE, src, m);
5022     log_write(bp);
5023     brelse(bp);
5024   }
5025 
5026   if(n > 0 && off > ip->size){
5027     ip->size = off;
5028     iupdate(ip);
5029   }
5030   return n;
5031 }
5032 
5033 
5034 
5035 
5036 
5037 
5038 
5039 
5040 
5041 
5042 
5043 
5044 
5045 
5046 
5047 
5048 
5049 
5050 // Directories
5051 
5052 int
5053 namecmp(const char *s, const char *t)
5054 {
5055   return strncmp(s, t, DIRSIZ);
5056 }
5057 
5058 // Look for a directory entry in a directory.
5059 // If found, set *poff to byte offset of entry.
5060 struct inode*
5061 dirlookup(struct inode *dp, char *name, uint *poff)
5062 {
5063   uint off, inum;
5064   struct dirent de;
5065 
5066   if(dp->type != T_DIR)
5067     panic("dirlookup not DIR");
5068 
5069   for(off = 0; off < dp->size; off += sizeof(de)){
5070     if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
5071       panic("dirlink read");
5072     if(de.inum == 0)
5073       continue;
5074     if(namecmp(name, de.name) == 0){
5075       // entry matches path element
5076       if(poff)
5077         *poff = off;
5078       inum = de.inum;
5079       return iget(dp->dev, inum);
5080     }
5081   }
5082 
5083   return 0;
5084 }
5085 
5086 
5087 
5088 
5089 
5090 
5091 
5092 
5093 
5094 
5095 
5096 
5097 
5098 
5099 
5100 // Write a new directory entry (name, inum) into the directory dp.
5101 int
5102 dirlink(struct inode *dp, char *name, uint inum)
5103 {
5104   int off;
5105   struct dirent de;
5106   struct inode *ip;
5107 
5108   // Check that name is not present.
5109   if((ip = dirlookup(dp, name, 0)) != 0){
5110     iput(ip);
5111     return -1;
5112   }
5113 
5114   // Look for an empty dirent.
5115   for(off = 0; off < dp->size; off += sizeof(de)){
5116     if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
5117       panic("dirlink read");
5118     if(de.inum == 0)
5119       break;
5120   }
5121 
5122   strncpy(de.name, name, DIRSIZ);
5123   de.inum = inum;
5124   if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
5125     panic("dirlink");
5126 
5127   return 0;
5128 }
5129 
5130 
5131 
5132 
5133 
5134 
5135 
5136 
5137 
5138 
5139 
5140 
5141 
5142 
5143 
5144 
5145 
5146 
5147 
5148 
5149 
5150 // Paths
5151 
5152 // Copy the next path element from path into name.
5153 // Return a pointer to the element following the copied one.
5154 // The returned path has no leading slashes,
5155 // so the caller can check *path=='\0' to see if the name is the last one.
5156 // If no name to remove, return 0.
5157 //
5158 // Examples:
5159 //   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
5160 //   skipelem("///a//bb", name) = "bb", setting name = "a"
5161 //   skipelem("a", name) = "", setting name = "a"
5162 //   skipelem("", name) = skipelem("////", name) = 0
5163 //
5164 static char*
5165 skipelem(char *path, char *name)
5166 {
5167   char *s;
5168   int len;
5169 
5170   while(*path == '/')
5171     path++;
5172   if(*path == 0)
5173     return 0;
5174   s = path;
5175   while(*path != '/' && *path != 0)
5176     path++;
5177   len = path - s;
5178   if(len >= DIRSIZ)
5179     memmove(name, s, DIRSIZ);
5180   else {
5181     memmove(name, s, len);
5182     name[len] = 0;
5183   }
5184   while(*path == '/')
5185     path++;
5186   return path;
5187 }
5188 
5189 
5190 
5191 
5192 
5193 
5194 
5195 
5196 
5197 
5198 
5199 
5200 // Look up and return the inode for a path name.
5201 // If parent != 0, return the inode for the parent and copy the final
5202 // path element into name, which must have room for DIRSIZ bytes.
5203 static struct inode*
5204 namex(char *path, int nameiparent, char *name)
5205 {
5206   struct inode *ip, *next;
5207 
5208   if(*path == '/')
5209     ip = iget(ROOTDEV, ROOTINO);
5210   else
5211     ip = idup(proc->cwd);
5212 
5213   while((path = skipelem(path, name)) != 0){
5214     ilock(ip);
5215     if(ip->type != T_DIR){
5216       iunlockput(ip);
5217       return 0;
5218     }
5219     if(nameiparent && *path == '\0'){
5220       // Stop one level early.
5221       iunlock(ip);
5222       return ip;
5223     }
5224     if((next = dirlookup(ip, name, 0)) == 0){
5225       iunlockput(ip);
5226       return 0;
5227     }
5228     iunlockput(ip);
5229     ip = next;
5230   }
5231   if(nameiparent){
5232     iput(ip);
5233     return 0;
5234   }
5235   return ip;
5236 }
5237 
5238 struct inode*
5239 namei(char *path)
5240 {
5241   char name[DIRSIZ];
5242   return namex(path, 0, name);
5243 }
5244 
5245 struct inode*
5246 nameiparent(char *path, char *name)
5247 {
5248   return namex(path, 1, name);
5249 }
