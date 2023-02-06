#include "rollforward_handler.hpp"

#define SUB(array, row_sz, i, j) (array[i * row_sz + j])

#define CACHELINE     8
#define LIVE_IN_ENV   0

#define LEVEL_ZERO  0
#define LEVEL_ONE   1

#include "code_loop_slice_declaration.hpp"

extern int64_t HEARTBEAT_loop_1_0_leftover(uint64_t *, uint64_t *);
typedef int64_t(*leftoverTaskPointer)(uint64_t *, uint64_t *);
leftoverTaskPointer leftoverTasks[1] = {
  &HEARTBEAT_loop_1_0_leftover
};

int64_t HEARTBEAT_loop1_optimized(uint64_t *, uint64_t, uint64_t);
typedef int64_t(*leafTaskPointer)(uint64_t *, uint64_t, uint64_t);
leafTaskPointer leafTasks[1] = {
  &HEARTBEAT_loop1_optimized
};

extern uint64_t *constLiveIns;

// Transformed loop slice
int64_t HEARTBEAT_loop0_slice(uint64_t *cxts, uint64_t startIter, uint64_t maxIter) {
  // load const live-ins
  int vertices = (int)constLiveIns[1];

  int64_t rc = 0;
#if defined(CHUNK_LOOP_ITERATIONS)
  int low, high;

  // store into live-in environment for loop1
  cxts[LEVEL_ONE * CACHELINE + LIVE_IN_ENV] = (uint64_t)&low;

  for (; ;) {
    low = (int)startIter;
    high = (int)std::min(maxIter, startIter + CHUNKSIZE_0);

    for (; low < high; low++) {
      rc = HEARTBEAT_loop1_slice(cxts, low, maxIter, 0, (uint64_t)vertices);
      if (rc > 0) {
        high = low + 1;
      }
    }

    // exit the chunk execution when either
    // 1. heartbeat promotion happens at a higher nested level and in the process of returnning
    // 2. all iterations are finished
    if (rc > 0 || low == (int)maxIter) {
      break;
    }

#if defined(ENABLE_ROLLFORWARD)
    __rf_handle_wrapper(rc, cxts, LEVEL_ZERO, leftoverTasks, nullptr, (uint64_t)(low - 1), maxIter, 0, 0);
#else
    rc = loop_handler(cxts, LEVEL_ZERO, leftoverTasks, nullptr, (uint64_t)(low - 1), maxIter, 0, 0);
#endif
    if (rc > 0) {
      break;
    }
    startIter = low;
  }
#else
  // store into live-in environment for loop1
  cxts[LEVEL_ONE * CACHELINE + LIVE_IN_ENV] = (uint64_t)&startIter;

  for (; (int)startIter < (int)maxIter; startIter++) {
    rc = HEARTBEAT_loop1_slice(cxts, startIter, maxIter, 0, (uint64_t)vertices);
    if (rc > 0) {
      maxIter = startIter + 1;
      continue;
    }

#if defined(ENABLE_ROLLFORWARD)
    __rf_handle_wrapper(rc, cxts, LEVEL_ZERO, leftoverTasks, nullptr, startIter, maxIter, 0, 0);
#else
    rc = loop_handler(cxts, LEVEL_ZERO, leftoverTasks, nullptr, startIter, maxIter, 0, 0);
#endif
    if (rc > 0) {
      maxIter = startIter + 1;
    }
  }
#endif

  return rc - 1;
}

