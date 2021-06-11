#include "types.h"
#include "stat.h"
#include "user.h"

#define READER 0
#define WRITER 1

int main(int argc, char *argv[]){
	// if(argc < 2){
	// 	printf(2, "You must enter exactly 1 number!\n");
	// 	exit();
	// }

	// int pattern = atoi(argv[1]);

	// rwtest(pattern);

	if (fork() == 0) {
      for (int i = 0; i < 5; i++) {
        if (fork() == 0) {
          rw_exec(WRITER);
          exit();
        }
      }
    }
    else if (fork() == 0) {
      for (int i = 0; i < 5; i++) {
        if (fork() == 0) {
          rw_exec(READER);
          exit();
        }
      }
    }

	while(wait() > -1);

    exit();
} 