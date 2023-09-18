#pragma once

#include <functional>

namespace kmeans {

extern int numObjects;
extern int numAttributes;
extern int **attributes;
extern int nclusters;
extern int threshold;
extern int **cluster_centres;

#if !defined(USE_HB_MANUAL) && !defined(USE_HB_COMPILER)
void run_bench(std::function<void()> const &bench_body,
               std::function<void()> const &bench_start,
               std::function<void()> const &bench_end);
#endif
void setup();
void finishup();

#if defined(USE_BASELINE)
int cluster_serial(int numObjects, int numAttributes, int **attributes, int num_nclusters, int threshold, int ***cluster_centres);
#elif defined(USE_OPENCILK)
int cluster_opencilk(int numObjects, int numAttributes, int **attributes, int num_nclusters, int threshold, int ***cluster_centres);
#elif defined(USE_OPENMP)
int cluster_openmp(int numObjects, int numAttributes, int **attributes, int num_nclusters, int threshold, int ***cluster_centres);
#endif

#if defined(TEST_CORRECTNESS)
void test_correctness();
#endif

} // namespace kmeans
