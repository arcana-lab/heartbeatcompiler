#pragma once

#include <cstdint>
#include <functional>

namespace plus_reduce_array {

extern uint64_t nb_items;
extern double *a;
extern double result;

#if !defined(USE_HB_MANUAL)
void run_bench(std::function<void()> const &bench_body,
               std::function<void()> const &bench_start,
               std::function<void()> const &bench_end);
#endif
void setup();
void finishup();

#if defined(USE_BASELINE)
double plus_reduce_array_serial(double* a, uint64_t lo, uint64_t hi);
#elif defined(USE_OPENCILK)
double plus_reduce_array_opencilk(double* a, uint64_t lo, uint64_t hi);
#elif defined(USE_OPENMP)
double plus_reduce_array_openmp(double* a, uint64_t lo, uint64_t hi);
#endif

#if defined(TEST_CORRECTNESS)
void test_correctness();
#endif

} // namespace plus_reduce_array