int64_t HEARTBEAT_loop1_slice(uint64_t *cxts, uint64_t startIter0, uint64_t maxIter0, uint64_t startIter, uint64_t maxIter) {
  // load const live-ins
  int *dist = (int *)constLiveIns[0];
  int vertices = (int)constLiveIns[1];
  int via = (int)constLiveIns[2];

  // load live-in environment
  int from = *(int *)cxts[LEVEL_ONE * CACHELINE + LIVE_IN_ENV];

  int64_t rc = 0;
#if defined(CHUNK_LOOP_ITERATIONS)
  int low, high;
  for (; ;) {
    low = (int)startIter;
    high = (int)std::min(maxIter, startIter + CHUNKSIZE_1);

    for (; low < high; low++) {
      if ((from != low) && (from != via) && (low != via)) {
        SUB(dist, vertices, from, low) =
          std::min(SUB(dist, vertices, from, low),
                  SUB(dist, vertices, from, via) + SUB(dist, vertices, via, low));
      }
    }
    if (low == (int)maxIter) {
      break;
    }

#if defined(ENABLE_ROLLFORWARD)
    __rf_handle_wrapper(rc, cxts, LEVEL_ONE, leftoverTasks, leafTasks, startIter0, maxIter0, uint64_t(low - 1), maxIter);
#else
    rc = loop_handler(cxts, LEVEL_ONE, leftoverTasks, leafTasks, startIter0, maxIter0, uint64_t(low - 1), maxIter);
#endif
    if (rc > 0) {
      break;
    }
    startIter = low;
  }
#else
  for (; (int)startIter < (int)maxIter; startIter++) {
    if ((from != (int)startIter) && (from != via) && ((int)startIter != via)) {
      SUB(dist, vertices, from, (int)startIter) =
        std::min(SUB(dist, vertices, from, (int)startIter),
                SUB(dist, vertices, from, via) + SUB(dist, vertices, via, (int)startIter));
    }

#if defined(ENABLE_ROLLFORWARD)
    __rf_handle_wrapper(rc, cxts, LEVEL_ONE, leftoverTasks, leafTasks, startIter0, maxIter0, startIter, maxIter);
#else
    rc = loop_handler(cxts, LEVEL_ONE, leftoverTasks, leafTasks, startIter0, maxIter0, startIter, maxIter);
#endif
    if (rc > 0) {
      maxIter = startIter + 1;
    }
  }
#endif

  return rc - 1;
}

// Transformed optimized loop slice
int64_t HEARTBEAT_loop1_optimized(uint64_t *cxt, uint64_t startIter, uint64_t maxIter) {
  // load const live-ins
  int *dist = (int *)constLiveIns[0];
  int vertices = (int)constLiveIns[1];
  int via = (int)constLiveIns[2];

  // load live-in environment
  int from = *(int *)cxt[LIVE_IN_ENV];

  int64_t rc = 0;
#if defined(CHUNK_LOOP_ITERATIONS)
  int low, high;
  for (; ;) {
    low = (int)startIter;
    high = (int)std::min(maxIter, startIter + CHUNKSIZE_1);

    for (; low < high; low++) {
      if ((from != low) && (from != via) && (low != via)) {
        SUB(dist, vertices, from, low) =
          std::min(SUB(dist, vertices, from, low),
                  SUB(dist, vertices, from, via) + SUB(dist, vertices, via, low));
      }
    }
    if (low == (int)maxIter) {
      break;
    }

#if defined(ENABLE_ROLLFORWARD)
    __rf_handle_optimized_wrapper(rc, cxt, (uint64_t)(low - 1), maxIter, &HEARTBEAT_loop1_optimized);
#else
    rc = loop_handler_optimized(cxt, (uint64_t)(low - 1), maxIter, &HEARTBEAT_loop1_optimized);
#endif
    if (rc > 0) {
      break;
    }
    startIter = low;
  }
#else
  for (; (int)startIter < (int)maxIter; startIter++) {
    if ((from != (int)startIter) && (from != via) && ((int)startIter != via)) {
      SUB(dist, vertices, from, (int)startIter) =
        std::min(SUB(dist, vertices, from, (int)startIter),
                SUB(dist, vertices, from, via) + SUB(dist, vertices, via, (int)startIter));
    }
#if defined(ENABLE_ROLLFORWARD)
    __rf_handle_optimized_wrapper(rc, startIter, maxIter, &HEARTBEAT_loop1_optimized);
#else
    rc = loop_handler_optimized(cxt, startIter, maxIter, &HEARTBEAT_loop1_optimized);
#endif
    if (rc > 0) {
      maxIter = startIter + 1;
    }
  }
#endif

  return rc - 1;
}
