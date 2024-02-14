#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "schedule_priority_rr.h"

Node *task_list = NULL;
Node *sorted_task_list = NULL;
Node* priority_arr[11];
// this list will contains all tasks that sorted in running order(based on lexicographical order)

void priority_rr_scheduler(){
    int time_counting = 0;
    int total_dispatcher_time = 0;
    copyList(&sorted_task_list, task_list);
    sortList(sorted_task_list);
    for(int i = 1; i <= 10; i++) priority_arr[i] = NULL;
    Node * cur = task_list;
    while(cur != NULL){
        insert(&priority_arr[cur->task->priority], cur->task);
        cur = cur->next;
    }

    int i = 10;
    while(i >= 1){
        if(priority_arr[i] != NULL) {
            sortList(priority_arr[i]);
            cur = priority_arr[i];
            if(cur->next != NULL){ //there are more than 1 task with the same priority
                //let's schedule by roud robin
                while(true){
                    cur = priority_arr[i];
                    bool hasTaskToRun = false;
                    while(cur != NULL){
                        Task *t = cur->task;
                        if(t->burst > 0){
                            hasTaskToRun = true;
                            if(t->cur_turn == 0) t->start_time = time_counting;
                            int slice = QUANTUM;
                            if(t->burst < QUANTUM) slice = t->burst;
                            run(t, slice);
                            t->cur_turn++;
                            time_counting += slice;
                            printf("        Time is now: %d\n", time_counting);
                            t->burst -= slice;
                            if(t->burst == 0) {
                                t->complete_time = time_counting;
                            }

                            Node *temp = cur->next;
                            while(temp != NULL && temp->task->burst == 0) temp = temp->next;
                            if(temp){
                                total_dispatcher_time++; //context switch
                            }
                            cur = temp;

                        }else cur = cur->next;
                    }

                    if(!hasTaskToRun) break;
                }

                
            }else{
                Task *t = cur->task;
                t->start_time = time_counting;
                run(t, t->burst);
                time_counting += t->burst;
                printf("        Time is now: %d\n", time_counting);
                t->complete_time = time_counting;

            }

            int j = i-1;
            for(; j >= 1; j--){
                if(priority_arr[j] != NULL){
                    i = j;
                    total_dispatcher_time++;
                    break;
                }
            }
            i = j;
        }else{
            i--;
        }

    }
    

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
    priority_rr_scheduler();
}

Task *pickNextTask(){
    return NULL;
}

