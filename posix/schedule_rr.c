#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "schedule_rr.h"

Node *task_list = NULL;
Node *sorted_task_list = NULL;
// this list will contains all tasks that sorted in running order(based on lexicographical order)


void rr_scheduler(){
    int time_counting = 0;
    int total_dispatcher_time = 0;
    copyList(&sorted_task_list, task_list);
    sortList(sorted_task_list);
    // Node *cur = running_order_list;
    while(1){
        Task* t = pickNextTask(); //select the best task
        if(t == NULL) break;
        if(t->cur_turn == 0) t->start_time = time_counting; //t->cur_turn == 0 means this is the first time that t is run
        int slice = QUANTUM;
        if(t->burst < QUANTUM){ //the remaining time of t < 10 --> just run t by remaining time
            slice = t->burst;
        }
        run(t, slice); //run this task
        t->cur_turn++; //update cur_turn
        time_counting += slice; //update time counting after run this task
        t->burst -= slice; 
        if(t->burst == 0) { //complete running of task t
            t->complete_time = time_counting; //update complete time = time_counting
            delete(&task_list, t); //remove completed task
        }
        printf("        Time is now: %d\n", time_counting);
        
        
        //move to next task
        if(pickNextTask()){ //there are tasks to switch to
            total_dispatcher_time++;
        }

    }

    printf("CPU Utilization: %.2f%%\n",  time_counting * 100.0 / (time_counting + total_dispatcher_time));
    runTaskInfor(sorted_task_list);
}

bool comesBefore(Task* t1, Task* t2){
    if(t1->cur_turn != t2->cur_turn) return t1->cur_turn < t2->cur_turn;
    return strcmp(t1->name, t2->name) < 0;
}

void add(char* name, int priority, int burst){
    Task *task = createTask(name, priority, burst);
    insert(&task_list, task);
}

void schedule(){
    rr_scheduler();
}

Task *pickNextTask(){
    if(task_list == NULL) return NULL;
    Node *cur = task_list;
    Task *best_sofar = NULL;
    while(cur != NULL){
        if(cur->task->burst > 0 && best_sofar == NULL) {
            best_sofar = cur->task;
            cur = cur->next;
            continue;
        }

        if(comesBefore(cur->task, best_sofar)){
            best_sofar = cur->task;
        }
        
        cur = cur->next;
    }

    return best_sofar;
}

