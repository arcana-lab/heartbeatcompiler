#pragma once

#include <functional>

namespace kmeans {

extern int numObjects;
extern int numAttributes;
extern float **attributes;
extern int nclusters;
extern float threshold;
extern float **cluster_centres;

#if !defined(USE_HB_MANUAL) && !defined(USE_HB_COMPILER)
void run_bench(std::function<void()> const &bench_body,
               std::function<void()> const &bench_start,
               std::function<void()> const &bench_end);
#endif
void setup();
void finishup();

#if defined(USE_BASELINE)
int cluster_serial(int numObjects, int numAttributes, float **attributes, int num_nclusters, float threshold, float ***cluster_centres);
#elif defined(USE_OPENCILK)
int cluster_opencilk(int numObjects, int numAttributes, float **attributes, int num_nclusters, float threshold, float ***cluster_centres);
#elif defined(USE_OPENMP)
int cluster_openmp(int numObjects, int numAttributes, float **attributes, int num_nclusters, float threshold, float ***cluster_centres);
#endif

#if defined(TEST_CORRECTNESS)
void test_correctness();
#endif

} // namespace kmeans
