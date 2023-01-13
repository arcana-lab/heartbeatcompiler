#include <stdio.h>
#include <iostream>

void HEARTBEAT_loop0(char **, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t &);
void HEARTBEAT_loop1(char **, uint64_t, uint64_t, uint64_t, uint64_t &);

void HEARTBEAT_loop0(char **a, uint64_t low1, uint64_t high1, uint64_t low2, uint64_t high2, uint64_t &r0) {
  for (uint64_t i = low1; i < high1; i++) {
    uint64_t r1 = 0;
    HEARTBEAT_loop1(a, i, low2, high2, r1);
    r0 += r1;
  }

  return;
}

void HEARTBEAT_loop1(char **a, uint64_t i, uint64_t low2, uint64_t high2, uint64_t &r1) {
  for (uint64_t j = low2; j < high2; j++) {
    r1 += a[i][j];
  }

  return;
}

int main (int argc, char *argv[]) {
  if (argc < 3) {
    std::cerr << "USAGE: " << argv[0] << " SIZE" << std::endl;
    return 1;
  }

  uint64_t n1 = atoll(argv[1]);
  uint64_t n2 = atoll(argv[2]);

  char **a = new char*[n1];
  for (uint64_t i = 0; i < n1; i++) {
    a[i] = new char[n2];
    for (uint64_t j = 0; j < n2; j++) {
      a[i][j] = 1;
    }
  }

  uint64_t r0 = 0;
  HEARTBEAT_loop0(a, 0, n1, 0, n2, r0);

  std::cout << "Sum of a array = " << r0 << std::endl;

  return 0;
}
