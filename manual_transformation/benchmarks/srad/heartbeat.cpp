#include "bench.hpp"
#if defined(HEARTBEAT_BRANCHES)
#include "heartbeat_branches.hpp"
#elif defined(HEARTBEAT_VERSIONING)
#include "heartbeat_versioning.hpp"
#else
#error "Need to specific the algorithm of heartbeat transformation: HEARTBEAT_{BRANCHES, VERSIONING}"
#endif
#if defined(COLLECT_KERNEL_TIME)
#include <stdio.h>
#include <chrono>
#endif

extern int rows, cols;
extern float *J, q0sqr;
extern float *dN, *dS, *dW, *dE;
extern float *c;
extern int *iN, *iS, *jE, *jW;
extern float lambda;

int main() {
#if defined(COLLECT_HEARTBEAT_POLLING_TIME)
  collect_heartbeat_polling_time_init();
#endif
#if defined(COLLECT_HEARTBEAT_PROMOTION_TIME)
  collect_heartbeat_promotion_time_init();
#endif
  setup();

#if defined(COLLECT_KERNEL_TIME)
  using clock = std::chrono::system_clock;
  using sec = std::chrono::duration<double>;
  const auto before = clock::now();
#endif

#if defined(HEARTBEAT_BRANCHES)
  loop_dispatcher([&] {
    srad_heartbeat_branches(rows, cols, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
  });
#elif defined(HEARTBEAT_VERSIONING)
  loop_dispatcher([&] {
    srad_heartbeat_versioning(rows, cols, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
  });
#else
  #error "Need to specific the algorithm of heartbeat transformation: HEARTBEAT_{BRANCHES, VERSIONING}"
#endif

#if defined(COLLECT_KERNEL_TIME)
  const sec duration = clock::now() - before;
  printf("\"kernel_exectime\":  %.2f\n", duration.count());
#endif

#if defined(TEST_CORRECTNESS)
  test_correctness();
#endif

  finishup();

#if defined(COLLECT_HEARTBEAT_POLLING_TIME)
  collect_heartbeat_polling_time_print();
#endif
#if defined(COLLECT_HEARTBEAT_PROMOTION_TIME)
  collect_heartbeat_promotion_time_print();
#endif
  return 0;
}
