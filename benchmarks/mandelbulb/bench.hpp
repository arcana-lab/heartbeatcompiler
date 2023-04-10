#pragma once

#include <cmath>
#include <taskparts/benchmark.hpp>

namespace mandelbulb {
  
double power = 8.0;
double maxLength = 2.0;

#if !defined(USE_HB_MANUAL) && !defined(USE_HB_COMPILER)
void run_bench(std::function<void()> const &bench_body,
               std::function<void()> const &bench_start,
               std::function<void()> const &bench_end);
#endif
void setup();
void finishup();

#if defined(USE_BASELINE)
unsigned char* mandelbulb_serial(int nx, int ny, int nz, int iterations,
				 double x0, double y0, double z0,
				 double x1, double y1, double z1);
#elif defined(USE_OPENCILK)
unsigned char* mandelbulb_cilk(int nx, int ny, int nz, int iterations,
			       double x0, double y0, double z0,
			       double x1, double y1, double z1);
#elif defined(USE_OPENMP)
unsigned char* mandelbulb_openmp(int nx, int ny, int nz, int iterations,
				 double x0, double y0, double z0,
				 double x1, double y1, double z1);
#endif

#if defined(TEST_CORRECTNESS)
void test_correctness();
#endif

} // namespace mandelbulb
