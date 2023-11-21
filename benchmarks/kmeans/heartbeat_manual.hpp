#pragma once

#include "loop_handler.hpp"
#include <cstdint>

namespace kmeans {

extern
void* mycalloc(std::size_t szb);

extern
int find_nearest_point(float  *pt,          /* [nfeatures] */
                       int     nfeatures,
                       float **pts,         /* [npts][nfeatures] */
                       int     npts);

float** kmeans_hb_manual(float **feature,    /* in: [npoints][nfeatures] */
                          int     nfeatures,
                          int     npoints,
                          int     nclusters,
                          float   threshold,
                          int    *membership); /* out: [npoints] */

inline
int cluster_hb_manual(int      numObjects,      /* number of input objects */
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