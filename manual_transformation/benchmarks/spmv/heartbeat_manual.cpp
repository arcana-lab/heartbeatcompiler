#include "heartbeat_manual.hpp"
#include "loop_handler.hpp"
#include <cstdint>
#include <alloca.h>

#define NUM_LEVELS_NEST0 2
#define LEVEL_ZERO 0
#define LEVEL_ONE 1
#define CACHELINE 8
#define LIVE_OUT_ENV 1
#define START_ITER 0
#define MAX_ITER 1

namespace spmv {

void HEARTBEAT_nest0_loop0(double *val, uint64_t *row_ptr, uint64_t *col_ind, double* x, double* y, uint64_t n);
double HEARTBEAT_nest0_loop1(double *val, uint64_t *row_ptr, uint64_t *col_ind, double* x, double* y, int i);

int64_t HEARTBEAT_nest0_loop0_slice(void *cxts, uint64_t myIndex, uint64_t startIter, uint64_t maxIter);
inline void HEARTBEAT_nest0_loop0_slice_wrapper(void *cxts, uint64_t myIndex, uint64_t startIter, uint64_t maxIter) {
  HEARTBEAT_nest0_loop0_slice(cxts, myIndex, startIter, maxIter);
}
int64_t HEARTBEAT_nest0_loop1_slice(void *cxts, uint64_t myIndex, uint64_t s0, uint64_t m0, uint64_t startIter, uint64_t maxIter);
inline void HEARTBEAT_nest0_loop1_slice_wrapper(void *cxts, uint64_t myIndex, uint64_t startIter, uint64_t maxIter) {
  HEARTBEAT_nest0_loop1_slice(cxts, myIndex, 0, 0, startIter, maxIter);
}
typedef void (*sliceTasksWrapperPointer)(void *, uint64_t, uint64_t, uint64_t);
sliceTasksWrapperPointer slice_tasks_nest0[2] = {
  &HEARTBEAT_nest0_loop0_slice_wrapper,
  &HEARTBEAT_nest0_loop1_slice_wrapper
};

void HEARTBEAT_nest0_loop_1_0_leftover(void *cxts, uint64_t myIndex, void *itersArr);
typedef void (*leftoverTasksPointer)(void *, uint64_t, void *);
leftoverTasksPointer leftover_tasks_nest0[1] = {
  &HEARTBEAT_nest0_loop_1_0_leftover
};

uint64_t leftover_selector_nest0(uint64_t receivingLevel, uint64_t splittingLevel) {
  return 0;
}

bool run_heartbeat = true;
uint64_t *constLiveIns_nest0;

// Outlined loops
void HEARTBEAT_nest0_loop0(double *val, uint64_t *row_ptr, uint64_t *col_ind, double* x, double* y, uint64_t n) {
  if (run_heartbeat) {
    run_heartbeat = false;

    // allocate const live-ins
    constLiveIns_nest0 = (uint64_t *)alloca(sizeof(uint64_t) * 5);
    constLiveIns_nest0[0] = (uint64_t)row_ptr;
    constLiveIns_nest0[1] = (uint64_t)y;
    constLiveIns_nest0[2] = (uint64_t)val;
    constLiveIns_nest0[3] = (uint64_t)col_ind;
    constLiveIns_nest0[4] = (uint64_t)x;

    // allocate cxts
    uint64_t cxts[NUM_LEVELS_NEST0 * CACHELINE];

    // invoke nest0_loop0 in heartbeat form
    HEARTBEAT_nest0_loop0_slice((void *)cxts, 0, 0, n);

    run_heartbeat = true;
  } else {
    for (uint64_t i = 0; i < n; i++) { // row loop
      double r = 0.0;
      r += HEARTBEAT_nest0_loop1(val, row_ptr, col_ind, x, y, i);
      y[i] = r;
    }
  }
}

double HEARTBEAT_nest0_loop1(double *val, uint64_t *row_ptr, uint64_t *col_ind, double* x, double* y, int i) {
  double r = 0.0;
  for (uint64_t k = row_ptr[i]; k < row_ptr[i + 1]; k++) { // col loop
    r += val[k] * x[col_ind[k]];
  }
  return r;
}

// Transformed loops
int64_t HEARTBEAT_nest0_loop0_slice(void *cxts, uint64_t myIndex, uint64_t startIter, uint64_t maxIter) {
  // load const live-ins
  uint64_t *row_ptr = (uint64_t *)constLiveIns_nest0[0];
  double *y = (double *)constLiveIns_nest0[1];

  // allocate reduction array (as live-out environment) for loop1
  double redArrLoop1LiveOut0[1 * CACHELINE];
  ((uint64_t *)cxts)[LEVEL_ONE * CACHELINE + LIVE_OUT_ENV] = (uint64_t)redArrLoop1LiveOut0;

  int64_t rc = 0;
#if defined(CHUNK_LOOP_ITERATIONS) && CHUNKSIZE_0 != 1
  for (; startIter < maxIter; startIter += CHUNKSIZE_0) {
    uint64_t low = startIter;
    uint64_t high = maxIter < startIter + CHUNKSIZE_0 ? maxIter : startIter + CHUNKSIZE_0;
    for (; low < high; low++) {
      double r = 0.0;
      rc = HEARTBEAT_nest0_loop1_slice(cxts, 0, low, maxIter, row_ptr[low], row_ptr[low + 1]);
      if (rc > 0) {
        // update the exit condition here because there might
        // be tail work to finish
        high = low + 1;
      }
      r += redArrLoop1LiveOut0[0 * CACHELINE];
      y[low] = r;
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
  for (; startIter < maxIter; startIter++) {
    double r = 0.0;
    rc = HEARTBEAT_nest0_loop1_slice(cxts, 0, startIter, maxIter, row_ptr[startIter], row_ptr[startIter + 1]);
    if (rc > 0) {
      // update the exit condition here because there might
      // be tail work to finish
      maxIter = startIter + 1;
    }
    r += redArrLoop1LiveOut0[0 * CACHELINE];
    y[startIter] = r;

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
  double *val = (double *)constLiveIns_nest0[2];
  uint64_t *col_ind = (uint64_t *)constLiveIns_nest0[3];
  double *x = (double *)constLiveIns_nest0[4];

  // load reduction array for live-outs and create a private copy variable
  double *redArrLoop1LiveOut0 = (double *)((uint64_t *)cxts)[LEVEL_ONE * CACHELINE + LIVE_OUT_ENV];
  double live_out_0 = 0.0;

  // allocate reduction array (as live-out environment) for kids of loop1
  double redArrLoop1LiveOut0Kids[2 * CACHELINE];
  ((uint64_t *)cxts)[LEVEL_ONE * CACHELINE + LIVE_OUT_ENV] = (uint64_t)redArrLoop1LiveOut0Kids;

  int64_t rc = 0;
#if defined(CHUNK_LOOP_ITERATIONS) && CHUNKSIZE_1 != 1
  for (; startIter < maxIter; startIter += CHUNKSIZE_1) {
    uint64_t low = startIter;
    uint64_t high = maxIter < startIter + CHUNKSIZE_1 ? maxIter : startIter + CHUNKSIZE_1;
    for (; low < high; low++) {
      live_out_0 += val[low] * x[col_ind[low]];
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
    live_out_0 += val[startIter] * x[col_ind[startIter]];

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

  // rc from a leaf loop slice can only be 0 or 1,
  // however, when considering a non-leaf loop need to
  // consider the scenario of returning up
  // reduction
  if (rc == 0) {  // no heartbeat promotion happens
    redArrLoop1LiveOut0[myIndex * CACHELINE] = live_out_0;
  } else {        // splittingLevel == myLevel
    redArrLoop1LiveOut0[myIndex * CACHELINE] = live_out_0 + redArrLoop1LiveOut0Kids[0 * CACHELINE] + redArrLoop1LiveOut0Kids[1 * CACHELINE];
  }

  // reset live-out environment
  ((uint64_t *)cxts)[LEVEL_ONE * CACHELINE + LIVE_OUT_ENV] = (uint64_t)redArrLoop1LiveOut0;

  return rc - 1;
}

// Leftover tasks
void HEARTBEAT_nest0_loop_1_0_leftover(void *cxts, uint64_t myIndex, void *itersArr) {
  // load startIter and maxIter
  uint64_t startIter = ((uint64_t *)itersArr)[LEVEL_ONE * 2 + START_ITER];
  uint64_t maxIter   = ((uint64_t *)itersArr)[LEVEL_ONE * 2 + MAX_ITER];

  HEARTBEAT_nest0_loop1_slice(cxts, 0, 0, 0, startIter, maxIter);

  return;
}

} // namespace spmv
