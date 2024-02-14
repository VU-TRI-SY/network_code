#include <stdbool.h>
#include "list.h"
#include "cpu.h"
#include "task.h"

#ifndef SCHEDULE_SJF_H
#define SCHEDULE_SJF_H

void sjf_scheduler();
bool comesBefore(Task *t1, Task *t2);
Task *pickNextTask();
#endif