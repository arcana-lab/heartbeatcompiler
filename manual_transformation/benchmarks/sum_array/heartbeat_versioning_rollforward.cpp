#include "rollforward_handler.hpp"
#include <cassert>

#define CACHELINE     8
#define LIVE_OUT_ENV  1

#define NUM_LEVELS  1
#define LEVEL_ZERO  0

#include "code_loop_slice_declaration.hpp"

extern uint64_t *constLiveIns;

// Transformed loop slice
int64_t HEARTBEAT_loop0_slice(uint64_t *cxt, uint64_t myIndex, uint64_t startIter, uint64_t maxIter) {
  // load const live-ins
  double *a = (double *)constLiveIns[0];

  // load my private copy of reduction array
  double *redArrLiveOut0 = (double *)cxt[LIVE_OUT_ENV];

  // allocate reduction array (as live-out environment) for kids
  double redArrLiveOut0Kids[2 * CACHELINE];
  cxt[LIVE_OUT_ENV] = (uint64_t)redArrLiveOut0Kids;

  int64_t rc = 0;
  double r_private = 0.0;
#if defined(CHUNK_LOOP_ITERATIONS) && CHUNKSIZE_0 != 1
  for (; startIter < maxIter; startIter += CHUNKSIZE_0) {
    uint64_t low = startIter;
    uint64_t high = std::min(maxIter, startIter + CHUNKSIZE_0);

    for (; low < high; low++) {
      r_private += a[low];
    }
    if (low == maxIter) {
      break;
    }

#if defined(ENABLE_ROLLFORWARD)
    __rf_handle_optimized_wrapper(rc, cxt, low - 1, maxIter, &HEARTBEAT_loop0_slice);
#else
    rc = loop_handler_optimized(cxt, low - 1, maxIter, &HEARTBEAT_loop0_slice);
#endif
    if (rc > 0) {
      break;
    }
  }
#else
  for (; startIter < maxIter; startIter++) {
    r_private += a[startIter];

#if defined(ENABLE_ROLLFORWARD)
    __rf_handle_optimized_wrapper(rc, cxt, startIter, maxIter, &HEARTBEAT_loop0_slice);
#else
    rc = loop_handler_optimized(cxt, startIter, maxIter, &HEARTBEAT_loop0_slice);
#endif
    if (rc > 0) {
      maxIter = startIter + 1;
    }
  }
#endif

  // reduction
  if (rc == 0) { // no heartbeat promotion happens
    redArrLiveOut0[myIndex * CACHELINE] = r_private;
  } else {  // splittingLevel == myLevel
    assert(rc == 1);
    redArrLiveOut0[myIndex * CACHELINE] = r_private + redArrLiveOut0Kids[0 * CACHELINE] + redArrLiveOut0Kids[1 * CACHELINE];
  }

  // reset live-out environment
  cxt[LIVE_OUT_ENV] = (uint64_t)redArrLiveOut0;

  return rc - 1;
}