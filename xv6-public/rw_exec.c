#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "bed.h"


#define READER 0
#define WRITER 1

int
main(int argc, char *argv[])
{
    if (fork() == 0) {
      for (int i = 0; i < 20; i++) {
        if (fork() == 0) {
          rw_exec(WRITER);
          exit();
        }
      }
    }
    else if (fork() == 0) {
      for (int i = 0; i < 20; i++) {
        if (fork() == 0) {
          rw_exec(READER);
          exit();
        }
      }
    }

  while(wait() > -1);

  exit();
} 