#include <cstdint>

namespace sum_array {

uint64_t n = 1000 * 1000 * 1000;
double result = 0.0;
double* a;

void setup() {
  a = new double[n];
  for (int i = 0; i < n; i++) {
    a[i] = 1.0;
  }
}

void finishup() {
  delete [] a;
}

#if defined(USE_OPENCILK)
double sum_array_opencilk(double* a, uint64_t lo, uint64_t hi) {
  cilk::reducer_opadd<double> r(0.0);
  cilk_for (uint64_t i = lo; i != hi; i++) {
    *r += a[i];
  }
  return r.get_value();
}
#elif defined(USE_OPENMP)
double sum_array_openmp(double* a, uint64_t lo, uint64_t hi) {
  double r = 0.0;
  #pragma omp parallel for reduction(+:r)
  for (uint64_t i = lo; i != hi; i++) {
    r += a[i];
  }
  return r;
}
#else
double sum_array_serial(double* a, uint64_t lo, uint64_t hi) {
  double r = 0.0;
  for (uint64_t i = lo; i != hi; i++) {
    r += a[i];
  }
  return r;
}
#endif

}