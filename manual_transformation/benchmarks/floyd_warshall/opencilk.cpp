#include "floyd_warshall.hpp"

using namespace floyd_warshall;

int main() {
  dist = init_input(vertices);

#if defined(USE_OPENCILK)
  floyd_warshall_opencilk(dist, vertices);
#endif

  free(dist);

  return 0;
}