#include "bench.hpp"
#include "utility.hpp"
#include <cmath>
#include <cstdlib>
#include <emmintrin.h>
#if !defined(USE_HB_MANUAL) && !defined(USE_HB_COMPILER)
#include <functional>
#endif

namespace mandelbrot {

#if defined(INPUT_BENCHMARKING)
  int _mb_height = 8384;
  int _mb_width = 8384;
  int _mb_max_depth = 100;
#elif defined(INPUT_TPAL)
  int _mb_height = 4192;
  int _mb_width = 4192;
  int _mb_max_depth = 100;
#elif defined(INPUT_TESTING)
  int _mb_height = 4192;
  int _mb_width = 4192;
  int _mb_max_depth = 100;
#else
  #error "Need to select input size, e.g., INPUT_{BENCHMARKING, TPAL, TESTING}"
#endif
unsigned char *output = nullptr;
double _mb_x0 = -2.5;
double _mb_y0 = -0.875;
double _mb_x1 = 1;
double _mb_y1 = 0.875;
double g = 2.0;

#if !defined(USE_HB_MANUAL) && !defined(USE_HB_COMPILER)
void run_bench(std::function<void()> const &bench_body,
               std::function<void()> const &bench_start,
               std::function<void()> const &bench_end) {
  utility::run([&] {
    bench_body();
  }, [&] {
    bench_start();
  }, [&] {
    bench_end();
  });
}
#endif

void setup() {
  for (int i = 0; i < 100; i++) {
    g /= sin(g);
  }
}

void finishup() {
  _mm_free(output);
}

#if defined(USE_BASELINE) || defined(TEST_CORRECTNESS)

unsigned char* mandelbrot_serial(double x0, double y0, double x1, double y1,
                                 int width, int height, int max_depth) {
  double xstep = (x1 - x0) / width;
  double ystep = (y1 - y0) / height;
  //  unsigned char* output = static_cast<unsigned char*>(_mm_malloc(width * height * sizeof(unsigned char), 64));
  unsigned char* output = (unsigned char*)malloc(width * height * sizeof(unsigned char));
  for(int j = 0; j < height; ++j) { // col loop
    for (int i = 0; i < width; ++i) { // row loop
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
  return output;
}

#endif

#if defined(USE_OPENCILK)

#include <cilk/cilk.h>

unsigned char* mandelbrot_opencilk(double x0, double y0, double x1, double y1,
                                 int width, int height, int max_depth) {
  double xstep = (x1 - x0) / width;
  double ystep = (y1 - y0) / height;
  //  unsigned char* output = static_cast<unsigned char*>(_mm_malloc(width * height * sizeof(unsigned char), 64));
  unsigned char* output = (unsigned char*)malloc(width * height * sizeof(unsigned char));
  cilk_for(int j = 0; j < height; ++j) { // col loop
    cilk_for (int i = 0; i < width; ++i) { // row loop
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
  return output;
}

#elif defined(USE_CILKPLUS)

#include <cilk/cilk.h>

unsigned char* mandelbrot_cilkplus(double x0, double y0, double x1, double y1,
                                 int width, int height, int max_depth) {
  double xstep = (x1 - x0) / width;
  double ystep = (y1 - y0) / height;
  //  unsigned char* output = static_cast<unsigned char*>(_mm_malloc(width * height * sizeof(unsigned char), 64));
  unsigned char* output = (unsigned char*)malloc(width * height * sizeof(unsigned char));
  cilk_for(int j = 0; j < height; ++j) { // col loop
    cilk_for (int i = 0; i < width; ++i) { // row loop
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
  return output;
}

#elif defined(USE_OPENMP)

#include <omp.h>

unsigned char* mandelbrot_openmp(double x0, double y0, double x1, double y1,
                                 int width, int height, int max_depth) {
  double xstep = (x1 - x0) / width;
  double ystep = (y1 - y0) / height;
  //  unsigned char* output = static_cast<unsigned char*>(_mm_malloc(width * height * sizeof(unsigned char), 64));
  unsigned char* output = (unsigned char*)malloc(width * height * sizeof(unsigned char));
  #pragma omp parallel for schedule(static)
  for(int j = 0; j < height; ++j) { // col loop
    // #pragma omp parallel for schedule(static)
    for (int i = 0; i < width; ++i) { // row loop
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
  return output;
}

#endif

#if defined(TEST_CORRECTNESS)

#include <stdio.h>

void test_correctness() {
  unsigned char *output2 = mandelbrot_serial(_mb_x0, _mb_y0, _mb_x1, _mb_y1, _mb_width, _mb_height, _mb_max_depth);
  int num_diffs = 0;
  for (int i = 0; i < _mb_height; i++) {
    for (int j = 0; j < _mb_width; j++) {
      if (output[i * _mb_width + j] != output2[i * _mb_width + j]) {
        num_diffs++;
      }
    }
  }
  if (num_diffs > 0) {
    printf("\033[0;31mINCORRECT!\033[0m");
    printf("  num_diffs = %d\n", num_diffs);
  } else {
    printf("\033[0;32mCORRECT!\033[0m\n");
  }
  free(output2);
}

#endif

} // namespace mandelbrot
