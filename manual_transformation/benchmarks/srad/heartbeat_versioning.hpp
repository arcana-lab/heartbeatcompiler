#pragma once

#include <cstdint>
#include <alloca.h>
#include <algorithm>
#include <cstdio>
#include "loop_handler_application.hpp"

#define CACHELINE         8
#define NUM_LEVELS_NEST0  2
#define NUM_LEVELS_NEST1  2
#define LIVE_IN_ENV       0
#define LIVE_OUT_ENV      1
#define START_ITER        0
#define MAX_ITER          1
#define LEVEL_ZERO        0
#define LEVEL_ONE         1
#define MY_INDEX          0

void srad_heartbeat_versioning(int rows, int cols, float *J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float *c, int *iN, int *iS, int *jE, int *jW, float lambda);
void HEARTBEAT_nest0_loop0(int rows, int cols, float *J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float *c, int *iN, int *iS, int *jE, int *jW, float lambda);
void HEARTBEAT_nest0_loop1(int cols, float *J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float *c, int *iN, int *iS, int *jE, int *jW, float lambda, int i);
void HEARTBEAT_nest1_loop0(int rows, int cols, float *J, float *dN, float *dS, float *dW, float *dE, float *c, int *iS, int *jE, float lambda);
void HEARTBEAT_nest1_loop1(int cols, float *J, float *dN, float *dS, float *dW, float *dE, float *c, int *iS, int *jE, float lambda, int i);

int64_t HEARTBEAT_nest0_loop0_slice(uint64_t *cxts, uint64_t myIndex, uint64_t startIter, uint64_t maxIter);
int64_t HEARTBEAT_nest0_loop0_slice_wrapper(uint64_t *cxts, uint64_t myIndex, uint64_t startIter, uint64_t maxIter) {
  return HEARTBEAT_nest0_loop0_slice(cxts, myIndex, startIter, maxIter);
}
int64_t HEARTBEAT_nest0_loop1_slice(uint64_t *cxts, uint64_t myIndex, uint64_t startIter0, uint64_t maxIter0, uint64_t startIter, uint64_t maxIter);
int64_t HEARTBEAT_nest0_loop1_slice_wrapper(uint64_t *cxts, uint64_t myIndex, uint64_t startIter, uint64_t maxIter) {
  return HEARTBEAT_nest0_loop1_slice(cxts, myIndex, 0, 0, startIter, maxIter);
}
int64_t HEARTBEAT_nest1_loop0_slice(uint64_t *cxts, uint64_t myIndex, uint64_t startIter, uint64_t maxIter);
int64_t HEARTBEAT_nest1_loop0_slice_wrapper(uint64_t *cxts, uint64_t myIndex, uint64_t startIter, uint64_t maxIter) {
  return HEARTBEAT_nest1_loop0_slice(cxts, myIndex, startIter, maxIter);
}
int64_t HEARTBEAT_nest1_loop1_slice(uint64_t *cxts, uint64_t myIndex, uint64_t startIter0, uint64_t maxIter0, uint64_t startIter, uint64_t maxIter);
int64_t HEARTBEAT_nest1_loop1_slice_wrapper(uint64_t *cxts, uint64_t myIndex, uint64_t startIter, uint64_t maxIter) {
  return HEARTBEAT_nest1_loop1_slice(cxts, myIndex, 0, 0, startIter, maxIter);
}
typedef int64_t(*sliceTasksWrapperPointer)(uint64_t * /* cxts */, uint64_t /* myIndex */, uint64_t /* startIter */, uint64_t /* maxIter */);
sliceTasksWrapperPointer slice_tasks_nest0[2] = {
  &HEARTBEAT_nest0_loop0_slice_wrapper,
  &HEARTBEAT_nest0_loop1_slice_wrapper
};
sliceTasksWrapperPointer slice_tasks_nest1[2] = {
  &HEARTBEAT_nest1_loop0_slice_wrapper,
  &HEARTBEAT_nest1_loop1_slice_wrapper
};

