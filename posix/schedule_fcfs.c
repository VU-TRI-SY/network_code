#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "schedule_fcfs.h"

Node *task_list = NULL;
Node *sorted_task_list = NULL; //only for bonus part
// this list will contains all tasks that sorted in running order(based on lexicographical order)

void fcfs_scheduler(){
    int time_counting = 0;
    int total_dispatcher_time = 0;
    copyList(&sorted_task_list, task_list); //copy tasks from task_list to sorted_task_list
    sortList(sorted_task_list);
    // Node *cur = running_order_list;
    while(1){
        Task* t = pickNextTask(); // t is the best task selected from task_list
        if(t == NULL) break; //there is no task to run
        delete(&task_list, t); //delete slected task_list

        t->start_time = time_counting;
        run(t, t->burst); //run this task
        time_counting += t->burst; //update time counting after run this task
        printf("        Time is now: %d\n", time_counting);
        t->complete_time = time_counting; //update complete time = time_counting
        
        //move to next task
        if(pickNextTask()){ //there are tasks to switch to
            total_dispatcher_time++;
        }

    }
    //%: %%
    //%d, %f, %s
    printf("CPU Utilization: %.2f%%\n",  time_counting * 100.0 / (time_counting + total_dispatcher_time));
    runTaskInfor(sorted_task_list);
}

bool comesBefore(char*a, char *b){
    return strcmp(a, b) < 0;
}

void add(char* name, int priority, int burst){
    Task *task = createTask(name, priority, burst);
    insert(&task_list, task);
}

void schedule(){
    fcfs_scheduler();
}

Task *pickNextTask(){
    if(task_list == NULL) return NULL;
    Node *cur = task_list;
    Task *best_sofar = cur->task; //best_sofar refers to the best Task (task with the smallest name order)
    while(cur != NULL){
        if(comesBefore(cur->task->name, best_sofar->name)){
            best_sofar = cur->task; 
        }
        
        cur = cur->next;
    }

    return best_sofar;
}

