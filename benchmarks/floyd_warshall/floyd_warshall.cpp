#include "floyd_warshall.hpp"
#include <cstdlib>
#include <climits>
#include <algorithm>
#if defined(USE_OPENCILK)
#include <cilk/cilk.h>
#endif
#if defined(USE_OPENMP)
#include <omp.h>
#endif
#if defined(TEST_CORRECTNESS)
#include <cstdio>
#endif

#define SUB(array, row_sz, i, j) (array[i * row_sz + j])
#define INF INT_MAX-1

#if defined(INPUT_BENCHMARKING)
  int vertices = 2048;
#elif defined(INPUT_TESTING)
  int vertices = 1024;
#elif defined(INPUT_TPAL)
  int vertices = 1024;
#else
  #error "Need to select input size, e.g., INPUT_{BENCHMARKING, TESTING, TPAL}"
#endif
int *dist = nullptr;

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

#if defined(USE_OPENCILK)
void floyd_warshall_opencilk(int* dist, int vertices) {
  for (int via = 0; via < vertices; via++) {
    cilk_for (int from = 0; from < vertices; from++) {
      cilk_for (int to = 0; to < vertices; to++) {
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
void floyd_warshall_openmp(int* dist, int vertices) {
  for (int via = 0; via < vertices; via++) {
#if defined(OMP_STATIC)
    #pragma omp parallel for schedule(static)
#elif defined(OMP_DYNAMIC)
    #pragma omp parallel for schedule(dynamic)
#elif defined(OMP_GUIDED)
    #pragma omp parallel for schedule(guided)
#else
    #error "Need to select OpenMP scheduler: STATIC, DYNAMIC or GUIDED"
#endif
    for (int from = 0; from < vertices; from++) {
      for (int to = 0; to < vertices; to++) {
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

void floyd_warshall_serial(int* dist, int vertices) {
  for (int via = 0; via < vertices; via++) {
    for (int from = 0; from < vertices; from++) {
      for (int to = 0; to < vertices; to++) {
        if ((from != to) && (from != via) && (to != via)) {
          SUB(dist, vertices, from, to) = 
            std::min(SUB(dist, vertices, from, to), 
                     SUB(dist, vertices, from, via) + SUB(dist, vertices, via, to));
        }
      }
    }
  }
}

#if defined(TEST_CORRECTNESS)
void test_correctness(int *dist) {
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
