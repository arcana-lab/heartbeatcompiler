#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <iostream>
#ifdef ORIGINAL_PROGRAM
#include <chrono>
#endif
#include "loop_handler.cpp"

uint64_t HEARTBEAT_loop1 (uint64_t sz){
  uint64_t result = 100;
  for (uint64_t i = 0; i < sz; i++) {
    if (i % 3 == 0) {
      result += 1;
    }
  }

  return result;
}

int main (int argc, char *argv[]) {
  if (argc < 2){
    std::cerr << "USAGE: " << argv[0] << " OUTER_ITERATIONS" << std::endl;
    return 1;
  }
  auto sz = atoll(argv[1]);

  
#ifdef ORIGINAL_PROGRAM
  using clock = std::chrono::system_clock;
  using sec = std::chrono::duration<double>;
  const auto before = clock::now();
#endif


  uint64_t result = HEARTBEAT_loop1(sz);

  std::cout << result << std::endl;
  
#ifdef ORIGINAL_PROGRAM
  const sec duration = clock::now() - before;
  printf("[{\"exectime\":  %f}]\n", duration.count());
#endif
  

  return 0;
}
