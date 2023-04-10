#include "bench.hpp"
#if defined(USE_HB_MANUAL)
#include "heartbeat_manual.hpp"
#endif
#if defined(USE_HB_COMPILER)
#include "heartbeat_compiler.hpp"
#endif

using namespace cg;

#if defined(USE_HB_COMPILER)
bool run_heartbeat = true;
#endif

scalar rnorm;
int cgitmax = 25;

int main() {
  run_bench([&] {
#if defined(USE_BASELINE)
    rnorm = conj_gradient_serial(n, col_ind, row_ptr, x, z, a, p, q, r, cgitmax);
#elif defined(USE_OPENCILK)
    rnorm = conj_gradient_cilk(n, col_ind, row_ptr, x, z, a, p, q, r, cgitmax);
#elif defined(USE_OPENMP)
    rnorm = conj_gradient_openmp(n, col_ind, row_ptr, x, z, a, p, q, r, cgitmax);
#elif defined(USE_HB_COMPILER)
    rnorm = conj_gradient_compiler(n, col_ind, row_ptr, x, z, a, p, q, r, cgitmax);
#endif

#if defined(TEST_CORRECTNESS)
    test_correctness();
#endif
  }, [&] {
    setup();
  }, [&] {
    finishup();
  });

#if defined(USE_HB_COMPILER)
  // dummy call to loop_handler
  loop_handler_level2(
    nullptr, 0,
    nullptr, nullptr, nullptr,
    0, 0,
    0, 0
  );
#if defined(ENABLE_ROLLFORWARD)
  int64_t rc = 0;
  __rf_handle_level2_wrapper(
    rc, nullptr, 0,
    nullptr, nullptr, nullptr,
    0, 0,
    0, 0
  );
#endif
#endif

  return 0;
}
