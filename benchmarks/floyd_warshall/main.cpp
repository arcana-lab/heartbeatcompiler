#include "bench.hpp"
#if defined(COLLECT_KERNEL_TIME)
#include <stdio.h>
#include <chrono>
#endif

extern int vertices;
extern int *dist;

int main() {
  setup();

#if defined(COLLECT_KERNEL_TIME)
  using clock = std::chrono::system_clock;
  using sec = std::chrono::duration<double>;
  const auto before = clock::now();
#endif

#if defined(USE_BASELINE)
  floyd_warshall_serial(dist, vertices);
#elif defined(USE_OPENCILK)
  floyd_warshall_opencilk(dist, vertices);
#elif defined(USE_OPENMP)
  floyd_warshall_openmp(dist, vertices);
#endif

#if defined(COLLECT_KERNEL_TIME)
  const sec duration = clock::now() - before;
  printf("\"kernel_exectime\":  %.2f\n", duration.count());
#endif

#if defined(TEST_CORRECTNESS)
  test_correctness(dist);
#endif

  finishup();

  return 0;
}
