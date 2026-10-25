//
// Support functions for system calls that involve file descriptors.
//

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "stat.h"
#include "proc.h"
#include "sysinfo.h"

struct devsw devsw[NDEV];
struct {
  struct spinlock lock;
  struct file file[NFILE];
  uint64 version; // FD membership, ref, type and mode; protected by lock
} ftable;

void
fileinit(void)
{
  initlock(&ftable.lock, "ftable");
}

// Allocate a file structure.
struct file*
filealloc(void)
{
  struct file *f;

  acquire(&ftable.lock);
  for(f = ftable.file; f < ftable.file + NFILE; f++){
    if(f->ref == 0){
      // Reset metadata before making a recycled slot visible to collectors.
      memset(f, 0, sizeof(*f));
      f->ref = 1;
      ftable.version++;
      release(&ftable.lock);
      return f;
    }
  }
  release(&ftable.lock);
  return 0;
}

// Increment ref count for file f.
struct file*
filedup(struct file *f)
{
  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("filedup");
  f->ref++;
  ftable.version++;
  release(&ftable.lock);
  return f;
}

// Close file f.  (Decrement ref count, close when reaches 0.)
void
fileclose(struct file *f)
{
  struct file ff;

  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("fileclose");
  ftable.version++;
  if(--f->ref > 0){
    release(&ftable.lock);
    return;
  }
  ff = *f;
  f->ref = 0;
  f->type = FD_NONE;
  f->readable = f->writable = 0;
  release(&ftable.lock);

  if(ff.type == FD_PIPE){
    pipeclose(ff.pipe, ff.writable);
  } else if(ff.type == FD_INODE || ff.type == FD_DEVICE){
    begin_op();
    iput(ff.ip);
    end_op();
  }
}

// Get metadata about file f.
// addr is a user virtual address, pointing to a struct stat.
int
filestat(struct file *f, uint64 addr)
{
  struct proc *p = myproc();
  struct stat st;
  
  if(f->type == FD_INODE || f->type == FD_DEVICE){
    ilock(f->ip);
    stati(f->ip, &st);
    iunlock(f->ip);
    if(copyout(p->pagetable, addr, (char *)&st, sizeof(st)) < 0)
      return -1;
    return 0;
  }
  return -1;
}

// Read from file f.
// addr is a user virtual address.
int
fileread(struct file *f, uint64 addr, int n)
{
  int r = 0;

  if(f->readable == 0)
    return -1;

  if(f->type == FD_PIPE){
    r = piperead(f->pipe, addr, n);
  } else if(f->type == FD_DEVICE){
    if(f->major < 0 || f->major >= NDEV || !devsw[f->major].read)
      return -1;
    r = devsw[f->major].read(1, addr, n);
  } else if(f->type == FD_INODE){
    ilock(f->ip);
    if((r = readi(f->ip, 1, addr, f->off, n)) > 0)
      f->off += r;
    iunlock(f->ip);
  } else {
    panic("fileread");
  }

  return r;
}

// Write to file f.
// addr is a user virtual address.
int
filewrite(struct file *f, uint64 addr, int n)
{
  int r, ret = 0;

  if(f->writable == 0)
    return -1;

  if(f->type == FD_PIPE){
    ret = pipewrite(f->pipe, addr, n);
  } else if(f->type == FD_DEVICE){
    if(f->major < 0 || f->major >= NDEV || !devsw[f->major].write)
      return -1;
    ret = devsw[f->major].write(1, addr, n);
  } else if(f->type == FD_INODE){
    // write a few blocks at a time to avoid exceeding
    // the maximum log transaction size, including
    // i-node, indirect block, allocation blocks,
    // and 2 blocks of slop for non-aligned writes.
    // this really belongs lower down, since writei()
    // might be writing a device like the console.
    int max = ((MAXOPBLOCKS-1-1-2) / 2) * BSIZE;
    int i = 0;
    while(i < n){
      int n1 = n - i;
      if(n1 > max)
        n1 = max;

      begin_op();
      ilock(f->ip);
      if ((r = writei(f->ip, 1, addr + i, f->off, n1)) > 0)
        f->off += r;
      iunlock(f->ip);
      end_op();

      if(r != n1){
        // error from writei
        break;
      }
      i += r;
    }
    ret = (i == n ? n : -1);
  } else {
    panic("filewrite");
  }

  return ret;
}


