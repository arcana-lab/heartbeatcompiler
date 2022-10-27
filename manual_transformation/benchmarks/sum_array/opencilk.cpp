#include "sum_array.hpp"
#include "stdio.h"
#if defined(COLLECT_KERNEL_TIME)
#include <chrono>
#endif

using namespace sum_array;

int main() {

  setup();

#if defined(COLLECT_KERNEL_TIME)
  using clock = std::chrono::system_clock;
  using sec = std::chrono::duration<double>;
  const auto before = clock::now();
#endif

#if defined(USE_OPENCILK)
  result = sum_array_opencilk(a, 0, n);
#endif

#if defined(COLLECT_KERNEL_TIME)
  const sec duration = clock::now() - before;
  printf("\"kernel_exectime\":  %.2f\n", duration.count());
#endif

  finishup();
  
  printf("result=%f\n",result);
  return 0;
}
