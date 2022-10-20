#include "floyd_warshall.hpp"

using namespace floyd_warshall;

int main() {
  setup();

#if defined(USE_OPENMP)
  floyd_warshall_openmp(dist, vertices);
#endif

  finishup();

  return 0;
}