#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"

void ResetMem(){
    for(int i = 0; i < MEMSIZE; i++) mem[i] = '.';
}

int best_fit(int num_byte){
    int res = -1; //store the starting index of space that satifies condition
    int opt_length = -1; //length of the smallest satisfying space

    int i = 0;
    while(i < MEMSIZE){
        if(mem[i] == '.'){ //i is the starting index of empty space --> measure the length of that space 
            int j = i;
            while(j < MEMSIZE && mem[j] == '.') j++; //move j to the end of current space

            int space_length = j - i;

            if(space_length >= num_byte){ //current space has length >= num_byte
                if(res == -1){
                    res = i;
                    opt_length = space_length;
                }else{
                    if(opt_length > space_length){
                        res = i;
                        opt_length = space_length;
                        //update optimal length
                    }
                }
            }

            i = j;            
        }else
            i++;
        
    }

    return res;
}

int first_fit(int num_byte){
    int i = 0;
    while(i < MEMSIZE){
        if(mem[i] == '.'){ //i is the starting index of empty space --> measure the length of that space 
            int j = i;
            while(j < MEMSIZE && mem[j] == '.') j++; //move j to the end of current space

            int space_length = j - i;

            if(space_length >= num_byte) //current space has length >= num_byte
                return i;

            i = j; //try to find the next satified space            
        }else
            i++;
        
    }

    return -1; //not found
}

int worst_fit(int num_byte){
    int res = -1; //store the starting index of space that satifies condition
    int opt_length = -1; //length of the smallest satisfying space

    int i = 0;
    while(i < MEMSIZE){
        if(mem[i] == '.'){ //i is the starting index of empty space --> measure the length of that space 
            int j = i;
            while(j < MEMSIZE && mem[j] == '.') j++; //move j to the end of current space

            int space_length = j - i;

            if(space_length >= num_byte){ //current space has length >= num_byte
                if(res == -1){
                    res = i;
                    opt_length = space_length;
                }else{
                    if(opt_length < space_length){
                        res = i;
                        opt_length = space_length;
                        //update optimal length
                    }
                }
            }

            i = j;            
        }else
            i++;
        
    }

    return res;
}

void Allocate(char process, int num_byte, char algorithm){
    int start_index = -1;
    char alg[255];
    if(algorithm == 'F' || algorithm == 'f') {
        start_index = first_fit(num_byte);
        strcpy(alg, "First-Fit");
    }

    else if(algorithm == 'B' || algorithm == 'b') {
        start_index = best_fit(num_byte);
        strcpy(alg, "Best-Fit");
    }
    else if(algorithm == 'W' || algorithm == 'w') {
        start_index = worst_fit(num_byte);
        strcpy(alg, "Worst-Fit");
    }

    if(start_index < 0) {
        printf("Insufficient contigous memory to allocate process %c of size %d\n", process, num_byte);
        return;
    }

    printf("Allocate %d bytes for process %c using %s\n", num_byte, process, alg);

    while(num_byte > 0){
        mem[start_index] = process;
        num_byte--;
        start_index++;
    }
}

void Free(char process){
    int check = 0;
    for(int i = 0; i < MEMSIZE; i++){
        if(mem[i] == process) {
            mem[i] = '.';
            check = 1;
        }
    }

    if(check){
        printf("Free all allocations owned by process %c\n", process);
    }else{
        printf("Process %c does not exist in the memory\n", process);
    }
}

void Show(){
    printf("Show the state of the memory pool:\n");
    for(int i = 0; i < MEMSIZE; i++){
        printf("%c", mem[i]);
    }

    printf("\n");
}

void Read(char* file_name){
    FILE* fptr = NULL;

    if(file_name == NULL) fptr = stdin;
    else fptr = fopen(file_name, "r");

    if(!fptr) {
        printf("Cannot open file %s\n", file_name);
        return;
    }
    
    char str[255];
    while(!feof(fptr)){
        str[0] = '\0';
        bool break_loop = false;
        fgets(str, 255, fptr);
        if(strlen(str) == 0) break;
        if(str[0] == '\n' || str[0] == '\0') break;
        char *token;
        
        token = strtok(str, " ");
        char process;
        int num_byte;
        char algorithm;
        switch (token[0])
        {
        case 'A':
            token = strtok(NULL, " "); //take the next token
            process = token[0];
            token = strtok(NULL, " ");
            num_byte = atoi(token);
            token = strtok(NULL, " ");
            algorithm = token[0];

            Allocate(process, num_byte, algorithm);
            break;
        case 'F':
            token = strtok(NULL, " "); //take the next token
            process = token[0];
            Free(process);
            break;
        case 'S':
            Show();
            break;
        case 'R':
            token = strtok(NULL, " "); //take the file name
            token[strlen(token)-1] = '\0';
            printf("Read the script in the file called %s and execute each command.\n", token);
            Read(token); //navigate to read some command from a file 
            break;
        case 'C':
            Compact();
            break;
        case 'E':
            fclose(fptr); //close file before exit
            Exit();
            break;
        default:
            break_loop = true;
            break;
        }

        if(break_loop) break;
    }

    fclose(fptr);
}

void Compact(){
    printf("Compacting memory:\n");
    int i = 0;
    while(true){
        //move i to the head of empty space
        //then move j to the head of allocated mem
        //shift left that allocated mem pointed by j (swap mem[i], mem[j])
        //repeate 3 steps above 

        while(i < MEMSIZE && mem[i] != '.') i++;
        if(i >= MEMSIZE) break;
        int j = i;
        while(j < MEMSIZE && mem[j] == '.') j++;
        if(j >= MEMSIZE) break;
        //shift left this allocated memory
        while(j < MEMSIZE && mem[j] != '.'){
            char temp = mem[i];
            mem[i] = mem[j];
            mem[j] = temp;
            i++;
            j++;
        }
    }
}

void Exit(){
    printf("Exiting.\n");
    exit(0);
}

int main(){
    ResetMem(); //set empty mem by setting all chars to '.'
    printf("Enter command (A-allocate, F-free, S-show, R-readScript, C-compact, E-exit):\n");
    Read(NULL); //read command from stdin
}
