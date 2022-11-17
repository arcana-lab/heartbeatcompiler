#include <cstdint>
#if defined(USE_OPENCILK)
#include <cilk/cilk.h>
#endif
#if defined(USE_OPENMP)
#include <omp.h>
#endif

namespace sum_array_nested {

uint64_t n1 = 500000;     // input size for benchmarking
uint64_t n2 = 500000;     // input size for benchmarking
// uint64_t n1 = 100000;     // input size for testing
// uint64_t n2 = 100000;     // input size for testing
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
#if defined(OMP_DYNAMIC)
  #pragma omp parallel for schedule(dynamic) reduction(+:r)
#elif defined(OMP_GUIDED) 
  #pragma omp parallel for schedule(guided) reduction(+:r)
#else  
  #pragma omp parallel for reduction(+:r)
#endif
  for (uint64_t i = lo1; i != hi1; i++) {
    for (uint64_t j = lo2; j != hi2; j++) {
      r += a[i][j];
    }
  }
  return r;
}
#else
uint64_t sum_array_nested_serial(char **a, uint64_t lo1, uint64_t hi1, uint64_t lo2, uint64_t hi2) {
  uint64_t r = 0;
  for (uint64_t i = lo1; i != hi1; i++) {
    for (uint64_t j = lo2; j != hi2; j++) {
      r += a[i][j];
    }
  }
  return r;
}
#endif

}
