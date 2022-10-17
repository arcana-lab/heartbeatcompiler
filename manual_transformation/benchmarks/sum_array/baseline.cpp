#include <stdint.h>
#include <stdio.h>
#include <algorithm>

double sum_array(double* a, uint64_t lo, uint64_t hi) {
  double r = 0.0;
  for (uint64_t i = lo; i != hi; i++) {
    r += a[i];
  }
  return r;
}

int main() {
  uint64_t n = 1000 * 1000 * 1000;
  double result = 0.0;
  double* a;

  a = new double[n];
  for (int i = 0; i < n; i++) {
    a[i] = 1.0;
  }

  result = sum_array(a, 0, n);

  delete [] a;
  
  printf("result=%f\n",result);
  return 0;
}
