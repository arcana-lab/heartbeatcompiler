#pragma once

#include <functional>

namespace mandelbrot {

extern unsigned char *output;
extern double _mb_x0;
extern double _mb_y0;
extern double _mb_x1;
extern double _mb_y1;
extern int _mb_height;
extern int _mb_width;
extern int _mb_max_depth;

#if !defined(USE_HB_MANUAL) && !defined(USE_HB_COMPILER)
void run_bench(std::function<void()> const &bench_body,
               std::function<void()> const &bench_start,
               std::function<void()> const &bench_end);
#endif
void setup();
void finishup();

#if defined(USE_BASELINE)
unsigned char* mandelbrot_serial(double x0, double y0, double x1, double y1,
                                 int width, int height, int max_depth);
#elif defined(USE_OPENCILK)
unsigned char* mandelbrot_opencilk(double x0, double y0, double x1, double y1,
                                   int width, int height, int max_depth);
#elif defined(USE_CILKPLUS)
unsigned char* mandelbrot_cilkplus(double x0, double y0, double x1, double y1,
                                   int width, int height, int max_depth);
#elif defined(USE_OPENMP)
unsigned char* mandelbrot_openmp(double x0, double y0, double x1, double y1,
                                 int width, int height, int max_depth);
#endif

#if defined(TEST_CORRECTNESS)
void test_correctness();
#endif

} // namespace mandelbrot
