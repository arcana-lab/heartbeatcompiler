#pragma once

#include <functional>

namespace mandelbulb {

extern unsigned char *output;
extern double _mb_x0;
extern double _mb_y0;
extern double _mb_z0;
extern double _mb_x1;
extern double _mb_y1;
extern double _mb_z1;
extern int _mb_nx;
extern int _mb_ny;
extern int _mb_nz;
extern int _mb_iterations;
extern double power;

#if !defined(USE_HB_MANUAL) && !defined(USE_HB_COMPILER)
void run_bench(std::function<void()> const &bench_body,
               std::function<void()> const &bench_start,
               std::function<void()> const &bench_end);
#endif
void setup();
void finishup();

#if defined(USE_BASELINE)
unsigned char* mandelbulb_serial(double x0, double y0, double z0,
                                 double x1, double y1, double z1,
                                 int nx, int ny, int nz, int iterations);
#elif defined(USE_OPENCILK)
unsigned char* mandelbulb_opencilk(double x0, double y0, double z0,
                                   double x1, double y1, double z1,
                                   int nx, int ny, int nz, int iterations);
#elif defined(USE_OPENMP)
unsigned char* mandelbulb_openmp(double x0, double y0, double z0,
                                 double x1, double y1, double z1,
                                 int nx, int ny, int nz, int iterations);
#endif

#if defined(TEST_CORRECTNESS)
void test_correctness();
#endif

} // namespace mandelbulb
