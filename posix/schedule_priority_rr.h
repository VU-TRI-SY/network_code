#include <stdbool.h>
#include "list.h"
#include "cpu.h"
#include "task.h"
#include "schedulers.h"
#ifndef SCHEDULE_PRIORITY_RR_H
#define SCHEDULE_PRIORITY_RR_H

void priority_rr_scheduler();
bool comesBefore(char *a, char *b);
Task *pickNextTask();
#endif