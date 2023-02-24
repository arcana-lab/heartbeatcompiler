#pragma once

void setup();
void finishup();

#if defined(USE_BASELINE)
  unsigned char* mandelbrot_serial(double x0, double y0, double x1, double y1,
                                   int width, int height, int max_depth);
#elif defined(USE_OPENCILK)
  unsigned char* mandelbrot_opencilk(double x0, double y0, double x1, double y1,
                                     int width, int height, int max_depth);
#elif defined(USE_OPENMP)
  unsigned char* mandelbrot_openmp(double x0, double y0, double x1, double y1,
                                   int width, int height, int max_depth);
#endif

#if defined(TEST_CORRECTNESS)
  void test_correctness(unsigned char *output);
#endif
