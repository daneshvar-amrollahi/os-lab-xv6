#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[])
{


  int pid0;
  
  printf(1, "test program: process_start_time\n\n");
  pid0 = fork();
  if (pid0)
    printf(1, "pid: %d, parent: %d\n", pid0, getpid());
    

  if(pid0){
    printf(1, "current process start time is %d\n", process_start_time());
    
  }  
  while(wait() != -1);
  
    
  exit();
}