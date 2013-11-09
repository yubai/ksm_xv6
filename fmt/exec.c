5950 #include "types.h"
5951 #include "param.h"
5952 #include "memlayout.h"
5953 #include "mmu.h"
5954 #include "proc.h"
5955 #include "defs.h"
5956 #include "x86.h"
5957 #include "elf.h"
5958 
5959 int
5960 exec(char *path, char **argv)
5961 {
5962   char *s, *last;
5963   int i, off;
5964   uint argc, sz, sp, ustack[3+MAXARG+1];
5965   struct elfhdr elf;
5966   struct inode *ip;
5967   struct proghdr ph;
5968   pde_t *pgdir, *oldpgdir;
5969 
5970   if((ip = namei(path)) == 0)
5971     return -1;
5972   ilock(ip);
5973   pgdir = 0;
5974 
5975   // Check ELF header
5976   if(readi(ip, (char*)&elf, 0, sizeof(elf)) < sizeof(elf))
5977     goto bad;
5978   if(elf.magic != ELF_MAGIC)
5979     goto bad;
5980 
5981   if((pgdir = setupkvm()) == 0)
5982     goto bad;
5983 
5984   // Load program into memory.
5985   sz = 0;
5986   for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
5987     if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
5988       goto bad;
5989     if(ph.type != ELF_PROG_LOAD)
5990       continue;
5991     if(ph.memsz < ph.filesz)
5992       goto bad;
5993     if((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0)
5994       goto bad;
5995     if(loaduvm(pgdir, (char*)ph.vaddr, ip, ph.off, ph.filesz) < 0)
5996       goto bad;
5997   }
5998   iunlockput(ip);
5999   ip = 0;
6000   // Allocate two pages at the next page boundary.
6001   // Make the first inaccessible.  Use the second as the user stack.
6002   sz = PGROUNDUP(sz);
6003   if((sz = allocuvm(pgdir, sz, sz + 2*PGSIZE)) == 0)
6004     goto bad;
6005   clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
6006   sp = sz;
6007 
6008   // Push argument strings, prepare rest of stack in ustack.
6009   for(argc = 0; argv[argc]; argc++) {
6010     if(argc >= MAXARG)
6011       goto bad;
6012     sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
6013     if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
6014       goto bad;
6015     ustack[3+argc] = sp;
6016   }
6017   ustack[3+argc] = 0;
6018 
6019   ustack[0] = 0xffffffff;  // fake return PC
6020   ustack[1] = argc;
6021   ustack[2] = sp - (argc+1)*4;  // argv pointer
6022 
6023   sp -= (3+argc+1) * 4;
6024   if(copyout(pgdir, sp, ustack, (3+argc+1)*4) < 0)
6025     goto bad;
6026 
6027   // Save program name for debugging.
6028   for(last=s=path; *s; s++)
6029     if(*s == '/')
6030       last = s+1;
6031   safestrcpy(proc->name, last, sizeof(proc->name));
6032 
6033   // Commit to the user image.
6034   oldpgdir = proc->pgdir;
6035   proc->pgdir = pgdir;
6036   proc->sz = sz;
6037   proc->tf->eip = elf.entry;  // main
6038   proc->tf->esp = sp;
6039   switchuvm(proc);
6040   freevm(oldpgdir);
6041   return 0;
6042 
6043  bad:
6044   if(pgdir)
6045     freevm(pgdir);
6046   if(ip)
6047     iunlockput(ip);
6048   return -1;
6049 }
