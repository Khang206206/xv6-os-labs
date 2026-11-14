#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/riscv.h"
#include "kernel/memlayout.h"
#include "kernel/sysinfo.h"
#include "kernel/fcntl.h"
#include "user/user.h"

static struct sysinfo info;

static void
check(int ok, char *message)
{
  if(!ok){
    fprintf(2, "sysinfotest: FAIL: %s (pid %d)\n", message, getpid());
    exit(1);
  }
}

static struct sysinfo_proc *
process(int pid)
{
  for(int i = 0; i < info.nproc; i++)
    if(info.procs[i].pid == pid)
      return &info.procs[i];
  return 0;
}

static struct sysinfo_file *
file(int id)
{
  for(int i = 0; i < info.nfile; i++)
    if(info.files[i].id == id)
      return &info.files[i];
  return 0;
}

static int
haspid(struct sysinfo_file *f, int pid)
{
  for(int i = 0; i < f->npids; i++)
    if(f->pids[i] == pid)
      return 1;
  return 0;
}

// Validate both directions of the file/FD/PID map on every successful call.
static void
validate(void)
{
  check(info.freemem <= PHYSTOP - KERNBASE && info.freemem % PGSIZE == 0,
        "free memory bounds/alignment");
  check(info.nfile >= 0 && info.nfile <= NFILE, "file count bounds");
  check(info.nproc > 0 && info.nproc <= NPROC, "process count bounds");
  check(process(getpid()) != 0, "caller missing");
  for(int i = 0; i < info.nfile; i++){
    struct sysinfo_file *f = &info.files[i];
    check(f->id >= 0 && f->id < NFILE, "file ID bounds");
    check(f->type >= SI_FILE_NONE && f->type <= SI_FILE_DEVICE, "file type");
    check(f->mode >= 0 && f->mode <= (SI_MODE_READ | SI_MODE_WRITE), "mode");
    check(f->ref > 0 && f->npids >= 0 && f->npids <= NPROC, "ref/PID count");
    for(int j = 0; j < i; j++)
      check(info.files[j].id != f->id, "duplicate file ID");
  }
  for(int i = 0; i < info.nproc; i++){
    struct sysinfo_proc *p = &info.procs[i];
    check(p->pid > 0 && p->ppid >= 0, "process PID/parent");
    check(p->state >= SI_PROC_USED && p->state <= SI_PROC_ZOMBIE, "process state");
    for(int j = 0; j < i; j++)
      check(info.procs[j].pid != p->pid, "duplicate process");
    int nfd = 0;
    for(int fd = 0; fd < NOFILE; fd++){
      if(p->file_id[fd] == -1)
        continue;
      struct sysinfo_file *f = file(p->file_id[fd]);
      check(f != 0, "FD references missing file");
      check(haspid(f, p->pid), "FD missing reverse PID");
      nfd++;
    }
    check(p->nfd == nfd, "FD count");
  }
  for(int i = 0; i < info.nfile; i++){
    struct sysinfo_file *f = &info.files[i];
    int refs = 0, npids = 0;
    for(int j = 0; j < info.nproc; j++){
      int found = 0;
      for(int fd = 0; fd < NOFILE; fd++){
        if(info.procs[j].file_id[fd] == f->id){
          refs++;
          found = 1;
        }
      }
      npids += found;
    }
    check(refs <= f->ref && npids == f->npids, "ref/PID relationship");
    for(int j = 0; j < f->npids; j++){
      struct sysinfo_proc *p = process(f->pids[j]);
      check(p != 0, "file references missing process");
      int found = 0;
      for(int fd = 0; fd < NOFILE; fd++)
        found |= p->file_id[fd] == f->id;
      check(found, "PID has no FD for file");
      for(int k = 0; k < j; k++)
        check(f->pids[k] != f->pids[j], "duplicate PID for file");
    }
  }
}

static void
take(void)
{
  // Bounded settling at pipe handoffs, never retry indefinitely.
  for(int i = 0; i < 10; i++){
    if(sysinfo(&info) == 0){
      validate();
      return;
    }
    sleep(1);
  }
  check(0, "no snapshot after bounded settling");
}

