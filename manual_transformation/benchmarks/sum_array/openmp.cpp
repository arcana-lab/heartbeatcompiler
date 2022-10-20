#include "sum_array.hpp"
#include "stdio.h"

using namespace sum_array;

int main() {

  setup();

#if defined(USE_OPENMP)
  result = sum_array_openmp(a, 0, n);
#endif

  finishup();
  
  printf("result=%f\n",result);
  return 0;
}
