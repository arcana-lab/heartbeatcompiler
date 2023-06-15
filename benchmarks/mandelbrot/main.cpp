#include "bench.hpp"
#if defined(INPUT_USER)
#include <stdio.h>
#endif
#if defined(USE_HB_MANUAL)
#include "heartbeat_manual.hpp"
#endif
#if defined(USE_HB_COMPILER)
#include "heartbeat_compiler.hpp"
#endif

using namespace mandelbrot;

#if defined(USE_HB_COMPILER)
bool run_heartbeat = true;
#endif

int main(int argc, char *argv[]) {

#if defined(INPUT_USER)
  if (argc < 4) {
    fprintf(stderr, "USAGE: %s height width max_depth\n", argv[0]);
    return 1;
  }
  _mb_height = atoi(argv[1]);
  _mb_width = atoi(argv[2]);
  _mb_max_depth = atoi(argv[3]);
#endif

  run_bench([&] {
#if defined(USE_BASELINE)
    output = mandelbrot_serial(_mb_x0, _mb_y0, _mb_x1, _mb_y1, _mb_height, _mb_width, _mb_max_depth);
#elif defined(USE_OPENCILK)
    output = mandelbrot_opencilk(_mb_x0, _mb_y0, _mb_x1, _mb_y1, _mb_height, _mb_width, _mb_max_depth);
#elif defined(USE_CILKPLUS)
    output = mandelbrot_cilkplus(_mb_x0, _mb_y0, _mb_x1, _mb_y1, _mb_height, _mb_width, _mb_max_depth);
#elif defined(USE_OPENMP)
    output = mandelbrot_openmp(_mb_x0, _mb_y0, _mb_x1, _mb_y1, _mb_height,_mb_width, _mb_max_depth);
#elif defined(USE_HB_MANUAL)
    output = mandelbrot_hb_manual(_mb_x0, _mb_y0, _mb_x1, _mb_y1, _mb_height, _mb_width, _mb_max_depth);
#elif defined(USE_HB_COMPILER)
    output = mandelbrot_hb_compiler(_mb_x0, _mb_y0, _mb_x1, _mb_y1, _mb_height, _mb_width, _mb_max_depth);
#if defined(ACC_JUSTIFY)
    output = mandelbrot_hb_compiler(_mb_x0, _mb_y0, _mb_x1, _mb_y1, 2000, 1000000, 10);
    output = mandelbrot_hb_compiler(_mb_x0, _mb_y0, _mb_x1, _mb_y1, 10, 2000, 1000000);
#endif
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
