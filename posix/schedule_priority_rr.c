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
        int prio = cur->task->priority;
        //arr[prio] is array list of tasks that have priority 'prio'
        insert(&priority_arr[prio], cur->task);
        cur = cur->next;
    }

    int i = 10;
    while(i >= 1){
        if(priority_arr[i] != NULL) {
            sortList(priority_arr[i]); //sort tasks by running order (alphabet order) 
            cur = priority_arr[i]; //cur runs from header
            if(cur->next != NULL){ //there are more than 1 task with the same priority 'i'
                //let's schedule by round robin
                while(true){
                    cur = priority_arr[i]; //cur is head of this linked list
                    bool hasTaskToRun = false; 
                    //hasTaskToRun is a flag representing that there are other tasks to run next in the current list
                    //purpose: to apply RR to next task in the current list
                    while(cur != NULL){ //this loop is used for partition resource for needed task
                        Task *t = cur->task;
                        if(t->burst > 0){ //t still needs more burst time
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

                            //now, we must check that after current task 't' that has more tasks to run by RR or not
                            Node *temp = cur->next; //starting from next task of 't'
                            while(temp != NULL && temp->task->burst == 0) temp = temp->next;
                            //after while, temp will be NULL or refers to a Node that contains task with >0 remaining burst time
                            if(temp){ //temp != NULL --> this task must be run by RR
                                total_dispatcher_time++; //context switch
                            }
                            cur = temp; //cur = temp: switch the task that referred by temp to run

                        }else { //current task no needs to run
                            //skip this task by moving cur forward
                            cur = cur->next;
                        }
                    }

                    if(!hasTaskToRun) break; //there is no task to run by RR --> stop
                }

                
            }else{ //only one task with priority 'i'
                Task *t = cur->task;
                t->start_time = time_counting;
                run(t, t->burst);
                time_counting += t->burst;
                printf("        Time is now: %d\n", time_counting);
                t->complete_time = time_counting;

            }


            //check the next task to run
            //if has no task --> stop
            //if has more tasks ---> update dispatcher_time to switch
            int j = i-1;
            for(; j >= 1; j--){
                if(priority_arr[j] != NULL){ //there are more than 1 task that need to run , this task has priority = 'j'
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

