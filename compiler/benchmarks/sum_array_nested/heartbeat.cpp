#include "bench.hpp"
#if defined(HEARTBEAT_VERSIONING)
#include "loop_handler.hpp"
#endif
#if defined(COLLECT_KERNEL_TIME)
#include <stdio.h>
#include <chrono>
#endif

uint64_t HEARTBEAT_loop0(char **, uint64_t, uint64_t, uint64_t, uint64_t);
uint64_t HEARTBEAT_loop1(char **, uint64_t, uint64_t, uint64_t);

uint64_t HEARTBEAT_loop0(char **a, uint64_t low1, uint64_t high1, uint64_t low2, uint64_t high2) {
  uint64_t r = 0;
  for (uint64_t i = low1; i < high1; i++) {
    r += HEARTBEAT_loop1(a, i, low2, high2);
  }

  return r;
}

uint64_t HEARTBEAT_loop1(char **a, uint64_t i, uint64_t low2, uint64_t high2) {
  uint64_t r = 0;
  for (uint64_t j = low2; j < high2; j++) {
    r += a[i][j];
  }

  return r;
}

bool run_heartbeat = true;

extern uint64_t n1;
extern uint64_t n2;
extern uint64_t result;
extern char **a;

int main(int argc, char *argv[]) {
  setup();

#if defined(COLLECT_KERNEL_TIME)
  using clock = std::chrono::system_clock;
  using sec = std::chrono::duration<double>;
  const auto before = clock::now();
#endif

  loop_dispatcher([&] {
    result = HEARTBEAT_loop0(a, 0, n1, 0, n2);
  });

#if defined(COLLECT_KERNEL_TIME)
  const sec duration = clock::now() - before;
  printf("\"kernel_exectime\":  %.2f\n", duration.count());
#endif

  finishup();

#if defined(HEARTBEAT_VERSIONING)
  // create dummy call here so loop_handler declarations
  // so that the function declaration will be emitted by the LLVM-IR
  loop_handler(nullptr, 0, nullptr, 0, 0, 0, 0);
#endif

  printf("result=%lu\n", result);
  return 0;
}
