#include "spinlock.h"

struct bed {
    struct spinlock lock;
    int name;
}; 