static int
fdid(int fd)
{
  struct sysinfo_proc *p = process(getpid());
  check(p != 0 && fd >= 0 && fd < NOFILE, "lookup FD");
  return p->file_id[fd];
}

static void
expectfd(int fd, int type, int mode)
{
  struct sysinfo_file *f = file(fdid(fd));
  check(f != 0 && f->type == type && f->mode == mode, "FD type/mode");
}

static void
print64(uint64 n)
{
  char digits[21];
  int i = 20;
  digits[i] = 0;
  do {
    digits[--i] = '0' + n % 10;
    n /= 10;
  } while(n);
  printf("%s", &digits[i]);
}

static void
dump(void)
{
  printf("sysinfo: %d files, %d processes, %d bytes\n",
         info.nfile, info.nproc, (int)sizeof(info));
  printf("freemem=");
  print64(info.freemem);
  printf(" bytes\n");
  for(int i = 0; i < info.nfile; i++){
    struct sysinfo_file *f = &info.files[i];
    printf("file id=%d type=%d mode=%d ref=%d npids=%d pids:",
           f->id, f->type, f->mode, f->ref, f->npids);
    for(int j = 0; j < f->npids; j++)
      printf(" %d", f->pids[j]);
    printf("\n");
  }
  for(int i = 0; i < info.nproc; i++){
    struct sysinfo_proc *p = &info.procs[i];
    printf("pid=%d ppid=%d state=%d nsched=", p->pid, p->ppid, p->state);
    print64(p->nsched);
    printf(" nfd=%d fd:file:", p->nfd);
    for(int fd = 0; fd < NOFILE; fd++)
      if(p->file_id[fd] != -1)
        printf(" %d:%d", fd, p->file_id[fd]);
    printf("\n");
  }
}

static void
testfreemem(void)
{
  const int bytes = 8 * PGSIZE;
  check(sbrk(bytes) != (char *)-1, "warm up heap");
  check(sbrk(-bytes) != (char *)-1, "release warmup heap");
  take();
  uint64 baseline = info.freemem;
  for(int i = 0; i < 5; i++){
    check(sbrk(bytes) != (char *)-1, "grow heap");
    take();
    check(info.freemem + bytes == baseline, "heap allocation consumes pages");
    check(sbrk(-bytes) != (char *)-1, "shrink heap");
    take();
    check(info.freemem == baseline, "heap pages returned");
  }
  for(int i = 0; i < 5; i++){
    int pid = fork(), status;
    check(pid >= 0, "memory test fork");
    if(pid == 0)
      exit(0);
    check(wait(&status) == pid && status == 0, "memory test wait");
    take();
    check(info.freemem == baseline, "fork pages returned");
  }
  printf("sysinfotest: freemem baseline=");
  print64(baseline);
  printf(" bytes; allocated=%d bytes; reclaimed OK\n", bytes);
}

