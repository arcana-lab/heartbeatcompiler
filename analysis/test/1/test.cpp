#include <stdio.h>
#include <iostream>

void HEARTBEAT_loop0(char *a, uint64_t sz, uint64_t &r) {
  for (uint64_t i = 0; i < sz; i++) {
    r += a[i];
  }

  return;
}

int main (int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "USAGE: " << argv[0] << " SIZE" << std::endl;
    return 1;
  }
  uint64_t sz = atoll(argv[1]);
  char *a = new char[sz];
  for (uint64_t i = 0; i < sz; i++) {
    a[i] = 1;
  }

  uint64_t r = 0;
  HEARTBEAT_loop0(a, sz, r);

  std::cout << "Sum of a array = " << r << std::endl;

  return 0;
}
