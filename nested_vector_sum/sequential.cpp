#include <assert.h>
#include <iostream>

int mySum (int v[], int size){
  int t=0;

  for (int i=0; i < size; i++){
    t += v[i];
  }
  
  return t;
}

int myOutermostSum (int **m, int mSize){

  /*
   * Allocate the main result
   */
  int totalT = 0;

  for (int j=0; j < mSize; j++){
    auto currentV = m[j];
    totalT += mySum(currentV, mSize);
    std::cout << "At iteration " << j << ": " << totalT << std::endl;
  }

  return totalT;
}

int main (int argc, char *argv[]){

  /*
   * Fetch the inputs
   */
  if (argc <= 1){
    std::cerr << "USAGE: " << argv[0] << " MATRIX_SIZE" << std::endl;
    return 1;
  }
  auto MSIZE = atoll(argv[1]);
  std::cout << "Matrix size = " << MSIZE << std::endl;

  /*
   * Allocate
   */
  auto m = new int*[MSIZE];
  for (auto i=0; i < MSIZE; i++){
    m[i] = new int[MSIZE];
  }

  /*
   * Initialize
   */
  for (auto i=0; i < MSIZE; i++){
    for (auto j=0; j < MSIZE; j++){
      m[i][j] = i * j;
    }
  }

  /*
   * Run
   */
  auto r = myOutermostSum(m, MSIZE);

  /*
   * Print the result
   */
  std::cout << "The result is " << r << std::endl;

  return 0;
}
