#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <iostream>
#ifdef ORIGINAL_PROGRAM
#include <chrono>
#endif
#include "loop_handler.cpp"

void HEARTBEAT_loop0 (uint64_t sz, int32_t *a, uint32_t innerIterations){
  for (uint64_t i = 0; i < sz; i++) {
    for (auto j = 0; j < innerIterations; j++){
      a[i] = (int32_t)sin((double)a[i]);
    }
  }
}

void HEARTBEAT_loop1 (uint64_t sz, int32_t *a, uint32_t innerIterations){
  for (uint64_t i = 0; i < sz; i++) {
    for (auto j = 0; j < innerIterations; j++){
      a[i]++;
    }
  }
}

int main (int argc, char *argv[]) {
  if (argc < 3){
    std::cerr << "USAGE: " << argv[0] << " OUTER_ITERATIONS INNER_ITERATIONS" << std::endl;
    return 1;
  }
  auto sz = atoll(argv[1]);
  auto inner = atoi(argv[2]);
  auto a = (int32_t*)calloc(sz, sizeof(int32_t));

  
  //HEARTBEAT_loop0(sz, a, inner);
#ifdef ORIGINAL_PROGRAM
  using clock = std::chrono::system_clock;
  using sec = std::chrono::duration<double>;
  const auto before = clock::now();
#endif


  HEARTBEAT_loop1(sz, a, inner);

  for (auto i=0; i < sz; i++){
    std::cout << a[i] << std::endl;
  }
  
#ifdef ORIGINAL_PROGRAM
  const sec duration = clock::now() - before;
  printf("[{\"exectime\":  %f}]\n", duration.count());
#endif
  

  //printf("%lld %d %d %d\n", sz, a[0], a[sz/2], a[sz-1]);

  return 0;
}
