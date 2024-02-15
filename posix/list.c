/**
 * Various list operations
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "list.h"
#include "task.h"

// add a new task to the list of tasks
void insert(Node **head, Task *newTask) {
    // add the new task to the list 
    Node *newNode = malloc(sizeof(struct node));

    newNode->task = newTask;
    newNode->next = *head;
    *head = newNode;
    //insert to head
}

// delete the selected task from the list
void delete(Node **head, Task *task) {
    Node *temp;
    Node *prev;

    temp = *head;
    // special case - beginning of list
    if (strcmp(task->name,temp->task->name) == 0) {
        *head = (*head)->next;
    }
    else {
        // interior or last element in the list
        prev = *head;
        temp = temp->next;
        while (strcmp(task->name,temp->task->name) != 0) {
            prev = temp;
            temp = temp->next;
        }

        prev->next = temp->next;
    }
}

// traverse the list
void traverse(Node *head) {
    Node *temp;
    temp = head;

    while (temp != NULL) {
        printf("[%s] [%d] [%d]\n",temp->task->name, temp->task->priority, temp->task->burst);
        temp = temp->next;
    }
}

Node *createNode(Task *task){
    Node *newNode = malloc(sizeof(Node));
    newNode->task = task;
    newNode->next = NULL;
    return newNode;
}


Task *createTask(char *name, int priority, int burst){
    Task *newTask = malloc(sizeof(Task));
    newTask->name = name;
    newTask->priority = priority;
    newTask->burst = burst;
    newTask->total_burst = burst;
    newTask->arrival_time = 0;
    newTask->complete_time = -1;
    newTask->start_time = -1;
    newTask->cur_turn = 0;
    return newTask;
}

void sortList(Node *head){
    //sort all tasks by the lexicographical order (increasing order)
    //using bubble sort
    bool check;
    do{
        Node *cur = head;
        check = false;
        while(cur != NULL){
            if(cur->next != NULL){ //there are nodes after cur
                if(cmp(cur->task->name, cur->next->task->name) == false){
                    //swap 2 nodes (just swap the tasks)
                    Task * temp = cur->task;
                    cur->task = cur->next->task;
                    cur->next->task = temp;
                    check = true;
                }
            }

            cur = cur->next;
        }
    }while(check == true);
}

void copyList(Node **head, Node *data_list){
    Node *cur = data_list;
    while(cur != NULL){
        insert(head, cur->task);
        cur = cur->next;
    }
}
bool cmp(char *a, char *b)
{
    return strcmp(a, b) < 0;
}

void runTaskInfor(Node *head){
    printf("%-3s|", "...");
    Node* cur = head;
    while(cur != NULL){
        printf(" %-4s|", cur->task->name);
        cur = cur->next;
    }
    printf("\n%-3s|", "TAT");
    cur = head;
    while(cur != NULL){
        printf(" %-4d|", cur->task->complete_time - cur->task->arrival_time);
        cur = cur->next;
    }

    printf("\n%-3s|", "WT");
    cur = head;
    while(cur != NULL){
        printf(" %-4d|", cur->task->complete_time - cur->task->arrival_time - cur->task->total_burst);
        cur = cur->next;
    }

    printf("\n%-3s|", "RT");
    cur = head;
    while(cur != NULL){
        printf(" %-4d|", cur->task->start_time - cur->task->arrival_time);
        cur = cur->next;
    }

    printf("\n");
}


