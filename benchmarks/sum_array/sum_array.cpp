#include "sum_array.hpp"
#include <cstdint>
#if defined(USE_OPENCILK)
#include <cilk/cilk.h>
#endif
#if defined(USE_OPENMP)
#include <omp.h>
#endif
#if defined(TEST_CORRECTNESS)
#include <cstdio>
#endif

#if defined(INPUT_BENCHMARKING)
  uint64_t n = 1000000000;
#elif defined(INPUT_TESTING)
  uint64_t n = 1000000000;
#else
  #error "Need to select input size, e.g., INPUT_{BENCHMARKING, TESTING}"
#endif
double result = 0.0;
double *a;

void setup() {
  a = new double[n];
  for (uint64_t i = 0; i < n; i++) {
    a[i] = 1.0;
  }
}

void finishup() {
  delete [] a;
}

#if defined(USE_OPENCILK)
void zero_double(void *view) {
  *(double *)view = 0.0;
}
void add_double(void *left, void *right) {
  *(double *)left += *(double *)right;
}
double sum_array_opencilk(double *a, uint64_t lo, uint64_t hi) {
  double cilk_reducer(zero_double, add_double) sum;
  cilk_for (uint64_t i = lo; i != hi; i++) {
    sum += a[i];
  }
  return sum;
}

#elif defined(USE_OPENMP)
double sum_array_openmp(double* a, uint64_t lo, uint64_t hi) {
  double r = 0;
#if defined(OMP_STATIC)
  #pragma omp parallel for schedule(static) reduction(+:r)
#elif defined(OMP_DYNAMIC)
  #pragma omp parallel for schedule(dynamic) reduction(+:r)
#elif defined(OMP_GUIDED)
  #pragma omp parallel for schedule(guided) reduction(+:r)
#else
  #error "Need to select OpenMP scheduler: STATIC, DYNAMIC or GUIDED"
#endif
  for (uint64_t i = lo; i != hi; i++) {
    r += a[i];
  }
  return r;
}

#endif

double sum_array_serial(double *a, uint64_t lo, uint64_t hi) {
  double r = 0;
  for (uint64_t i = lo; i != hi; i++) {
    r += a[i];
  }
  return r;
}
