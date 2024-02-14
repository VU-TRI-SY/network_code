#include <stdbool.h>
#include "list.h"
#include "cpu.h"
#include "task.h"

#ifndef SCHEDULE_RR_H
#define SCHEDULE_RR_H

void rr_scheduler();
bool comesBefore(Task *t1, Task *t2);
Task *pickNextTask();
#endif