6200 #include "types.h"
6201 #include "x86.h"
6202 
6203 void*
6204 memset(void *dst, int c, uint n)
6205 {
6206   if ((int)dst%4 == 0 && n%4 == 0){
6207     c &= 0xFF;
6208     stosl(dst, (c<<24)|(c<<16)|(c<<8)|c, n/4);
6209   } else
6210     stosb(dst, c, n);
6211   return dst;
6212 }
6213 
6214 int
6215 memcmp(const void *v1, const void *v2, uint n)
6216 {
6217   const uchar *s1, *s2;
6218 
6219   s1 = v1;
6220   s2 = v2;
6221   while(n-- > 0){
6222     if(*s1 != *s2)
6223       return *s1 - *s2;
6224     s1++, s2++;
6225   }
6226 
6227   return 0;
6228 }
6229 
6230 void*
6231 memmove(void *dst, const void *src, uint n)
6232 {
6233   const char *s;
6234   char *d;
6235 
6236   s = src;
6237   d = dst;
6238   if(s < d && s + n > d){
6239     s += n;
6240     d += n;
6241     while(n-- > 0)
6242       *--d = *--s;
6243   } else
6244     while(n-- > 0)
6245       *d++ = *s++;
6246 
6247   return dst;
6248 }
6249 
6250 // memcpy exists to placate GCC.  Use memmove.
6251 void*
6252 memcpy(void *dst, const void *src, uint n)
6253 {
6254   return memmove(dst, src, n);
6255 }
6256 
6257 int
6258 strncmp(const char *p, const char *q, uint n)
6259 {
6260   while(n > 0 && *p && *p == *q)
6261     n--, p++, q++;
6262   if(n == 0)
6263     return 0;
6264   return (uchar)*p - (uchar)*q;
6265 }
6266 
6267 char*
6268 strncpy(char *s, const char *t, int n)
6269 {
6270   char *os;
6271 
6272   os = s;
6273   while(n-- > 0 && (*s++ = *t++) != 0)
6274     ;
6275   while(n-- > 0)
6276     *s++ = 0;
6277   return os;
6278 }
6279 
6280 // Like strncpy but guaranteed to NUL-terminate.
6281 char*
6282 safestrcpy(char *s, const char *t, int n)
6283 {
6284   char *os;
6285 
6286   os = s;
6287   if(n <= 0)
6288     return os;
6289   while(--n > 0 && (*s++ = *t++) != 0)
6290     ;
6291   *s = 0;
6292   return os;
6293 }
6294 
6295 
6296 
6297 
6298 
6299 
6300 int
6301 strlen(const char *s)
6302 {
6303   int n;
6304 
6305   for(n = 0; s[n]; n++)
6306     ;
6307   return n;
6308 }
6309 
6310 
6311 
6312 
6313 
6314 
6315 
6316 
6317 
6318 
6319 
6320 
6321 
6322 
6323 
6324 
6325 
6326 
6327 
6328 
6329 
6330 
6331 
6332 
6333 
6334 
6335 
6336 
6337 
6338 
6339 
6340 
6341 
6342 
6343 
6344 
6345 
6346 
6347 
6348 
6349 
