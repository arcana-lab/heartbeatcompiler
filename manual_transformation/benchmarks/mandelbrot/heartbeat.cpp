#include "mandelbrot.hpp"
#if defined(HEARTBEAT_BRANCHES)
#include "heartbeat_branches.hpp"
#elif defined(HEARTBEAT_VERSIONING)
#include "heartbeat_versioning.hpp"
#else
#error "Need to specific the version of heartbeat, e.g., HEARTBEAT_BRANCHES, HEARTBEAT_VERSIONING"
#endif
#if defined(COLLECT_KERNEL_TIME)
#include <stdio.h>
#include <chrono>
#endif

void loop_dispatcher(
  void (*f)(double, double, double, double, int, int, int, unsigned char *&),
  double x0,
  double y0,
  double x1,
  double y1,
  int width,
  int height,
  int max_depth,
  unsigned char *&output
) {
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    (*f)(x0, y0, x1, y1, width, height, max_depth, output);
  });
}

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
  loop_dispatcher(&mandelbrot_heartbeat_branches, _mb_x0, _mb_y0, _mb_x1, _mb_y1, _mb_width, _mb_height, _mb_max_depth, output);
#elif defined(HEARTBEAT_VERSIONING)
  loop_dispatcher(&mandelbrot_heartbeat_versioning, _mb_x0, _mb_y0, _mb_x1, _mb_y1, _mb_width, _mb_height, _mb_max_depth, output);
#else
  #error "Need to specific the version of heartbeat, e.g., HEARTBEAT_BRANCHES, HEARTBEAT_VERSIONING"
#endif

#if defined(COLLECT_KERNEL_TIME)
  const sec duration = clock::now() - before;
  printf("\"kernel_exectime\":  %.2f\n", duration.count());
#endif

#if defined(TEST_CORRECTNESS)
  test_correctness(output);
#endif

  finishup(output);

#if defined(COLLECT_HEARTBEAT_POLLING_TIME)
  collect_heartbeat_polling_time_print();
#endif
#if defined(COLLECT_HEARTBEAT_PROMOTION_TIME)
  collect_heartbeat_promotion_time_print();
#endif
  return 0;

}
