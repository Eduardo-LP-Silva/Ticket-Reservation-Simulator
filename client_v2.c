#include <stdio.h>

#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  printf("** Running process %d (PGID %d) **\n", getpid(), getpgrp());

  if (argc == 4)
    printf("ARGS: %s | %s | %s\n", argv[1], argv[2], argv[3]);

  sleep(1);

  return 0;
}
