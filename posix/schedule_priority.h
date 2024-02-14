#include <stdbool.h>
#include "list.h"
#include "cpu.h"
#include "task.h"

#ifndef SCHEDULE_PRIORITY_H
#define SCHEDULE_PRIORITY_H

void priority_scheduler();
bool comesBefore(Task *t1, Task *t2);
Task *pickNextTask();
#endif