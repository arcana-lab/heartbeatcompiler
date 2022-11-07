#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#define RDTSC(rdtsc_val_) do { \
  uint32_t rdtsc_hi_, rdtsc_lo_; \
  __asm__ volatile ("rdtsc" : "=a" (rdtsc_lo_), "=d" (rdtsc_hi_)); \
  rdtsc_val_ = (uint64_t)rdtsc_hi_ << 32 | rdtsc_lo_; \
} while (0)


int main() {
	uint64_t start, end;

	for (int i = 0; i < 1000; i++) {
		RDTSC(start);
		usleep(1000);
		RDTSC(end);
		printf("%d,%lu\n", i, (end - start) * 1000);
	}
}
