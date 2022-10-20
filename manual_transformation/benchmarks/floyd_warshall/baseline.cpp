#include "floyd_warshall.hpp"

using namespace floyd_warshall;

int main() {
  setup();

  floyd_warshall_serial(dist, vertices);

  finishup();

  return 0;
}
