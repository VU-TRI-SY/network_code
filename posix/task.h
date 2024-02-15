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
    int burst; //current burst time ~ remaining burst time
    int total_burst; //init burst time ~ original burst time
    int arrival_time; 
    int complete_time;
    int start_time;
    int cur_turn; //the number of time that task is run by RR
} Task; 

#endif
