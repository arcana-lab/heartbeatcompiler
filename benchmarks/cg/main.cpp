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

int main() {

  run_bench([&] {
#if defined(USE_BASELINE)
    rnorm = conj_grad_serial(n, col_ind, row_ptr, x, z, val, p, q, r);
#elif defined(USE_OPENCILK)
    rnorm = conj_grad_opencilk(n, col_ind, row_ptr, x, z, val, p, q, r);
#elif defined(USE_OPENMP)
    rnorm = conj_grad_openmp(n, col_ind, row_ptr, x, z, val, p, q, r);
#elif defined(USE_HB_COMPILER)
    rnorm = conj_grad_hb_compiler(n, col_ind, row_ptr, x, z, val, p, q, r);
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
