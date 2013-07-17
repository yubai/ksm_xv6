3550 struct buf {
3551   int flags;
3552   uint dev;
3553   uint sector;
3554   struct buf *prev; // LRU cache list
3555   struct buf *next;
3556   struct buf *qnext; // disk queue
3557   uchar data[512];
3558 };
3559 #define B_BUSY  0x1  // buffer is locked by some process
3560 #define B_VALID 0x2  // buffer has been read from disk
3561 #define B_DIRTY 0x4  // buffer needs to be written to disk
3562 
3563 
3564 
3565 
3566 
3567 
3568 
3569 
3570 
3571 
3572 
3573 
3574 
3575 
3576 
3577 
3578 
3579 
3580 
3581 
3582 
3583 
3584 
3585 
3586 
3587 
3588 
3589 
3590 
3591 
3592 
3593 
3594 
3595 
3596 
3597 
3598 
3599 
