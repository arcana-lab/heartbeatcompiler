#include "heartbeat_manual.hpp"
#include "loop_handler.hpp"
#include <cstdint>
#include <alloca.h>

#define SUB(array, row_sz, i, j) (array[i * row_sz + j])

#define unlikely(expr) __builtin_expect(!!(expr), 0)
#define NUM_LEVELS_NEST0 2
#define LEVEL_ZERO 0
#define LEVEL_ONE 1
#define CACHELINE 8
#define START_ITER 0
#define MAX_ITER 1
#define LIVE_IN_ENV 2

namespace floyd_warshall {

void HEARTBEAT_nest0_loop0(int *dist, int vertices, int via);
void HEARTBEAT_nest0_loop1(int *dist, int vertices, int via, int from);

int64_t HEARTBEAT_nest0_loop0_slice(uint64_t *cxts, uint64_t myIndex);
int64_t HEARTBEAT_nest0_loop1_slice(uint64_t *cxts, uint64_t myIndex);
typedef int64_t (*sliceTasksPointer)(uint64_t *, uint64_t);
sliceTasksPointer slice_tasks_nest0[2] = {
  &HEARTBEAT_nest0_loop0_slice,
  &HEARTBEAT_nest0_loop1_slice
};

void HEARTBEAT_nest0_loop_1_0_leftover(uint64_t *cxts, uint64_t myIndex);
typedef void (*leftoverTasksPointer)(uint64_t *, uint64_t);
leftoverTasksPointer leftover_tasks_nest0[1] = {
  &HEARTBEAT_nest0_loop_1_0_leftover
};

uint64_t leftover_selector_nest0(uint64_t receivingLevel, uint64_t splittingLevel) {
  return 0;
}

bool run_heartbeat = true;
uint64_t *constLiveIns_nest0;

// Outlined loops
void HEARTBEAT_nest0_loop0(int *dist, int vertices, int via) {
  if (run_heartbeat) {
    run_heartbeat = false;

    // allocate const live-ins
    constLiveIns_nest0 = (uint64_t *)alloca(sizeof(uint64_t) * 3);
    constLiveIns_nest0[0] = (uint64_t)dist;
    constLiveIns_nest0[1] = (uint64_t)vertices;
    constLiveIns_nest0[2] = (uint64_t)via;

    // allocate cxts
    uint64_t cxts[NUM_LEVELS_NEST0 * CACHELINE];

    // set start/max iterations for loop0
    cxts[LEVEL_ZERO * CACHELINE + START_ITER] = (uint64_t)0;
    cxts[LEVEL_ZERO * CACHELINE + MAX_ITER] = (uint64_t)vertices;

    // invoke loop0 in heartbeat form
    HEARTBEAT_nest0_loop0_slice(cxts, 0);

    run_heartbeat = true;
  } else {
    for(int from = 0; from < vertices; from++) {
      HEARTBEAT_nest0_loop1(dist, vertices, via, from);
    }
  }
}

void HEARTBEAT_nest0_loop1(int *dist, int vertices, int via, int from) {
  for(int to = 0; to < vertices; to++) {
    if ((from != to) && (from != via) && (to != via)) {
      SUB(dist, vertices, from, to) = 
        std::min(SUB(dist, vertices, from, to), 
                  SUB(dist, vertices, from, via) + SUB(dist, vertices, via, to));
    }
  }
}

// Transformed loops
int64_t HEARTBEAT_nest0_loop0_slice(uint64_t *cxts, uint64_t myIndex) {
  // load start/max iterations
  uint64_t startIter = cxts[LEVEL_ZERO * CACHELINE + START_ITER];
  uint64_t maxIter = cxts[LEVEL_ZERO * CACHELINE + MAX_ITER];

  // load const live-ins
  int vertices = (int)constLiveIns_nest0[1];

  int64_t rc = 0;
#if defined(CHUNK_LOOP_ITERATIONS) && CHUNKSIZE_0 != 0
  uint64_t low, high;
  // store &live-in as live-in environment for loop1
  cxts[LEVEL_ONE * CACHELINE + LIVE_IN_ENV] = (uint64_t)&low;

  for (; startIter < maxIter; startIter += CHUNKSIZE_0) {
    low = startIter;
    high = maxIter < startIter + CHUNKSIZE_0 ? maxIter : startIter + CHUNKSIZE_0;
    for (; low < high; low++) {
      // store current iteration for loop0
      cxts[LEVEL_ZERO * CACHELINE + START_ITER] = low;
      // set start/max iterations for loop1
      cxts[LEVEL_ONE * CACHELINE + START_ITER] = (uint64_t)0;
      cxts[LEVEL_ONE * CACHELINE + MAX_ITER] = (uint64_t)vertices;
      rc = HEARTBEAT_nest0_loop1_slice(cxts, 0);
      if (rc > 0) {
        // update the exit condition here because there might
        // be tail work to finish
        high = low + 1;
      }
    }

    // exit the chunk execution when either
    // 1. heartbeat promotion happens at a higher nested level and in the process of returnning
    // 2. all iterations are finished
    if (rc > 0 || low == maxIter) {
      break;
    }

#if !defined(ENABLE_ROLLFORWARD)
    if (unlikely(heartbeat_polling())) {
      cxts[LEVEL_ZERO * CACHELINE + START_ITER] = low - 1;
      rc = loop_handler(
        cxts, LEVEL_ZERO,
        slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0
      );
      if (rc > 0) {
        break;
      }
    }
#else
    cxts[LEVEL_ZERO * CACHELINE + START_ITER] = low - 1;
    __rf_handle_wrapper(
      rc, cxts, LEVEL_ZERO,
      slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0
    );
    if (rc > 0) {
      break;
    }
#endif
  }
#else
  // store &live-in as live-in environment for loop1
  cxts[LEVEL_ONE * CACHELINE + LIVE_IN_ENV] = (uint64_t)&startIter;

  for (; startIter < maxIter; startIter++) {
    // store current iteration for loop0
    cxts[LEVEL_ZERO * CACHELINE + START_ITER] = startIter;
    // set start/max iterations for loop1
    cxts[LEVEL_ONE * CACHELINE + START_ITER] = (uint64_t)0;
    cxts[LEVEL_ONE * CACHELINE + MAX_ITER] = (uint64_t)vertices;
    rc = HEARTBEAT_nest0_loop1_slice(cxts, 0);
    if (rc > 0) {
      // update the exit condition here because there might
      // be tail work to finish
      maxIter = startIter + 1;
    }

    // check the status of rc because, might not need
    // to call the loop_handler in the process of returnning up
    if (rc > 0) {
      break;
    }

#if !defined(ENABLE_ROLLFORWARD)
    if (unlikely(heartbeat_polling())) {
      cxts[LEVEL_ZERO * CACHELINE + START_ITER] = startIter;
      rc = loop_handler(
        cxts, LEVEL_ZERO,
        slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0
      );
      if (rc > 0) {
        break;
      }
    }
#else
    cxts[LEVEL_ZERO * CACHELINE + START_ITER] = startIter;
    __rf_handle_wrapper(
      rc, cxts, LEVEL_ZERO,
      slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0
    );
    if (rc > 0) {
      break;
    }
#endif
  }
#endif

  return rc - 1;
}

int64_t HEARTBEAT_nest0_loop1_slice(uint64_t *cxts, uint64_t myIndex) {
  // load start/max iterations
  uint64_t startIter = cxts[LEVEL_ONE * CACHELINE + START_ITER];
  uint64_t maxIter = cxts[LEVEL_ONE * CACHELINE + MAX_ITER];

  // load const live-ins
  int *dist = (int *)constLiveIns_nest0[0];
  int vertices = (int)constLiveIns_nest0[1];
  int via = (int)constLiveIns_nest0[2];

  // load live-ins
  int from = *(int *)cxts[LEVEL_ONE * CACHELINE + LIVE_IN_ENV];

  int64_t rc = 0;
#if defined(CHUNK_LOOP_ITERATIONS) && CHUNKSIZE_1 != 0
  for (; startIter < maxIter; startIter += CHUNKSIZE_1) {
    uint64_t low = startIter;
    uint64_t high = maxIter < startIter + CHUNKSIZE_1 ? maxIter : startIter + CHUNKSIZE_1;
    for (; low < high; low++) {
      if ((from != low) && (from != via) && (low != via)) {
        SUB(dist, vertices, from, low) = 
          std::min(SUB(dist, vertices, from, low), 
                    SUB(dist, vertices, from, via) + SUB(dist, vertices, via, low));
      }
    }

    if (low == maxIter) {
      break;
    }

#if !defined(ENABLE_ROLLFORWARD)
    if (unlikely(heartbeat_polling())) {
      cxts[LEVEL_ONE * CACHELINE + START_ITER] = low - 1;
      rc = loop_handler(
        cxts, LEVEL_ONE,
        slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0
      );
      if (rc > 0) {
        break;
      }
    }
#else
    cxts[LEVEL_ONE * CACHELINE + START_ITER] = low - 1;
    __rf_handle_wrapper(
      rc, cxts, LEVEL_ONE,
      slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0
    );
    if (rc > 0) {
      break;
    }
#endif
  }
#else
  for(; startIter < maxIter; startIter++) {
    if ((from != startIter) && (from != via) && (startIter != via)) {
      SUB(dist, vertices, from, startIter) = 
        std::min(SUB(dist, vertices, from, startIter), 
                  SUB(dist, vertices, from, via) + SUB(dist, vertices, via, startIter));
    }

#if !defined(ENABLE_ROLLFORWARD)
    if (unlikely(heartbeat_polling())) {
      cxts[LEVEL_ONE * CACHELINE + START_ITER] = startIter;
      rc = loop_handler(
        cxts, LEVEL_ONE,
        slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0
      );
      if (rc > 0) {
        break;
      }
    }
#else
    cxts[LEVEL_ONE * CACHELINE + START_ITER] = startIter;
    __rf_handle_wrapper(
      rc, cxts, LEVEL_ONE,
      slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0
    );
    if (rc > 0) {
      break;
    }
#endif
  }
#endif

  return rc - 1;
}

// Leftover tasks
void HEARTBEAT_nest0_loop_1_0_leftover(uint64_t *cxts, uint64_t myIndex) {
  int64_t rc = 0;
  rc = HEARTBEAT_nest0_loop1_slice(cxts, myIndex);
  if (rc > 0) {
    return;
  }

  cxts[LEVEL_ZERO * CACHELINE + START_ITER]++;
  rc = HEARTBEAT_nest0_loop0_slice(cxts, myIndex);
  if (rc > 0) {
    return;
  }

  return;
}

} // namespace floyd_warshall
