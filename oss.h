#include <stdio.h>
#include <unistd.h> //for pid_t and exec

#define BILLION 1000000000L
#define PERMS 0644

struct PCB {
int occupied; // either true or false
pid_t pid; // process id of this child
double sec; // time when it was forked
double nano; // time when it was forked 
};
struct PCB processTable[20];