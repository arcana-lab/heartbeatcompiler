#include "sum_array.hpp"
#include "stdio.h"

using namespace sum_array;

int main() {

  setup();

#if defined(USE_OPENCILK)
  result = sum_array_opencilk(a, 0, n);
#endif

  finishup();
  
  printf("result=%f\n",result);
  return 0;
}
