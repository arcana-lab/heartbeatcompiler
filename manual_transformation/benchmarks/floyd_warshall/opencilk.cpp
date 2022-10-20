#include "floyd_warshall.hpp"

using namespace floyd_warshall;

int main() {
  setup();

#if defined(USE_OPENCILK)
  floyd_warshall_opencilk(dist, vertices);
#endif

  finishup();

  return 0;
}
