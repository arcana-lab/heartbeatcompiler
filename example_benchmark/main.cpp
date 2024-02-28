#include <stdio.h>
#include <stdint.h>

#if defined(USE_BASELINE)
#include <functional>
#include <taskparts/benchmark.hpp>
#elif defined(USE_OPENMP)
#include <omp.h>
#include "utility.hpp"
#elif defined(USE_HB_COMPILER)
#include "promotion_handler.hpp"
#endif

bool run_heartbeat = true;

int64_t HEARTBEAT_loop(int64_t n) {
  int64_t sum = 0;
  for (int64_t i = 0; i < n; i++) {
    if (i % 2 ==0) sum += i;
    else sum += 2 * i;
  }
  return sum;
}

void heartbeat_region(int64_t n, int64_t &sum) {
  sum = HEARTBEAT_loop(n);
}

int main () {
  int64_t sum = 0;
  int64_t n = 1000000000;

#if defined(USE_BASELINE)
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
#elif defined(USE_OPENMP)
  utility::run([&] {
#elif defined(USE_HB_COMPILER)
  run_bench([&] {
#endif

#if defined(USE_BASELINE)
  for (int64_t i = 0; i < n; i++) {
    if (i % 2 ==0) sum += i;
    else sum += 2 * i;
  }

#elif defined(USE_OPENMP)
#if defined(OMP_SCHEDULE_STATIC)
  #pragma omp parallel for schedule(static) reduction(+:sum)
#elif defined(OMP_SCHEDULE_DYNAMIC)
  #pragma omp parallel for schedule(dynamic) reduction(+:sum)
#endif
  for (int64_t i = 0; i < n; i++) {
    if (i % 2 ==0) sum += i;
    else sum += 2 * i;
  }

#elif defined(USE_HB_COMPILER)
  heartbeat_region(n, sum);
#endif

#if defined(USE_BASELINE)
  }, [&] (auto sched) {
  }, [&] (auto sched) {
  });
#else
  }, [&] {}, [&] {});
#endif

  printf("sum = %ld\n", sum);
  return 0;
}