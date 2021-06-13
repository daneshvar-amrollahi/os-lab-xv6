#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
//#include "spinlock.h"
#include "bed.h"

#define READER 0
#define WRITER 1

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.


int
rand_int(int low, int high)
{
  int rand;
  acquire(&tickslock);
  rand = (ticks * ticks * ticks * 71413) % (high - low + 1) + low;
  release(&tickslock);
  return rand;
}


static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  p->queue = 1; //default: RR
  p->exec_cycle = 0;
  p->last_cpu_time = 0;

  p->priority = rand_int(1, 1000);
  p->priority_ratio = 1;
  p->arrival_time_ratio = 1;
  p->executed_cycle_ratio = 1;
  p->creation_time = ticks;

  acquire(&tickslock);
  p->creation_time = ticks;
  release(&tickslock);

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();
  //curproc->creation_time = ticks;

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);
  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.

float
get_rank(struct proc* p)
{
  return 
    p->priority * p->priority_ratio
    + p->creation_time * p->arrival_time_ratio
    + p->exec_cycle * p->executed_cycle_ratio;
}

struct proc*
bjf_finder(void)
{
  struct proc* p;
  struct proc* min_proc = 0;
  float min_rank = 1000000;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){ 
    if (p->state != RUNNABLE || p->queue != 3)
      continue;
    if (get_rank(p) < min_rank){
      min_proc = p;
      min_rank = get_rank(p);
    }
  }

  return min_proc;
}

struct proc*
round_robin_finder(void)
{
  struct proc *p;
  struct proc *best = 0;

  int now = ticks;
  int max_proc = -100000;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE || p->queue != 1)
        continue;

      if(now - p->last_cpu_time > max_proc){
        max_proc = now - p->last_cpu_time;
        best = p;
      }
  }
  return best;
}

struct proc*
min_priority_finder(void)
{
  struct proc *p;
  struct proc *min_proc = 0;

  int mn = 2e9;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if (p->state != RUNNABLE || p->queue != 2)
        continue;

      if (p->priority < mn)
      {
        mn = p->priority;
        min_proc = p;
      }
  }

  return min_proc;
}

struct proc*
fcfs_finder(void)
{
  struct proc *p;
  struct proc *first_proc = 0;

  int mn = 2e9;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if (p->state != RUNNABLE || p->queue != 4)
        continue;

      if (p->creation_time < mn)
      {
        mn = p->creation_time;
        first_proc = p;
      }
  }
  return first_proc;
}

//queue: C B A                                                              1
//queue: C B    -    A waiting                                              2
//queue: B C    -    A waiting                                              3
//queue: A B C  -                                                           4
//queue: A B         last_cpu_time(A): 1   ,    last_cpu_time(B): 3         5
//

//5 - 1 > 5 - 3 ---> A is chosen

void
scheduler(void)
{
  struct proc *p = 0;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);

    // struct proc *min_rank;
    // int found = -1;
    // for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    //   if(p->queue != 3 || p->state != RUNNABLE)
    //     continue;
    //   if(found == -1){
    //     min_rank = p;
    //     found = 1;
    //   }
    //   else if(get_rank(p) < get_rank(min_rank))
    //     min_rank = p;
    // }

    p = round_robin_finder();

    if (p == 0)
      p = min_priority_finder();

    if(p == 0)
      p = bjf_finder();
    
    if (p == 0)
      p = fcfs_finder();

    if (p == 0) {
      release(&ptable.lock);
      continue;
    }
    c->proc = p;
    switchuvm(p);
    p->state = RUNNING;

    swtch(&(c->scheduler), p->context);
    switchkvm();

    c->proc = 0;

    // for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    //   if(p->state != RUNNABLE)
    //     continue;

    //   // Switch to chosen process.  It is the process's job
    //   // to release ptable.lock and then reacquire it
    //   // before jumping back to us.
    //   c->proc = p;
    //   switchuvm(p);
    //   p->state = RUNNING;

    //   swtch(&(c->scheduler), p->context);
    //   switchkvm();

    //   // Process is done running for now.
    //   // It should have changed its p->state before coming back.
    //   c->proc = 0;
    // }
    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  myproc()->last_cpu_time = ticks;
  myproc()->exec_cycle += 0.1;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

int calculate_biggest_perfect_square(int n)
{
    int ans = 1;
    while (ans * ans < n)
      ans++;
    ans--;
  
    return ans * ans;
}

void 
get_ancestors(int pid)
{ 
    struct proc *p;

    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){ 
      if (p->pid == pid)
        break;
    }

    while (p->pid != 1)
    {
      cprintf("my id: %d, ", p->pid);
      cprintf("my parent id: %d\n ", p->parent->pid);

      p = p->parent;
    }  

    //cprintf("my parent id: %d\n ", p->parent->pid);
    
    release(&ptable.lock);

    //pid = p->parent->pid;

}

