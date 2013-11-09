7750 # Initial process execs /init.
7751 
7752 #include "syscall.h"
7753 #include "traps.h"
7754 
7755 
7756 # exec(init, argv)
7757 .globl start
7758 start:
7759   pushl $argv
7760   pushl $init
7761   pushl $0  // where caller pc would be
7762   movl $SYS_exec, %eax
7763   int $T_SYSCALL
7764 
7765 # for(;;) exit();
7766 exit:
7767   movl $SYS_exit, %eax
7768   int $T_SYSCALL
7769   jmp exit
7770 
7771 # char init[] = "/init\0";
7772 init:
7773   .string "/init\0"
7774 
7775 # char *argv[] = { init, 0 };
7776 .p2align 2
7777 argv:
7778   .long init
7779   .long 0
7780 
7781 
7782 
7783 
7784 
7785 
7786 
7787 
7788 
7789 
7790 
7791 
7792 
7793 
7794 
7795 
7796 
7797 
7798 
7799 
