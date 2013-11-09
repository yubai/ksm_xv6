5250 //
5251 // File descriptors
5252 //
5253 
5254 #include "types.h"
5255 #include "defs.h"
5256 #include "param.h"
5257 #include "fs.h"
5258 #include "file.h"
5259 #include "spinlock.h"
5260 
5261 struct devsw devsw[NDEV];
5262 struct {
5263   struct spinlock lock;
5264   struct file file[NFILE];
5265 } ftable;
5266 
5267 void
5268 fileinit(void)
5269 {
5270   initlock(&ftable.lock, "ftable");
5271 }
5272 
5273 // Allocate a file structure.
5274 struct file*
5275 filealloc(void)
5276 {
5277   struct file *f;
5278 
5279   acquire(&ftable.lock);
5280   for(f = ftable.file; f < ftable.file + NFILE; f++){
5281     if(f->ref == 0){
5282       f->ref = 1;
5283       release(&ftable.lock);
5284       return f;
5285     }
5286   }
5287   release(&ftable.lock);
5288   return 0;
5289 }
5290 
5291 
5292 
5293 
5294 
5295 
5296 
5297 
5298 
5299 
5300 // Increment ref count for file f.
5301 struct file*
5302 filedup(struct file *f)
5303 {
5304   acquire(&ftable.lock);
5305   if(f->ref < 1)
5306     panic("filedup");
5307   f->ref++;
5308   release(&ftable.lock);
5309   return f;
5310 }
5311 
5312 // Close file f.  (Decrement ref count, close when reaches 0.)
5313 void
5314 fileclose(struct file *f)
5315 {
5316   struct file ff;
5317 
5318   acquire(&ftable.lock);
5319   if(f->ref < 1)
5320     panic("fileclose");
5321   if(--f->ref > 0){
5322     release(&ftable.lock);
5323     return;
5324   }
5325   ff = *f;
5326   f->ref = 0;
5327   f->type = FD_NONE;
5328   release(&ftable.lock);
5329 
5330   if(ff.type == FD_PIPE)
5331     pipeclose(ff.pipe, ff.writable);
5332   else if(ff.type == FD_INODE){
5333     begin_trans();
5334     iput(ff.ip);
5335     commit_trans();
5336   }
5337 }
5338 
5339 
5340 
5341 
5342 
5343 
5344 
5345 
5346 
5347 
5348 
5349 
5350 // Get metadata about file f.
5351 int
5352 filestat(struct file *f, struct stat *st)
5353 {
5354   if(f->type == FD_INODE){
5355     ilock(f->ip);
5356     stati(f->ip, st);
5357     iunlock(f->ip);
5358     return 0;
5359   }
5360   return -1;
5361 }
5362 
5363 // Read from file f.
5364 int
5365 fileread(struct file *f, char *addr, int n)
5366 {
5367   int r;
5368 
5369   if(f->readable == 0)
5370     return -1;
5371   if(f->type == FD_PIPE)
5372     return piperead(f->pipe, addr, n);
5373   if(f->type == FD_INODE){
5374     ilock(f->ip);
5375     if((r = readi(f->ip, addr, f->off, n)) > 0)
5376       f->off += r;
5377     iunlock(f->ip);
5378     return r;
5379   }
5380   panic("fileread");
5381 }
5382 
5383 
5384 
5385 
5386 
5387 
5388 
5389 
5390 
5391 
5392 
5393 
5394 
5395 
5396 
5397 
5398 
5399 
5400 // Write to file f.
5401 int
5402 filewrite(struct file *f, char *addr, int n)
5403 {
5404   int r;
5405 
5406   if(f->writable == 0)
5407     return -1;
5408   if(f->type == FD_PIPE)
5409     return pipewrite(f->pipe, addr, n);
5410   if(f->type == FD_INODE){
5411     // write a few blocks at a time to avoid exceeding
5412     // the maximum log transaction size, including
5413     // i-node, indirect block, allocation blocks,
5414     // and 2 blocks of slop for non-aligned writes.
5415     // this really belongs lower down, since writei()
5416     // might be writing a device like the console.
5417     int max = ((LOGSIZE-1-1-2) / 2) * 512;
5418     int i = 0;
5419     while(i < n){
5420       int n1 = n - i;
5421       if(n1 > max)
5422         n1 = max;
5423 
5424       begin_trans();
5425       ilock(f->ip);
5426       if ((r = writei(f->ip, addr + i, f->off, n1)) > 0)
5427         f->off += r;
5428       iunlock(f->ip);
5429       commit_trans();
5430 
5431       if(r < 0)
5432         break;
5433       if(r != n1)
5434         panic("short filewrite");
5435       i += r;
5436     }
5437     return i == n ? n : -1;
5438   }
5439   panic("filewrite");
5440 }
5441 
5442 
5443 
5444 
5445 
5446 
5447 
5448 
5449 
