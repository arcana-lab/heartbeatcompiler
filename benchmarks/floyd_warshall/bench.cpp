#include "bench.hpp"
#include <cstdint>
#include <cstdlib>
#include <climits>
#include <algorithm>
#if !defined(USE_HB_MANUAL) && !defined(USE_HB_COMPILER)
#include "utility.hpp"
#include <functional>
#endif

#define SUB(array, row_sz, i, j) (array[i * row_sz + j])
#define INF INT_MAX-1

namespace floyd_warshall {

#if defined(INPUT_BENCHMARKING)
  #if defined(FW_1K)
    int vertices = 1024;
  #elif defined(FW_2K)
    int vertices = 2048;
  #else
    #error "Need to select input class: FW_{1K, 2K}"
  #endif
#elif defined(INPUT_TPAL)
  #if defined(FW_1K)
    int vertices = 1024;
  #elif defined(FW_2K)
    int vertices = 2048;
  #else
    #error "Need to select input class: FW_{1K, 2K}"
  #endif
#elif defined(INPUT_TESTING)
  int vertices = 1024;
#else
  #error "Need to select input size: INPUT_{BENCHMARKING, TPAL, TESTING}"
#endif
int *dist = nullptr;

#if !defined(USE_HB_MANUAL) && !defined(USE_HB_COMPILER)
void run_bench(std::function<void()> const &bench_body,
               std::function<void()> const &bench_start,
               std::function<void()> const &bench_end) {
  utility::run([&] {
    bench_body();
  }, [&] {
    bench_start();
  }, [&] {
    bench_end();
  });
}
#endif

auto init_input(int vertices) {
  int *dist = (int *)malloc(sizeof(int) * vertices * vertices);
  for (int i = 0; i < vertices; i++) {
    for (int j = 0; j < vertices; j++) {
      SUB(dist, vertices, i, j) = ((i == j) ? 0 : INF);
    }
  }
  for (int i = 0; i < vertices; i++) {
    for (int j = 0; j < vertices; j++) {
      if (i == j) {
        SUB(dist, vertices, i, j) = 0;
      } else {
        int num = i + j;
        if (num % 3 == 0) {
          SUB(dist, vertices, i, j) = num / 2;
        } else if (num % 2 == 0) {
          SUB(dist, vertices, i, j) = num * 2;
        } else {
          SUB(dist, vertices, i, j) = num;
        }
      }
    }
  }
  return dist;
}

void setup() {
  dist = init_input(vertices);
}

void finishup() {
  free(dist);
}

#if defined(USE_BASELINE) || defined(TEST_CORRECTNESS)

void floyd_warshall_serial(int* dist, int vertices) {
  for(int via = 0; via < vertices; via++) {
    for(int from = 0; from < vertices; from++) {
      for(int to = 0; to < vertices; to++) {
        if ((from != to) && (from != via) && (to != via)) {
          SUB(dist, vertices, from, to) = 
            std::min(SUB(dist, vertices, from, to), 
                     SUB(dist, vertices, from, via) + SUB(dist, vertices, via, to));
        }
      }
    }
  }
}

#endif

#if defined(USE_OPENCILK)

#include <cilk/cilk.h>

void floyd_warshall_opencilk(int* dist, int vertices) {
  for(int via = 0; via < vertices; via++) {
    cilk_for(int from = 0; from < vertices; from++) {
      cilk_for(int to = 0; to < vertices; to++) {
        if ((from != to) && (from != via) && (to != via)) {
          SUB(dist, vertices, from, to) = 
            std::min(SUB(dist, vertices, from, to), 
                     SUB(dist, vertices, from, via) + SUB(dist, vertices, via, to));
        }
      }
    }
  }
}

#elif defined(USE_CILKPLUS)

#include <cilk/cilk.h>

void floyd_warshall_cilkplus(int* dist, int vertices) {
  for(int via = 0; via < vertices; via++) {
    cilk_for(int from = 0; from < vertices; from++) {
      cilk_for(int to = 0; to < vertices; to++) {
        if ((from != to) && (from != via) && (to != via)) {
          SUB(dist, vertices, from, to) = 
            std::min(SUB(dist, vertices, from, to), 
                     SUB(dist, vertices, from, via) + SUB(dist, vertices, via, to));
        }
      }
    }
  }
}

#elif defined(USE_OPENMP)

#include <omp.h>

void floyd_warshall_openmp(int* dist, int vertices) {
  for(int via = 0; via < vertices; via++) {
#if defined(OMP_SCHEDULE_STATIC)
    #pragma omp parallel for schedule(static)
#elif defined(OMP_SCHEDULE_DYNAMIC)
    #pragma omp parallel for schedule(dynamic)
#elif defined(OMP_SCHEDULE_GUIDED)
    #pragma omp parallel for schedule(guided)
#endif
    for(int from = 0; from < vertices; from++) {
#if defined(OMP_NESTED_SCHEDULING)
#if defined(OMP_SCHEDULE_STATIC)
      #pragma omp parallel for schedule(static)
#elif defined(OMP_SCHEDULE_DYNAMIC)
      #pragma omp parallel for schedule(dynamic)
#elif defined(OMP_SCHEDULE_GUIDED)
      #pragma omp parallel for schedule(guided)
#endif
#endif
      for(int to = 0; to < vertices; to++) {
        if ((from != to) && (from != via) && (to != via)) {
          SUB(dist, vertices, from, to) = 
            std::min(SUB(dist, vertices, from, to), 
                     SUB(dist, vertices, from, via) + SUB(dist, vertices, via, to));
        }
      }
    }
  }
}

#endif

#if defined(TEST_CORRECTNESS)

#include <stdio.h>

void test_correctness() {
  int *dist2 = init_input(vertices);
  floyd_warshall_serial(dist2, vertices);
  int num_diffs = 0;
  for (int i = 0; i < vertices; i++) {
    for (int j = 0; j < vertices; j++) {
      if (SUB(dist, vertices, i, j) != SUB(dist2, vertices, i, j)) {
        num_diffs++;
      }
    }
  }
  if (num_diffs > 0) {
    printf("\033[0;31mINCORRECT!\033[0m");
    printf("  num_diffs = %d\n", num_diffs);
  } else {
    printf("\033[0;32mCORRECT!\033[0m\n");
  }
  free(dist2);
}

#endif

} // namespace floyd_warshall
