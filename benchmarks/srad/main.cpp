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

using namespace srad;

#if defined(USE_HB_COMPILER)
#if defined(RUN_HEARTBEAT)
  bool run_heartbeat = true;
#else
  bool run_heartbeat = false;
#endif
#endif

int main(int argc, char *argv[]) {

#if defined(INPUT_USER)
  if (argc < 3) {
    fprintf(stderr, "USAGE: %s rows cols\n", argv[0]);
    return 1;
  }
  rows = atoi(argv[1]);
  cols = atoi(argv[2]);
#endif

  run_bench([&] {
    for (int i = 0; i < 30; i++) {
#if defined(USE_BASELINE)
      srad_serial(rows, cols, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
#elif defined(USE_OPENCILK)
      srad_opencilk(rows, cols, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
#elif defined(USE_CILKPLUS)
      srad_cilkplus(rows, cols, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
#elif defined(USE_OPENMP)
      srad_openmp(rows, cols, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
#elif defined(USE_HB_MANUAL)
      srad_hb_manual(rows, cols, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
#elif defined(USE_HB_COMPILER)
      srad_hb_compiler(rows, cols, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
#endif
    }

#if TEST_CORRECTNESS
    test_correctness();
#endif
  }, [&] {
    setup();
  }, [&] {
    finishup();
  });

  return 0;
}
