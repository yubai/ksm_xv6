4200 #include "types.h"
4201 #include "defs.h"
4202 #include "param.h"
4203 #include "spinlock.h"
4204 #include "fs.h"
4205 #include "buf.h"
4206 
4207 // Simple logging. Each system call that might write the file system
4208 // should be surrounded with begin_trans() and commit_trans() calls.
4209 //
4210 // The log holds at most one transaction at a time. Commit forces
4211 // the log (with commit record) to disk, then installs the affected
4212 // blocks to disk, then erases the log. begin_trans() ensures that
4213 // only one system call can be in a transaction; others must wait.
4214 //
4215 // Allowing only one transaction at a time means that the file
4216 // system code doesn't have to worry about the possibility of
4217 // one transaction reading a block that another one has modified,
4218 // for example an i-node block.
4219 //
4220 // Read-only system calls don't need to use transactions, though
4221 // this means that they may observe uncommitted data. I-node and
4222 // buffer locks prevent read-only calls from seeing inconsistent data.
4223 //
4224 // The log is a physical re-do log containing disk blocks.
4225 // The on-disk log format:
4226 //   header block, containing sector #s for block A, B, C, ...
4227 //   block A
4228 //   block B
4229 //   block C
4230 //   ...
4231 // Log appends are synchronous.
4232 
4233 // Contents of the header block, used for both the on-disk header block
4234 // and to keep track in memory of logged sector #s before commit.
4235 struct logheader {
4236   int n;
4237   int sector[LOGSIZE];
4238 };
4239 
4240 struct log {
4241   struct spinlock lock;
4242   int start;
4243   int size;
4244   int busy; // a transaction is active
4245   int dev;
4246   struct logheader lh;
4247 };
4248 
4249 
4250 struct log log;
4251 
4252 static void recover_from_log(void);
4253 
4254 void
4255 initlog(void)
4256 {
4257   if (sizeof(struct logheader) >= BSIZE)
4258     panic("initlog: too big logheader");
4259 
4260   struct superblock sb;
4261   initlock(&log.lock, "log");
4262   readsb(ROOTDEV, &sb);
4263   log.start = sb.size - sb.nlog;
4264   log.size = sb.nlog;
4265   log.dev = ROOTDEV;
4266   recover_from_log();
4267 }
4268 
4269 // Copy committed blocks from log to their home location
4270 static void
4271 install_trans(void)
4272 {
4273   int tail;
4274 
4275   for (tail = 0; tail < log.lh.n; tail++) {
4276     struct buf *lbuf = bread(log.dev, log.start+tail+1); // read log block
4277     struct buf *dbuf = bread(log.dev, log.lh.sector[tail]); // read dst
4278     memmove(dbuf->data, lbuf->data, BSIZE);  // copy block to dst
4279     bwrite(dbuf);  // write dst to disk
4280     brelse(lbuf);
4281     brelse(dbuf);
4282   }
4283 }
4284 
4285 // Read the log header from disk into the in-memory log header
4286 static void
4287 read_head(void)
4288 {
4289   struct buf *buf = bread(log.dev, log.start);
4290   struct logheader *lh = (struct logheader *) (buf->data);
4291   int i;
4292   log.lh.n = lh->n;
4293   for (i = 0; i < log.lh.n; i++) {
4294     log.lh.sector[i] = lh->sector[i];
4295   }
4296   brelse(buf);
4297 }
4298 
4299 
4300 // Write in-memory log header to disk.
4301 // This is the true point at which the
4302 // current transaction commits.
4303 static void
4304 write_head(void)
4305 {
4306   struct buf *buf = bread(log.dev, log.start);
4307   struct logheader *hb = (struct logheader *) (buf->data);
4308   int i;
4309   hb->n = log.lh.n;
4310   for (i = 0; i < log.lh.n; i++) {
4311     hb->sector[i] = log.lh.sector[i];
4312   }
4313   bwrite(buf);
4314   brelse(buf);
4315 }
4316 
4317 static void
4318 recover_from_log(void)
4319 {
4320   read_head();
4321   install_trans(); // if committed, copy from log to disk
4322   log.lh.n = 0;
4323   write_head(); // clear the log
4324 }
4325 
4326 void
4327 begin_trans(void)
4328 {
4329   acquire(&log.lock);
4330   while (log.busy) {
4331     sleep(&log, &log.lock);
4332   }
4333   log.busy = 1;
4334   release(&log.lock);
4335 }
4336 
4337 
4338 
4339 
4340 
4341 
4342 
4343 
4344 
4345 
4346 
4347 
4348 
4349 
4350 void
4351 commit_trans(void)
4352 {
4353   if (log.lh.n > 0) {
4354     write_head();    // Write header to disk -- the real commit
4355     install_trans(); // Now install writes to home locations
4356     log.lh.n = 0;
4357     write_head();    // Erase the transaction from the log
4358   }
4359 
4360   acquire(&log.lock);
4361   log.busy = 0;
4362   wakeup(&log);
4363   release(&log.lock);
4364 }
4365 
4366 // Caller has modified b->data and is done with the buffer.
4367 // Append the block to the log and record the block number,
4368 // but don't write the log header (which would commit the write).
4369 // log_write() replaces bwrite(); a typical use is:
4370 //   bp = bread(...)
4371 //   modify bp->data[]
4372 //   log_write(bp)
4373 //   brelse(bp)
4374 void
4375 log_write(struct buf *b)
4376 {
4377   int i;
4378 
4379   if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1)
4380     panic("too big a transaction");
4381   if (!log.busy)
4382     panic("write outside of trans");
4383 
4384   for (i = 0; i < log.lh.n; i++) {
4385     if (log.lh.sector[i] == b->sector)   // log absorbtion?
4386       break;
4387   }
4388   log.lh.sector[i] = b->sector;
4389   struct buf *lbuf = bread(b->dev, log.start+i+1);
4390   memmove(lbuf->data, b->data, BSIZE);
4391   bwrite(lbuf);
4392   brelse(lbuf);
4393   if (i == log.lh.n)
4394     log.lh.n++;
4395   b->flags |= B_DIRTY; // XXX prevent eviction
4396 }
4397 
4398 
4399 
4400 // Blank page.
4401 
4402 
4403 
4404 
4405 
4406 
4407 
4408 
4409 
4410 
4411 
4412 
4413 
4414 
4415 
4416 
4417 
4418 
4419 
4420 
4421 
4422 
4423 
4424 
4425 
4426 
4427 
4428 
4429 
4430 
4431 
4432 
4433 
4434 
4435 
4436 
4437 
4438 
4439 
4440 
4441 
4442 
4443 
4444 
4445 
4446 
4447 
4448 
4449 
