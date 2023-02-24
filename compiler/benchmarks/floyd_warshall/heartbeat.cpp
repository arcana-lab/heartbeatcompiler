#include "bench.hpp"
#include <algorithm>
#if defined(HEARTBEAT_VERSIONING)
#include "loop_handler.hpp"
#endif
#if defined(COLLECT_KERNEL_TIME)
#include <stdio.h>
#include <chrono>
#endif

#define SUB(array, row_sz, i, j) (array[i * row_sz + j])

void HEARTBEAT_loop0(int *dist, uint64_t vertices, uint64_t via);
void HEARTBEAT_loop1(int *dist, uint64_t vertices, uint64_t via, uint64_t from);

void HEARTBEAT_loop0(int *dist, uint64_t vertices, uint64_t via) {
  for (uint64_t from = 0; from < vertices; from++) {
    HEARTBEAT_loop1(dist, vertices, via, from);
  }
}

void HEARTBEAT_loop1(int *dist, uint64_t vertices, uint64_t via, uint64_t from) {
  for (uint64_t to = 0; to < vertices; to++) {
    if ((from != to) && (from != via) && (to != via)) {
      SUB(dist, vertices, from, to) = 
        std::min(SUB(dist, vertices, from, to), 
                  SUB(dist, vertices, from, via) + SUB(dist, vertices, via, to));
    }
  }
}

bool run_heartbeat = true;

extern int vertices;
extern int *dist;

int main() {
  setup();

#if defined(COLLECT_KERNEL_TIME)
  using clock = std::chrono::system_clock;
  using sec = std::chrono::duration<double>;
  const auto before = clock::now();
#endif

  loop_dispatcher([&] {
    for (uint64_t via = 0; via < vertices; via++) {
      HEARTBEAT_loop0(dist, (uint64_t)vertices, via);
    }
  });

#if defined(COLLECT_KERNEL_TIME)
  const sec duration = clock::now() - before;
  printf("\"kernel_exectime\":  %.2f\n", duration.count());
#endif

#if defined(TEST_CORRECTNESS)
  test_correctness(dist);
#endif

  finishup();

#if defined(HEARTBEAT_VERSIONING)
  // create dummy call here so loop_handler declarations
  // so that the function declaration will be emitted by the LLVM-IR
  loop_handler(nullptr, 0, nullptr, 0, 0, 0, 0);
#endif

  return 0;
}
