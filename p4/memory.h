#include <stdbool.h>
#define MEMSIZE 80

char mem[MEMSIZE];

//find the starting index of space that satifies the condition of algorithm
int best_fit(int num_byte);

int first_fit(int num_byte);

int worst_fit(int num_byte);

void Allocate(char process, int num_byte, char algorithm);

void Free(char process);

void Show();

void Read(char* file_name);

void Compact();

void Exit();

void ResetMem();