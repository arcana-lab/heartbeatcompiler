#include "heartbeat_manual.hpp"
#include "loop_handler.hpp"
#include <cstdint>
#include <alloca.h>

#define NUM_LEVELS_NEST0 1
#define CACHELINE 8
#define LIVE_OUT_ENV 1

namespace kmeans {

double HEARTBEAT_nest0_loop0(float **feature,
                             int nfeatures,
                             int npoints,
                             int nclusters,
                             int *membership,
                             float **clusters,
                             int **partial_new_centers_len,
                             float ***partial_new_centers);
int64_t HEARTBEAT_nest0_loop0_slice(void *cxts, uint64_t myIndex, uint64_t startIter, uint64_t maxIter);

bool run_heartbeat = true;
uint64_t *constLiveIns_nest0;

// Outlined loops
double HEARTBEAT_nest0_loop0(float **feature,
                             int nfeatures,
                             int npoints,
                             int nclusters,
                             int *membership,
                             float **clusters,
                             int **partial_new_centers_len,
                             float ***partial_new_centers) {
  double delta = 0.0;

  if (run_heartbeat) {
    run_heartbeat = false;

    // allocate const live-ins
    constLiveIns_nest0 = (uint64_t *)alloca(sizeof(uint64_t) * 7);
    constLiveIns_nest0[0] = (uint64_t)feature;
    constLiveIns_nest0[1] = (uint64_t)nfeatures;
    constLiveIns_nest0[2] = (uint64_t)nclusters;
    constLiveIns_nest0[3] = (uint64_t)membership;
    constLiveIns_nest0[4] = (uint64_t)clusters;
    constLiveIns_nest0[5] = (uint64_t)partial_new_centers_len;
    constLiveIns_nest0[6] = (uint64_t)partial_new_centers;

    // allocate cxts
    uint64_t cxts[NUM_LEVELS_NEST0 * CACHELINE];

    // allocate reduction array (as live-out environment) for loop0
    double redArrLoop0LiveOut0[1 * CACHELINE];
    cxts[LIVE_OUT_ENV] = (uint64_t)redArrLoop0LiveOut0;

    // invoke nest0_loop0 in heartbeat form
    HEARTBEAT_nest0_loop0_slice((void *)cxts, 0, 0, (uint64_t)npoints);

    // reduction
    delta += redArrLoop0LiveOut0[0 * CACHELINE];

    run_heartbeat = true;
  } else {
    for (int i=0; i<npoints; i++) {
      /* find the index of nestest cluster centers */
      int index = find_nearest_point(feature[i], nfeatures, clusters, nclusters);
      /* if membership changes, increase delta by 1 */
      if (membership[i] != index) delta += 1.0;

      /* assign the membership to object i */
      membership[i] = index;

      /* update new cluster centers : sum of objects located within */
      // new_centers_len[index]++;
      partial_new_centers_len[sched_getcpu()][index]++;
      for (int j=0; j<nfeatures; j++)          
        // new_centers[index][j] += feature[i][j];
        partial_new_centers[sched_getcpu()][index][j] += feature[i][j];
    }
  }

  return delta;
}

// Transformed loops
int64_t HEARTBEAT_nest0_loop0_slice(void *cxts, uint64_t myIndex, uint64_t startIter, uint64_t maxIter) {
  // load const live-ins
  float **feature = (float **)constLiveIns_nest0[0];
  int nfeatures = (int)constLiveIns_nest0[1];
  int nclusters = (int)constLiveIns_nest0[2];
  int *membership = (int *)constLiveIns_nest0[3];
  float **clusters = (float **)constLiveIns_nest0[4];
  int **partial_new_centers_len = (int **)constLiveIns_nest0[5];
  float ***partial_new_centers = (float ***)constLiveIns_nest0[6];

  // load reduction array for live-outs and create a private copy
  double *redArrLoop0LiveOut0 = (double *)((uint64_t *)cxts)[LIVE_OUT_ENV];
  double live_out_0 = 0.0;

  // allocate reduction array (as live-out environment) for kids of loop0
  double redArrLoop0LiveOut0Kids[2 * CACHELINE];
  ((uint64_t *)cxts)[LIVE_OUT_ENV] = (uint64_t)redArrLoop0LiveOut0Kids;

  int64_t rc = 0;
#if defined(CHUNK_LOOP_ITERATIONS) && CHUNKSIZE_0 != 1
  // here the predict to compare has to be '<' not '!=',
  // otherwise there's an infinite loop bug
  for (; startIter < maxIter; startIter += CHUNKSIZE_0) {
    uint64_t low = startIter;
    uint64_t high = maxIter < startIter + CHUNKSIZE_0 ? maxIter : startIter + CHUNKSIZE_0;
    for (; low < high; low++) {
      /* find the index of nestest cluster centers */
      int index = find_nearest_point(feature[low], nfeatures, clusters, nclusters);
      /* if membership changes, increase delta by 1 */
      if (membership[low] != index) live_out_0 += 1.0;

      /* assign the membership to object i */
      membership[low] = index;

      /* update new cluster centers : sum of objects located within */
      // new_centers_len[index]++;
      partial_new_centers_len[sched_getcpu()][index]++;
      for (int j=0; j<nfeatures; j++)          
        // new_centers[index][j] += feature[i][j];
        partial_new_centers[sched_getcpu()][index][j] += feature[low][j];
    }

    if (low == maxIter) {
      // early exit and don't call the loop_handler,
      // this avoids the overhead if the loop count is small
      break;
    }

#if defined(ENABLE_SOFTWARE_POLLING)
    rc = loop_handler_level1(cxts, &HEARTBEAT_nest0_loop0_slice, low - 1, maxIter);
#else
    __rf_handle_level1_wrapper(rc, cxts, &HEARTBEAT_nest0_loop0_slice, low - 1, maxIter);
#endif
    if (rc > 0) {
      break;
    }
  }
#else
  for (; startIter < maxIter; startIter++) {
    /* find the index of nestest cluster centers */
    int index = find_nearest_point(feature[startIter], nfeatures, clusters, nclusters);
    /* if membership changes, increase delta by 1 */
    if (membership[startIter] != index) live_out_0 += 1.0;

    /* assign the membership to object i */
    membership[startIter] = index;

    /* update new cluster centers : sum of objects located within */
    // new_centers_len[index]++;
    partial_new_centers_len[sched_getcpu()][index]++;
    for (int j=0; j<nfeatures; j++)          
      // new_centers[index][j] += feature[i][j];
      partial_new_centers[sched_getcpu()][index][j] += feature[startIter][j];

#if defined(ENABLE_SOFTWARE_POLLING)
    rc = loop_handler_level1(cxts, &HEARTBEAT_nest0_loop0_slice, startIter, maxIter);
#else
    __rf_handle_level1_wrapper(rc, cxts, &HEARTBEAT_nest0_loop0_slice, startIter, maxIter);
#endif
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
    redArrLoop0LiveOut0[myIndex * CACHELINE] = live_out_0;
  } else {        // splittingLevel == myLevel
    redArrLoop0LiveOut0[myIndex * CACHELINE] = live_out_0 + redArrLoop0LiveOut0Kids[0 * CACHELINE] + redArrLoop0LiveOut0Kids[1 * CACHELINE];
  }

  // reset live-out environment
  ((uint64_t *)cxts)[LIVE_OUT_ENV] = (uint64_t)redArrLoop0LiveOut0;

  return rc - 1;
}

} // namespace kmeans
