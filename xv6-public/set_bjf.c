#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"



int
main(int argc, char *argv[])
{  
  set_bjf_params(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));

  exit();
}