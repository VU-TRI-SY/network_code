// length of a time quantum
#include "task.h"
#define QUANTUM 10
#ifndef CPU_H
#define CPU_H
// run the specified task for the following time slice
void run(Task *task, int slice);
#endif