#include "heartbeat_manual.hpp"
#include "loop_handler.hpp"
#include <cstdint>

#define unlikely(expr) __builtin_expect(!!(expr), 0)
#define NUM_LEVELS 1
#define LEVEL_ZERO 0
#define CACHELINE 8
#define START_ITER 0
#define MAX_ITER 1
#define LIVE_OUT_ENV 3

#if defined(ENABLE_ROLLFORWARD)
extern volatile int __rf_signal;

extern "C" {

__attribute__((used))
__attribute__((always_inline))
static bool __rf_test (void) {
  int yes;
  asm volatile ("movl $__rf_signal, %0" : "=r" (yes) : : );
  return yes == 1;
}

}
#endif

namespace plus_reduce_array {

double HEARTBEAT_loop0(double *a, uint64_t lo, uint64_t hi);

int64_t HEARTBEAT_loop0_slice(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem);
typedef int64_t (*sliceTasksPointer)(uint64_t *, uint64_t *, uint64_t, task_memory_t *);
sliceTasksPointer slice_tasks[1] = {
  &HEARTBEAT_loop0_slice
};

#if defined(RUN_HEARTBEAT)
  bool run_heartbeat = true;
#else
  bool run_heartbeat = false;
#endif

// Outlined loops
double HEARTBEAT_loop0(double *a, uint64_t lo, uint64_t hi) {
  double r = 0.0;

  if (run_heartbeat) {
    run_heartbeat = false;

    // allocate const live-ins
    uint64_t constLiveIns[1];
    constLiveIns[0] = (uint64_t)a;

    // allocate cxts
    uint64_t cxts[NUM_LEVELS * CACHELINE];

    // allocate reduction array (as live-out environment) for loop0
    double redArrLoop0LiveOut0[1 * CACHELINE];
    cxts[LIVE_OUT_ENV] = (uint64_t)redArrLoop0LiveOut0;

    // set start/max iterations for loop0
    cxts[LEVEL_ZERO * CACHELINE + START_ITER] = (uint64_t)lo;
    cxts[LEVEL_ZERO * CACHELINE + MAX_ITER] = (uint64_t)hi;

    // allocate the task memory struct and initialize
    task_memory_t tmem;
    heartbeat_start(&tmem);

    // invoke loop0 in heartbeat form
    HEARTBEAT_loop0_slice(cxts, constLiveIns, 0, &tmem);

    // reduction
    r += redArrLoop0LiveOut0[0 * CACHELINE];

    run_heartbeat = true;
  } else {
    for (uint64_t i = lo; i != hi; i++) {
      r += a[i];
    }
  }

  return r;
}

// Transformed loops
int64_t HEARTBEAT_loop0_slice(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem) {
  // load start/max iterations
  uint64_t startIter = cxts[LEVEL_ZERO * CACHELINE + START_ITER];
  uint64_t maxIter = cxts[LEVEL_ZERO * CACHELINE + MAX_ITER];

  // load const live-ins
  double *a = (double *)constLiveIns[0];

  // load reduction array for live-outs and create a private copy
  double *redArrLoop0LiveOut0 = (double *)cxts[LIVE_OUT_ENV];
  double live_out_0 = 0.0;

  // allocate reduction array (as live-out environment) for kids of loop0
  double redArrLoop0LiveOut0Kids[2 * CACHELINE];
  cxts[LIVE_OUT_ENV] = (uint64_t)redArrLoop0LiveOut0Kids;

  int64_t rc = 0;
#if defined(CHUNK_LOOP_ITERATIONS)
  // here the predict to compare has to be '<' not '!=',
  // otherwise there's an infinite loop bug
  uint64_t chunksize;
  for (; startIter < maxIter; startIter += chunksize) {
    chunksize = get_chunksize(tmem);
    uint64_t low = startIter;
    uint64_t high = maxIter < startIter + chunksize ? maxIter : startIter + chunksize;
    for (; low < high; low++) {
      live_out_0 += a[low];
    }

#if defined(ENABLE_HEARTBEAT)
    // early exit and don't call the loop_handler,
    // this avoids the overhead if the loop count is small
    if (update_and_has_remaining_chunksize(tmem, high - startIter, chunksize)) {
      break;
    }

#if defined(ENABLE_SOFTWARE_POLLING)
    if (unlikely(heartbeat_polling(tmem))) {
      cxts[LEVEL_ZERO * CACHELINE + START_ITER] = low - 1;
      rc = loop_handler(
        cxts, constLiveIns, LEVEL_ZERO, NUM_LEVELS, tmem,
        slice_tasks, nullptr, nullptr
      );
      if (rc > 0) {
        break;
      }
    }
#else
    if(unlikely(__rf_test())) {
      cxts[LEVEL_ZERO * CACHELINE + START_ITER] = low - 1;
      __rf_handle_wrapper(
        rc, cxts, constLiveIns, LEVEL_ZERO, NUM_LEVELS, tmem,
        slice_tasks, nullptr, nullptr
      );
      if (rc > 0) {
        break;
      }
    }
#endif
#endif
  }
#else
  for (; startIter != maxIter; startIter++) {
    live_out_0 += a[startIter];

#if defined(ENABLE_HEARTBEAT)
#if defined(ENABLE_SOFTWARE_POLLING)
    if (unlikely(heartbeat_polling(tmem))) {
      cxts[LEVEL_ZERO * CACHELINE + START_ITER] = startIter;
      rc = loop_handler(
        cxts, constLiveIns, LEVEL_ZERO, NUM_LEVELS, tmem,
        slice_tasks, nullptr, nullptr
      );
      if (rc > 0) {
        break;
      }
    }
#else
    if(unlikely(__rf_test())) {
      cxts[LEVEL_ZERO * CACHELINE + START_ITER] = startIter;
      __rf_handle_wrapper(
        rc, cxts, constLiveIns, LEVEL_ZERO, NUM_LEVELS, tmem,
        slice_tasks, nullptr, nullptr
      );
      if (rc > 0) {
        break;
      }
    }
#endif
#endif
  }
#endif

  // rc from a leaf loop slice can only be 0 or 1,
  // however, when considering a non-leaf loop need to
  // consider the scenario of returning up
  // reduction
  if (rc == 0) {  // no heartbeat promotion happens
    redArrLoop0LiveOut0[myIndex * CACHELINE] = live_out_0;
  } else {        // splittingLevel == myLevel
    redArrLoop0LiveOut0[myIndex * CACHELINE] = live_out_0 + redArrLoop0LiveOut0Kids[0 * CACHELINE] + redArrLoop0LiveOut0Kids[1 * CACHELINE];
  }

  // reset live-out environment
  cxts[LIVE_OUT_ENV] = (uint64_t)redArrLoop0LiveOut0;

  return rc - 1;
}

} // namespace plus_reduce_array
