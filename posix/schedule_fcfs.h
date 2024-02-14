#include <stdbool.h>
#include "list.h"
#include "cpu.h"
#include "task.h"
#include "schedulers.h"
#ifndef SCHEDULE_FCFC_H
#define SCHEDULE_FCFC_H

void fcfs_scheduler();
bool comesBefore(char *a, char *b);
Task *pickNextTask();
#endif