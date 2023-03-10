#pragma once

#include "loop_handler.hpp"
#include <cstdlib>
#include <thread>

namespace kmeans {

extern
void* mycalloc(std::size_t szb);

extern
int find_nearest_point(float  *pt,          /* [nfeatures] */
                       int     nfeatures,
                       float **pts,         /* [npts][nfeatures] */
                       int     npts);

double HEARTBEAT_nest0_loop0(float **feature,
                             int nfeatures,
                             int npoints,
                             int nclusters,
                             int *membership,
                             float **clusters,
                             int **partial_new_centers_len,
                             float ***partial_new_centers);

/*----< kmeans_clustering() >---------------------------------------------*/
__inline
float** kmeans_hb_manual(float **feature,    /* in: [npoints][nfeatures] */
                          int     nfeatures,
                          int     npoints,
                          int     nclusters,
                          float   threshold,
                          int    *membership) /* out: [npoints] */
{

  int      i, j, n=0, loop=0;
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

    delta += HEARTBEAT_nest0_loop0(feature, nfeatures, npoints, nclusters, membership, clusters, partial_new_centers_len, partial_new_centers);
      
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

inline
int cluster_hb_manual(int numObjects, int numAttributes, float **attributes, int num_nclusters, float threshold, float ***cluster_centres) {
  int     nclusters;
  int    *membership;
  float **tmp_cluster_centres;

  membership = (int*) malloc(numObjects * sizeof(int));

  nclusters=num_nclusters;

  //srand(7);
	
  tmp_cluster_centres = kmeans_hb_manual(attributes,
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

} // namespace kmeans