// Internal helpers used by sysfile.c/pipe.c. Never sleep with ftable.lock held.
// Caller owns the new file and initializes its resource fields before setting
// exported metadata. Its own FD may already exist (open), but no other process
// may operate on f yet; a collector can observe FD_NONE during initialization.
void
filesetmeta(struct file *f, int type, int readable, int writable)
{
  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("filesetmeta");
  f->type = type;
  f->readable = readable;
  f->writable = writable;
  ftable.version++;
  release(&ftable.lock);
}

// Transfer the caller's existing reference into a free FD of the current proc.
// Failure leaves reference ownership with the caller.
int
fileallocfd(struct file *f)
{
  struct proc *p = myproc();
  acquire(&ftable.lock);
  for(int fd = 0; fd < NOFILE; fd++){
    if(p->ofile[fd] == 0){
      p->ofile[fd] = f;
      ftable.version++;
      release(&ftable.lock);
      return fd;
    }
  }
  release(&ftable.lock);
  return -1;
}

// Only the current process can detach its own descriptors.
// Return the existing reference; caller must fileclose AFTER we release lock.
struct file*
filedetachfd(struct proc *p, int fd)
{
  struct file *f;
  if(fd < 0 || fd >= NOFILE)
    return 0;
  acquire(&ftable.lock);
  f = p->ofile[fd];
  if(f){
    p->ofile[fd] = 0;
    ftable.version++;
  }
  release(&ftable.lock);
  return f;
}

// fork: src is current, dst has no FDs and cannot run yet; caller holds dst lock.
void
filecopyfds(struct proc *src, struct proc *dst)
{
  acquire(&ftable.lock);
  for(int fd = 0; fd < NOFILE; fd++){
    if(dst->ofile[fd])
      panic("filecopyfds: dst");
    struct file *f = src->ofile[fd];
    if(f){
      if(f->ref < 1)
        panic("filecopyfds: ref");
      f->ref++;
      dst->ofile[fd] = f;
      ftable.version++;
    }
  }
  release(&ftable.lock);
}

uint64
sysinfo_file_version(void)
{
  uint64 v;
  acquire(&ftable.lock);
  v = ftable.version;
  release(&ftable.lock);
  return v;
}

uint64
sysinfo_files(struct sysinfo *si)
{
  uint64 v;
  acquire(&ftable.lock);
  si->nfile = 0;
  for(int id = 0; id < NFILE; id++){
    struct file *f = &ftable.file[id];
    if(f->ref <= 0)
      continue;
    struct sysinfo_file *out = &si->files[si->nfile++];
    out->id = id;
    switch(f->type){
    case FD_NONE:   out->type = SI_FILE_NONE; break;
    case FD_PIPE:   out->type = SI_FILE_PIPE; break;
    case FD_INODE:  out->type = SI_FILE_INODE; break;
    case FD_DEVICE: out->type = SI_FILE_DEVICE; break;
    default: panic("sysinfo_files: type");
    }
    out->mode = (f->readable ? SI_MODE_READ : 0)
              | (f->writable ? SI_MODE_WRITE : 0);
    out->ref = f->ref;
    // npids/pids belong to the syscall merger, not this collector.
  }
  v = ftable.version;
  release(&ftable.lock);
  return v;
}

// Caller holds p->lock. Do not acquire any process lock under ftable.lock.
void
sysinfo_fds(struct proc *p, struct sysinfo_proc *out)
{
  acquire(&ftable.lock);
  out->nfd = 0;
  for(int fd = 0; fd < NOFILE; fd++){
    struct file *f = p->ofile[fd];
    out->file_id[fd] = -1;
    if(f){
      // All open file pointers originate in filealloc's fixed table.
      out->file_id[fd] = f - ftable.file;
      out->nfd++;
    }
  }
  release(&ftable.lock);
}
