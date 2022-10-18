#include "floyd_warshall.hpp"

using namespace floyd_warshall;

int main() {
  dist = init_input(vertices);

  floyd_warshall_serial(dist, vertices);

  free(dist);

  return 0;
}