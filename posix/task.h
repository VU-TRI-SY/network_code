/**
 * Representation of a task in the system.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef TASK_H
#define TASK_H

// representation of a task
typedef struct task_ {
    char *name;
    int priority;
    int burst;
    int arrival_time;
    int complete_time;
    int start_time;
} Task; 

#endif
