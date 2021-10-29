#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <iostream>
#include "loop_handler.cpp"

void HEARTBEAT_loop0 (uint64_t sz, int32_t *a){
  for (uint64_t i = 0; i < sz; i++) {
    a[i] = (int32_t)sin((double)a[i]);
  }
}

int main (int argc, char *argv[]) {
  if (argc < 2){
    std::cerr << "USAGE: " << argv[0] << " ITERATIONS" << std::endl;
    return 1;
  }
  auto sz = atoll(argv[1]);
  auto a = (int32_t*)calloc(sz, sizeof(int32_t));
  
  HEARTBEAT_loop0(sz, a);

  printf("%lld %d %d %d\n", sz, a[0], a[sz/2], a[sz-1]);

  return 0;
}
