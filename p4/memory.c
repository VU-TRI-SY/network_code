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

    if(algorithm == 'F' || algorithm == 'f') start_index = first_fit(num_byte);
    else if(algorithm == 'B' || algorithm == 'b') start_index = best_fit(num_byte);
    else if(algorithm == 'W' || algorithm == 'w') start_index = worst_fit(num_byte);

    if(start_index < 0) {
        printf("Cannot allocate %d bytes for the process %c\n", num_byte, process);
        return;
    }

    while(num_byte > 0){
        mem[start_index] = process;
        num_byte--;
        start_index++;
    }
}

void Free(char process){
    for(int i = 0; i < MEMSIZE; i++){
        if(mem[i] == process) mem[i] = '.';
    }
}

void Show(){
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
        bool break_loop = false;
        fgets(str, 255, fptr);
        if(strlen(str) == 0) break;

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

}

void Exit(){
    exit(0);
}

int main(int argc, char** argv){
    ResetMem();
    if(argc == 1){
        Read(NULL);
    }else if(argc == 3){
        if((strcmp(argv[1], "R") == 0 || strcmp(argv[1], "r") == 0)){
            Read(argv[2]);
        }
    }
}
