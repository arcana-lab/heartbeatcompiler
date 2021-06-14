#include <assert.h>
#include <iostream>
#include "cmdline.hpp"
#include "mcsl_fjnative.hpp"

int mySum (int v[], int size){
  int t=0;

  for (int i=0; i < size; i++){
    t += v[i];
  }
  
  return t;
}

int64_t fib_fjnative(int64_t n) {
  if (n <= 1) {
    return n;
  } else {
    int64_t r1, r2;
    mcsl::fork2([&] {
      r1 = fib_fjnative(n-1);
    }, [&] {
      r2 = fib_fjnative(n-2);
    });
    return r1 + r2;
  }
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

  int64_t n = 30;
  int64_t dst = 0;

  mcsl::launch([&] {
  }, [&] {
    printf("result %ld\n", dst);
  }, [&] {
    dst = fib_fjnative(n);
  });

  return 0;
}
