3700 // On-disk file system format.
3701 // Both the kernel and user programs use this header file.
3702 
3703 // Block 0 is unused.
3704 // Block 1 is super block.
3705 // Blocks 2 through sb.ninodes/IPB hold inodes.
3706 // Then free bitmap blocks holding sb.size bits.
3707 // Then sb.nblocks data blocks.
3708 // Then sb.nlog log blocks.
3709 
3710 #define ROOTINO 1  // root i-number
3711 #define BSIZE 512  // block size
3712 
3713 // File system super block
3714 struct superblock {
3715   uint size;         // Size of file system image (blocks)
3716   uint nblocks;      // Number of data blocks
3717   uint ninodes;      // Number of inodes.
3718   uint nlog;         // Number of log blocks
3719 };
3720 
3721 #define NDIRECT 12
3722 #define NINDIRECT (BSIZE / sizeof(uint))
3723 #define MAXFILE (NDIRECT + NINDIRECT)
3724 
3725 // On-disk inode structure
3726 struct dinode {
3727   short type;           // File type
3728   short major;          // Major device number (T_DEV only)
3729   short minor;          // Minor device number (T_DEV only)
3730   short nlink;          // Number of links to inode in file system
3731   uint size;            // Size of file (bytes)
3732   uint addrs[NDIRECT+1];   // Data block addresses
3733 };
3734 
3735 // Inodes per block.
3736 #define IPB           (BSIZE / sizeof(struct dinode))
3737 
3738 // Block containing inode i
3739 #define IBLOCK(i)     ((i) / IPB + 2)
3740 
3741 // Bitmap bits per block
3742 #define BPB           (BSIZE*8)
3743 
3744 // Block containing bit for block b
3745 #define BBLOCK(b, ninodes) (b/BPB + (ninodes)/IPB + 3)
3746 
3747 // Directory is a file containing a sequence of dirent structures.
3748 #define DIRSIZ 14
3749 
3750 struct dirent {
3751   ushort inum;
3752   char name[DIRSIZ];
3753 };
3754 
3755 
3756 
3757 
3758 
3759 
3760 
3761 
3762 
3763 
3764 
3765 
3766 
3767 
3768 
3769 
3770 
3771 
3772 
3773 
3774 
3775 
3776 
3777 
3778 
3779 
3780 
3781 
3782 
3783 
3784 
3785 
3786 
3787 
3788 
3789 
3790 
3791 
3792 
3793 
3794 
3795 
3796 
3797 
3798 
3799 
