3800 struct file {
3801   enum { FD_NONE, FD_PIPE, FD_INODE } type;
3802   int ref; // reference count
3803   char readable;
3804   char writable;
3805   struct pipe *pipe;
3806   struct inode *ip;
3807   uint off;
3808 };
3809 
3810 
3811 // in-memory copy of an inode
3812 struct inode {
3813   uint dev;           // Device number
3814   uint inum;          // Inode number
3815   int ref;            // Reference count
3816   int flags;          // I_BUSY, I_VALID
3817 
3818   short type;         // copy of disk inode
3819   short major;
3820   short minor;
3821   short nlink;
3822   uint size;
3823   uint addrs[NDIRECT+1];
3824 };
3825 #define I_BUSY 0x1
3826 #define I_VALID 0x2
3827 
3828 // table mapping major device number to
3829 // device functions
3830 struct devsw {
3831   int (*read)(struct inode*, char*, int);
3832   int (*write)(struct inode*, char*, int);
3833 };
3834 
3835 extern struct devsw devsw[];
3836 
3837 #define CONSOLE 1
3838 
3839 
3840 
3841 
3842 
3843 
3844 
3845 
3846 
3847 
3848 
3849 
