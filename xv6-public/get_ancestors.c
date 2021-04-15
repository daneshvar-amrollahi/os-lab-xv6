#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[])
{

    int saved_ebx;

  int pid0, pid1, pid2;
  //pid1, pid2;
  printf(1, "test program: forking childs\n\n");
  pid0 = fork();
  if (pid0)
    printf(1, "pid: %d, parent: %d\n", pid0, getpid());
    
  pid1 = fork();
  if (pid1 && pid0 == 0)
    printf(1, "pid: %d, parent: %d\n", pid1, getpid());

  pid2 = fork();
  if (pid2 && pid1 == 0 && pid0 == 0)
    printf(1, "pid: %d, parent: %d\n", pid2, getpid());



  if(pid0 == 0 && pid1 == 0 && pid2){
    printf(1, "User: printing ancestors for pid: %d\n" , pid2);

    asm volatile(
    "movl %%ebx, %0;" // saved_ebx = ebx
    "movl %1, %%ebx;" // ebx = number
    : "=r" (saved_ebx)
    : "r"(pid2)
  );

  //printf(1, "Biggest perfect square lower than %d is: %d\n" , number , calculate_biggest_perfect_square());
    get_ancestors();
    asm("movl %0, %%ebx" : : "r"(saved_ebx)); // ebx = saved_ebx -> restore
  }  
  while(wait() != -1);
  
    
  exit();
}