static void
testfiles(void)
{
  check(mkdir("si-tmp") == 0, "create test directory (si-tmp must not exist)");
  take();
  int baseline = info.nfile;
  int fd = open("si-tmp/file", O_CREATE | O_RDWR);
  check(fd >= 0, "create test file");
  check(write(fd, "x", 1) == 1, "write test file");
  take();
  int id = fdid(fd), ref = file(id)->ref;
  check(info.nfile == baseline + 1, "open adds object");
  expectfd(fd, SI_FILE_INODE, SI_MODE_READ | SI_MODE_WRITE);
  int other = open("si-tmp/file", O_RDONLY);
  check(other >= 0, "second open");
  char byte;
  check(read(other, &byte, 1) == 1 && byte == 'x', "read test file");
  take();
  check(fdid(other) != id && info.nfile == baseline + 2, "same inode, separate objects");
  expectfd(other, SI_FILE_INODE, SI_MODE_READ);
  int alias = dup(fd);
  check(alias >= 0, "dup");
  take();
  check(fdid(alias) == id && info.nfile == baseline + 2, "dup shares object");
  check(file(id)->ref == ref + 1 && file(id)->npids == 1, "dup ref and unique PID");
  check(close(alias) == 0, "close dup");
  take();
  check(fdid(alias) == -1 && file(id)->ref == ref, "close dup drops ref");
  check(close(fd) == 0 && close(other) == 0, "close opens");
  take();
  check(file(id) == 0 && info.nfile == baseline, "last close reclaims object");
  fd = open("si-tmp/file", O_WRONLY);
  check(fd >= 0, "write-only open");
  take();
  expectfd(fd, SI_FILE_INODE, SI_MODE_WRITE);
  close(fd);
  fd = open("si-tmp", O_RDONLY);
  check(fd >= 0, "directory open");
  take();
  expectfd(fd, SI_FILE_INODE, SI_MODE_READ);
  close(fd);
  fd = open("console", O_RDWR);
  check(fd >= 0, "console open");
  take();
  expectfd(fd, SI_FILE_DEVICE, SI_MODE_READ | SI_MODE_WRITE);
  close(fd);
  int ends[2];
  check(pipe(ends) == 0, "pipe");
  take();
  expectfd(ends[0], SI_FILE_PIPE, SI_MODE_READ);
  expectfd(ends[1], SI_FILE_PIPE, SI_MODE_WRITE);
  check(fdid(ends[0]) != fdid(ends[1]), "pipe has two objects");
  close(ends[0]);
  close(ends[1]);

  fd = open("si-tmp/file", O_RDONLY);
  check(fd >= 0, "open for FD limit");
  int aliases[NOFILE], n = 0;
  while(n < NOFILE && (alias = dup(fd)) >= 0)
    aliases[n++] = alias;
  take();
  check(process(getpid())->nfd == NOFILE, "fill descriptor table");
  id = fdid(fd);
  ref = file(id)->ref;
  int count = info.nfile;
  check(dup(fd) == -1 && open("si-tmp/file", O_RDONLY) == -1 &&
        pipe(ends) == -1, "full FD table failures");
  take();
  check(info.nfile == count && file(id)->ref == ref, "allocation rollback refs");
  check(n > 0 && close(aliases[--n]) == 0, "leave one free FD");
  check(pipe(ends) == -1, "pipe fails with only one free FD");
  take();
  check(info.nfile == count && file(id)->ref == ref - 1 &&
        process(getpid())->nfd == NOFILE - 1, "partial pipe allocation rollback");
  for(int i = 0; i < n; i++)
    close(aliases[i]);
  close(fd);
  take();
  check(info.nfile == baseline, "file tests release objects");
  check(unlink("si-tmp/file") == 0 && unlink("si-tmp") == 0, "remove test files");
  printf("sysinfotest: files/dup/modes/limits OK\n");
}

static void
sendbyte(int fd)
{
  check(write(fd, "x", 1) == 1, "pipe signal");
}

static void
recvbyte(int fd)
{
  char byte;
  check(read(fd, &byte, 1) == 1, "pipe receive");
}

static void
reap(int pid)
{
  int status;
  check(wait(&status) == pid && status == 0, "child exit status");
}

