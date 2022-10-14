#ifndef TASKPARTS_TPALRTS
#error "need to compile with tpal flags, e.g., TASKPARTS_TPALRTS"
#endif
#include <taskparts/benchmark.hpp>
#include <cstdint>
#include <cmath>
#include <complex>
#include <emmintrin.h>

unsigned char* output = nullptr;
double _mb_x0 = -2.5;
double _mb_y0 = -0.875;
double _mb_x1 = 1;
double _mb_y1 = 0.875;
int height = 4192;
// Width should be a multiple of 8
int width = 4192;
int max_depth = 100;
double _mb_g = 2.0;

#define D_row 32

void mandelbrot_interrupt_col_loop(double x0, double y0, double x1, double y1,
                                  int width, int height, int max_depth,
                                  unsigned char* output, double xstep, double ystep,
				   int col_lo, int col_hi);
void mandelbrot_interrupt_row_loop(double x0, double y0, double x1, double y1,
                                  int width, int height, int max_depth,
                                  unsigned char* output, double xstep, double ystep,
				   int col_lo, int col_hi, int row_lo, int row_hi);

/* Outlined-loop functions */
/* ======================= */

void mandelbrot_interrupt_col_loop(double x0, double y0, double x1, double y1,
                                  int width, int height, int max_depth,
                                  unsigned char* output, double xstep, double ystep,
                                  int col_lo, int col_hi) {
  if (! (col_lo < col_hi)) {
    return;
  }
  for(; ; ) { // col loop
    int row_lo = 0;
    int row_hi = width;
    if (! (row_lo < row_hi)) {
      break;
    }
    for (; ; ) {
      int row_lo2 = row_lo;
      int row_hi2 = std::min(row_lo + D_row, row_hi);
      for (; row_lo2 < row_hi2; ++row_lo2) { // row loop
        double z_real = x0 + row_lo2*xstep;
        double z_imaginary = y0 + col_lo*ystep;
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
        output[col_lo*width + row_lo2] = static_cast<unsigned char>(static_cast<double>(depth) / max_depth * 255);
      }
      row_lo = row_lo2;
      if (! (row_lo < row_hi)) {
        break;
      }
    }
    col_lo++;
    if (! (col_lo < col_hi)) {
      break;
    }
  }
}

void mandelbrot_interrupt_row_loop(double x0, double y0, double x1, double y1,
                                  int width, int height, int max_depth,
                                  unsigned char* output, double xstep, double ystep,
                                  int col_lo, int col_hi, int row_lo, int row_hi) {
  if (! (row_lo < row_hi)) {
    return;
  }
  for (; ; ) {
    int row_lo2 = row_lo;
    int row_hi2 = std::min(row_lo + D_row, row_hi);
    for (; row_lo2 < row_hi2; ++row_lo2) { // row loop
      double z_real = x0 + row_lo2*xstep;
      double z_imaginary = y0 + col_lo*ystep;
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
      output[col_lo*width + row_lo2] = static_cast<unsigned char>(static_cast<double>(depth) / max_depth * 255);
    }
    row_lo = row_lo2;
    if (! (row_lo < row_hi)) {
      break;
    }
  }
}

unsigned char* mandelbrot_interrupt(double x0, double y0, double x1, double y1,
                                 int width, int height, int max_depth) {
  double xstep = (x1 - x0) / width;
  double ystep = (y1 - y0) / height;
  //unsigned char* output = static_cast<unsigned char*>(_mm_malloc(width * height * sizeof(unsigned char), 64));
  unsigned char* output = (unsigned char*)malloc(width * height * sizeof(unsigned char));
  mandelbrot_interrupt_col_loop(x0, y0, x1, y1, width, height, max_depth, output, xstep, ystep, 0, height);
  return output;
}

int main() {

  for (int i = 0; i < 100; i++) {
    _mb_g /= sin(_mb_g);
  }

  output = mandelbrot_interrupt(_mb_x0, _mb_y0, _mb_x1, _mb_y1, width, height, max_depth);

  _mm_free(output);
  
  return 0;
}
