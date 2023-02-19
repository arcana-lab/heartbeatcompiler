#include "sum_array_nested.hpp"
#include "stdio.h"
#if defined(COLLECT_KERNEL_TIME)
#include <chrono>
#endif

extern uint64_t n1;
extern uint64_t n2;
extern uint64_t result;
extern char **a;

int main() {
  setup();

#if defined(COLLECT_KERNEL_TIME)
  using clock = std::chrono::system_clock;
  using sec = std::chrono::duration<double>;
  const auto before = clock::now();
#endif

#if defined(USE_BASELINE)
  result = sum_array_nested_serial(a, 0, n1, 0, n2);
#elif defined(USE_OPENCILK)
  result = sum_array_nested_opencilk(a, 0, n1, 0, n2);
#elif defined(USE_OPENMP)
  result = sum_array_nested_openmp(a, 0, n1, 0, n2);
#endif

#if defined(COLLECT_KERNEL_TIME)
  const sec duration = clock::now() - before;
  printf("\"kernel_exectime\":  %.2f\n", duration.count());
#endif

  finishup();
  
  printf("result=%lu\n", result);
  return 0;
}
