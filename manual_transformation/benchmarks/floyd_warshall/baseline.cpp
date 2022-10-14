#ifndef TASKPARTS_TPALRTS
#error "need to compile with tpal flags, e.g., TASKPARTS_TPALRTS"
#endif
#include <taskparts/benchmark.hpp>
#include <algorithm>
#include <cassert>
#include <climits>

#define SUB(array, row_sz, i, j) (array[i * row_sz + j])

#define INF           INT_MAX-1
#define D 32

int vertices = 3000;
int* dist = nullptr;

void floyd_warshall_interrupt(int* dist, int vertices, int via_lo, int via_hi) {
  for(; via_lo < via_hi; via_lo++) {
    int from_lo = 0;
    int from_hi = vertices;
    if (! (from_lo < from_hi)) {
      return;
    }
    for (; ; ) {
      int to_lo = 0;
      int to_hi = vertices;
      if (! (to_lo < to_hi)) {
        return;
      }
      for (; ; ) {
        int to_lo2 = to_lo;
        int to_hi2 = std::min(to_lo + D, to_hi);
        for(; to_lo2 < to_hi2; to_lo2++) {
          if ((from_lo != to_lo2) && (from_lo != via_lo) && (to_lo2 != via_lo)) {
            SUB(dist, vertices, from_lo, to_lo2) = 
              std::min(SUB(dist, vertices, from_lo, to_lo2), 
                       SUB(dist, vertices, from_lo, via_lo) + SUB(dist, vertices, via_lo, to_lo2));
          }
        }
        to_lo = to_lo2;
        if (! (to_lo < to_hi)) {
          break;
        }
      }
      from_lo++;
      if (! (from_lo < from_hi)) {
        break;
      }
    }
  }
}

auto init_input(int vertices) {
  int* dist = (int*)malloc(sizeof(int) * vertices * vertices);
  for(int i = 0; i < vertices; i++) {
    for(int j = 0; j < vertices; j++) {
      SUB(dist, vertices, i, j) = ((i == j) ? 0 : INF);
    }
  }
  for (int i = 0 ; i < vertices ; i++) {
    for (int j = 0 ; j< vertices; j++) {
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
};

int main() {

  dist = init_input(vertices);

  floyd_warshall_interrupt(dist, vertices, 0, vertices);

  free(dist);

  return 0;
}