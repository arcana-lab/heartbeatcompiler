#include "heartbeat_manual.hpp"
#include "loop_handler.hpp"
#include <alloca.h>

#define SUB(array, row_sz, i, j) (array[i * row_sz + j])

#define NUM_LEVELS 2
#define LEVEL_ZERO 0
#define LEVEL_ONE 1
#define CACHELINE 8
#define LIVE_IN_ENV 0
#define START_ITER 0
#define MAX_ITER 1

namespace floyd_warshall {

void HEARTBEAT_nest0_loop0(int *dist, int vertices, int via);
void HEARTBEAT_nest0_loop1(int *dist, int vertices, int via, int from);

int64_t HEARTBEAT_nest0_loop0_slice(void *cxts, uint64_t myIndex, uint64_t startIter, uint64_t maxIter);
inline int64_t HEARTBEAT_nest0_loop0_slice_wrapper(void *cxts, uint64_t myIndex, uint64_t startIter, uint64_t maxIter) {
  return HEARTBEAT_nest0_loop0_slice(cxts, myIndex, startIter, maxIter);
}
int64_t HEARTBEAT_nest0_loop1_slice(void *cxts, uint64_t myIndex, uint64_t s0, uint64_t m0, uint64_t startIter, uint64_t maxIter);
inline int64_t HEARTBEAT_nest0_loop1_slice_wrapper(void *cxts, uint64_t myIndex, uint64_t startIter, uint64_t maxIter) {
  return HEARTBEAT_nest0_loop1_slice(cxts, myIndex, 0, 0, startIter, maxIter);
}
typedef int64_t (*sliceTasksWrapperPointer)(void *, uint64_t, uint64_t, uint64_t);
sliceTasksWrapperPointer slice_tasks_nest0[2] = {
  &HEARTBEAT_nest0_loop0_slice_wrapper,
  &HEARTBEAT_nest0_loop1_slice_wrapper
};

int64_t HEARTBEAT_nest0_loop_1_0_leftover(void *cxts, uint64_t myIndex, void *itersArr);
typedef int64_t (*leftoverTasksPointer)(void *, uint64_t, void *);
leftoverTasksPointer leftover_tasks_nest0[1] = {
  &HEARTBEAT_nest0_loop_1_0_leftover
};

uint64_t leftover_selector_nest0(uint64_t splittingLevel, uint64_t receivingLevel) {
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
    uint64_t cxts[NUM_LEVELS * CACHELINE];

    // invoke loop0 in heartbeat form
    HEARTBEAT_nest0_loop0_slice((void *)cxts, 0, 0, (uint64_t)vertices);

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
int64_t HEARTBEAT_nest0_loop0_slice(void *cxts, uint64_t myIndex, uint64_t startIter, uint64_t maxIter) {
  // load const live-ins
  int vertices = (int)constLiveIns_nest0[1];

  int64_t rc = 0;
#if defined(CHUNK_LOOP_ITERATIONS) && CHUNKSIZE_0 != 1
  uint64_t low, high;
  // store &live-in as live-in environment for loop1
  ((uint64_t *)cxts)[LEVEL_ONE * CACHELINE + LIVE_IN_ENV] = (uint64_t)&low;

  for (; startIter < maxIter; startIter += CHUNKSIZE_0) {
    low = startIter;
    high = maxIter < startIter + CHUNKSIZE_0 ? maxIter : startIter + CHUNKSIZE_0;
    for (; low < high; low++) {
      rc = HEARTBEAT_nest0_loop1_slice(cxts, 0, low, maxIter, 0, (uint64_t)vertices);
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
    rc = loop_handler_level2(
      cxts, LEVEL_ZERO,
      slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0,
      low - 1, maxIter,
      0, 0
    );
#else
    __rf_handle_level2_wrapper(
      rc, cxts, LEVEL_ZERO,
      slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0,
      low - 1, maxIter,
      0, 0
    );
#endif
    if (rc > 0) {
      break;
    }
  }
#else
  // store &live-in as live-in environment for loop1
  ((uint64_t *)cxts)[LEVEL_ONE * CACHELINE + LIVE_IN_ENV] = (uint64_t)&startIter;

  for (; startIter < maxIter; startIter++) {
    rc = HEARTBEAT_nest0_loop1_slice(cxts, 0, startIter, maxIter, 0, (uint64_t)vertices);
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
    rc = loop_handler_level2(
      cxts, LEVEL_ZERO,
      slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0,
      startIter, maxIter,
      0, 0
    );
#else
    __rf_handle_level2_wrapper(
      rc, cxts, LEVEL_ZERO,
      slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0,
      startIter, maxIter,
      0, 0
    );
#endif
    if (rc > 0) {
      break;
    }
  }
#endif

  return rc - 1;
}

int64_t HEARTBEAT_nest0_loop1_slice(void *cxts, uint64_t myIndex, uint64_t s0, uint64_t m0, uint64_t startIter, uint64_t maxIter) {
  // load const live-ins
  int *dist = (int *)constLiveIns_nest0[0];
  int vertices = (int)constLiveIns_nest0[1];
  int via = (int)constLiveIns_nest0[2];

  // load live-ins
  int from = *(int *)((uint64_t *)cxts)[LEVEL_ONE * CACHELINE + LIVE_IN_ENV];

  int64_t rc = 0;
#if defined(CHUNK_LOOP_ITERATIONS) && CHUNKSIZE_1 != 1
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
    rc = loop_handler_level2(
      cxts, LEVEL_ONE,
      slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0,
      s0, m0,
      low - 1, maxIter
    );
#else
    __rf_handle_level2_wrapper(
      rc, cxts, LEVEL_ONE,
      slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0,
      s0, m0,
      low - 1, maxIter
    );
#endif
    if (rc > 0) {
      break;
    }
  }
#else
  for(; startIter < maxIter; startIter++) {
    if ((from != startIter) && (from != via) && (startIter != via)) {
      SUB(dist, vertices, from, startIter) = 
        std::min(SUB(dist, vertices, from, startIter), 
                  SUB(dist, vertices, from, via) + SUB(dist, vertices, via, startIter));
    }

#if !defined(ENABLE_ROLLFORWARD)
    rc = loop_handler_level2(
      cxts, LEVEL_ONE,
      slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0,
      s0, m0,
      startIter, maxIter
    );
#else
    __rf_handle_level2_wrapper(
      rc, cxts, LEVEL_ONE,
      slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0,
      s0, m0,
      startIter, maxIter
    );
#endif
    if (rc > 0) {
      break;
    }
  }
#endif

  return rc - 1;
}

// Leftover tasks
int64_t HEARTBEAT_nest0_loop_1_0_leftover(void *cxts, uint64_t myIndex, void *itersArr) {
  // load startIter and maxIter
  uint64_t startIter = ((uint64_t *)itersArr)[LEVEL_ONE * 2 + START_ITER];
  uint64_t maxIter   = ((uint64_t *)itersArr)[LEVEL_ONE * 2 + MAX_ITER];

  HEARTBEAT_nest0_loop1_slice(cxts, 0, 0, 0, startIter, maxIter);

  return 0;
}

} // namespace floyd_warshall
