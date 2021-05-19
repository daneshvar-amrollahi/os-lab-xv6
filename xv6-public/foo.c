#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc, char *argv[])
{
    int pid[10];
    for (int i = 0 ; i < 5 ; i++)
    {
        pid[i] = fork();
        if (pid[i] == 0)
        {
            for (int j = 0 ; j < 5000000000 ; j++);
            exit();
        }
    }
    while (wait());
    return 0;
}