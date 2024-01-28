// Sudoku puzzle verifier and solver

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/*
bool checkRows(int grid[][], int n){

  return true/false;
}

bool checkCols(int grid[][], int n){

  return true/false;
}

bool checkSubGrids(int grid[][], int n){

  return true/false;
}
*/

typedef struct Param{
  int **grid; //2-d array: matrix
  int *n; //size of matrix (n = num_rows = num_cols)
} Param;

void *checkRows(void *arg){ //arg: pointer refer to a Param that holds matrix and size of it
  Param *data = (Param *) arg; //casting to Param *
  bool *res = malloc(sizeof(bool));
  int n = *(data->n); //data->n: get the address of an integer that stores size of matrix
  int **grid = data->grid; //get matrix
  int vst[n+1]; //vst[x] = 1: number x is stored on the row
  //each row has n numbers (1->n)
  for(int i = 1; i <= n; i++){
    for(int x = 1; x <= n; x++) vst[x] = 0; //set all numbers on a row i to 0 
    for(int j = 1; j <= n; j++){ //traverse all numbers on row i
      int val = grid[i][j]; 

      if(vst[val] == 1) { //the  number val already exists on row i
        *res = false; //duplicate number val
        return res;
      }else vst[val] = 1; //mark val is visited
    }
  }

  *res = true;
  return res;
}
// takes puzzle size and grid[][] representing sudoku puzzle
// and tow booleans to be assigned: complete and valid.
// row-0 and column-0 is ignored for convenience, so a 9x9 puzzle
// has grid[1][1] as the top-left element and grid[9]9] as bottom right
// A puzzle is complete if it can be completed with no 0s in it
// If complete, a puzzle is valid if all rows/columns/boxes have numbers from 1
// to psize For incomplete puzzles, we cannot say anything about validity
void checkPuzzle(int psize, int **grid, bool *complete, bool *valid) {
  // YOUR CODE GOES HERE and in HELPER FUNCTIONS
  *valid = true;
  *complete = true;
}

// takes filename and pointer to grid[][]
// returns size of Sudoku puzzle and fills grid
int readSudokuPuzzle(char *filename, int ***grid) {
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    printf("Could not open file %s\n", filename);
    exit(EXIT_FAILURE);
  }
  int psize;
  fscanf(fp, "%d", &psize);
  int **agrid = (int **)malloc((psize + 1) * sizeof(int *));
  for (int row = 1; row <= psize; row++) {
    agrid[row] = (int *)malloc((psize + 1) * sizeof(int));
    for (int col = 1; col <= psize; col++) {
      fscanf(fp, "%d", &agrid[row][col]);
    }
  }
  fclose(fp);
  *grid = agrid;
  return psize;
}

// takes puzzle size and grid[][]
// prints the puzzle
void printSudokuPuzzle(int psize, int **grid) {
  printf("%d\n", psize);
  for (int row = 1; row <= psize; row++) {
    for (int col = 1; col <= psize; col++) {
      printf("%d ", grid[row][col]);
    }
    printf("\n");
  }
  printf("\n");
}

// takes puzzle size and grid[][]
// frees the memory allocated
void deleteSudokuPuzzle(int psize, int **grid) {
  for (int row = 1; row <= psize; row++) {
    free(grid[row]);
  }
  free(grid);
}

// expects file name of the puzzle as argument in command line
int main(int argc, char **argv) {
  if (argc != 2) {
    printf("usage: ./sudoku puzzle.txt\n");
    return EXIT_FAILURE;
  }
  // grid is a 2D array
  int **grid = NULL;
  // find grid size and fill grid
  int sudokuSize = readSudokuPuzzle(argv[1], &grid);
  bool valid = false;
  bool complete = false;
  checkPuzzle(sudokuSize, grid, &complete, &valid);
  printf("Complete puzzle? ");
  printf(complete ? "true\n" : "false\n");
  if (complete) {
    printf("Valid puzzle? ");
    printf(valid ? "true\n" : "false\n");
  }
  printSudokuPuzzle(sudokuSize, grid);
  deleteSudokuPuzzle(sudokuSize, grid);
  return EXIT_SUCCESS;
}
