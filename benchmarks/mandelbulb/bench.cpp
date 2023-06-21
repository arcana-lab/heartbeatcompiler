#include "bench.hpp"
#include <cmath>
#include <cstdlib>
#include <cstdint>
#if !defined(USE_HB_MANUAL) && !defined(USE_HB_COMPILER)
#include "utility.hpp"
#include <functional>
#endif

// benchmark adapted from https://www.fountainware.com/Funware/Mandelbrot3D/Mandelbrot3d.htm
namespace mandelbulb {

#if defined(INPUT_BENCHMARKING)
  int _mb_nx = 100;
  int _mb_ny = 200;
  int _mb_nz = 300;
  int _mb_iterations = 400;
#elif defined(INPUT_TPAL)
  int _mb_nx = 64;
  int _mb_ny = 64;
  int _mb_nz = 64;
  int _mb_iterations = 10;
#elif defined(INPUT_TESTING)
  int _mb_nx = 16;
  int _mb_ny = 16;
  int _mb_nz = 16;
  int _mb_iterations = 10;
#elif defined(INPUT_USER)
  int _mb_nx;
  int _mb_ny;
  int _mb_nz;
  int _mb_iterations;
#else
  #error "Need to select input size, e.g., INPUT_{BENCHMARKING, TPAL, TESTING, USER}"
#endif
unsigned char *output = nullptr;
double _mb_x0 = -2.5;
double _mb_y0 = -0.875;
double _mb_z0 = -2.5;
double _mb_x1 = 1;
double _mb_y1 = 0.875;
double _mb_z1 = 2.5;
double power = 8.0;

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

void setup() {}

void finishup() {
  free(output);
}

#if defined(USE_BASELINE) || defined(TEST_CORRECTNESS)

unsigned char* mandelbulb_serial(double x0, double y0, double z0,
                                 double x1, double y1, double z1,
                                 int nx, int ny, int nz, int iterations) {
  unsigned char* output = (unsigned char*)malloc(nx * ny * nz * sizeof(unsigned char));
  double xstep = (x1 - x0) / nx;
  double ystep = (y1 - y0) / ny;
  double zstep = (z1 - z0) / nz;
  for (int i = 0; i < nx; ++i) {
    for(int j = 0; j < ny; ++j) {
      for (int k = 0; k < nz; ++k) {
        double x = x0 + i * xstep;
        double y = y0 + i * ystep;
        double z = z0 + i * zstep;
        uint64_t iter = 0;
        for (; iter < iterations; iter++) {
          auto r = sqrt(x*x + y*y + z*z);
          auto theta = acos(z / r);
          auto phi = atan(y / x);
          x = pow(r, power) * sin(power * theta) * cos(power * phi) + x;
          y = pow(r, power) * sin(power * theta) * sin(power * phi) + y;
          z = pow(r, power) * cos(power * theta) + z;
          if (sqrt(sqrt(x)+sqrt(y)+sqrt(z)) > 1.0) {
            break;
          }
        }
        output[(i*ny + j) * nz + k] = static_cast<unsigned char>(static_cast<double>(iter) / iterations * 255);
      }
    }
  }
  return output;
}

#endif

#if defined(USE_OPENCILK)

#include <cilk/cilk.h>

unsigned char* mandelbulb_opencilk(double x0, double y0, double z0,
                                   double x1, double y1, double z1,
                                   int nx, int ny, int nz, int iterations) {
  unsigned char* output = (unsigned char*)malloc(nx * ny * nz * sizeof(unsigned char));
  double xstep = (x1 - x0) / nx;
  double ystep = (y1 - y0) / ny;
  double zstep = (z1 - z0) / nz;
  cilk_for (int i = 0; i < nx; ++i) {
    cilk_for(int j = 0; j < ny; ++j) {
      cilk_for (int k = 0; k < nz; ++k) {
        double x = x0 + i * xstep;
        double y = y0 + i * ystep;
        double z = z0 + i * zstep;
        uint64_t iter = 0;
        for (; iter < iterations; iter++) {
          auto r = sqrt(x*x + y*y + z*z);
          auto theta = acos(z / r);
          auto phi = atan(y / x);
          x = pow(r, power) * sin(power * theta) * cos(power * phi) + x;
          y = pow(r, power) * sin(power * theta) * sin(power * phi) + y;
          z = pow(r, power) * cos(power * theta) + z;
          if (sqrt(sqrt(x)+sqrt(y)+sqrt(z)) > 1.0) {
            break;
          }
        }
        output[(i*ny + j) * nz + k] = static_cast<unsigned char>(static_cast<double>(iter) / iterations * 255);
      }
    }
  }
  return output;
}

#elif defined(USE_OPENMP)

#include <omp.h>

unsigned char* mandelbulb_openmp(double x0, double y0, double z0,
                                 double x1, double y1, double z1,
                                 int nx, int ny, int nz, int iterations) {
  unsigned char* output = (unsigned char*)malloc(nx * ny * nz * sizeof(unsigned char));
  double xstep = (x1 - x0) / nx;
  double ystep = (y1 - y0) / ny;
  double zstep = (z1 - z0) / nz;
#if defined(OMP_SCHEDULE_STATIC)
  #pragma omp parallel for schedule(static)
#elif defined(OMP_SCHEDULE_DYNAMIC)
  #pragma omp parallel for schedule(dynamic)
#elif defined(OMP_SCHEDULE_GUIDED)
  #pragma omp parallel for schedule(guided)
#endif
  for (int i = 0; i < nx; ++i) {
#if defined(OMP_NESTED_SCHEDULING)
#if defined(OMP_SCHEDULE_STATIC)
    #pragma omp parallel for schedule(static)
#elif defined(OMP_SCHEDULE_DYNAMIC)
    #pragma omp parallel for schedule(dynamic)
#elif defined(OMP_SCHEDULE_GUIDED)
    #pragma omp parallel for schedule(guided)
#endif
#endif
    for(int j = 0; j < ny; ++j) {
#if defined(OMP_NESTED_SCHEDULING)
#if defined(OMP_SCHEDULE_STATIC)
     #pragma omp parallel for schedule(static)
#elif defined(OMP_SCHEDULE_DYNAMIC)
      #pragma omp parallel for schedule(dynamic)
#elif defined(OMP_SCHEDULE_GUIDED)
      #pragma omp parallel for schedule(guided)
#endif
#endif
      for (int k = 0; k < nz; ++k) {
        double x = x0 + i * xstep;
        double y = y0 + i * ystep;
        double z = z0 + i * zstep;
        uint64_t iter = 0;
        for (; iter < iterations; iter++) {
          auto r = sqrt(x*x + y*y + z*z);
          auto theta = acos(z / r);
          auto phi = atan(y / x);
          x = pow(r, power) * sin(power * theta) * cos(power * phi) + x;
          y = pow(r, power) * sin(power * theta) * sin(power * phi) + y;
          z = pow(r, power) * cos(power * theta) + z;
          if (sqrt(sqrt(x)+sqrt(y)+sqrt(z)) > 1.0) {
            break;
          }
        }
        output[(i*ny + j) * nz + k] = static_cast<unsigned char>(static_cast<double>(iter) / iterations * 255);
      }
    }
  }
  return output;
}

#endif

#if defined(TEST_CORRECTNESS)

#include <stdio.h>

void test_correctness() {
  unsigned char *output2 = mandelbulb_serial(_mb_x0, _mb_y0, _mb_z0,
                                             _mb_x1, _mb_y1, _mb_z1,
                                             _mb_nx, _mb_ny, _mb_nz, _mb_iterations);
  int num_diffs = 0;
  for (int i = 0; i < _mb_nx; i++) {
    for (int j = 0; j < _mb_ny; j++) {
      for (int k = 0; k < _mb_nz; k++) {
        if (output[(i*_mb_ny) * _mb_nz + k] != output2[(i*_mb_ny) * _mb_nz + k]) {
          num_diffs++;
        }
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

} // namespace mandelbulb
