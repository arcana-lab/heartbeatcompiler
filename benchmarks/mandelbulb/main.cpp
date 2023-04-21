#include "bench.hpp"
#if defined(USE_HB_COMPILER)
#include "heartbeat_compiler.hpp"
#endif

using namespace mandelbulb;

#if defined(USE_HB_COMPILER)
bool run_heartbeat = true;
#endif

scalar rnorm;
int cgitmax = 25;

int main() {
  run_bench([&] {
#if defined(USE_BASELINE)
    output = mandelbulb_serial(_mb_nx, _mb_ny, _mb_nz, _mb_iterations, _mb_x0, _mb_y0, _mb_z0, _mb_x1, _mb_y1, _mb_z1);
#elif defined(USE_OPENCILK)
    output = mandelbulb_cilk(_mb_nx, _mb_ny, _mb_nz, _mb_iterations, _mb_x0, _mb_y0, _mb_z0, _mb_x1, _mb_y1, _mb_z1);
#elif defined(USE_OPENMP)
    output = mandelbulb_openmp(_mb_nx, _mb_ny, _mb_nz, _mb_iterations, _mb_x0, _mb_y0, _mb_z0, _mb_x1, _mb_y1, _mb_z1);
#elif defined(USE_HB_COMPILER)
    output = mandelbulb_compiler(_mb_nx, _mb_ny, _mb_nz, _mb_iterations, _mb_x0, _mb_y0, _mb_z0, _mb_x1, _mb_y1, _mb_z1);
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
