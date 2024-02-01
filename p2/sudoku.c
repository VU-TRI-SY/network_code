// Sudoku puzzle verifier and solver

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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

typedef struct Param
{
  int **grid; // 2-d array: matrix
  int *n;     // size of matrix (n = num_rows = num_cols)
} Param;

void *checkRows(void *arg)
{                             // arg: pointer refer to a Param that holds matrix and size of it
  Param *data = (Param *)arg; // casting to Param *
  bool *res = malloc(sizeof(bool));
  int n = *(data->n);      // data->n: get the address of an integer that stores size of matrix
  int **grid = data->grid; // get matrix
  int vst[n + 1];          // vst[x] = 1: number x is stored on the row
  // each row has n numbers (1->n)
  for (int i = 1; i <= n; i++)
  { // traverse through each row
    for (int x = 1; x <= n; x++)
      vst[x] = 0; // set all numbers on a row i to 0

    for (int j = 1; j <= n; j++)
    { // traverse all numbers on row i
      int val = grid[i][j];

      if (vst[val] == 1)
      {               // the  number val already exists on row i
        *res = false; // duplicate number val
        return (void *)res;
      }
      else
        vst[val] = 1; // mark val is visited
    }
  }

  *res = true;
  return (void *)res;
}

void *checkCols(void *arg)
{
  Param *data = (Param *)arg;
  bool *res = malloc(sizeof(bool));
  int n = *(data->n);
  int **grid = data->grid;
  int vst[n + 1]; // vst[x] = 1/0 ~ visited/unvisted

  for (int j = 1; j <= n; j++)
  { // go through all cols
    for (int x = 1; x <= n; x++)
      vst[x] = 0; // reset all numbers at col j to 0=unvisited

    for (int i = 1; i <= n; i++)
    {
      int val = grid[i][j];

      if (vst[val] == 1)
      {
        *res = false;
        return (void *)res;
      }
    }
  }

  *res = true;
  return (void *)res;
}

void *checkSubgrids(void *arg)
{
  Param *data = (Param *)arg;
  bool *res = malloc(sizeof(bool));
  int n = *(data->n);
  int subgrid_size = sqrt(n);
  int **grid = data->grid;
  int vst[n + 1]; // vst[x] = 1/0 ~ visited/unvisted for each subgrid

  for (int i = 1; i <= n; i += subgrid_size)
  {
    // i is the row index of point at the top left of this subgrid
    // j is the col index of top left point of this subgrid
    for (int j = 1; j <= n; j += subgrid_size)
    {
      // point with coordinate (i, j) is the top left of this subgrid
      // from that point, expand to subgrid that has size subgrid_size * subgrid_size

      // let's traverse through all elements of this subgrid
      //  the index of row and col of each element on this subgrid will be started at row index i and col index j

      // before traversing, we must reset all numbers of this subgrid to unvisited~0
      for (int x = 1; x <= n; x++)
        vst[x] = 0;
      for (int r = i; r <= subgrid_size + i - 1; r++)
      {
        for (int c = j; c <= subgrid_size + j - 1; c++)
        {
          int val = grid[r][c];

          if (vst[val] == 1)
          {
            *res = false;
            return (void *)res;
          }
        }
      }
    }
  }

  *res = true;
  return (void *)res;
}

