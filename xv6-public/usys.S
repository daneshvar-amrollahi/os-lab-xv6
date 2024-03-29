#include "syscall.h"
#include "traps.h"

#define SYSCALL(name) \
  .globl name; \
  name: \
    movl $SYS_ ## name, %eax; \
    int $T_SYSCALL; \
    ret

SYSCALL(fork)
SYSCALL(exit)
SYSCALL(wait)
SYSCALL(pipe)
SYSCALL(read)
SYSCALL(write)
SYSCALL(close)
SYSCALL(kill)
SYSCALL(exec)
SYSCALL(open)
SYSCALL(mknod)
SYSCALL(unlink)
SYSCALL(fstat)
SYSCALL(link)
SYSCALL(mkdir)
SYSCALL(chdir)
SYSCALL(dup)
SYSCALL(getpid)
SYSCALL(sbrk)
SYSCALL(sleep)
SYSCALL(uptime)
SYSCALL(calculate_biggest_perfect_square)
SYSCALL(get_ancestors)
SYSCALL(set_sleep)
SYSCALL(set_date)
SYSCALL(get_descendants)
SYSCALL(process_start_time)
SYSCALL(print_all_proc)
SYSCALL(set_queue)
SYSCALL(set_priority)
SYSCALL(set_bjf_params)
SYSCALL(multiple_acquire)
SYSCALL(rwtest)
SYSCALL(rw_exec)
SYSCALL(wr_exec)
SYSCALL(shm_getat)
SYSCALL(shm_detach)
SYSCALL(shm_ctl)