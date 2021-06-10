#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[])
{


  int cnt = atoi(argv[1]);
  
  
  printf(1, "Calling acquire() %d times\n", cnt);
  multiple_acquire(cnt);
    
    
  exit();
}