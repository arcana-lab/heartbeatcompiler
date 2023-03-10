#include "bench.hpp"
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <thread>
#if !defined(USE_HB_MANUAL) && !defined(USE_HB_COMPILER)
#include <functional>
#include <taskparts/benchmark.hpp>
#endif

#define RANDOM_MAX 2147483647

#ifndef FLT_MAX
#define FLT_MAX 3.40282347e+38
#endif

namespace kmeans {

#if defined(INPUT_BENCHMARKING)
  int numObjects = 1000000;
#elif defined(INPUT_TPAL)
  int numObjects = 1000000;
#elif defined(INPUT_TESTING)
  int numObjects = 100000;
#else
  #error "Need to select input size: INPUT_{BENCHMARKING, TPAL, TESTING}"
#endif

using kmeans_input_type = struct kmeans_input_struct {
  int nObj;
  int nFeat;
  float** attributes;
};

kmeans_input_type in;
float** attributes;
int numAttributes;
int nclusters=5;
float   threshold = 0.001;
float **cluster_centres=NULL;

#if !defined(USE_HB_MANUAL) && !defined(USE_HB_COMPILER)
void run_bench(std::function<void()> const &bench_body,
               std::function<void()> const &bench_start,
               std::function<void()> const &bench_end) {
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    bench_body();
  }, [&] (auto sched) {
    bench_start();
  }, [&] (auto sched) {
    bench_end();
  });
}
#endif

template <typename T>
void zero_init(T* a, std::size_t n) {
  volatile T* b = (volatile T*)a;
  for (std::size_t i = 0; i < n; i++) {
    b[i] = 0;
  }
}

void* mycalloc(std::size_t szb) {
  int* a = (int*)malloc(szb);
  zero_init(a, szb/sizeof(int));
  return a;
}

/*----< euclid_dist_2() >----------------------------------------------------*/
/* multi-dimensional spatial Euclid distance square */
__inline
float euclid_dist_2(float *pt1,
                    float *pt2,
                    int    numdims)
{
  int i;
  float ans=0.0;

  for (i=0; i<numdims; i++)
    ans += (pt1[i]-pt2[i]) * (pt1[i]-pt2[i]);

  return(ans);
}

// __inline
int find_nearest_point(float  *pt,          /* [nfeatures] */
                       int     nfeatures,
                       float **pts,         /* [npts][nfeatures] */
                       int     npts)
{
  int index, i;
  float min_dist=FLT_MAX;
  /* find the cluster center id with min distance to pt */
  for (i=0; i<npts; i++) {
    float dist;
    dist = euclid_dist_2(pt, pts[i], nfeatures);  /* no need square root */
    if (dist < min_dist) {
      min_dist = dist;
      index    = i;
    }
  }
  return(index);
}

inline
uint64_t hash(uint64_t u) {
  uint64_t v = u * 3935559000370003845ul + 2691343689449507681ul;
  v ^= v >> 21;
  v ^= v << 37;
  v ^= v >>  4;
  v *= 4768777513237032717ul;
  v ^= v << 20;
  v ^= v >> 41;
  v ^= v <<  5;
  return v;
}

// Features are integers from 0 to 255 by default
auto kmeans_inputgen(int nObj, int nFeat = 34) -> kmeans_input_type {
  int numObjects = nObj;
  int numAttributes = nFeat;
  float** attributes;
  attributes    = (float**)malloc(numObjects*             sizeof(float*));
  attributes[0] = (float*) malloc(numObjects*numAttributes*sizeof(float));
  for (int i=1; i<numObjects; i++) {
    attributes[i] = attributes[i-1] + numAttributes;
  }
  for ( int i = 0; i < nObj; i++ ) {
    for ( int j = 0; j < numAttributes; j++ ) {
      attributes[i][j] = (float)hash(j) / (float)RAND_MAX;
    }
  }
  kmeans_input_type in = { nObj, nFeat, attributes };
  return in;
}

void setup() {
  in = kmeans_inputgen(numObjects);
  attributes = in.attributes;
  numAttributes = in.nFeat;
}

void finishup() {
  assert(in.attributes[0] != nullptr);
  free(in.attributes[0]);
  assert(in.attributes != nullptr);
  free(in.attributes);
}

#if defined(USE_BASELINE) || defined(TEST_CORRECTNESS)

