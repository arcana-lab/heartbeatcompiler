#include <cmath>
#include <cstdlib>
#include <emmintrin.h>
#if defined(USE_OPENCILK)
#include <cilk/cilk.h>
#endif
#if defined(USE_OPENMP)
#include <omp.h>
#endif
#if defined(TEST_CORRECTNESS)
#include <cstdio>
#endif

namespace mandelbrot {

unsigned char *output = nullptr;
double _mb_x0 = -2.5;
double _mb_y0 = -0.875;
double _mb_x1 = 1;
double _mb_y1 = 0.875;
int _mb_height = 4192;
// Width should be a multiple of 8
int _mb_width = 4192;
int _mb_max_depth = 100;
double g = 2.0;

void setup() {
  for (int i = 0; i < 100; i++) {
    g /= sin(g);
  }
}

void finishup(unsigned char *output) {
  _mm_free(output);
}

#if defined(USE_OPENCILK)
unsigned char* mandelbrot_opencilk(double x0, double y0, double x1, double y1,
                               int width, int height, int max_depth) {
  double xstep = (x1 - x0) / width;
  double ystep = (y1 - y0) / height;
  //unsigned char* output = static_cast<unsigned char*>(_mm_malloc(width * height * sizeof(unsigned char), 64));
  unsigned char* output = (unsigned char*)malloc(width * height * sizeof(unsigned char));
  cilk_for(int j = 0; j < height; ++j) {
    cilk_for(int i = 0; i < width; ++i) {
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
unsigned char* mandelbrot_openmp(double x0, double y0, double x1, double y1,
                               int width, int height, int max_depth) {
  double xstep = (x1 - x0) / width;
  double ystep = (y1 - y0) / height;
  //unsigned char* output = static_cast<unsigned char*>(_mm_malloc(width * height * sizeof(unsigned char), 64));
  unsigned char* output = (unsigned char*)malloc(width * height * sizeof(unsigned char));
  #pragma omp parallel for collapse(2)
  for(int j = 0; j < height; ++j) {
    for(int i = 0; i < width; ++i) {
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
#else
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

}