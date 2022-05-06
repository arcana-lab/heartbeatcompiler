#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <iostream>
#ifdef ORIGINAL_PROGRAM
#include <chrono>
#endif
#include "loop_handler.cpp"

typedef struct {
  int32_t f0;
  int32_t f1;
} my_t;

uint64_t HEARTBEAT_loop1 (uint64_t sz, int32_t *a, uint32_t innerIterations){
  uint64_t tot = 0;
  for (uint64_t i = 0; i < sz; i++) {
    my_t t;
    t.f0 = a[i];
    t.f1 = 0;
    for (auto j = 0; j < innerIterations; j++){
      if (j % 20){
        t.f0++;
      } {
        t.f1 += 2;
      }
    }
    a[i]  = t.f1;
    tot += (uint64_t)a[i];
  }

  return tot;
}

int main (int argc, char *argv[]) {
  if (argc < 3){
    std::cerr << "USAGE: " << argv[0] << " OUTER_ITERATIONS INNER_ITERATIONS" << std::endl;
    return 1;
  }
  auto sz = atoll(argv[1]);
  auto inner = atoi(argv[2]);
  auto a = (int32_t*)calloc(sz, sizeof(int32_t));

#ifdef ORIGINAL_PROGRAM
  using clock = std::chrono::system_clock;
  using sec = std::chrono::duration<double>;
  const auto before = clock::now();
#endif

  auto t = HEARTBEAT_loop1(sz, a, inner);
  std::cout << t << std::endl;

#ifdef ORIGINAL_PROGRAM
  const sec duration = clock::now() - before;
  printf("[{\"exectime\":  %f}]\n", duration.count());
#endif
  

  //printf("%lld %d %d %d\n", sz, a[0], a[sz/2], a[sz-1]);

  return 0;
}
