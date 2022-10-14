#ifndef TASKPARTS_TPALRTS
#error "need to compile with tpal flags, e.g., TASKPARTS_TPALRTS"
#endif
#include <taskparts/benchmark.hpp>

void sum_array_heartbeat(double* a, uint64_t lo, uint64_t hi, double r, double* dst);

/* Outlined-loop functions */
/* ======================= */

#define D 64

void sum_array_heartbeat(double* a, uint64_t lo, uint64_t hi, double r, double* dst) {
  if (! (lo < hi)) {
    goto exit;
  }
  for (; ; ) {
    uint64_t lo2 = lo;
    uint64_t hi2 = std::min(lo + D, hi);
    for (; lo2 < hi2; lo2++) {
      r += a[lo2];
    }
    lo = lo2;
    if (! (lo < hi)) {
      break;
    }

  }
 exit:
  *dst = r;
}

int main() {
  uint64_t n = taskparts::cmdline::parse_or_default_long("n", 1000 * 1000 * 1000);
  double result = 0.0;
  double* a;

  a = new double[n];
  for (int i = 0; i < n; i++) {
    a[i] = 1.0;
  }

  sum_array_heartbeat(a, 0, n, 0.0, &result);

  delete [] a;
  
  printf("result=%f\n",result);
  return 0;
}
