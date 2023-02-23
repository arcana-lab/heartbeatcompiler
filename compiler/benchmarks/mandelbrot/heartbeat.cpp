#include "mandelbrot.hpp"
#include <cstdlib>
#if defined(HEARTBEAT_VERSIONING)
#include "loop_handler.hpp"
#endif
#if defined(COLLECT_KERNEL_TIME)
#include <stdio.h>
#include <chrono>
#endif

void HEARTBEAT_loop0(double x0, double y0, double xstep, double ystep, uint64_t width, uint64_t height, int max_depth, unsigned char *output);
void HEARTBEAT_loop1(double x0, double y0, double xstep, double ystep, uint64_t j, uint64_t width, int max_depth, unsigned char *output);

void HEARTBEAT_loop0(double x0, double y0, double xstep, double ystep, uint64_t width, uint64_t height, int max_depth, unsigned char *output) {
  for (uint64_t j = 0; j < height; ++j) { // col loop
    HEARTBEAT_loop1(x0, y0, xstep, ystep, j, width, max_depth, output);
  }
}

void HEARTBEAT_loop1(double x0, double y0, double xstep, double ystep, uint64_t j, uint64_t width, int max_depth, unsigned char *output) {
  for (uint64_t i = 0; i < width; ++i) { // row loop
    double z_real = x0 + i*xstep;
    double z_imaginary = y0 + j*ystep;
    double c_real = z_real;
    double c_imaginary = z_imaginary;
    double depth = 0;
    while(depth < max_depth) {
      if(z_real * z_real + z_imaginary * z_imaginary > 4.0) {
        break;
      }
      double temp_real = z_real*z_real - z_imaginary*z_imaginary;
      double temp_imaginary = 2.0*z_real*z_imaginary;
      z_real = c_real + temp_real;
      z_imaginary = c_imaginary + temp_imaginary;
      ++depth;
    }
    output[j*width + i] = static_cast<unsigned char>(static_cast<double>(depth) / max_depth * 255);
  }
}

bool run_heartbeat = true;

extern unsigned char *output;
extern double _mb_x0;
extern double _mb_y0;
extern double _mb_x1;
extern double _mb_y1;
extern int _mb_height;
extern int _mb_width;
extern int _mb_max_depth;

int main() {
  setup();

#if defined(COLLECT_KERNEL_TIME)
  using clock = std::chrono::system_clock;
  using sec = std::chrono::duration<double>;
  const auto before = clock::now();
#endif

  loop_dispatcher([&] {
    double xstep = (_mb_x1 - _mb_x0) / _mb_width;
    double ystep = (_mb_y1 - _mb_y0) / _mb_height;
    output = (unsigned char*)malloc(_mb_width * _mb_height * sizeof(unsigned char));

    HEARTBEAT_loop0(_mb_x0, _mb_y0, xstep, ystep, (uint64_t)_mb_width, (uint64_t)_mb_height, _mb_max_depth, output);
  });

#if defined(COLLECT_KERNEL_TIME)
  const sec duration = clock::now() - before;
  printf("\"kernel_exectime\":  %.2f\n", duration.count());
#endif

#if defined(TEST_CORRECTNESS)
  test_correctness(output);
#endif

  finishup();

#if defined(HEARTBEAT_VERSIONING)
  // create dummy call here so loop_handler declarations
  // so that the function declaration will be emitted by the LLVM-IR
  loop_handler(nullptr, 0, nullptr, 0, 0, 0, 0);
#endif

  return 0;
}
