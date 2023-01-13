#include <stdio.h>
#include <iostream>

uint64_t *b;
uint64_t *c;

void HEARTBEAT_loop0 (uint64_t **, uint64_t, uint64_t);
void HEARTBEAT_loop1 (uint64_t **, uint64_t, uint64_t);

void HEARTBEAT_loop0 (uint64_t **a, uint64_t sz, uint64_t innerIterations) {
  for (uint64_t i = 0; i < sz; i++) {
    b[i]++;
    HEARTBEAT_loop1(a, i, innerIterations);
    c[i]++;
  }
}

void HEARTBEAT_loop1 (uint64_t **a, uint64_t i, uint64_t innerIterations) {
  for (uint64_t j = 0; j < innerIterations; j++) {
    a[i][j]++;
  }

  return;
}

int main (int argc, char *argv[]) {
  if (argc < 3) {
    std::cerr << "USAGE: " << argv[0] << " OUTER_ITERATIONS INNER_ITERATIONS" << std::endl;
    return 1;
  }
  uint64_t sz = atoll(argv[1]);
  uint64_t inner = atoi(argv[2]);
  uint64_t **a = (uint64_t **)calloc(sz, sizeof(uint64_t));
  for (uint64_t i = 0; i < sz; i++) {
    a[i] = (uint64_t *)calloc(inner, sizeof(uint64_t));
  }
  b = (uint64_t *)calloc(sz, sizeof(uint64_t));
  c = (uint64_t *)calloc(sz, sizeof(uint64_t));

  HEARTBEAT_loop0(a, sz, inner);

  uint64_t accum_b = 0;
  uint64_t accum_c = 0;
  for (uint64_t i = 0; i < sz; i++) {
    uint64_t accum_a_i = 0;
    for (uint64_t j = 0; j < inner; j++) {
      accum_a_i += a[i][j];
    }
    std::cout << i << ": " << accum_a_i << std::endl;
    accum_b += b[i];
    accum_c += c[i];
  }
  std::cout << "Sum of b array = " << accum_b << std::endl;
  std::cout << "Sum of c array = " << accum_c << std::endl;

  return 0;
}
