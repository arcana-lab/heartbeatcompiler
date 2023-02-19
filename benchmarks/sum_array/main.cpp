#include "sum_array.hpp"
#include "stdio.h"
#if defined(COLLECT_KERNEL_TIME)
#include <chrono>
#endif

extern uint64_t n;
extern double result;
extern double *a;

int main() {
  setup();

#if defined(COLLECT_KERNEL_TIME)
  using clock = std::chrono::system_clock;
  using sec = std::chrono::duration<double>;
  const auto before = clock::now();
#endif

#if defined(USE_BASELINE)
  result = sum_array_serial(a, 0, n);
#elif defined(USE_OPENCILK)
  result = sum_array_opencilk(a, 0, n);
#elif defined(USE_OPENMP)
  result = sum_array_openmp(a, 0, n);
#endif

#if defined(COLLECT_KERNEL_TIME)
  const sec duration = clock::now() - before;
  printf("\"kernel_exectime\":  %.2f\n", duration.count());
#endif

  finishup();
  
  printf("result=%f\n", result);
  return 0;
}
