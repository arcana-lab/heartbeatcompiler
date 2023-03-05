#include "heartbeat_manual.hpp"
#include "loop_handler.hpp"
#include <cstdint>
#include <alloca.h>

namespace plus_reduce_array {

#define NUM_LEVELS 1
#define CACHELINE 8
#define LIVE_OUT_ENV 1

double HEARTBEAT_nest0_loop0(double *a, uint64_t lo, uint64_t hi);
int HEARTBEAT_nest0_loop0_slice(uint64_t *cxts, uint64_t myIndex, uint64_t startIter, uint64_t maxIter);

bool run_heartbeat = true;
uint64_t *constLiveIns;

// Outlined loops
double HEARTBEAT_nest0_loop0(double *a, uint64_t lo, uint64_t hi) {
  double r = 0.0;

  if (run_heartbeat) {
    run_heartbeat = false;

    // allocate const live-ins
    constLiveIns = (uint64_t *)alloca(sizeof(uint64_t) * 1);
    constLiveIns[0] = (uint64_t)a;

    // allocate cxts
    uint64_t cxts[NUM_LEVELS * CACHELINE];

    // allocate reduction array (as live-out environment) for loop0
    double redArrLoop0LiveOut0[1 * CACHELINE];
    cxts[LIVE_OUT_ENV] = (uint64_t)redArrLoop0LiveOut0;

    // invoke loop0 in heartbeat form
    HEARTBEAT_nest0_loop0_slice(cxts, 0, lo, hi);

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

int HEARTBEAT_nest0_loop0_slice(uint64_t *cxts, uint64_t myIndex, uint64_t startIter, uint64_t maxIter) {
  // load const live-ins
  double *a = (double *)constLiveIns[0];

  // load reduction array for live-outs
  double *redArrLoop0LiveOut0 = (double *)cxts[LIVE_OUT_ENV];

  // allocate reduction array (as live-out environment) for kids of loop0
  double redArrLoop0LiveOut0Kids[2 * CACHELINE];
  cxts[LIVE_OUT_ENV] = (uint64_t)redArrLoop0LiveOut0Kids;

  int rc = 0;
  double r_private = 0.0;
#if defined(CHUNK_LOOP_ITERATIONS) && CHUNKSIZE_0 != 1
  // here the predict to compare has to be '<' not '!=',
  // otherwise there's an infinite loop bug
  for (; startIter < maxIter; startIter += CHUNKSIZE_0) {
    uint64_t low = startIter;
    uint64_t high = maxIter < startIter + CHUNKSIZE_0 ? maxIter : startIter + CHUNKSIZE_0;
    for (; low < high; low++) {
      r_private += a[low];
    }

    if (low == maxIter) {
      // early exit and don't call the loop_handler,
      // this avoids the overhead if the loop count is small
      break;
    }

    rc = loop_handler_level1(cxts, &HEARTBEAT_nest0_loop0_slice, low - 1, maxIter);
    if (rc > 0) {
      break;
    }
  }
#else
  for (; startIter != maxIter; startIter++) {
    r_private += a[startIter];

    rc = loop_handler_level1(cxts, &HEARTBEAT_nest0_loop0_slice, startIter, maxIter);
    if (rc > 0) {
      // call to loop_handler block post-dominates the body of the loop
      // so there's no need to modify the exit condition of the loop
      break;
    }
  }
#endif

  // rc from a leaf loop slice can only be 0 or 1,
  // however, when considering a non-leaf loop need to
  // consider the scenario of returning up
  // reduction
  if (rc == 0) {  // no heartbeat promotion happens
    redArrLoop0LiveOut0[myIndex * CACHELINE] = r_private;
  } else {        // splittingLevel == myLevel
    redArrLoop0LiveOut0[myIndex * CACHELINE] = r_private + redArrLoop0LiveOut0Kids[0 * CACHELINE] + redArrLoop0LiveOut0Kids[1 * CACHELINE];
  }

  // reset live-out environment
  cxts[LIVE_OUT_ENV] = (uint64_t)redArrLoop0LiveOut0;

  return rc - 1;
}

} // namespace plus_reduce_array
