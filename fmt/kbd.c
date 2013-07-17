7200 #include "types.h"
7201 #include "x86.h"
7202 #include "defs.h"
7203 #include "kbd.h"
7204 
7205 int
7206 kbdgetc(void)
7207 {
7208   static uint shift;
7209   static uchar *charcode[4] = {
7210     normalmap, shiftmap, ctlmap, ctlmap
7211   };
7212   uint st, data, c;
7213 
7214   st = inb(KBSTATP);
7215   if((st & KBS_DIB) == 0)
7216     return -1;
7217   data = inb(KBDATAP);
7218 
7219   if(data == 0xE0){
7220     shift |= E0ESC;
7221     return 0;
7222   } else if(data & 0x80){
7223     // Key released
7224     data = (shift & E0ESC ? data : data & 0x7F);
7225     shift &= ~(shiftcode[data] | E0ESC);
7226     return 0;
7227   } else if(shift & E0ESC){
7228     // Last character was an E0 escape; or with 0x80
7229     data |= 0x80;
7230     shift &= ~E0ESC;
7231   }
7232 
7233   shift |= shiftcode[data];
7234   shift ^= togglecode[data];
7235   c = charcode[shift & (CTL | SHIFT)][data];
7236   if(shift & CAPSLOCK){
7237     if('a' <= c && c <= 'z')
7238       c += 'A' - 'a';
7239     else if('A' <= c && c <= 'Z')
7240       c += 'a' - 'A';
7241   }
7242   return c;
7243 }
7244 
7245 void
7246 kbdintr(void)
7247 {
7248   consoleintr(kbdgetc);
7249 }
