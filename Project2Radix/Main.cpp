#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <algorithm>

#include "LSDRadix.h"
#include "MultiCoreRadix.h"

void fillArrayWithRandom(int * array, int size, int low, int high, int seed);
void printArray(int* array, int arrSize);

int main(int argc, char **argv)
{
  int arrSize = 20;
  int* array = new int[arrSize];  
  fillArrayWithRandom(array, arrSize, pow(2, 24), pow(2, 31), 555);

  printf("Input array:\n");
  printArray(array, arrSize);

  /*
  LSDRadix lsd = LSDRadix();
  int* sortedArray = lsd.sort(array, arrSize);
  */
  MultiCoreRadix mcr = MultiCoreRadix();
  mcr.setup(arrSize);
  int* sortedArray = mcr.sort(array, arrSize);

  printf("Sorted array:\n");
  printArray(sortedArray, arrSize);



  return 0;
}

void fillArrayWithRandom(int * array, int size, int low, int high, int seed)
{
  srand(seed + (unsigned) time(0));
    for(int i=0; i < size; i++){ 
        array[i] = rand() % ( (high+1) - low ) + low;
  } 
}

void printArray(int* array, int arrSize) 
{
  printf("[");  
  for (int i = 0; i < arrSize; i++) {
    printf(" %d ", array[i]);
    if (i != arrSize - 1) {
      printf("|");
    }  
  }
  printf("]\n");
}
