In order to implement a system call, we need to follow the steps below:

  1: Put the declarations for userspace system calls in user.h.
  2: Put the definitions for the system calls: add SYSCALL(name) in usys.S.
  3: Put the system calls number in syscall.h.
  4: Put the system call vector index in syscall table in syscall.c.
  5: Implement the system calls in kernel mode (definitions could be in sysfile.c).  (May need to put the declarations in defs.h)
    
  
