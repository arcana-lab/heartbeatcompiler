#include <emmintrin.h>
#include <algorithm>
#include <cmath>

unsigned char* output = nullptr;
double _mb_x0 = -2.5;
double _mb_y0 = -0.875;
double _mb_x1 = 1;
double _mb_y1 = 0.875;
int _mb_height = 4192;
// Width should be a multiple of 8
int _mb_width = 4192;
int _mb_max_depth = 100;
double g = 2.0;

unsigned char* mandelbrot(double x0, double y0, double x1, double y1,
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

int main() {

  for (int i = 0; i < 100; i++) {
    g /= sin(g);
  }

  output = mandelbrot(_mb_x0, _mb_y0, _mb_x1, _mb_y1, _mb_width, _mb_height, _mb_max_depth);

  _mm_free(output);
  
  return 0;
}
