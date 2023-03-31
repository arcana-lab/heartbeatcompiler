#include "bench.hpp"
#include "utility.hpp"
#include <cstdint>
#include <cstdlib>
#if !defined(USE_HB_MANUAL) && !defined(USE_HB_COMPILER)
#include <functional>
#include <taskparts/benchmark.hpp>
#endif

namespace plus_reduce_array {

#if defined(INPUT_BENCHMARKING)
  uint64_t nb_items = 100 * 1000 * 1000;
#elif defined(INPUT_TPAL)
  uint64_t nb_items = 100 * 1000 * 1000;
#elif defined(INPUT_TESTING)
  uint64_t nb_items = 100 * 100 * 100;
#else
  #error "Need to select input size: INPUT_{BENCHMARKING, TPAL, TESTING}"
#endif
double *a;
double result = 0.0;

#if !defined(USE_HB_MANUAL) && !defined(USE_HB_COMPILER)
void run_bench(std::function<void()> const &bench_body,
               std::function<void()> const &bench_start,
               std::function<void()> const &bench_end) {
#if defined(USE_BASELINE)
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    bench_body();
  }, [&] (auto sched) {
    bench_start();
  }, [&] (auto sched) {
    bench_end();
  });
#else
  utility::run([&] {
    bench_body();
  }, [&] {
    bench_start();
  }, [&] {
    bench_end();
  });
#endif
}
#endif

void setup() {
  a = (double*)malloc(sizeof(double)*nb_items);
  for (uint64_t i = 0; i < nb_items; i++) {
    a[i] = 1.0;
  }
}

void finishup() {
  free(a);
}

#if defined(USE_BASELINE) || defined(TEST_CORRECTNESS)

double plus_reduce_array_serial(double* a, uint64_t lo, uint64_t hi) {
  double r = 0.0;
  for (uint64_t i = lo; i != hi; i++) {
    r += a[i];
  }
  return r;
}

#endif

#if defined(USE_OPENCILK)

#include <cilk/cilk.h>

void zero_double(void *view) {
  *(double *)view = 0.0;
}

void add_double(void *left, void *right) {
  *(double *)left += *(double *)right;
}

double plus_reduce_array_opencilk(double* a, uint64_t lo, uint64_t hi) {
  double cilk_reducer(zero_double, add_double) r;
  for (uint64_t i = lo; i != hi; i++) {
    r += a[i];
  }
  return r;
}

#elif defined(USE_OPENMP)

#include <omp.h>

double plus_reduce_array_openmp(double* a, uint64_t lo, uint64_t hi) {
  double r = 0.0;
  #pragma omp parallel for schedule(static) reduction(+:r)
  for (uint64_t i = lo; i != hi; i++) {
    r += a[i];
  }
  return r;
}

#endif

#if defined(TEST_CORRECTNESS)

#include <stdio.h>

void test_correctness() {
  double result_ref = plus_reduce_array_serial(a, 0, nb_items);
  double epsilon = 0.01;
  if (std::abs(result - result_ref) > epsilon) {
    printf("result = %.2f, result_ref = %.2f\n", result, result_ref);
    printf("\033[0;31mINCORRECT!\033[0m\n");
  } else {
    printf("\033[0;32mCORRECT!\033[0m\n");
  }
}

#endif

} // namespace plus_reduce_array
