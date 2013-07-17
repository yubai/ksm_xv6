7250 // Console input and output.
7251 // Input is from the keyboard or serial port.
7252 // Output is written to the screen and serial port.
7253 
7254 #include "types.h"
7255 #include "defs.h"
7256 #include "param.h"
7257 #include "traps.h"
7258 #include "spinlock.h"
7259 #include "fs.h"
7260 #include "file.h"
7261 #include "memlayout.h"
7262 #include "mmu.h"
7263 #include "proc.h"
7264 #include "x86.h"
7265 
7266 static void consputc(int);
7267 
7268 static int panicked = 0;
7269 
7270 static struct {
7271   struct spinlock lock;
7272   int locking;
7273 } cons;
7274 
7275 static void
7276 printint(int xx, int base, int sign)
7277 {
7278   static char digits[] = "0123456789abcdef";
7279   char buf[16];
7280   int i;
7281   uint x;
7282 
7283   if(sign && (sign = xx < 0))
7284     x = -xx;
7285   else
7286     x = xx;
7287 
7288   i = 0;
7289   do{
7290     buf[i++] = digits[x % base];
7291   }while((x /= base) != 0);
7292 
7293   if(sign)
7294     buf[i++] = '-';
7295 
7296   while(--i >= 0)
7297     consputc(buf[i]);
7298 }
7299 
7300 // Print to the console. only understands %d, %x, %p, %s.
7301 void
7302 cprintf(char *fmt, ...)
7303 {
7304   int i, c, locking;
7305   uint *argp;
7306   char *s;
7307 
7308   locking = cons.locking;
7309   if(locking)
7310     acquire(&cons.lock);
7311 
7312   if (fmt == 0)
7313     panic("null fmt");
7314 
7315   argp = (uint*)(void*)(&fmt + 1);
7316   for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
7317     if(c != '%'){
7318       consputc(c);
7319       continue;
7320     }
7321     c = fmt[++i] & 0xff;
7322     if(c == 0)
7323       break;
7324     switch(c){
7325     case 'd':
7326       printint(*argp++, 10, 1);
7327       break;
7328     case 'x':
7329     case 'p':
7330       printint(*argp++, 16, 0);
7331       break;
7332     case 's':
7333       if((s = (char*)*argp++) == 0)
7334         s = "(null)";
7335       for(; *s; s++)
7336         consputc(*s);
7337       break;
7338     case '%':
7339       consputc('%');
7340       break;
7341     default:
7342       // Print unknown % sequence to draw attention.
7343       consputc('%');
7344       consputc(c);
7345       break;
7346     }
7347   }
7348 
7349 
7350   if(locking)
7351     release(&cons.lock);
7352 }
7353 
7354 void
7355 panic(char *s)
7356 {
7357   int i;
7358   uint pcs[10];
7359 
7360   cli();
7361   cons.locking = 0;
7362   cprintf("cpu%d: panic: ", cpu->id);
7363   cprintf(s);
7364   cprintf("\n");
7365   getcallerpcs(&s, pcs);
7366   for(i=0; i<10; i++)
7367     cprintf(" %p", pcs[i]);
7368   panicked = 1; // freeze other CPU
7369   for(;;)
7370     ;
7371 }
7372 
7373 
7374 
7375 
7376 
7377 
7378 
7379 
7380 
7381 
7382 
7383 
7384 
7385 
7386 
7387 
7388 
7389 
7390 
7391 
7392 
7393 
7394 
7395 
7396 
7397 
7398 
7399 
7400 #define BACKSPACE 0x100
7401 #define CRTPORT 0x3d4
7402 static ushort *crt = (ushort*)P2V(0xb8000);  // CGA memory
7403 
7404 static void
7405 cgaputc(int c)
7406 {
7407   int pos;
7408 
7409   // Cursor position: col + 80*row.
7410   outb(CRTPORT, 14);
7411   pos = inb(CRTPORT+1) << 8;
7412   outb(CRTPORT, 15);
7413   pos |= inb(CRTPORT+1);
7414 
7415   if(c == '\n')
7416     pos += 80 - pos%80;
7417   else if(c == BACKSPACE){
7418     if(pos > 0) --pos;
7419   } else
7420     crt[pos++] = (c&0xff) | 0x0700;  // black on white
7421 
7422   if((pos/80) >= 24){  // Scroll up.
7423     memmove(crt, crt+80, sizeof(crt[0])*23*80);
7424     pos -= 80;
7425     memset(crt+pos, 0, sizeof(crt[0])*(24*80 - pos));
7426   }
7427 
7428   outb(CRTPORT, 14);
7429   outb(CRTPORT+1, pos>>8);
7430   outb(CRTPORT, 15);
7431   outb(CRTPORT+1, pos);
7432   crt[pos] = ' ' | 0x0700;
7433 }
7434 
7435 void
7436 consputc(int c)
7437 {
7438   if(panicked){
7439     cli();
7440     for(;;)
7441       ;
7442   }
7443 
7444   if(c == BACKSPACE){
7445     uartputc('\b'); uartputc(' '); uartputc('\b');
7446   } else
7447     uartputc(c);
7448   cgaputc(c);
7449 }
7450 #define INPUT_BUF 128
7451 struct {
7452   struct spinlock lock;
7453   char buf[INPUT_BUF];
7454   uint r;  // Read index
7455   uint w;  // Write index
7456   uint e;  // Edit index
7457 } input;
7458 
7459 #define C(x)  ((x)-'@')  // Control-x
7460 
7461 void
7462 consoleintr(int (*getc)(void))
7463 {
7464   int c;
7465 
7466   acquire(&input.lock);
7467   while((c = getc()) >= 0){
7468     switch(c){
7469     case C('P'):  // Process listing.
7470       procdump();
7471       break;
7472     case C('U'):  // Kill line.
7473       while(input.e != input.w &&
7474             input.buf[(input.e-1) % INPUT_BUF] != '\n'){
7475         input.e--;
7476         consputc(BACKSPACE);
7477       }
7478       break;
7479     case C('H'): case '\x7f':  // Backspace
7480       if(input.e != input.w){
7481         input.e--;
7482         consputc(BACKSPACE);
7483       }
7484       break;
7485     default:
7486       if(c != 0 && input.e-input.r < INPUT_BUF){
7487         c = (c == '\r') ? '\n' : c;
7488         input.buf[input.e++ % INPUT_BUF] = c;
7489         consputc(c);
7490         if(c == '\n' || c == C('D') || input.e == input.r+INPUT_BUF){
7491           input.w = input.e;
7492           wakeup(&input.r);
7493         }
7494       }
7495       break;
7496     }
7497   }
7498   release(&input.lock);
7499 }
7500 int
7501 consoleread(struct inode *ip, char *dst, int n)
7502 {
7503   uint target;
7504   int c;
7505 
7506   iunlock(ip);
7507   target = n;
7508   acquire(&input.lock);
7509   while(n > 0){
7510     while(input.r == input.w){
7511       if(proc->killed){
7512         release(&input.lock);
7513         ilock(ip);
7514         return -1;
7515       }
7516       sleep(&input.r, &input.lock);
7517     }
7518     c = input.buf[input.r++ % INPUT_BUF];
7519     if(c == C('D')){  // EOF
7520       if(n < target){
7521         // Save ^D for next time, to make sure
7522         // caller gets a 0-byte result.
7523         input.r--;
7524       }
7525       break;
7526     }
7527     *dst++ = c;
7528     --n;
7529     if(c == '\n')
7530       break;
7531   }
7532   release(&input.lock);
7533   ilock(ip);
7534 
7535   return target - n;
7536 }
7537 
7538 
7539 
7540 
7541 
7542 
7543 
7544 
7545 
7546 
7547 
7548 
7549 
7550 int
7551 consolewrite(struct inode *ip, char *buf, int n)
7552 {
7553   int i;
7554 
7555   iunlock(ip);
7556   acquire(&cons.lock);
7557   for(i = 0; i < n; i++)
7558     consputc(buf[i] & 0xff);
7559   release(&cons.lock);
7560   ilock(ip);
7561 
7562   return n;
7563 }
7564 
7565 void
7566 consoleinit(void)
7567 {
7568   initlock(&cons.lock, "console");
7569   initlock(&input.lock, "input");
7570 
7571   devsw[CONSOLE].write = consolewrite;
7572   devsw[CONSOLE].read = consoleread;
7573   cons.locking = 1;
7574 
7575   picenable(IRQ_KBD);
7576   ioapicenable(IRQ_KBD, 0);
7577 }
7578 
7579 
7580 
7581 
7582 
7583 
7584 
7585 
7586 
7587 
7588 
7589 
7590 
7591 
7592 
7593 
7594 
7595 
7596 
7597 
7598 
7599 