bool solveSudoku(Param *arg,int i ,int j)
{
  Param *data = (Param* ) arg;
  int n = *(data->n);
  int **grid = data->grid;
  // i and j are the indexes of empty position that we are trying to find the number [1,.9] and fill into

  if(i > n || j > n){ //i and j are not valid
    //call functions to check rows, cols and subgrids
    //create 3 threads
    // pthread_attr_t attr;

    // //declare 3 threads
    // pthread_t tid1;
    // pthread_t tid2;
    // pthread_t tid3;

    // //assign default values for attr
    // pthread_attr_init(&attr);

    // //create 3 threads
    // pthread_create(&tid1, &attr, checkRows, (void *)arg);
    // pthread_create(&tid1, &attr, checkCols, (void *)arg);
    // pthread_create(&tid1, &attr, checkSubgrids, (void *)arg);

    // //3 variables to store returned values
    // bool *t1_result;
    // bool *t2_result;
    // bool *t3_result;

    // //wait until 3 functions end
    // pthread_join(tid1, (void **)&t1_result);
    // pthread_join(tid2, (void **)&t2_result);
    // pthread_join(tid3, (void **)&t3_result);

    // if(*t1_result && *t2_result && *t3_result) {
    //   free(t1_result);
    //   free(t2_result);
    //   free(t3_result);
    //   return true;
    // }else{
    //   rree(t1_result);
    //   free(t2_result);
    //   free(t3_result);
    //   return false;
    // }

    bool r1 = checkRows(arg);
    bool r2 = checkCols(arg);
    bool r3 = checkSubgrids(arg);
    // return r1 && r2 && r3;
    return true;
  } else{
    if(grid[i][j] == 0){
      for(int num = 1; num <= 9; num++){
        grid[i][j] = num; //make decison by filling num into grid[i][j]
        //try to fill other cells by calling solveSudoku();
        int r = i;
        int c = j;
        if(c + 1 > n){
          //jump to next row;
          r++;
          c = 1;
        }else{
          c++;
        }

        /*
        num is the current decision we are trying to make for gird[i][j]
        if this decision leads a valid matrix, the true value will be returned
        */
        if(solveSudoku(arg, r, c)){
          return true;
        }
      }
    }else{
      int r = i;
      int c = j;
      if(c + 1 > n){
        //jump to next row;
        r++;
        c = 1;
      }else{
        c++;
      }
      if(solveSudoku(arg, r, c)){
        return true;
      }
    }

    return false;
  }

}

// takes puzzle size and grid[][] representing sudoku puzzle
// and tow booleans to be assigned: complete and valid.
// row-0 and column-0 is ignored for convenience, so a 9x9 puzzle
// has grid[1][1] as the top-left element and grid[9]9] as bottom right
// A puzzle is complete if it can be completed with no 0s in it
// If complete, a puzzle is valid if all rows/columns/boxes have numbers from 1
// to psize For incomplete puzzles, we cannot say anything about validity
void checkPuzzle(int psize, int **grid, bool *complete, bool *valid)
{
  // YOUR CODE GOES HERE and in HELPER FUNCTIONS
  //call sudoku
  *valid = true;
  *complete = true;
}

// takes filename and pointer to grid[][]
// returns size of Sudoku puzzle and fills grid
int readSudokuPuzzle(char *filename, int ***grid)
{
  FILE *fp = fopen(filename, "r");
  if (fp == NULL)
  {
    printf("Could not open file %s\n", filename);
    exit(EXIT_FAILURE);
  }
  int psize;
  fscanf(fp, "%d", &psize);
  int **agrid = (int **)malloc((psize + 1) * sizeof(int *));
  for (int row = 1; row <= psize; row++)
  {
    agrid[row] = (int *)malloc((psize + 1) * sizeof(int));
    for (int col = 1; col <= psize; col++)
    {
      fscanf(fp, "%d", &agrid[row][col]);
    }
  }
  fclose(fp);
  *grid = agrid;
  return psize;
}

// takes puzzle size and grid[][]
// prints the puzzle
void printSudokuPuzzle(int psize, int **grid)
{
  printf("%d\n", psize);
  for (int row = 1; row <= psize; row++)
  {
    for (int col = 1; col <= psize; col++)
    {
      printf("%d ", grid[row][col]);
    }
    printf("\n");
  }
  printf("\n");
}

// takes puzzle size and grid[][]
// frees the memory allocated
void deleteSudokuPuzzle(int psize, int **grid)
{
  for (int row = 1; row <= psize; row++)
  {
    free(grid[row]);
  }
  free(grid);
}

// expects file name of the puzzle as argument in command line
int main(int argc, char **argv)
{
  if (argc != 2)
  {
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
  if (complete)
  {
    printf("Valid puzzle? ");
    printf(valid ? "true\n" : "false\n");
  }
  printSudokuPuzzle(sudokuSize, grid);
  deleteSudokuPuzzle(sudokuSize, grid);
  return EXIT_SUCCESS;
}
