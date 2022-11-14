#include "sum_array_nested.hpp"
#include "stdio.h"
#if defined(COLLECT_KERNEL_TIME)
#include <chrono>
#endif

using namespace sum_array_nested;

int main() {
  setup();

#if defined(COLLECT_KERNEL_TIME)
  using clock = std::chrono::system_clock;
  using sec = std::chrono::duration<double>;
  const auto before = clock::now();
#endif

  result = sum_array_nested_serial(a, 0, n1, 0, n2);

#if defined(COLLECT_KERNEL_TIME)
  const sec duration = clock::now() - before;
  printf("\"kernel_exectime\":  %.2f\n", duration.count());
#endif

  finishup();
  
  printf("result=%lu\n", result);
  return 0;
}
