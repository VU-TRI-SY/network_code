#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "task.h"

#define MIN_PRIORITY 1
#define MAX_PRIORITY 10

#ifndef SCHEDULERS_H
#define SCHEDULERS_H
// add a task to the list 
void add(char *name, int priority, int burst);

// invoke the scheduler
void schedule();

#endif