int64_t HEARTBEAT_nest0_loop_1_0_leftover(uint64_t *cxts, uint64_t myIndex, uint64_t *itersArr);
int64_t HEARTBEAT_nest1_loop_1_0_leftover(uint64_t *cxts, uint64_t myIndex, uint64_t *itersArr);
typedef int64_t(*leftoverTasksPointer)(uint64_t * /* cxts */, uint64_t /* myIndex */, uint64_t * /* itersArr */);
leftoverTasksPointer leftover_tasks_nest0[1] = {
  &HEARTBEAT_nest0_loop_1_0_leftover
};
leftoverTasksPointer leftover_tasks_nest1[1] = {
  &HEARTBEAT_nest1_loop_1_0_leftover
};

uint64_t leftover_selector_nest0(uint64_t splittingLevel, uint64_t receivingLevel) {
  return 0;
}
uint64_t leftover_selector_nest1(uint64_t splittingLevel, uint64_t receivingLevel) {
  return 0;
}

bool run_heartbeat = true;
uint64_t *constLiveIns_nest0;
uint64_t *constLiveIns_nest1;

void srad_heartbeat_versioning(int rows, int cols, float *J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float *c, int *iN, int *iS, int *jE, int *jW, float lambda) {
  if (run_heartbeat) {
    run_heartbeat = false;

    // allocate const live-ins
    constLiveIns_nest0 = (uint64_t *)alloca(sizeof(uint64_t) * 13);
    constLiveIns_nest0[0] = (uint64_t)cols;
    constLiveIns_nest0[1] = (uint64_t)J;
    constLiveIns_nest0[2] = (uint64_t)&q0sqr;
    constLiveIns_nest0[3] = (uint64_t)dN;
    constLiveIns_nest0[4] = (uint64_t)dS;
    constLiveIns_nest0[5] = (uint64_t)dW;
    constLiveIns_nest0[6] = (uint64_t)dE;
    constLiveIns_nest0[7] = (uint64_t)c;
    constLiveIns_nest0[8] = (uint64_t)iN;
    constLiveIns_nest0[9] = (uint64_t)iS;
    constLiveIns_nest0[10] = (uint64_t)jE;
    constLiveIns_nest0[11] = (uint64_t)jW;
    constLiveIns_nest0[12] = (uint64_t)&lambda;

    // allocate cxts
    uint64_t cxts[NUM_LEVELS_NEST0 * CACHELINE];

    // invoke nest0_loop0 in heartbeat form
    HEARTBEAT_nest0_loop0_slice(cxts, MY_INDEX, 0, (uint64_t)rows);

    run_heartbeat = true;
  } else {
    HEARTBEAT_nest0_loop0(rows, cols, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
  }
  if (run_heartbeat) {
    run_heartbeat = false;

    // allocate const live-ins
    constLiveIns_nest1 = (uint64_t *)alloca(sizeof(uint64_t) * 10);
    constLiveIns_nest1[0] = (uint64_t)cols;
    constLiveIns_nest1[1] = (uint64_t)J;
    constLiveIns_nest1[2] = (uint64_t)dN;
    constLiveIns_nest1[3] = (uint64_t)dS;
    constLiveIns_nest1[4] = (uint64_t)dW;
    constLiveIns_nest1[5] = (uint64_t)dE;
    constLiveIns_nest1[6] = (uint64_t)c;
    constLiveIns_nest1[7] = (uint64_t)iS;
    constLiveIns_nest1[8] = (uint64_t)jE;
    constLiveIns_nest1[9] = (uint64_t)&lambda;

    // allocate cxts
    uint64_t cxts[NUM_LEVELS_NEST1 * CACHELINE];

    // invoke nest1_loop0 in heartbeat form
    HEARTBEAT_nest1_loop0_slice(cxts, MY_INDEX, 0, (uint64_t)rows);

    run_heartbeat = true;
  } else {
    HEARTBEAT_nest1_loop0(rows, cols, J, dN, dS, dW, dE, c, iS, jE, lambda);
  }
}

// Outlined loops
/*
 * Loop nest 0
 */
void HEARTBEAT_nest0_loop0(int rows, int cols, float *J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float *c, int *iN, int *iS, int *jE, int *jW, float lambda) {
  for (int i = 0; i < rows; i++) {
    HEARTBEAT_nest0_loop1(cols, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda, i);
  }
}

void HEARTBEAT_nest0_loop1(int cols, float *J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float *c, int *iN, int *iS, int *jE, int *jW, float lambda, int i) {
  for (int j = 0; j < cols; j++) {
    int k = i * cols + j;
    float Jc = J[k];

    // directional derivates
    dN[k] = J[iN[i] * cols + j] - Jc;
    dS[k] = J[iS[i] * cols + j] - Jc;
    dW[k] = J[i * cols + jW[j]] - Jc;
    dE[k] = J[i * cols + jE[j]] - Jc;
    
    float G2 = (dN[k]*dN[k] + dS[k]*dS[k] 
    + dW[k]*dW[k] + dE[k]*dE[k]) / (Jc*Jc);

    float L = (dN[k] + dS[k] + dW[k] + dE[k]) / Jc;

    float num  = (0.5*G2) - ((1.0/16.0)*(L*L)) ;
    float den  = 1 + (.25*L);
    float qsqr = num/(den*den);

    // diffusion coefficent (equ 33)
    den = (qsqr-q0sqr) / (q0sqr * (1+q0sqr)) ;
    c[k] = 1.0 / (1.0+den) ;
              
    // saturate diffusion coefficent
    if (c[k] < 0) {c[k] = 0;}
    else if (c[k] > 1) {c[k] = 1;}
  }
}

int64_t HEARTBEAT_nest0_loop0_slice(uint64_t *cxts, uint64_t myIndex, uint64_t startIter, uint64_t maxIter) {
  // load const live-ins
  int cols = (int)constLiveIns_nest0[0];

  int64_t rc = 0;
#if defined(CHUNK_LOOP_ITERATIONS) && CHUNKSIZE_0 != 1
  uint64_t low, high;

  // store into live-in environment for nest0_loop1
  cxts[LEVEL_ONE * CACHELINE + LIVE_IN_ENV] = (uint64_t)&low;

  for (; startIter < maxIter; startIter += CHUNKSIZE_0) {
    low = startIter;
    high = std::min(maxIter, startIter + CHUNKSIZE_0);

    for (; low < high; low++) {
      rc = HEARTBEAT_nest0_loop1_slice(cxts, MY_INDEX, low, maxIter, 0, (uint64_t)cols);
      if (rc > 0) {
        break;
      }
    }

    // exit the chunk execution when either
    // 1. heartbeat promotion happens at a higher nested level and in the process of returnning
    // 2. all iterations are finished
    if (rc > 0 || low == maxIter) {
      break;
    }

    // call to loop_handler
    rc = loop_handler_level2(
      cxts,
      LEVEL_ZERO,
      slice_tasks_nest0,
      leftover_tasks_nest0,
      &leftover_selector_nest0,
      low - 1,
      maxIter,
      0,
      0
    );
    if (rc > 0) {
      break;
    }
  }
#else
  // store into live-in environment for nest0_loop1
  cxts[LEVEL_ONE * CACHELINE + LIVE_IN_ENV] = (uint64_t)&startIter;

  for (; startIter < maxIter; startIter++) {
    rc = HEARTBEAT_nest0_loop1_slice(cxts, MY_INDEX, startIter, maxIter, 0, (uint64_t)cols);
    if (rc > 0) {
      break;
    }

    // call to loop_handler
    rc = loop_handler_level2(
      cxts,
      LEVEL_ZERO,
      slice_tasks_nest0,
      leftover_tasks_nest0,
      &leftover_selector_nest0,
      startIter,
      maxIter,
      0,
      0
    );
    if (rc > 0) {
      break;
    }
  }
#endif

  return rc - 1;
}

int64_t HEARTBEAT_nest0_loop1_slice(uint64_t *cxts, uint64_t myIndex, uint64_t startIter0, uint64_t maxIter0, uint64_t startIter, uint64_t maxIter) {
  // load const live-ins
  int cols = (int)constLiveIns_nest0[0];
  float *J = (float *)constLiveIns_nest0[1];
  float q0sqr = *(float *)constLiveIns_nest0[2];
  float *dN = (float *)constLiveIns_nest0[3];
  float *dS = (float *)constLiveIns_nest0[4];
  float *dW = (float *)constLiveIns_nest0[5];
  float *dE = (float *)constLiveIns_nest0[6];
  float *c = (float *)constLiveIns_nest0[7];
  int *iN = (int *)constLiveIns_nest0[8];
  int *iS = (int *)constLiveIns_nest0[9];
  int *jE = (int *)constLiveIns_nest0[10];
  int *jW = (int *)constLiveIns_nest0[11];
  float lambda = *(float *)constLiveIns_nest0[12];

  // load live-in environment
  int i = *(int *)cxts[LEVEL_ONE * CACHELINE + LIVE_IN_ENV];

  int64_t rc = 0;
#if defined(CHUNK_LOOP_ITERATIONS) && CHUNKSIZE_1 != 1
  for (; startIter < maxIter; startIter += CHUNKSIZE_1) {
    uint64_t low = startIter;
    uint64_t high = std::min(maxIter, startIter + CHUNKSIZE_1);

    for (; low < high; low++) {
      int k = i * cols + low;
      float Jc = J[k];
 
      // directional derivates
      dN[k] = J[iN[i] * cols + low] - Jc;
      dS[k] = J[iS[i] * cols + low] - Jc;
      dW[k] = J[i * cols + jW[low]] - Jc;
      dE[k] = J[i * cols + jE[low]] - Jc;
			
      float G2 = (dN[k]*dN[k] + dS[k]*dS[k] 
		  + dW[k]*dW[k] + dE[k]*dE[k]) / (Jc*Jc);

      float L = (dN[k] + dS[k] + dW[k] + dE[k]) / Jc;

      float num  = (0.5*G2) - ((1.0/16.0)*(L*L)) ;
      float den  = 1 + (.25*L);
      float qsqr = num/(den*den);
 
      // diffusion coefficent (equ 33)
      den = (qsqr-q0sqr) / (q0sqr * (1+q0sqr)) ;
      c[k] = 1.0 / (1.0+den) ;
                
      // saturate diffusion coefficent
      if (c[k] < 0) {c[k] = 0;}
      else if (c[k] > 1) {c[k] = 1;}
    }
    if (low == maxIter) {
      break;
    }

    // call to loop_handler
    rc = loop_handler_level2(
      cxts,
      LEVEL_ONE,
      slice_tasks_nest0,
      leftover_tasks_nest0,
      &leftover_selector_nest0,
      startIter0,
      maxIter0,
      low - 1,
      maxIter
    );
    if (rc > 0) {
      break;
    }
  }
#else
  for (; startIter < maxIter; startIter++) {
    int k = i * cols + startIter;
    float Jc = J[k];

    // directional derivates
    dN[k] = J[iN[i] * cols + startIter] - Jc;
    dS[k] = J[iS[i] * cols + startIter] - Jc;
    dW[k] = J[i * cols + jW[startIter]] - Jc;
    dE[k] = J[i * cols + jE[startIter]] - Jc;
    
    float G2 = (dN[k]*dN[k] + dS[k]*dS[k] 
    + dW[k]*dW[k] + dE[k]*dE[k]) / (Jc*Jc);

    float L = (dN[k] + dS[k] + dW[k] + dE[k]) / Jc;

    float num  = (0.5*G2) - ((1.0/16.0)*(L*L)) ;
    float den  = 1 + (.25*L);
    float qsqr = num/(den*den);

    // diffusion coefficent (equ 33)
    den = (qsqr-q0sqr) / (q0sqr * (1+q0sqr)) ;
    c[k] = 1.0 / (1.0+den) ;
              
    // saturate diffusion coefficent
    if (c[k] < 0) {c[k] = 0;}
    else if (c[k] > 1) {c[k] = 1;}

    // call to loop_handler
    rc = loop_handler_level2(
      cxts,
      LEVEL_ONE,
      slice_tasks_nest0,
      leftover_tasks_nest0,
      &leftover_selector_nest0,
      startIter0,
      maxIter0,
      startIter,
      maxIter
    );
    if (rc > 0) {
      break;
    }
  }
#endif

  return rc - 1;
}

int64_t HEARTBEAT_nest0_loop_1_0_leftover(uint64_t *cxts, uint64_t myIndex, uint64_t *itersArr) {
  // load startIter and maxIter
  uint64_t startIter = itersArr[LEVEL_ONE * 2 + START_ITER];
  uint64_t maxIter   = itersArr[LEVEL_ONE * 2 + MAX_ITER];

#ifndef LEFTOVER_SPLITTABLE



#else

  return HEARTBEAT_nest0_loop1_slice(cxts, myIndex, 0, 0, startIter, maxIter);

#endif
}

/*
 * Loop nest 1
 */
void HEARTBEAT_nest1_loop0(int rows, int cols, float *J, float *dN, float *dS, float *dW, float *dE, float *c, int *iS, int *jE, float lambda) {
  for (int i = 0; i < rows; i++) {
    HEARTBEAT_nest1_loop1(cols, J, dN, dS, dW, dE, c, iS, jE, lambda, i);
  }
}

void HEARTBEAT_nest1_loop1(int cols, float *J, float *dN, float *dS, float *dW, float *dE, float *c, int *iS, int *jE, float lambda, int i) {
  for (int j = 0; j < cols; j++) {
    // current index
    int k = i * cols + j;
              
    // diffusion coefficent
    float cN = c[k];
    float cS = c[iS[i] * cols + j];
    float cW = c[k];
    float cE = c[i * cols + jE[j]];

    // divergence (equ 58)
    float D = cN * dN[k] + cS * dS[k] + cW * dW[k] + cE * dE[k];
              
    // image update (equ 61)
    J[k] = J[k] + 0.25*lambda*D;
  }
}

int64_t HEARTBEAT_nest1_loop0_slice(uint64_t *cxts, uint64_t myIndex, uint64_t startIter, uint64_t maxIter) {
  // load const live-ins
  int cols = (int)constLiveIns_nest1[0];

  int64_t rc = 0;
#if defined(CHUNK_LOOP_ITERATIONS) && CHUNKSIZE_0 != 1
  uint64_t low, high;

  // store into live-in environment for nest1_loop1
  cxts[LEVEL_ONE * CACHELINE + LIVE_IN_ENV] = (uint64_t)&low;

  for (; startIter < maxIter; startIter += CHUNKSIZE_0) {
    low = startIter;
    high = std::min(maxIter, startIter + CHUNKSIZE_0);

    for (; low < high; low++) {
      rc = HEARTBEAT_nest1_loop1_slice(cxts, MY_INDEX, low, maxIter, 0, (uint64_t)cols);
      if (rc > 0) {
        break;
      }
    }

    // exit the chunk execution when either
    // 1. heartbeat promotion happens at a higher nested level and in the process of returnning
    // 2. all iterations are finished
    if (rc > 0 || low == maxIter) {
      break;
    }

    // call to loop_handler
    rc = loop_handler_level2(
      cxts,
      LEVEL_ZERO,
      slice_tasks_nest1,
      leftover_tasks_nest1,
      &leftover_selector_nest1,
      low - 1,
      maxIter,
      0,
      0
    );
    if (rc > 0) {
      break;
    }
  }
#else
  // store into live-in environment for nest1_loop1
  cxts[LEVEL_ONE * CACHELINE + LIVE_IN_ENV] = (uint64_t)&startIter;

  for (; startIter < maxIter; startIter++) {
    rc = HEARTBEAT_nest1_loop1_slice(cxts, MY_INDEX, startIter, maxIter, 0, (uint64_t)cols);
    if (rc > 0) {
      break;
    }

    // call to loop_handler
    rc = loop_handler_level2(
      cxts,
      LEVEL_ZERO,
      slice_tasks_nest1,
      leftover_tasks_nest1,
      &leftover_selector_nest1,
      startIter,
      maxIter,
      0,
      0
    );
    if (rc > 0) {
      break;
    }
  }
#endif

  return rc - 1;
}

int64_t HEARTBEAT_nest1_loop1_slice(uint64_t *cxts, uint64_t myIndex, uint64_t startIter0, uint64_t maxIter0, uint64_t startIter, uint64_t maxIter) {
  // load const live-ins
  int cols = (int)constLiveIns_nest1[0];
  float *J = (float *)constLiveIns_nest1[1];
  float *dN = (float *)constLiveIns_nest1[2];
  float *dS = (float *)constLiveIns_nest1[3];
  float *dW = (float *)constLiveIns_nest1[4];
  float *dE = (float *)constLiveIns_nest1[5];
  float *c = (float *)constLiveIns_nest1[6];
  int *iS = (int *)constLiveIns_nest1[7];
  int *jE = (int *)constLiveIns_nest1[8];
  float lambda = *(float *)constLiveIns_nest1[9];

  // load live-in environment
  int i = *(int *)cxts[LEVEL_ONE * CACHELINE + LIVE_IN_ENV];

  int64_t rc = 0;
#if defined(CHUNK_LOOP_ITERATIONS) && CHUNKSIZE_1 != 1
  for (; startIter < maxIter; startIter += CHUNKSIZE_1) {
    uint64_t low = startIter;
    uint64_t high = std::min(maxIter, startIter + CHUNKSIZE_1);

    for (; low < high; low++) {
      // current index
      int k = i * cols + low;
                
      // diffusion coefficent
      float cN = c[k];
      float cS = c[iS[i] * cols + low];
      float cW = c[k];
      float cE = c[i * cols + jE[low]];

      // divergence (equ 58)
      float D = cN * dN[k] + cS * dS[k] + cW * dW[k] + cE * dE[k];
                
      // image update (equ 61)
      J[k] = J[k] + 0.25*lambda*D;
    }
    if (low == maxIter) {
      break;
    }

    // call to loop_handler
    rc = loop_handler_level2(
      cxts,
      LEVEL_ONE,
      slice_tasks_nest1,
      leftover_tasks_nest1,
      &leftover_selector_nest1,
      startIter0,
      maxIter0,
      low - 1,
      maxIter
    );
    if (rc > 0) {
      break;
    }
  }
#else
  for (; startIter < maxIter; startIter++) {
    // current index
    int k = i * cols + startIter;
              
    // diffusion coefficent
    float cN = c[k];
    float cS = c[iS[i] * cols + startIter];
    float cW = c[k];
    float cE = c[i * cols + jE[startIter]];

    // divergence (equ 58)
    float D = cN * dN[k] + cS * dS[k] + cW * dW[k] + cE * dE[k];
              
    // image update (equ 61)
    J[k] = J[k] + 0.25*lambda*D;

    // call to loop_handler
    rc = loop_handler_level2(
      cxts,
      LEVEL_ONE,
      slice_tasks_nest1,
      leftover_tasks_nest1,
      &leftover_selector_nest1,
      startIter0,
      maxIter0,
      startIter,
      maxIter
    );
    if (rc > 0) {
      break;
    }
  }
#endif

  return rc - 1;
}

int64_t HEARTBEAT_nest1_loop_1_0_leftover(uint64_t *cxts, uint64_t myIndex, uint64_t *itersArr) {
  // load startIter and maxIter
  uint64_t startIter = itersArr[LEVEL_ONE * 2 + START_ITER];
  uint64_t maxIter   = itersArr[LEVEL_ONE * 2 + MAX_ITER];

#ifndef LEFTOVER_SPLITTABLE



#else

  return HEARTBEAT_nest1_loop1_slice(cxts, myIndex, 0, 0, startIter, maxIter);

#endif
}
