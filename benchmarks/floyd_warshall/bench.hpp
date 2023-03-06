#pragma once

#include <functional>

namespace floyd_warshall {

extern int vertices;
extern int *dist;

#if !defined(USE_HB_MANUAL) && !defined(USE_HB_COMPILER)
void run_bench(std::function<void()> const &bench_body,
               std::function<void()> const &bench_start,
               std::function<void()> const &bench_end);
#endif
void setup();
void finishup();

#if defined(USE_BASELINE)
  void floyd_warshall_serial(int* dist, int vertices);
#elif defined(USE_OPENCILK)
  void floyd_warshall_opencilk(int* dist, int vertices);
#elif defined(USE_OPENMP)
  void floyd_warshall_openmp(int* dist, int vertices);
#endif

#if defined(TEST_CORRECTNESS)
void test_correctness();
#endif

} // namespace floyd_warshall
