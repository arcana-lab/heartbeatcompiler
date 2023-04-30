#include "bench.hpp"
#include <cstdint>
#if !defined(USE_HB_MANUAL) && !defined(USE_HB_COMPILER)
#include "utility.hpp"
#include <functional>
#endif

namespace nested_plus_reduce_array {

#if defined(INPUT_BENCHMARKING)
  uint64_t nb_items1 = 10 * 1000 ;
  uint64_t nb_items2 = 10 * 1000 ;
#elif defined(INPUT_TESTING)
  uint64_t nb_items1 = 10 * 10 ;
  uint64_t nb_items2 = 10 * 10 ;
#else
  #error "Need to select input size: INPUT_{BENCHMARKING, TESTING}"
#endif
double **a;
double result = 0.0;

#if !defined(USE_HB_MANUAL) && !defined(USE_HB_COMPILER)
void run_bench(std::function<void()> const &bench_body,
               std::function<void()> const &bench_start,
               std::function<void()> const &bench_end) {
  utility::run([&] {
    bench_body();
  }, [&] {
    bench_start();
  }, [&] {
    bench_end();
  });
}
#endif

void setup() {
  a = (double **)malloc(sizeof(double *) * nb_items1);
  for (uint64_t i = 0; i < nb_items1; i++) {
    a[i] = (double *)malloc(sizeof(double) * nb_items1);
    for (uint64_t j = 0; j < nb_items1; j++) {
      a[i][j] = 1.0;
    }
  }
}

void finishup() {
  for (uint64_t i = 0; i < nb_items1; i++) {
    delete [] a[i];
  }
  delete [] a;
}

#if defined(USE_BASELINE) || defined(TEST_CORRECTNESS)

double nested_plus_reduce_array_serial(double** a, uint64_t lo1, uint64_t hi1, uint64_t lo2, uint64_t hi2) {
  double r = 0.0;
  for (uint64_t i = lo1; i != hi1; i++) {
    for (uint64_t j = lo2; j != hi2 ; j++) {
      r += a[i][j];
    }
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

double nested_plus_reduce_array_opencilk(double** a, uint64_t lo1, uint64_t hi1, uint64_t lo2, uint64_t hi2) {
  double cilk_reducer(zero_double, add_double) r;
  cilk_for (uint64_t i = lo1; i != hi1; i++) {
    cilk_for (uint64_t j = lo2; j != hi2 ; j++) {
      r += a[i][j];
    }
  }
  return r;
}

#elif defined(USE_OPENMP)

#include <omp.h>

double nested_plus_reduce_array_openmp(double** a, uint64_t lo1, uint64_t hi1, uint64_t lo2, uint64_t hi2) {
  double r = 0.0;
#if defined(OMP_SCHEDULE_STATIC)
  #pragma omp parallel for schedule(static) reduction(+:r)
#elif defined(OMP_SCHEDULE_DYNAMIC)
  #pragma omp parallel for schedule(dynamic) reduction(+:r)
#elif defined(OMP_SCHEDULE_GUIDED)
  #pragma omp parallel for schedule(guided) reduction(+:r)
#endif
  for (uint64_t i = lo1; i != hi1; i++) {
#if defined(OMP_NESTED_SCHEDULING)
#if defined(OMP_SCHEDULE_STATIC)
    #pragma omp parallel for schedule(static) reduction(+:r)
#elif defined(OMP_SCHEDULE_DYNAMIC)
    #pragma omp parallel for schedule(dynamic) reduction(+:r)
#elif defined(OMP_SCHEDULE_GUIDED)
    #pragma omp parallel for schedule(guided) reduction(+:r)
#endif
#endif
    for (uint64_t j = lo2; j != hi2 ; j++) {
      r += a[i][j];
    }
  }
  return r;
}

#endif

#if defined(TEST_CORRECTNESS)

#include <stdio.h>

void test_correctness() {
  double result_ref = nested_plus_reduce_array_serial(a, 0, nb_items1, 0, nb_items2);
  double epsilon = 0.01;
  if (std::abs(result - result_ref) > epsilon) {
    printf("result = %.2f, result_ref = %.2f\n", result, result_ref);
    printf("\033[0;31mINCORRECT!\033[0m\n");
  } else {
    printf("result = %.2f, result_ref = %.2f\n", result, result_ref);
    printf("\033[0;32mCORRECT!\033[0m\n");
  }
}

#endif

} // namespace nested_plus_reduce_array
