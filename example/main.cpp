#include <stdio.h>
#include <stdint.h>
#include "promotion_handler.hpp"

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

  run_bench([&] {
#if defined(USE_BASELINE)
    for (int64_t i = 0; i < n; i++) {
      if (i % 2 ==0) sum += i;
      else sum += 2 * i;
    }
#elif defined(USE_HB_COMPILER)
    heartbeat_region(n, sum);
#endif
  }, [&] {}, [&] {});

  printf("sum = %ld\n", sum);
  return 0;
}
