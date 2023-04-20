#include "bench.hpp"
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

int main() {

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

#if defined(USE_HB_COMPILER)
  heartbeat_polling();
  // dummy call to loop_handler
  loop_handler(
    nullptr, 0,
    nullptr, nullptr, nullptr
  );
#if defined(ENABLE_ROLLFORWARD)
  int64_t rc = 0;
  __rf_handle_wrapper(
    rc, nullptr, 0,
    nullptr, nullptr, nullptr
  );
#endif
#endif

  return 0;
}
