#include "bench.hpp"
// benchmark adapted from https://www.fountainware.com/Funware/Mandelbrot3D/Mandelbrot3d.htm

namespace mandelbulb {

#if defined(INPUT_BENCHMARKING)
  int _mb_nz = 128;
  int _mb_ny = 128;
  int _mb_nz = 128;
  int _mb_iterations = 10;
#elif defined(INPUT_TPAL)
  int _mb_nz = 64;
  int _mb_ny = 64;
  int _mb_nz = 64;
  int _mb_iterations = 10;
#elif defined(INPUT_TESTING)
  int _mb_nz = 16;
  int _mb_ny = 16;
  int _mb_nz = 16;
  int _mb_iterations = 10;
#else
  #error "Need to select input size, e.g., INPUT_{BENCHMARKING, TPAL, TESTING}"
#endif
unsigned char *output = nullptr;
double _mb_x0 = -2.5;
double _mb_y0 = -0.875;
double _mb_z0 = -2.5;
double _mb_x1 = 1;
double _mb_y1 = 0.875;
double _mb_z1 = 2.5;

#if defined(USE_BASELINE) || defined(TEST_CORRECTNESS)
  
unsigned char* mandelbulb_serial(int nx, int ny, int nz, int iterations,
				 double x0, double y0, double z0,
				 double x1, double y1, double z1) {
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

unsigned char* mandelbulb_cilk(int nx, int ny, int nz, int iterations,
			       double x0, double y0, double z0,
			       double x1, double y1, double z1) {
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

unsigned char* mandelbulb_openmp(int nx, int ny, int nz, int iterations,
				 double x0, double y0, double z0,
				 double x1, double y1, double z1) {
  unsigned char* output = (unsigned char*)malloc(nx * ny * nz * sizeof(unsigned char));
  double xstep = (x1 - x0) / nx;
  double ystep = (y1 - y0) / ny;
  double zstep = (z1 - z0) / nz;
#pragma omp parallel for schedule(static)
  for (int i = 0; i < nx; ++i) {
#pragma omp parallel for schedule(static)
    for(int j = 0; j < ny; ++j) {
#pragma omp parallel for schedule(static)
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

}
  
#endif
  
} // namespace mandelbulb
