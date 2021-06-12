#include "types.h"
#include "stat.h"
#include "user.h"

#define READER 0
#define WRITER 1

int main(int argc, char *argv[]){
	int pattern = atoi(argv[1]);
  int priority = atoi(argv[2]);

	//rwtest(pattern);

  
  int digit_count = 0;
  int pattern_copy = pattern;
  while(pattern_copy)
  {
    pattern_copy /= 2;
    digit_count++;
  }

  int reader_writers[digit_count];
  for (int i = 0; i < digit_count; i++)
  {
    reader_writers[i] = ((1 << i) & (pattern)) ? 1 : 0;
  }

  for(int i = digit_count - 1; i >= 0; i--)
    printf(1, "%d", reader_writers[i]);

  printf(1, "\n");

  for (int i = digit_count - 2; i >= 0 ; i--)
  {
    int pid = fork();
    if (pid == 0) {
      if (priority == READER)
        rw_exec(reader_writers[i]);
      else if (priority == WRITER)
        wr_exec(reader_writers[i]);

      exit();
    }
  }

  while(wait() != -1);
  

  return 0;
} 