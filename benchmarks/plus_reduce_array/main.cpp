#include "bench.hpp"
#if defined(USE_HB_MANUAL)
#include "heartbeat_manual.hpp"
#endif
#if defined(USE_HB_COMPILER)
#include "heartbeat_compiler.hpp"
#endif

using namespace plus_reduce_array;

#if defined(USE_HB_COMPILER)
bool run_heartbeat = true;
#endif

int main() {

  run_bench([&] {
    for (int i = 0; i < 10; i++) {
#if defined(USE_BASELINE)
      result += plus_reduce_array_serial(a, 0, nb_items);
#elif defined(USE_OPENCILK)
      result += plus_reduce_array_opencilk(a, 0, nb_items);
#elif defined(USE_CILKPLUS)
      result += plus_reduce_array_cilkplus(a, 0, nb_items);
#elif defined(USE_OPENMP)
      result += plus_reduce_array_openmp(a, 0, nb_items);
#elif defined(USE_HB_MANUAL)
      result += plus_reduce_array_hb_manual(a, 0, nb_items);
#elif defined(USE_HB_COMPILER)
      result += plus_reduce_array_hb_compiler(a, 0, nb_items);
#endif
    }

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
