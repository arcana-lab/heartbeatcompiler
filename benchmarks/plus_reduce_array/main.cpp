#include "bench.hpp"
#if defined(USE_HB_MANUAL)
#include "heartbeat_manual.hpp"
#endif

using namespace plus_reduce_array;

int main() {

  run_bench([&] {
#if defined(USE_BASELINE)
    result = plus_reduce_array_serial(a, 0, nb_items);
#elif defined(USE_OPENCILK)
    result = plus_reduce_array_opencilk(a, 0, nb_items);
#elif defined(USE_OPENMP)
    result = plus_reduce_array_openmp(a, 0, nb_items);
#elif defined(USE_HB_MANUAL)
    result = plus_reduce_array_hb_manual(a, 0, nb_items);
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