static void
testprocs(void)
{
  int go[2], ready[2];
  int shared = open("README", O_RDONLY);
  check(shared >= 0, "fork shared file");
  int alias = dup(shared);
  check(alias >= 0, "fork shared dup");
  check(pipe(go) == 0 && pipe(ready) == 0, "process sync pipes");
  take();
  int parent = getpid(), count = info.nproc;
  for(int i = 0; i < 32; i++)
    sleep(1);
  take();
  uint64 before = process(parent)->nsched;
  int inherited[NOFILE];
  memmove(inherited, process(parent)->file_id, sizeof(inherited));
  int sharedid = fdid(shared), ref = file(sharedid)->ref;
  int pid = fork();
  check(pid >= 0, "fork");
  if(pid == 0){
    close(go[1]);
    close(ready[0]);
    take();
    struct sysinfo_proc *p = process(getpid());
    check(p->ppid == parent && p->nsched > 0, "new child parent/scheduling");
    uint64 first = p->nsched;
    check(write(ready[1], &first, sizeof(first)) == sizeof(first), "child count");
    recvbyte(go[0]);
    take();
    check(process(getpid())->nsched > first, "child rescheduled after blocking");
    sendbyte(ready[1]);
    recvbyte(go[0]);
    exit(0);
  }
  close(go[0]);
  close(ready[1]);
  uint64 first;
  check(read(ready[0], &first, sizeof(first)) == sizeof(first), "read child count");
  check(first < before, "child starts with fresh scheduling count");
  int sleeping = 0;
  for(int i = 0; i < 20; i++){
    take();
    struct sysinfo_proc *child = process(pid);
    check(child != 0 && child->ppid == parent, "child listed with parent");
    if(child->state == SI_PROC_SLEEPING){
      sleeping = 1;
      break;
    }
    sleep(1);
  }
  check(sleeping && info.nproc == count + 1, "blocked child counted");
  struct sysinfo_proc *me = process(parent), *child = process(pid);
  check(me->nsched > before, "parent scheduling count increases");
  for(int fd = 0; fd < NOFILE; fd++){
    int id = inherited[fd];
    if(fd == go[1] || fd == ready[0])
      check(child->file_id[fd] == -1, "child closed unused sync FD");
    else
      check(child->file_id[fd] == id, "fork shares file IDs");
    if(id != -1 && me->file_id[fd] == id && child->file_id[fd] == id)
      check(haspid(file(id), parent) && haspid(file(id), pid), "fork reverse PID map");
  }
  check(file(sharedid)->ref == ref + 2 && file(sharedid)->npids == 2,
        "fork duplicates both refs but adds one PID");
  sendbyte(go[1]);
  recvbyte(ready[0]);
  sendbyte(go[1]);
  int zombie = 0;
  for(int i = 0; i < 50; i++){
    take();
    child = process(pid);
    if(child && child->state == SI_PROC_ZOMBIE){
      check(child->nfd == 0 && child->ppid == parent, "zombie parent/no FDs");
      zombie = 1;
      break;
    }
    sleep(1);
  }
  check(zombie && info.nproc == count + 1, "zombie counted before wait");
  reap(pid);
  take();
  check(process(pid) == 0 && info.nproc == count, "wait removes process");
  check(file(sharedid)->ref == ref && file(sharedid)->npids == 1, "exit drops child refs");
  close(go[1]);
  close(ready[0]);
  close(alias);
  close(shared);
  printf("sysinfotest: fork/process/scheduling/zombie OK\n");
}

static void
testbad(void)
{
  check(sysinfo((struct sysinfo *)MAXVA) == -1, "non-user address rejected");
  take();
  check(sysinfo((struct sysinfo *)(uint64)-1) == -1, "overflow address rejected");
  take();
  // The last 16 writable bytes precede an unmapped page; discard partial output.
  uint64 old = (uint64)sbrk(0);
  int grow = PGSIZE - old % PGSIZE;
  check(sbrk(grow) != (char *)-1, "allocate partial destination");
  uint64 end = old + grow;
  check(sysinfo((struct sysinfo *)(end - 16)) == -1, "partial copyout failure");
  check(sbrk(-grow) != (char *)-1, "release partial destination");
  take();
  // The user ELF maps the text page at zero without write permission.
  check(sysinfo((struct sysinfo *)0) == -1, "read-only text destination rejected");
  take();
  check(sysinfo((struct sysinfo *)TRAMPOLINE) == -1, "protected mapping rejected");
  take();
  int count = info.nfile, nfd = process(getpid())->nfd;
  check(pipe((int *)MAXVA) == -1, "pipe copyout failure");
  take();
  check(info.nfile == count && process(getpid())->nfd == nfd,
        "pipe copyout rolls back files and FDs");
  printf("sysinfotest: invalid/partial/protected buffers and recovery OK\n");
}

