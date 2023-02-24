#include "bench.hpp"
#include <cstdint>
#if defined(USE_OPENCILK)
#include <cilk/cilk.h>
#endif
#if defined(USE_OPENMP)
#include <omp.h>
#endif

#if defined(INPUT_BENCHMARKING)
  uint64_t n1 = 400000;
  uint64_t n2 = 400000;
#elif defined(INPUT_TESTING)
  uint64_t n1 = 100000;
  uint64_t n2 = 100000;
#else
  #error "Need to select input size, e.g., INPUT_{BENCHMARKING, TESTING"
#endif
uint64_t result = 0;
char **a;

void setup() {
  a = new char*[n1];
  for (uint64_t i = 0; i < n1; i++) {
    a[i] = new char[n2];
    for (uint64_t j = 0; j < n2; j++) {
      a[i][j] = 1;
    }
  }
}

void finishup() {
  for (uint64_t i = 0; i < n1; i++) {
    delete [] a[i];
  }
  delete [] a;
}

#if defined(USE_OPENCILK)
void zero_uint64_t(void *view) {
  *(uint64_t *)view = 0;
}
void add_uint64_t(void *left, void *right) {
  *(uint64_t *)left += *(uint64_t *)right;
}
uint64_t sum_array_nested_opencilk(char **a, uint64_t lo1, uint64_t hi1, uint64_t lo2, uint64_t hi2) {
  uint64_t cilk_reducer(zero_uint64_t, add_uint64_t) sum;
  cilk_for (uint64_t i = lo1; i != hi1; i++) {
    for (uint64_t j = lo2; j != hi2; j++) {
      sum += a[i][j];
    }
  }
  return sum;
}

#elif defined(USE_OPENMP)
uint64_t sum_array_nested_openmp(char **a, uint64_t lo1, uint64_t hi1, uint64_t lo2, uint64_t hi2) {
  uint64_t r = 0;
#if defined(OMP_STATIC)
  #pragma omp parallel for schedule(static) reduction(+:r)
#elif defined(OMP_DYNAMIC)
  #pragma omp parallel for schedule(dynamic) reduction(+:r)
#elif defined(OMP_GUIDED)
  #pragma omp parallel for schedule(guided) reduction(+:r)
#else
  #error "Need to select OpenMP scheduler: STATIC, DYNAMIC or GUIDED"
#endif
  for (uint64_t i = lo1; i != hi1; i++) {
    for (uint64_t j = lo2; j != hi2; j++) {
      r += a[i][j];
    }
  }
  return r;
}

#endif

uint64_t sum_array_nested_serial(char **a, uint64_t lo1, uint64_t hi1, uint64_t lo2, uint64_t hi2) {
  uint64_t r = 0;
  for (uint64_t i = lo1; i != hi1; i++) {
    for (uint64_t j = lo2; j != hi2; j++) {
      r += a[i][j];
    }
  }
  return r;
}
