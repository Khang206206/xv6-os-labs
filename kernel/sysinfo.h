#ifndef XV6_SYSINFO_H
#define XV6_SYSINFO_H

#include "types.h"
#include "param.h"

#define SI_FILE_NONE    0
#define SI_FILE_PIPE    1
#define SI_FILE_INODE   2
#define SI_FILE_DEVICE  3

#define SI_MODE_READ    1
#define SI_MODE_WRITE   2

#define SI_PROC_UNUSED    0
#define SI_PROC_USED      1
#define SI_PROC_SLEEPING  2
#define SI_PROC_RUNNABLE  3
#define SI_PROC_RUNNING   4
#define SI_PROC_ZOMBIE    5

struct sysinfo_file {
  int id;                 // ftable slot; may be reused between snapshots
  int type;
  int mode;
  int ref;
  int npids;
  int pids[NPROC];         // distinct PIDs
};

struct sysinfo_proc {
  int pid;
  int ppid;
  int state;
  int nfd;
  uint64 nsched;
  int file_id[NOFILE];     // indexed by fd; -1 for a closed descriptor
};

struct sysinfo {
  int nfile;
  int nproc;
  struct sysinfo_file files[NFILE];
  struct sysinfo_proc procs[NPROC];
  uint64 freemem;
};

#endif