void 
get_descendants(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){ 
    if (p->parent->pid == pid)
    {
      cprintf("my id: %d, ", p->pid);
      cprintf("my parent id: %d\n ", pid);

      release(&ptable.lock);
      get_descendants(p->pid);
      acquire(&ptable.lock);
    }
  }
  release(&ptable.lock);
}


void 
set_sleep(int n)
{
  uint ticks0;
  ticks0 = ticks;

  while(ticks - ticks0 < n * 100)
      sti();

}


char*
get_state_string(int state)
{
  if (state == 0) {
    return "UNUSED";
  }
  else if (state == 1) {
    return "EMBRYO";
  }
  else if (state == 2) {
    return "SLEEPING";
  }
  else if (state == 3) {
    return "RUNNABLE";
  }
  else if (state == 4) {
    return "RUNNING";
  }
  else if (state == 5) {
    return "ZOMBIE";
  }
  else {
    return "";
  }
}

char*
get_queue_string(int queue)
{
  if (queue == 1)
    return "RR";
  else if (queue == 2)
    return "PRIR";
  else if (queue == 3)
    return "BJF";
  else
    return "FCFS";
}

int
get_int_len(int num)
{
  int len = 0;
  if (num == 0)
    return 1;
  while (num > 0) {
    len++;
    num = num / 10;
  }
  return len;
}

int round(float num){
  if(num - (int)(num) <= 0.5)
    return (int)(num);
  return (int)(num) + 1;
}

void 
print_all_proc()
{
  struct proc *p;

  cprintf("name");
  for (int i = 0 ; i < 5 - 4 ; i++)
    cprintf(" ");
  
  cprintf("pid");
  for (int i = 0 ; i < 5 - 3 ; i++)
    cprintf(" ");

  cprintf("state");
  for (int i = 0 ; i < 10 - 5 ; i++)
    cprintf(" ");

  cprintf("queue");
  for (int i = 0 ; i < 6 - 5 ; i++)
    cprintf(" ");

  cprintf("priority");
  for (int i = 0; i < 10 - 8 ; i++)
    cprintf(" ");

  
  cprintf("p_ratio");
  for (int i = 0 ; i < 10 - 7 ; i++)
    cprintf(" ");

  cprintf("at_ratio");
  for (int i = 0 ; i < 10 - 8 ; i++)
    cprintf(" ");

  cprintf("ec_ratio");
  for (int i = 0 ; i < 10 - 8 ; i++)
    cprintf(" ");

  cprintf("arr_time");
  for (int i = 0 ; i < 10 - 8 ; i++)
    cprintf(" ");

  cprintf("exec_c*10");
  for (int i = 0; i < 10 - 10 ; i++)
    cprintf(" ");


  cprintf("\n");
  for (int i = 0 ; i < 86 ; i++)
    cprintf("-");
  cprintf("\n");



  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){ 
    if (p->state == UNUSED)
      continue;


    cprintf(p->name);
    for (int i = 0 ; i < 5 - strlen(p->name) ; i++)
      cprintf(" ");

    cprintf("%d", p->pid);
    for (int i = 0 ; i < 5 - get_int_len(p->pid) ; i++)
      cprintf(" ");

    cprintf(get_state_string(p->state));
    for (int i = 0 ; i < 10 - strlen(get_state_string(p->state)) ; i++)
      cprintf(" "); 

    cprintf(get_queue_string(p->queue));
    for (int i = 0 ; i < 6 - strlen(get_queue_string(p->queue)); i++)
      cprintf(" ");

    cprintf("%d", p->priority);
    for (int i = 0; i < 10 - get_int_len(p->priority); i++)
      cprintf(" ");


    cprintf("%d", p->priority_ratio);
    for (int i = 0 ; i < 10 - get_int_len(p->priority_ratio) ; i++)
      cprintf(" ");

    cprintf("%d", p->arrival_time_ratio);
    for (int i = 0 ; i < 10 - get_int_len(p->arrival_time_ratio) ; i++)
      cprintf(" ");

    cprintf("%d", p->executed_cycle_ratio);
    for (int i = 0 ; i < 10 - get_int_len(p->executed_cycle_ratio) ; i++)
      cprintf(" ");

    cprintf("%d", p->creation_time);
    for (int i = 0 ; i < 10 - get_int_len(p->creation_time) ; i++)
      cprintf(" ");

    cprintf("%d", round(p->exec_cycle * 10));

    cprintf("\n");
  }
  release(&ptable.lock);
  
}

void
set_queue(int pid, int queue)
{
  struct proc *p;

  acquire(&ptable.lock);
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->pid == pid)
      p->queue = queue;
  }
  release(&ptable.lock);
}

