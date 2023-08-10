#include "bench.hpp"
#if defined(USE_HB_MANUAL)
#include "heartbeat_manual.hpp"
#endif
#if defined(USE_HB_COMPILER)
#include "heartbeat_compiler.hpp"
#endif

using namespace spmv;

#if defined(USE_HB_COMPILER)
#if defined(RUN_HEARTBEAT)
  bool run_heartbeat = true;
#else
  bool run_heartbeat = false;
#endif
#endif

int main() {

  run_bench([&] {
#if !defined(ACC_SPMV_STATS) && !defined(SPMV_DETECTION_RATE_ANALYSIS)
    for (int i = 0; i < 10; i++) {
#endif
#if defined(USE_BASELINE)
      spmv_serial(val, row_ptr, col_ind, x, y, nb_rows);
#elif defined(USE_OPENCILK)
      spmv_opencilk(val, row_ptr, col_ind, x, y, nb_rows);
#elif defined(USE_CILKPLUS)
      spmv_cilkplus(val, row_ptr, col_ind, x, y, nb_rows);
#elif defined(USE_OPENMP)
      spmv_openmp(val, row_ptr, col_ind, x, y, nb_rows);
#elif defined(USE_HB_MANUAL)
      spmv_hb_manual(val, row_ptr, col_ind, x, y, nb_rows);
#elif defined(USE_HB_COMPILER)
      spmv_hb_compiler(val, row_ptr, col_ind, x, y, nb_rows);
#endif
#if !defined(ACC_SPMV_STATS) && !defined(SPMV_DETECTION_RATE_ANALYSIS)
    }
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
