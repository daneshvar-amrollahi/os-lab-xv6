#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_calculate_biggest_perfect_square(void)
{
  int number = myproc()->tf->ebx; //register after eax
  cprintf("Kernel: sys_calculate_biggest_perfect_square() called for number %d\n", number);
  return calculate_biggest_perfect_square(number);
}

void
sys_get_ancestors(void)
{
  int pid = myproc()->tf->ebx; 
  cprintf("Kernel: sys_get_ancesrots() called for pid %d\n", pid);
  get_ancestors(pid);
}

void 
sys_set_sleep(void)
{
  int n;
  if (argint(0, &n) < 0)
    cprintf("Kernal: sys_set_sleep() has a problem.\n");
  else
    set_sleep(n);
}

void 
sys_set_date(void)
{
  struct rtcdate *r;
  if(argptr(0, (void*)&r, sizeof(*r)) < 0)
    cprintf("Kernel: sys_set_date() has a problem.\n");

  cmostime(r);
}

void 
sys_get_descendants(void)
{
  int pid = myproc()->tf->ebx; 
  cprintf("Kernel: sys_get_descendants() called for pid %d\n", pid);
  get_descendants(pid);
}

int
sys_process_start_time(void)
{
  struct proc *curproc = myproc();
  return (int)(curproc->creation_time); 
}

void
sys_print_all_proc(void)
{
  print_all_proc();
}

void
sys_set_queue(void)
{
  int pid, queue;
  argint(0, &pid);
  argint(1, &queue);
  set_queue(pid, queue);
}

void
sys_set_priority(void)
{
  int pid, priority;
  argint(0, &pid);
  argint(1, &priority);
  set_priority(pid, priority);
}

void sys_set_bjf_params(void)
{
  int pid, priority_ratio, arrival_time_ratio, executed_cycle_ratio;
  argint(0, &pid);
  argint(1, &priority_ratio);
  argint(2, &arrival_time_ratio);
  argint(3, &executed_cycle_ratio);
  set_bjf_params(pid, priority_ratio, arrival_time_ratio, executed_cycle_ratio);
}

void
sys_multiple_acquire(void)
{
  int cnt;
  argint(0, &cnt);
  multiple_acquire(cnt);
}