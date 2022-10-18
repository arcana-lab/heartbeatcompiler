#include "sum_array.hpp"
#include "stdio.h"

using namespace sum_array;

int main() {

  setup();

  result = sum_array_serial(a, 0, n);

  finishup();
  
  printf("result=%f\n",result);
  return 0;
}