void
set_priority(int pid, int priority){
  struct proc *p;

  acquire(&ptable.lock);
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->pid == pid)
      p->priority = priority;
  }
  release(&ptable.lock); 
}

void
set_bjf_params(int pid, int priority_ratio, int arrival_time_ratio, int executed_cycle_ratio)
{
  struct proc *p;

  acquire(&ptable.lock);
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->pid == pid)
    {
      p->priority_ratio = priority_ratio;
      p->arrival_time_ratio = arrival_time_ratio;
      p->executed_cycle_ratio = executed_cycle_ratio; 
    }
  }
  release(&ptable.lock); 
}

void
multiple_acquire(int cnt)
{
  if (cnt == 0)
  {
    release(&tickslock);
    return;
  }

  cprintf("Acquiring... cnt = %d\n", cnt);
  acquire(&tickslock);
  multiple_acquire(cnt - 1); 
}

struct bed r_bed = {.lock.locked = 0};
struct spinlock lk = {.locked = 0};
int readers_count = 0;
int writers_count = 0;
int is_writing = 0;
int is_reading = 0;

void 
rw_exec(int type)
{
  
    if (type == READER) {
      acquire(&lk);
      readers_count++;
      cprintf("Reader PID %d: behind while\n", myproc()->pid);

      while (is_writing)
        sleep(&r_bed, &lk);

      cprintf("Reader PID %d: passed while\n", myproc()->pid);

      is_reading++;
      release(&lk);

      cprintf("Reader PID %d: reading started\n", myproc()->pid);
      cprintf("Reader PID %d: reading done\n", myproc()->pid);

      acquire(&lk);
      is_reading--;
      readers_count--;
      cprintf("Reader PID %d: exiting\n", myproc()->pid);
      wakeup(&r_bed);
      release(&lk);
    }


    else if (type == WRITER) {
      acquire(&lk);
      // if (readers_count > 0)
      //   sleep(&r_bed, &lk);

      cprintf("Writer PID %d: behind while\n", myproc()->pid);
      while (is_reading || is_writing || readers_count)
        sleep(&r_bed, &lk);

      cprintf("Writer PID %d: passed while\n", myproc()->pid);
      is_writing++;
      release(&lk);


      cprintf("Writer PID %d: writing started\n", myproc()->pid);
      
      set_sleep(3);

      cprintf("Writer PID %d: writing done\n", myproc()->pid);

      acquire(&lk);
      is_writing--;
      cprintf("Writer PID %d: exiting\n", myproc()->pid);
      wakeup(&r_bed);
      release(&lk);
    }
  
}

void 
wr_exec(int type)
{

    if (type == WRITER) {
        acquire(&lk);
        writers_count++;
        cprintf("Writer PID %d: behind while\n", myproc()->pid);
        while (is_writing || is_reading)
          sleep(&r_bed, &lk);
        cprintf("Writer PID %d: passed while\n", myproc()->pid);
        is_writing++;
        release(&lk);

        cprintf("Writer PID %d: writing started\n", myproc()->pid);
        
        cprintf("Writer PID %d: writing done\n", myproc()->pid);

        acquire(&lk);
        is_writing--;
        writers_count--;
        cprintf("Writer PID %d: exiting\n", myproc()->pid);
        wakeup(&r_bed);
        release(&lk);
    }

    else if (type == READER) {
        acquire(&lk);
        readers_count++;
        cprintf("Reader PID %d: behind while\n", myproc()->pid);
        while (is_writing || writers_count)
          sleep(&r_bed, &lk);
        cprintf("Reader PID %d: passed while\n", myproc()->pid);
        is_reading++;
        release(&lk);


        cprintf("Reader PID %d: reading started\n", myproc()->pid);
        set_sleep(3);
        cprintf("Reader PID %d: reading done\n", myproc()->pid);

        acquire(&lk);
        is_reading--;
        readers_count--;
        cprintf("Reader PID %d: exiting\n", myproc()->pid);
        wakeup(&r_bed);
        release(&lk);
    }
}

void
rwtest(int pattern)
{
  int digit_count = 0;
  int pattern_copy = pattern;
  while(pattern_copy)
  {
    pattern_copy /= 2;
    digit_count++;
  }

  int reader_writers[20];
  for (int i = 0; i < digit_count; i++)
  {
    reader_writers[i] = ((1 << i) & (pattern)) ? 1 : 0;
  }

  for(int i = digit_count - 1; i >= 0; i--)
    cprintf("%d", reader_writers[i]);

  cprintf("\n");
  // rw_exec(reader_writers[0]);
  
  for (int i = 0; i < digit_count - 1; i++)
  {
    int pid = 1;
    //cprintf("fork output = %d.\n", pid);
    if (pid == 0) {
      cprintf("child of fork()\n");
      rw_exec(reader_writers[i]);
      exit();
    }
  }
  while(wait() > -1);
  
}