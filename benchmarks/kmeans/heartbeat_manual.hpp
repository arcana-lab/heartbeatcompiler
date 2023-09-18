#pragma once

#include "loop_handler.hpp"
#include <cstdint>

namespace kmeans {

extern
void* mycalloc(std::size_t szb);

extern
int find_nearest_point(int  *pt,          /* [nfeatures] */
                       int     nfeatures,
                       int **pts,         /* [npts][nfeatures] */
                       int     npts);

int** kmeans_hb_manual(int **feature,    /* in: [npoints][nfeatures] */
                          int     nfeatures,
                          int     npoints,
                          int     nclusters,
                          int   threshold,
                          int    *membership); /* out: [npoints] */

inline
int cluster_hb_manual(int      numObjects,      /* number of input objects */
		 int      numAttributes,   /* size of attribute of each object */
		 int  **attributes,      /* [numObjects][numAttributes] */
		 int      num_nclusters,
		 int    threshold,       /* in:   */
		 int ***cluster_centres /* out: [best_nclusters][numAttributes] */
    
		 )
{
  int     nclusters;
  int    *membership;
  int **tmp_cluster_centres;

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