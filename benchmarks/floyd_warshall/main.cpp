#include "bench.hpp"
#if defined(INPUT_USER)
#include <stdio.h>
#endif
#if defined(USE_HB_MANUAL)
#include "heartbeat_manual.hpp"
#endif
#if defined(USE_HB_COMPILER)
#include "heartbeat_compiler.hpp"
#endif

using namespace floyd_warshall;

#if defined(USE_HB_COMPILER)
bool run_heartbeat = true;
#endif

int main(int argc, char *argv[]) {

#if defined(INPUT_USER)
  if (argc < 2) {
    fprintf(stderr, "USAGE: %s vertices\n", argv[0]);
    return 1;
  }
  vertices = atoi(argv[1]);
#endif

  run_bench([&] {
#if defined(USE_BASELINE)
    floyd_warshall_serial(dist, vertices);
#elif defined(USE_OPENCILK)
    floyd_warshall_opencilk(dist, vertices);
#elif defined(USE_CILKPLUS)
    floyd_warshall_cilkplus(dist, vertices);
#elif defined(USE_OPENMP)
    floyd_warshall_openmp(dist, vertices);
#elif defined(USE_HB_MANUAL)
    floyd_warshall_hb_manual(dist, vertices);
#elif defined(USE_HB_COMPILER)
    floyd_warshall_hb_compiler(dist, vertices);
#endif

#if defined(TEST_CORRECTNESS)
    test_correctness();
#endif
  }, [&] {
    setup();
  }, [&] {
    finishup();
  });

  return 0;
}
