7800 #include "syscall.h"
7801 #include "traps.h"
7802 
7803 #define SYSCALL(name) \
7804   .globl name; \
7805   name: \
7806     movl $SYS_ ## name, %eax; \
7807     int $T_SYSCALL; \
7808     ret
7809 
7810 SYSCALL(fork)
7811 SYSCALL(exit)
7812 SYSCALL(wait)
7813 SYSCALL(pipe)
7814 SYSCALL(read)
7815 SYSCALL(write)
7816 SYSCALL(close)
7817 SYSCALL(kill)
7818 SYSCALL(exec)
7819 SYSCALL(open)
7820 SYSCALL(mknod)
7821 SYSCALL(unlink)
7822 SYSCALL(fstat)
7823 SYSCALL(link)
7824 SYSCALL(mkdir)
7825 SYSCALL(chdir)
7826 SYSCALL(dup)
7827 SYSCALL(getpid)
7828 SYSCALL(sbrk)
7829 SYSCALL(sleep)
7830 SYSCALL(uptime)
7831 
7832 
7833 
7834 
7835 
7836 
7837 
7838 
7839 
7840 
7841 
7842 
7843 
7844 
7845 
7846 
7847 
7848 
7849 
