#pragma once

#include <cstdint>
#include <functional>

namespace nested_plus_reduce_array {

extern uint64_t nb_items1;
extern uint64_t nb_items2;
extern double **a;
extern double result;

#if !defined(USE_HB_MANUAL) && !defined(USE_HB_COMPILER)
void run_bench(std::function<void()> const &bench_body,
               std::function<void()> const &bench_start,
               std::function<void()> const &bench_end);
#endif
void setup();
void finishup();

#if defined(USE_BASELINE)
double nested_plus_reduce_array_serial(double** a, uint64_t lo1, uint64_t hi1, uint64_t lo2, uint64_t hi2);
#elif defined(USE_OPENCILK)
double nested_plus_reduce_array_opencilk(double** a, uint64_t lo1, uint64_t hi1, uint64_t lo2, uint64_t hi2);
#elif defined(USE_OPENMP)
double nested_plus_reduce_array_openmp(double** a, uint64_t lo1, uint64_t hi1, uint64_t lo2, uint64_t hi2);
#endif

#if defined(TEST_CORRECTNESS)
void test_correctness();
#endif

} // namespace nested_plus_reduce_array
