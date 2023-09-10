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

namespace kmeans {

double HEARTBEAT_loop0(float **feature,
                       int nfeatures,
                       int npoints,
                       int nclusters,
                       int *membership,
                       float **clusters,
                       int *new_centers_len,
                       float **new_centers);

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

// Entry function
float** kmeans_hb_manual(float **feature,    /* in: [npoints][nfeatures] */
                          int     nfeatures,
                          int     npoints,
                          int     nclusters,
                          float   threshold,
                          int    *membership) /* out: [npoints] */
{

  int      i, j, n=0, index, loop=0;
  int     *new_centers_len; /* [nclusters]: no. of points in each cluster */
  float    delta;
  float  **clusters;   /* out: [nclusters][nfeatures] */
  float  **new_centers;     /* [nclusters][nfeatures] */
  

  /* allocate space for returning variable clusters[] */
  clusters    = (float**) malloc(nclusters *             sizeof(float*));
  clusters[0] = (float*)  malloc(nclusters * nfeatures * sizeof(float));
  for (i=1; i<nclusters; i++)
    clusters[i] = clusters[i-1] + nfeatures;

  /* randomly pick cluster centers */
  for (i=0; i<nclusters; i++) {
    //n = (int)rand() % npoints;
    for (j=0; j<nfeatures; j++)
      clusters[i][j] = feature[n][j];
    n++;
  }

  for (i=0; i<npoints; i++)
    membership[i] = -1;

  /* need to initialize new_centers_len and new_centers[0] to all 0 */
  new_centers_len = (int*) mycalloc(nclusters * sizeof(int));

  new_centers    = (float**) malloc(nclusters *            sizeof(float*));
  new_centers[0] = (float*)  mycalloc(nclusters * nfeatures * sizeof(float));
  for (i=1; i<nclusters; i++)
    new_centers[i] = new_centers[i-1] + nfeatures;
 
  
  do {
		
    delta = HEARTBEAT_loop0(feature, nfeatures, npoints, nclusters, membership, clusters, new_centers_len, new_centers);

    /* replace old cluster centers with new_centers */
    for (i=0; i<nclusters; i++) {
      for (j=0; j<nfeatures; j++) {
	      if (new_centers_len[i] > 0)
	        clusters[i][j] = new_centers[i][j] / new_centers_len[i];
	      new_centers[i][j] = 0.0;   /* set back to 0 */
      }
      new_centers_len[i] = 0;   /* set back to 0 */
    }
            
    //delta /= npoints;
  } while (delta > threshold);

  
  free(new_centers[0]);
  free(new_centers);
  free(new_centers_len);

  return clusters;
}

// Outlined loops
double HEARTBEAT_loop0(float **feature,
                       int nfeatures,
                       int npoints,
                       int nclusters,
                       int *membership,
                       float **clusters,
                       int *new_centers_len,
                       float **new_centers) {
  double delta = 0.0;

  if (run_heartbeat) {
    run_heartbeat = false;

    // allocate const live-ins
    uint64_t constLiveIns[7];
    constLiveIns[0] = (uint64_t)feature;
    constLiveIns[1] = (uint64_t)nfeatures;
    constLiveIns[2] = (uint64_t)nclusters;
    constLiveIns[3] = (uint64_t)membership;
    constLiveIns[4] = (uint64_t)clusters;
    constLiveIns[5] = (uint64_t)new_centers_len;
    constLiveIns[6] = (uint64_t)new_centers;

    // allocate cxts
    uint64_t cxts[NUM_LEVELS * CACHELINE];

    // allocate reduction array (as live-out environment) for loop0
    double redArrLoop0LiveOut0[1 * CACHELINE];
    cxts[LIVE_OUT_ENV] = (uint64_t)redArrLoop0LiveOut0;

    // set start/max iterations for loop0
    cxts[LEVEL_ZERO * CACHELINE + START_ITER] = (uint64_t)0;
    cxts[LEVEL_ZERO * CACHELINE + MAX_ITER] = (uint64_t)(uint64_t)npoints;

    // allocate the task memory struct and initialize
    task_memory_t tmem;
    heartbeat_start(&tmem);

    // invoke loop0 in heartbeat form
    HEARTBEAT_loop0_slice(cxts, constLiveIns, 0, &tmem);

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
      new_centers_len[index]++;
      for (int j=0; j<nfeatures; j++)          
	      new_centers[index][j] += feature[i][j];
    }
  }

  return delta;
}

// Transformed loops
int64_t HEARTBEAT_loop0_slice(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem) {
  // load start/max iterations
  uint64_t startIter = cxts[LEVEL_ZERO * CACHELINE + START_ITER];
  uint64_t maxIter = cxts[LEVEL_ZERO * CACHELINE + MAX_ITER];

  // load const live-ins
  float **feature = (float **)constLiveIns[0];
  int nfeatures = (int)constLiveIns[1];
  int nclusters = (int)constLiveIns[2];
  int *membership = (int *)constLiveIns[3];
  float **clusters = (float **)constLiveIns[4];
  int *new_centers_len = (int *)constLiveIns[5];
  float **new_centers = (float **)constLiveIns[6];

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
      /* find the index of nestest cluster centers */
      int index = find_nearest_point(feature[low], nfeatures, clusters, nclusters);
      /* if membership changes, increase delta by 1 */
      if (membership[low] != index) live_out_0 += 1.0;

      /* assign the membership to object i */
      membership[low] = index;

      /* update new cluster centers : sum of objects located within */
      new_centers_len[index]++;
      for (int j=0; j<nfeatures; j++)          
        new_centers[index][j] += feature[low][j];
    }

    if (low == maxIter) {
      // early exit and don't call the loop_handler,
      // this avoids the overhead if the loop count is small
      break;
    }

#if defined(ENABLE_HEARTBEAT)
#if !defined(PROMOTION_INSERTION_OVERHEAD_ANALYSIS)
    // early exit and don't call the loop_handler,
    // this avoids the overhead if the loop count is small
    if (update_and_has_remaining_chunksize(tmem, high - startIter, chunksize)) {
      break;
    }
#endif

#if defined(ENABLE_SOFTWARE_POLLING)
#if !defined(PROMOTION_INSERTION_OVERHEAD_ANALYSIS)
    if (unlikely(heartbeat_polling(tmem))) {
#endif
      cxts[LEVEL_ZERO * CACHELINE + START_ITER] = low - 1;
      rc = loop_handler(
        cxts, constLiveIns, LEVEL_ZERO, NUM_LEVELS, tmem,
        slice_tasks, nullptr, nullptr
      );
      if (rc > 0) {
        break;
      }
#if !defined(PROMOTION_INSERTION_OVERHEAD_ANALYSIS)
    }
#endif
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
  for (; startIter < maxIter; startIter++) {
    /* find the index of nestest cluster centers */
    int index = find_nearest_point(feature[startIter], nfeatures, clusters, nclusters);
    /* if membership changes, increase delta by 1 */
    if (membership[startIter] != index) live_out_0 += 1.0;

    /* assign the membership to object i */
    membership[startIter] = index;

    /* update new cluster centers : sum of objects located within */
    new_centers_len[index]++;
    for (int j=0; j<nfeatures; j++)          
      new_centers[index][j] += feature[startIter][j];

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

} // namespace kmeans
