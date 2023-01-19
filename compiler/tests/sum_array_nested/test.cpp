// #include "loop_handler.hpp"
#include <iostream>
#include <cstdlib>
#include <cstdint>

uint64_t HEARTBEAT_loop0(char **, uint64_t, uint64_t, uint64_t, uint64_t);
uint64_t HEARTBEAT_loop1(char **, uint64_t, uint64_t, uint64_t);

uint64_t HEARTBEAT_loop0(char **a, uint64_t low1, uint64_t high1, uint64_t low2, uint64_t high2) {
  uint64_t r = 0;
  for (uint64_t i = low1; i < high1; i++) {
    r += HEARTBEAT_loop1(a, i, low2, high2);
  }

  return r;
}

uint64_t HEARTBEAT_loop1(char **a, uint64_t i, uint64_t low2, uint64_t high2) {
  uint64_t r = 0;
  for (uint64_t j = low2; j < high2; j++) {
    r += a[i][j];
  }

  return r;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cerr << "USAGE: " << argv[0] << " OUTER_SIZE INNER_SIZE" << std::endl;
  }

  uint64_t n1 = atoll(argv[1]);
  uint64_t n2 = atoll(argv[2]);

  // setup
  char **a = new char*[n1];
  for (uint64_t i = 0; i < n1; i++) {
    a[i] = new char[n2];
    for (uint64_t j = 0; j < n2; j++) {
      a[i][j] = 1;
    }
  }

  uint64_t r = 0;
#if defined(USE_BASELINE)
  for (uint64_t i = 0; i < n1; i++) {
    for (uint64_t j = 0; j < n2; j++) {
      r += a[i][j];
    }
  }
#elif defined(USE_HEARTBEAT)
  r = HEARTBEAT_loop0(a, 0, n1, 0, n2);
#endif

  // finishup
  for (uint64_t i = 0; i < n1; i++) {
    delete [] a[i];
  }
  delete [] a;

  std::cout << r << std::endl;

  return 0;
}
