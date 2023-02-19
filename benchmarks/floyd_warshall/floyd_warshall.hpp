#pragma once

void setup();
void finishup();

#if defined(USE_BASELINE)
  void floyd_warshall_serial(int* dist, int vertices);
#elif defined(USE_OPENCILK)
  void floyd_warshall_opencilk(int* dist, int vertices);
#elif defined(USE_OPENMP)
  void floyd_warshall_openmp(int* dist, int vertices);
#endif

#if defined(TEST_CORRECTNESS)
  void test_correctness(int *dist);
#endif
