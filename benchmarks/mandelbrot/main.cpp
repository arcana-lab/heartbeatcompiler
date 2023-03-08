#include "bench.hpp"
#if defined(USE_HB_MANUAL)
#include "heartbeat_manual.hpp"
#endif
#if defined(USE_HB_COMPILER)
#include "heartbeat_compiler.hpp"
#include "loop_handler.hpp"
#endif

using namespace mandelbrot;

int main() {

  run_bench([&] {
#if defined(USE_BASELINE)
    output = mandelbrot_serial(_mb_x0, _mb_y0, _mb_x1, _mb_y1, _mb_width, _mb_height, _mb_max_depth);
#elif defined(USE_OPENCILK)
    output = mandelbrot_opencilk(_mb_x0, _mb_y0, _mb_x1, _mb_y1, _mb_width, _mb_height, _mb_max_depth);
#elif defined(USE_OPENMP)
    output = mandelbrot_openmp(_mb_x0, _mb_y0, _mb_x1, _mb_y1, _mb_width, _mb_height, _mb_max_depth);
#elif defined(USE_HB_MANUAL)
    output = mandelbrot_hb_manual(_mb_x0, _mb_y0, _mb_x1, _mb_y1, _mb_width, _mb_height, _mb_max_depth);
#elif defined(USE_HB_COMPILER)
    output = mandelbrot_hb_compiler(_mb_x0, _mb_y0, _mb_x1, _mb_y1, _mb_width, _mb_height, _mb_max_depth);
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
#endif

  return 0;
}
