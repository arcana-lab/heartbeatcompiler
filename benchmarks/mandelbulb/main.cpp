#include "bench.hpp"
#if defined(USE_HB_MANUAL)
#include "heartbeat_manual.hpp"
#endif
#if defined(USE_HB_COMPILER)
#include "heartbeat_compiler.hpp"
#endif

using namespace mandelbulb;

#if defined(USE_HB_COMPILER)
bool run_heartbeat = true;
#endif

int main() {

  run_bench([&] {
#if defined(USE_BASELINE)
    output = mandelbulb_serial(_mb_x0, _mb_y0, _mb_z0, _mb_x1, _mb_y1, _mb_z1, _mb_nx, _mb_ny, _mb_nz, _mb_iterations);
#elif defined(USE_OPENCILK)
    output = mandelbulb_opencilk(_mb_x0, _mb_y0, _mb_z0, _mb_x1, _mb_y1, _mb_z1, _mb_nx, _mb_ny, _mb_nz, _mb_iterations);
#elif defined(USE_OPENMP)
    output = mandelbulb_openmp(_mb_x0, _mb_y0, _mb_z0, _mb_x1, _mb_y1, _mb_z1, _mb_nx, _mb_ny, _mb_nz, _mb_iterations);
#elif defined(USE_HB_MANUAL)
    output = mandelbulb_hb_manual(_mb_x0, _mb_y0, _mb_z0, _mb_x1, _mb_y1, _mb_z1, _mb_nx, _mb_ny, _mb_nz, _mb_iterations, power);
#elif defined(USE_HB_COMPILER)
    output = mandelbulb_hb_compiler(_mb_x0, _mb_y0, _mb_z0, _mb_x1, _mb_y1, _mb_z1, _mb_nx, _mb_ny, _mb_nz, _mb_iterations, power);
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
