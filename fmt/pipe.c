6050 #include "types.h"
6051 #include "defs.h"
6052 #include "param.h"
6053 #include "mmu.h"
6054 #include "proc.h"
6055 #include "fs.h"
6056 #include "file.h"
6057 #include "spinlock.h"
6058 
6059 #define PIPESIZE 512
6060 
6061 struct pipe {
6062   struct spinlock lock;
6063   char data[PIPESIZE];
6064   uint nread;     // number of bytes read
6065   uint nwrite;    // number of bytes written
6066   int readopen;   // read fd is still open
6067   int writeopen;  // write fd is still open
6068 };
6069 
6070 int
6071 pipealloc(struct file **f0, struct file **f1)
6072 {
6073   struct pipe *p;
6074 
6075   p = 0;
6076   *f0 = *f1 = 0;
6077   if((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
6078     goto bad;
6079   if((p = (struct pipe*)kalloc()) == 0)
6080     goto bad;
6081   p->readopen = 1;
6082   p->writeopen = 1;
6083   p->nwrite = 0;
6084   p->nread = 0;
6085   initlock(&p->lock, "pipe");
6086   (*f0)->type = FD_PIPE;
6087   (*f0)->readable = 1;
6088   (*f0)->writable = 0;
6089   (*f0)->pipe = p;
6090   (*f1)->type = FD_PIPE;
6091   (*f1)->readable = 0;
6092   (*f1)->writable = 1;
6093   (*f1)->pipe = p;
6094   return 0;
6095 
6096 
6097 
6098 
6099 
6100  bad:
6101   if(p)
6102     kfree((char*)p);
6103   if(*f0)
6104     fileclose(*f0);
6105   if(*f1)
6106     fileclose(*f1);
6107   return -1;
6108 }
6109 
6110 void
6111 pipeclose(struct pipe *p, int writable)
6112 {
6113   acquire(&p->lock);
6114   if(writable){
6115     p->writeopen = 0;
6116     wakeup(&p->nread);
6117   } else {
6118     p->readopen = 0;
6119     wakeup(&p->nwrite);
6120   }
6121   if(p->readopen == 0 && p->writeopen == 0){
6122     release(&p->lock);
6123     kfree((char*)p);
6124   } else
6125     release(&p->lock);
6126 }
6127 
6128 
6129 int
6130 pipewrite(struct pipe *p, char *addr, int n)
6131 {
6132   int i;
6133 
6134   acquire(&p->lock);
6135   for(i = 0; i < n; i++){
6136     while(p->nwrite == p->nread + PIPESIZE){  //DOC: pipewrite-full
6137       if(p->readopen == 0 || proc->killed){
6138         release(&p->lock);
6139         return -1;
6140       }
6141       wakeup(&p->nread);
6142       sleep(&p->nwrite, &p->lock);  //DOC: pipewrite-sleep
6143     }
6144     p->data[p->nwrite++ % PIPESIZE] = addr[i];
6145   }
6146   wakeup(&p->nread);  //DOC: pipewrite-wakeup1
6147   release(&p->lock);
6148   return n;
6149 }
6150 int
6151 piperead(struct pipe *p, char *addr, int n)
6152 {
6153   int i;
6154 
6155   acquire(&p->lock);
6156   while(p->nread == p->nwrite && p->writeopen){  //DOC: pipe-empty
6157     if(proc->killed){
6158       release(&p->lock);
6159       return -1;
6160     }
6161     sleep(&p->nread, &p->lock); //DOC: piperead-sleep
6162   }
6163   for(i = 0; i < n; i++){  //DOC: piperead-copy
6164     if(p->nread == p->nwrite)
6165       break;
6166     addr[i] = p->data[p->nread++ % PIPESIZE];
6167   }
6168   wakeup(&p->nwrite);  //DOC: piperead-wakeup
6169   release(&p->lock);
6170   return i;
6171 }
6172 
6173 
6174 
6175 
6176 
6177 
6178 
6179 
6180 
6181 
6182 
6183 
6184 
6185 
6186 
6187 
6188 
6189 
6190 
6191 
6192 
6193 
6194 
6195 
6196 
6197 
6198 
6199 
