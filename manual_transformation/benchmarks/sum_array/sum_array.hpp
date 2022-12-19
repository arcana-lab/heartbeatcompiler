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

namespace sum_array {

#if defined(INPUT_BENCHMARKING)
  uint64_t n = 250000000000;
#elif defined(INPUT_TESTING)
  uint64_t n = 10000000000;
#else
  #error "Need to select input size, e.g., INPUT_BENCHMARKING, INPUT_TESTING"
#endif
uint64_t result = 0;
char *a;

void setup() {
  a = new char[n];
  for (uint64_t i = 0; i < n; i++) {
    a[i] = 1;
  }
}

void finishup() {
  delete [] a;
}

#if defined(USE_OPENCILK)
void zero_uint64_t(void *view) {
  *(uint64_t *)view = 0;
}
void add_uint64_t(void *left, void *right) {
  *(uint64_t *)left += *(uint64_t *)right;
}
uint64_t sum_array_opencilk(char *a, uint64_t lo, uint64_t hi) {
  uint64_t cilk_reducer(zero_uint64_t, add_uint64_t) sum;
  cilk_for (uint64_t i = lo; i != hi; i++) {
    sum += a[i];
  }
  return sum;
}
#elif defined(USE_OPENMP)
uint64_t sum_array_openmp(char* a, uint64_t lo, uint64_t hi) {
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
  for (uint64_t i = lo; i != hi; i++) {
    r += a[i];
  }
  return r;
}
#endif

uint64_t sum_array_serial(char *a, uint64_t lo, uint64_t hi) {
  uint64_t r = 0;
  for (uint64_t i = lo; i != hi; i++) {
    r += a[i];
  }
  return r;
}

}
