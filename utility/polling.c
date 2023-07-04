#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include "loop_handler.hpp"

#define RDTSC(rdtsc_val_) do { \
  uint32_t rdtsc_hi_, rdtsc_lo_; \
  __asm__ volatile ("rdtsc" : "=a" (rdtsc_lo_), "=d" (rdtsc_hi_)); \
  rdtsc_val_ = (uint64_t)rdtsc_hi_ << 32 | rdtsc_lo_; \
} while (0)


int main() {

  run_bench([&] {
    uint64_t start, end;
    for (int i = 0; i < 200; i++) {
      RDTSC(start);
      if (__builtin_expect(!!(heartbeat_polling()), 0)) {
        RDTSC(end); // counting cycles including heartbeat check and updating timestamp array
                    // the timestamp array is managed by the runtime
        printf ("heartbeat arrives\n");
      } else {
        RDTSC(end); // counting cycles only for a heartbeat check
      }
      printf("%d -- %lu\n", i, (end - start));
    }
  }, [&] {
  }, [&] {
  });

  return 0;
}
