/**
 * list data structure containing the tasks in the system
 */
#ifndef LIST_H
#define LIST_H
#include "task.h"
#include <stdbool.h>

typedef struct node {
    Task *task;
    struct node *next;
} Node;

// typedef struct node Node;

// insert and delete operations.
void insert(Node **head, Task *task);
void delete(Node **head, Task *task);
void traverse(Node *head);
Node* createNode(Task *task);
Task *createTask(char *name, int priority, int burst);
void sortList(Node *head);
void copyList(Node **head, Node *data_list);
bool cmp(char *a, char *b);
void runTaskInfor(Node *head);
#endif