/*----< kmeans_clustering() >---------------------------------------------*/
float** kmeans_serial(float **feature,    /* in: [npoints][nfeatures] */
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

// ========================================
// allocate core local array
unsigned int numThreads = std::thread::hardware_concurrency();
int **partial_new_centers_len = new int *[numThreads];
for (int t = 0; t < numThreads; t++) {
  partial_new_centers_len[t] = new int[nclusters];
}
float ***partial_new_centers = new float **[numThreads];
for (int t = 0; t < numThreads; t++) {
  partial_new_centers[t] = new float *[nclusters];
  for (int i = 0; i < nclusters; i++) {
    partial_new_centers[t][i] = new float[nfeatures];
  }
}
// ========================================

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
		
    delta = 0.0;

    for (i=0; i<npoints; i++) {
      /* find the index of nestest cluster centers */
      index = find_nearest_point(feature[i], nfeatures, clusters, nclusters);
      /* if membership changes, increase delta by 1 */
      if (membership[i] != index) delta += 1.0;

      /* assign the membership to object i */
      membership[i] = index;

      /* update new cluster centers : sum of objects located within */
      // new_centers_len[index]++;
      partial_new_centers_len[sched_getcpu()][index]++;
      for (j=0; j<nfeatures; j++)          
	      // new_centers[index][j] += feature[i][j];
        partial_new_centers[sched_getcpu()][index][j] += feature[i][j];
    }
      
// ========================================
// let the main thread perform the array reduction
for (int t = 0; t < numThreads; t++) {
  for (int i = 0; i < nclusters; i++) {
    new_centers_len[i] += partial_new_centers_len[t][i];
    partial_new_centers_len[t][i] = 0;
    for (int j = 0; j < nfeatures; j++) {
      new_centers[i][j] += partial_new_centers[t][i][j];
      partial_new_centers[t][i][j] = 0.0;
    }
  }
}
// ========================================

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

// ========================================
// delete thread local arrays
for (int t = 0; t < numThreads; t++) {
  delete [] partial_new_centers_len[t];
}
delete [] partial_new_centers_len;
for (int t = 0; t < numThreads; t++) {
  for (int i = 0; i < nclusters; i++) {
    delete [] partial_new_centers[t][i];
  }
  delete [] partial_new_centers[t];
}
delete [] partial_new_centers;
// ========================================
  free(new_centers[0]);
  free(new_centers);
  free(new_centers_len);

  return clusters;
}

/*---< cluster() >-----------------------------------------------------------*/
int cluster_serial(int      numObjects,      /* number of input objects */
		 int      numAttributes,   /* size of attribute of each object */
		 float  **attributes,      /* [numObjects][numAttributes] */
		 int      num_nclusters,
		 float    threshold,       /* in:   */
		 float ***cluster_centres /* out: [best_nclusters][numAttributes] */
    
		 )
{
  int     nclusters;
  int    *membership;
  float **tmp_cluster_centres;

  membership = (int*) malloc(numObjects * sizeof(int));

  nclusters=num_nclusters;

  //srand(7);
	
  tmp_cluster_centres = kmeans_serial(attributes,
				      numAttributes,
				      numObjects,
				      nclusters,
				      threshold,
				      membership);

  if (*cluster_centres) {
    free((*cluster_centres)[0]);
    free(*cluster_centres);
  }
  *cluster_centres = tmp_cluster_centres;

   
  free(membership);

  return 0;
}

#endif

#if defined(USE_OPENCILK)

#elif defined (USE_OPENMP)

#endif

#if defined(TEST_CORRECTNESS)

#include <stdio.h>

void test_correctness() {
  float **cluster_centres_ref = NULL;
  cluster_serial(numObjects, numAttributes, attributes, nclusters, threshold, &cluster_centres_ref);
  uint64_t num_diffs = 0;
  double epsilon = 0.01;
  for (uint64_t i = 0; i != nclusters; i++) {
    for (uint64_t j = 0; j != numAttributes; j++) {
      auto diff = std::abs(cluster_centres[i][j] - cluster_centres_ref[i][j]);
      if (diff > epsilon) {
        printf("diff=%f cluster_centres[i]=%f cluster_centres_ref[i]=%f at i, j=%ld, %ld\n", diff, cluster_centres[i][j], cluster_centres_ref[i][j], i, j);
        num_diffs++;
      }
    }
  }
  if (num_diffs > 0) {
    printf("\033[0;31mINCORRECT!\033[0m");
    printf("  num_diffs = %lu\n", num_diffs);
  } else {
    printf("\033[0;32mCORRECT!\033[0m\n");
  }
  //printf("num_diffs %ld\n", num_diffs);
  free(cluster_centres_ref[0]);
  free(cluster_centres_ref);
}

#endif

} // namespace kmeans
