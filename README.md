# OSLab-xv6
A modified xv6 operating system with several extra features.

## 1. Introduction to xv6

* Added contributors names to boot message.
* Shell features:

    1. ```CTRL + K```: Cursor jumps left one word 
    2. ```CTRL + L```: Cursor jumps right one word
    3. ```CTRL + I```: Line is deleted upto current char above cursor

* Comparing two string by adding ```strdiff``` command.

## 2. System calls
* ```calculate_biggest_perfect_square(int n)```: returns the biggest perfect square smaller than
* ```get_ancestors(int pid)```: prints all the ancestors of the process associated with pid
* ```get_descendants(int pid)```: prints all the descendants of the process associated with pid
* ```set_sleep(int n)```: sleeps the process for n seconds

## 3. CPU Scheduling
Modified the scheduler by adding 4 different levels for processes. 

0. Level 0: Round Robin 
1. Level 1: Minimum Priority
2. Level 2: Best Job First (BJF)
3. Level 3: First Come First Served (FCFS)



Some system calls added: 
* ```set_queue(int pid, int queue)```: Sets the level for the process
* ```set_priority(int pid, int priority)```: Sets the priority value for those processes in Level 1
* ```set_bjf_params(int pid, int priority_ratio, int arrival_time_ratio, int executed_cycle_ratio)```: Sets the parameters needed to chose the best job


View all processes in a beautiful table with ```ps``` command.


## 4. Synchronization

1. Allowing to acquire a lock multiple times. The user program ```multiple_acquire``` is added for test. 

2. Reader/Writer problem:
* Reader Priority: ```rwtest <num> 0```
* Writer Priority: ```rwtest <num> 1```

where *num* in binary, represents a queue of readers (0) and writers (1) waiting to enter a space. 