static void
testmanyprocs(void)
{
  int go[2], ready[2], children[NPROC / 4];
  check(pipe(go) == 0 && pipe(ready) == 0, "many-process sync pipes");
  take();
  int baseline = info.nproc, nfile = info.nfile, parent = getpid();
  int n = sizeof(children) / sizeof(children[0]);
  for(int i = 0; i < n; i++){
    children[i] = fork();
    check(children[i] >= 0, "many-process fork");
    if(children[i] == 0){
      close(go[1]);
      close(ready[0]);
      int fds[NOFILE], nfd = 0, fd;
      while(nfd < NOFILE && (fd = open("README", O_RDONLY)) >= 0)
        fds[nfd++] = fd;
      sendbyte(ready[1]);
      recvbyte(go[0]);
      for(int j = 0; j < nfd; j++)
        close(fds[j]);
      exit(0);
    }
  }
  close(go[0]);
  close(ready[1]);
  for(int i = 0; i < n; i++)
    recvbyte(ready[0]);
  take();
  check(info.nproc == baseline + n, "many-process count");
  check(info.nfile == NFILE, "global file table saturation with default limits");
  for(int i = 0; i < n; i++){
    struct sysinfo_proc *p = process(children[i]);
    check(p != 0 && p->ppid == parent, "many-process parent");
    // Sending one token releases one reader; the others remain blocked.
    sendbyte(go[1]);
  }
  for(int i = 0; i < n; i++){
    int status, pid = wait(&status), found = 0;
    for(int j = 0; j < n; j++)
      if(children[j] == pid){
        children[j] = -1;
        found = 1;
      }
    check(found && status == 0, "many-process child status");
  }
  close(go[1]);
  close(ready[0]);
  take();
  check(info.nproc == baseline, "many-process cleanup");
  check(info.nfile == nfile - 4, "full file table and sync pipe cleanup");
  printf("sysinfotest: %d synchronized children/full file table OK\n", n);
}

static void
testconcurrent(void)
{
  int start[2], ready[2], workers[3];
  check(pipe(start) == 0 && pipe(ready) == 0, "load sync pipes");
  for(int w = 0; w < 3; w++){
    workers[w] = fork();
    check(workers[w] >= 0, "load worker fork");
    if(workers[w] == 0){
      close(start[1]);
      close(ready[0]);
      sendbyte(ready[1]);
      recvbyte(start[0]);
      for(int i = 0; i < 40; i++){
        if(w == 0){
          int fd = open("README", O_RDONLY);
          check(fd >= 0, "load open");
          int alias = dup(fd);
          check(alias >= 0, "load dup");
          close(alias);
          close(fd);
        } else if(w == 1){
          int pid = fork();
          check(pid >= 0, "load fork");
          if(pid == 0)
            exit(0);
          reap(pid);
        }
        int rc = sysinfo(&info);
        check(rc == 0 || rc == -1, "load return code");
        if(rc == 0)
          validate();
      }
      exit(0);
    }
  }
  close(start[0]);
  close(ready[1]);
  for(int i = 0; i < 3; i++)
    recvbyte(ready[0]);
  for(int i = 0; i < 3; i++)
    sendbyte(start[1]);
  int successes = 0, busy = 0;
  for(int i = 0; i < 80; i++){
    int rc = sysinfo(&info);
    check(rc == 0 || rc == -1, "concurrent return code");
    if(rc == 0){
      validate();
      successes++;
    } else {
      busy++;
    }
  }
  for(int i = 0; i < 3; i++){
    int status, pid = wait(&status), found = 0;
    for(int j = 0; j < 3; j++)
      if(workers[j] == pid){
        workers[j] = -1;
        found = 1;
      }
    check(found && status == 0, "load worker status");
  }
  close(start[1]);
  close(ready[0]);
  check(sysinfo(&info) == 0, "single call succeeds after load stops");
  validate();
  printf("sysinfotest: concurrent callers/load OK (%d success, %d busy)\n",
         successes, busy);
}

int
main(void)
{
  printf("sysinfotest: start\n");
  take();
  dump();
  testfreemem();
  testfiles();
  testprocs();
  testbad();
  testmanyprocs();
  testconcurrent();
  take();
  dump();
  printf("sysinfotest: OK\n");
  exit(0);
}
