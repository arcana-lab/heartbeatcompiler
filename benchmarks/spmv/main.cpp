#include "bench.hpp"
#if defined(USE_HB_MANUAL)
#include "heartbeat_manual.hpp"
#endif
#if defined(USE_HB_COMPILER)
#include "heartbeat_compiler.hpp"
#include "loop_handler.hpp"
#endif

using namespace spmv;

#if defined(USE_HB_COMPILER)
bool run_heartbeat = true;
#endif

int main() {

  run_bench([&] {
#if defined(USE_BASELINE)
    spmv_serial(val, row_ptr, col_ind, x, y, nb_rows);
#elif defined(USE_OPENCILK)
    spmv_opencilk(val, row_ptr, col_ind, x, y, nb_rows);
#elif defined(USE_OPENMP)
    spmv_openmp(val, row_ptr, col_ind, x, y, nb_rows);
#elif defined(USE_HB_MANUAL)
    spmv_hb_manual(val, row_ptr, col_ind, x, y, nb_rows);
#elif defined(USE_HB_COMPILER)
    spmv_hb_compiler(val, row_ptr, col_ind, x, y, nb_rows);
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
    0, 0,
  );
#endif

  return 0;
}
