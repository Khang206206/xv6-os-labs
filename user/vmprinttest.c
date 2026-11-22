#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char **argv)
{
  char *args[] = {"echo", "normal-after-errors", 0};
  if(argc == 2 && strcmp(argv[1], "ordinary") == 0){
    exec("echo", args);
    exit(1);
  }
  if(execpt("missing-vmpt", args) != -1 ||
     execpt("README", args) != -1 ||
     execpt("echo", (char **)-1) != -1){
    fprintf(2, "vmprinttest: FAIL\n");
    exit(1);
  }
  printf("vmprinttest: OK\n");
  exit(0);
}
