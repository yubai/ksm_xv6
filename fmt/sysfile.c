5450 //
5451 // File-system system calls.
5452 // Mostly argument checking, since we don't trust
5453 // user code, and calls into file.c and fs.c.
5454 //
5455 
5456 #include "types.h"
5457 #include "defs.h"
5458 #include "param.h"
5459 #include "stat.h"
5460 #include "mmu.h"
5461 #include "proc.h"
5462 #include "fs.h"
5463 #include "file.h"
5464 #include "fcntl.h"
5465 
5466 // Fetch the nth word-sized system call argument as a file descriptor
5467 // and return both the descriptor and the corresponding struct file.
5468 static int
5469 argfd(int n, int *pfd, struct file **pf)
5470 {
5471   int fd;
5472   struct file *f;
5473 
5474   if(argint(n, &fd) < 0)
5475     return -1;
5476   if(fd < 0 || fd >= NOFILE || (f=proc->ofile[fd]) == 0)
5477     return -1;
5478   if(pfd)
5479     *pfd = fd;
5480   if(pf)
5481     *pf = f;
5482   return 0;
5483 }
5484 
5485 // Allocate a file descriptor for the given file.
5486 // Takes over file reference from caller on success.
5487 static int
5488 fdalloc(struct file *f)
5489 {
5490   int fd;
5491 
5492   for(fd = 0; fd < NOFILE; fd++){
5493     if(proc->ofile[fd] == 0){
5494       proc->ofile[fd] = f;
5495       return fd;
5496     }
5497   }
5498   return -1;
5499 }
5500 int
5501 sys_dup(void)
5502 {
5503   struct file *f;
5504   int fd;
5505 
5506   if(argfd(0, 0, &f) < 0)
5507     return -1;
5508   if((fd=fdalloc(f)) < 0)
5509     return -1;
5510   filedup(f);
5511   return fd;
5512 }
5513 
5514 int
5515 sys_read(void)
5516 {
5517   struct file *f;
5518   int n;
5519   char *p;
5520 
5521   if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
5522     return -1;
5523   return fileread(f, p, n);
5524 }
5525 
5526 int
5527 sys_write(void)
5528 {
5529   struct file *f;
5530   int n;
5531   char *p;
5532 
5533   if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
5534     return -1;
5535   return filewrite(f, p, n);
5536 }
5537 
5538 int
5539 sys_close(void)
5540 {
5541   int fd;
5542   struct file *f;
5543 
5544   if(argfd(0, &fd, &f) < 0)
5545     return -1;
5546   proc->ofile[fd] = 0;
5547   fileclose(f);
5548   return 0;
5549 }
5550 int
5551 sys_fstat(void)
5552 {
5553   struct file *f;
5554   struct stat *st;
5555 
5556   if(argfd(0, 0, &f) < 0 || argptr(1, (void*)&st, sizeof(*st)) < 0)
5557     return -1;
5558   return filestat(f, st);
5559 }
5560 
5561 // Create the path new as a link to the same inode as old.
5562 int
5563 sys_link(void)
5564 {
5565   char name[DIRSIZ], *new, *old;
5566   struct inode *dp, *ip;
5567 
5568   if(argstr(0, &old) < 0 || argstr(1, &new) < 0)
5569     return -1;
5570   if((ip = namei(old)) == 0)
5571     return -1;
5572 
5573   begin_trans();
5574 
5575   ilock(ip);
5576   if(ip->type == T_DIR){
5577     iunlockput(ip);
5578     commit_trans();
5579     return -1;
5580   }
5581 
5582   ip->nlink++;
5583   iupdate(ip);
5584   iunlock(ip);
5585 
5586   if((dp = nameiparent(new, name)) == 0)
5587     goto bad;
5588   ilock(dp);
5589   if(dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0){
5590     iunlockput(dp);
5591     goto bad;
5592   }
5593   iunlockput(dp);
5594   iput(ip);
5595 
5596   commit_trans();
5597 
5598   return 0;
5599 
5600 bad:
5601   ilock(ip);
5602   ip->nlink--;
5603   iupdate(ip);
5604   iunlockput(ip);
5605   commit_trans();
5606   return -1;
5607 }
5608 
5609 // Is the directory dp empty except for "." and ".." ?
5610 static int
5611 isdirempty(struct inode *dp)
5612 {
5613   int off;
5614   struct dirent de;
5615 
5616   for(off=2*sizeof(de); off<dp->size; off+=sizeof(de)){
5617     if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
5618       panic("isdirempty: readi");
5619     if(de.inum != 0)
5620       return 0;
5621   }
5622   return 1;
5623 }
5624 
5625 
5626 
5627 
5628 
5629 
5630 
5631 
5632 
5633 
5634 
5635 
5636 
5637 
5638 
5639 
5640 
5641 
5642 
5643 
5644 
5645 
5646 
5647 
5648 
5649 
5650 int
5651 sys_unlink(void)
5652 {
5653   struct inode *ip, *dp;
5654   struct dirent de;
5655   char name[DIRSIZ], *path;
5656   uint off;
5657 
5658   if(argstr(0, &path) < 0)
5659     return -1;
5660   if((dp = nameiparent(path, name)) == 0)
5661     return -1;
5662 
5663   begin_trans();
5664 
5665   ilock(dp);
5666 
5667   // Cannot unlink "." or "..".
5668   if(namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
5669     goto bad;
5670 
5671   if((ip = dirlookup(dp, name, &off)) == 0)
5672     goto bad;
5673   ilock(ip);
5674 
5675   if(ip->nlink < 1)
5676     panic("unlink: nlink < 1");
5677   if(ip->type == T_DIR && !isdirempty(ip)){
5678     iunlockput(ip);
5679     goto bad;
5680   }
5681 
5682   memset(&de, 0, sizeof(de));
5683   if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
5684     panic("unlink: writei");
5685   if(ip->type == T_DIR){
5686     dp->nlink--;
5687     iupdate(dp);
5688   }
5689   iunlockput(dp);
5690 
5691   ip->nlink--;
5692   iupdate(ip);
5693   iunlockput(ip);
5694 
5695   commit_trans();
5696 
5697   return 0;
5698 
5699 
5700 bad:
5701   iunlockput(dp);
5702   commit_trans();
5703   return -1;
5704 }
5705 
5706 static struct inode*
5707 create(char *path, short type, short major, short minor)
5708 {
5709   uint off;
5710   struct inode *ip, *dp;
5711   char name[DIRSIZ];
5712 
5713   if((dp = nameiparent(path, name)) == 0)
5714     return 0;
5715   ilock(dp);
5716 
5717   if((ip = dirlookup(dp, name, &off)) != 0){
5718     iunlockput(dp);
5719     ilock(ip);
5720     if(type == T_FILE && ip->type == T_FILE)
5721       return ip;
5722     iunlockput(ip);
5723     return 0;
5724   }
5725 
5726   if((ip = ialloc(dp->dev, type)) == 0)
5727     panic("create: ialloc");
5728 
5729   ilock(ip);
5730   ip->major = major;
5731   ip->minor = minor;
5732   ip->nlink = 1;
5733   iupdate(ip);
5734 
5735   if(type == T_DIR){  // Create . and .. entries.
5736     dp->nlink++;  // for ".."
5737     iupdate(dp);
5738     // No ip->nlink++ for ".": avoid cyclic ref count.
5739     if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
5740       panic("create dots");
5741   }
5742 
5743   if(dirlink(dp, name, ip->inum) < 0)
5744     panic("create: dirlink");
5745 
5746   iunlockput(dp);
5747 
5748   return ip;
5749 }
5750 int
5751 sys_open(void)
5752 {
5753   char *path;
5754   int fd, omode;
5755   struct file *f;
5756   struct inode *ip;
5757 
5758   if(argstr(0, &path) < 0 || argint(1, &omode) < 0)
5759     return -1;
5760   if(omode & O_CREATE){
5761     begin_trans();
5762     ip = create(path, T_FILE, 0, 0);
5763     commit_trans();
5764     if(ip == 0)
5765       return -1;
5766   } else {
5767     if((ip = namei(path)) == 0)
5768       return -1;
5769     ilock(ip);
5770     if(ip->type == T_DIR && omode != O_RDONLY){
5771       iunlockput(ip);
5772       return -1;
5773     }
5774   }
5775 
5776   if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
5777     if(f)
5778       fileclose(f);
5779     iunlockput(ip);
5780     return -1;
5781   }
5782   iunlock(ip);
5783 
5784   f->type = FD_INODE;
5785   f->ip = ip;
5786   f->off = 0;
5787   f->readable = !(omode & O_WRONLY);
5788   f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
5789   return fd;
5790 }
5791 
5792 
5793 
5794 
5795 
5796 
5797 
5798 
5799 
5800 int
5801 sys_mkdir(void)
5802 {
5803   char *path;
5804   struct inode *ip;
5805 
5806   begin_trans();
5807   if(argstr(0, &path) < 0 || (ip = create(path, T_DIR, 0, 0)) == 0){
5808     commit_trans();
5809     return -1;
5810   }
5811   iunlockput(ip);
5812   commit_trans();
5813   return 0;
5814 }
5815 
5816 int
5817 sys_mknod(void)
5818 {
5819   struct inode *ip;
5820   char *path;
5821   int len;
5822   int major, minor;
5823 
5824   begin_trans();
5825   if((len=argstr(0, &path)) < 0 ||
5826      argint(1, &major) < 0 ||
5827      argint(2, &minor) < 0 ||
5828      (ip = create(path, T_DEV, major, minor)) == 0){
5829     commit_trans();
5830     return -1;
5831   }
5832   iunlockput(ip);
5833   commit_trans();
5834   return 0;
5835 }
5836 
5837 
5838 
5839 
5840 
5841 
5842 
5843 
5844 
5845 
5846 
5847 
5848 
5849 
5850 int
5851 sys_chdir(void)
5852 {
5853   char *path;
5854   struct inode *ip;
5855 
5856   if(argstr(0, &path) < 0 || (ip = namei(path)) == 0)
5857     return -1;
5858   ilock(ip);
5859   if(ip->type != T_DIR){
5860     iunlockput(ip);
5861     return -1;
5862   }
5863   iunlock(ip);
5864   iput(proc->cwd);
5865   proc->cwd = ip;
5866   return 0;
5867 }
5868 
5869 int
5870 sys_exec(void)
5871 {
5872   char *path, *argv[MAXARG];
5873   int i;
5874   uint uargv, uarg;
5875 
5876   if(argstr(0, &path) < 0 || argint(1, (int*)&uargv) < 0){
5877     return -1;
5878   }
5879   memset(argv, 0, sizeof(argv));
5880   for(i=0;; i++){
5881     if(i >= NELEM(argv))
5882       return -1;
5883     if(fetchint(uargv+4*i, (int*)&uarg) < 0)
5884       return -1;
5885     if(uarg == 0){
5886       argv[i] = 0;
5887       break;
5888     }
5889     if(fetchstr(uarg, &argv[i]) < 0)
5890       return -1;
5891   }
5892   return exec(path, argv);
5893 }
5894 
5895 
5896 
5897 
5898 
5899 
5900 int
5901 sys_pipe(void)
5902 {
5903   int *fd;
5904   struct file *rf, *wf;
5905   int fd0, fd1;
5906 
5907   if(argptr(0, (void*)&fd, 2*sizeof(fd[0])) < 0)
5908     return -1;
5909   if(pipealloc(&rf, &wf) < 0)
5910     return -1;
5911   fd0 = -1;
5912   if((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0){
5913     if(fd0 >= 0)
5914       proc->ofile[fd0] = 0;
5915     fileclose(rf);
5916     fileclose(wf);
5917     return -1;
5918   }
5919   fd[0] = fd0;
5920   fd[1] = fd1;
5921   return 0;
5922 }
5923 
5924 
5925 
5926 
5927 
5928 
5929 
5930 
5931 
5932 
5933 
5934 
5935 
5936 
5937 
5938 
5939 
5940 
5941 
5942 
5943 
5944 
5945 
5946 
5947 
5948 
5949 
