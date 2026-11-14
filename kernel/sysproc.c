#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sleeplock.h"
#include "sysinfo.h"

static struct sysinfo snapshot;
static struct sleeplock snapshot_lock;

void
sysinfoinit(void)
{
  initsleeplock(&snapshot_lock, "sysinfo");
}

// Join by ftable slot, counting each process only once per file.
static int
sysinfo_pids(void)
{
  if(snapshot.nfile < 0 || snapshot.nfile > NFILE ||
     snapshot.nproc < 0 || snapshot.nproc > NPROC)
    return -1;
  for(int i = 0; i < snapshot.nproc; i++){
    char seen[NFILE] = {0};
    struct sysinfo_proc *p = &snapshot.procs[i];
    for(int fd = 0; fd < NOFILE; fd++){
      int id = p->file_id[fd];
      if(id == -1)
        continue;
      int j;
      for(j = 0; j < snapshot.nfile; j++)
        if(snapshot.files[j].id == id)
          break;
      if(id < 0 || id >= NFILE || j == snapshot.nfile)
        return -1;
      if(!seen[j]){
        struct sysinfo_file *f = &snapshot.files[j];
        if(f->npids >= NPROC)
          return -1;
        f->pids[f->npids++] = p->pid;
        seen[j] = 1;
      }
    }
  }
  return 0;
}

uint64
sys_sysinfo(void)
{
  uint64 addr;
  int result = -1;

  argaddr(0, &addr);
  acquiresleep(&snapshot_lock);
  for(int attempt = 0; attempt < 3; attempt++){
    memset(&snapshot, 0, sizeof(snapshot));
    uint64 version = sysinfo_file_version();
    sysinfo_procs(&snapshot);
    if(sysinfo_files(&snapshot) != version)
      continue;
    if(sysinfo_pids() == 0){
      snapshot.freemem = kfreemem();
      result = copyout(myproc()->pagetable, addr,
                       (char *)&snapshot, sizeof(snapshot));
    }
    break;
  }
  releasesleep(&snapshot_lock);
  return result;
}

uint64
sys_trace(void){
  int mask;
  argint(0, &mask);
  myproc()->trace_mask = mask;
  return 0;
}

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
