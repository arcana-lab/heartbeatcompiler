#include "bench.hpp"
#if defined(USE_HB_MANUAL)
#include "heartbeat_manual.hpp"
#endif
#if defined(USE_HB_COMPILER)
#include "heartbeat_compiler.hpp"
#endif

using namespace kmeans;

#if defined(USE_HB_COMPILER)
#if defined(RUN_HEARTBEAT)
  bool run_heartbeat = true;
#else
  bool run_heartbeat = false;
#endif
#endif

int main() {

  run_bench([&] {
#if defined(USE_BASELINE)
    cluster_serial(numObjects, numAttributes, attributes, nclusters, threshold, &cluster_centres);
#elif defined(USE_OPENCILK)
    cluster_opencilk(numObjects, numAttributes, attributes, nclusters, threshold, &cluster_centres);
#elif defined(USE_OPENMP)
    cluster_openmp(numObjects, numAttributes, attributes, nclusters, threshold, &cluster_centres);
#elif defined(USE_HB_MANUAL)
    cluster_hb_manual(numObjects, numAttributes, attributes, nclusters, threshold, &cluster_centres);
#elif defined(USE_HB_COMPILER)
    cluster_hb_compiler(numObjects, numAttributes, attributes, nclusters, threshold, &cluster_centres);
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
