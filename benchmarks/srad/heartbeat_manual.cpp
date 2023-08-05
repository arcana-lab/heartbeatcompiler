#include "heartbeat_manual.hpp"
#include "loop_handler.hpp"
#include <cstdint>

#define unlikely(expr) __builtin_expect(!!(expr), 0)
#define NUM_LEVELS_NEST0 2
#define NUM_LEVELS_NEST1 2
#define LEVEL_ZERO 0
#define LEVEL_ONE 1
#define CACHELINE 8
#define START_ITER 0
#define MAX_ITER 1
#define LIVE_IN_ENV 2

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

namespace srad {

#if defined(RUN_HEARTBEAT)
  bool run_heartbeat = true;
#else
  bool run_heartbeat = false;
#endif

void HEARTBEAT_nest0_loop0(int rows, int cols, float *J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float *c, int *iN, int *iS, int *jE, int *jW);
void HEARTBEAT_nest1_loop0(int rows, int cols, float *J, float *dN, float *dS, float *dW, float *dE, float *c, int *iS, int *jE, float lambda);

// Entry function
void srad_hb_manual(int rows, int cols, int size_I, int size_R, float* I, float* J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float* c, int* iN, int* iS, int* jE, int* jW, float lambda) {
  HEARTBEAT_nest0_loop0(rows, cols, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW);
  HEARTBEAT_nest1_loop0(rows, cols, J, dN, dS, dW, dE, c, iS, jE, lambda);
}

/* ========================================
 * Loop nest 0
 * ========================================
 */
void HEARTBEAT_nest0_loop1(int cols, float *J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float *c, int *iN, int *iS, int *jE, int *jW, int i);

int64_t HEARTBEAT_nest0_loop0_slice(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem);
int64_t HEARTBEAT_nest0_loop1_slice(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem);
typedef int64_t (*sliceTasksPointer)(uint64_t *, uint64_t *, uint64_t, task_memory_t *);
sliceTasksPointer slice_tasks_nest0[2] = {
  &HEARTBEAT_nest0_loop0_slice,
  &HEARTBEAT_nest0_loop1_slice
};

void HEARTBEAT_nest0_loop_1_0_leftover(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem);
typedef void (*leftoverTasksPointer)(uint64_t *, uint64_t *, uint64_t, task_memory_t *);
leftoverTasksPointer leftover_tasks_nest0[1] = {
  &HEARTBEAT_nest0_loop_1_0_leftover
};

uint64_t leftover_selector_nest0(uint64_t receivingLevel, uint64_t splittingLevel) {
  return 0;
}

// Outlined loops
void HEARTBEAT_nest0_loop0(int rows, int cols, float *J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float *c, int *iN, int *iS, int *jE, int *jW) {
  if (run_heartbeat) {
    run_heartbeat = false;

    // allocate const live-ins
    uint64_t constLiveIns[12];
    constLiveIns[0] = (uint64_t)cols;
    constLiveIns[1] = (uint64_t)J;
    constLiveIns[2] = (uint64_t)&q0sqr;
    constLiveIns[3] = (uint64_t)dN;
    constLiveIns[4] = (uint64_t)dS;
    constLiveIns[5] = (uint64_t)dW;
    constLiveIns[6] = (uint64_t)dE;
    constLiveIns[7] = (uint64_t)c;
    constLiveIns[8] = (uint64_t)iN;
    constLiveIns[9] = (uint64_t)iS;
    constLiveIns[10] = (uint64_t)jE;
    constLiveIns[11] = (uint64_t)jW;

    // allocate cxts
    uint64_t cxts[NUM_LEVELS_NEST0 * CACHELINE];

    // set start/max iterations for loop0
    cxts[LEVEL_ZERO * CACHELINE + START_ITER] = (uint64_t)0;
    cxts[LEVEL_ZERO * CACHELINE + MAX_ITER] = (uint64_t)rows;

    // allocate the task memory struct and initialize
    task_memory_t tmem;
    heartbeat_start(&tmem);

    // invoke loop0 in heartbeat form
    HEARTBEAT_nest0_loop0_slice(cxts, constLiveIns, 0, &tmem);

    run_heartbeat = true;
  } else {
    for (int i = 0 ; i < rows ; i++) {
      HEARTBEAT_nest0_loop1(cols, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, i);
    }
  }
}

void HEARTBEAT_nest0_loop1(int cols, float *J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float *c, int *iN, int *iS, int *jE, int *jW, int i) {
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

// Transformed loops
int64_t HEARTBEAT_nest0_loop0_slice(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem) {
  // load start/max iterations
  uint64_t startIter = cxts[LEVEL_ZERO * CACHELINE + START_ITER];
  uint64_t maxIter = cxts[LEVEL_ZERO * CACHELINE + MAX_ITER];

  // load const live-ins
  int cols = (int)constLiveIns[0];

  int64_t rc = 0;
  // store &live-in as live-in environment for loop1
  ((uint64_t *)cxts)[LEVEL_ONE * CACHELINE + LIVE_IN_ENV] = (uint64_t)&startIter;

  for (; startIter < maxIter; startIter++) {
    // store current iteration for loop0
    cxts[LEVEL_ZERO * CACHELINE + START_ITER] = startIter;
    // set start/max iterations for loop1
    cxts[LEVEL_ONE * CACHELINE + START_ITER] = (uint64_t)0;
    cxts[LEVEL_ONE * CACHELINE + MAX_ITER] = (uint64_t)cols;
    rc = HEARTBEAT_nest0_loop1_slice(cxts, constLiveIns, 0, tmem);

    // check the status of rc because, might not need
    // to call the loop_handler in the process of returnning up
    if (rc > 0) {
      break;
    }
  }

  return rc - 1;
}

int64_t HEARTBEAT_nest0_loop1_slice(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem) {
  // load start/max iterations
  uint64_t startIter = cxts[LEVEL_ONE * CACHELINE + START_ITER];
  uint64_t maxIter = cxts[LEVEL_ONE * CACHELINE + MAX_ITER];

  // load const live-ins
  int cols = (int)constLiveIns[0];
  float *J = (float *)constLiveIns[1];
  float q0sqr = *(float *)constLiveIns[2];
  float *dN = (float *)constLiveIns[3];
  float *dS = (float *)constLiveIns[4];
  float *dW = (float *)constLiveIns[5];
  float *dE = (float *)constLiveIns[6];
  float *c = (float *)constLiveIns[7];
  int *iN = (int *)constLiveIns[8];
  int *iS = (int *)constLiveIns[9];
  int *jE = (int *)constLiveIns[10];
  int *jW = (int *)constLiveIns[11];

  // load live-ins
  int i = *(int *)((uint64_t *)cxts)[LEVEL_ONE * CACHELINE + LIVE_IN_ENV];

  int64_t rc = 0;
#if defined(CHUNK_LOOP_ITERATIONS)
  uint64_t chunksize;
  for (; startIter < maxIter; startIter += chunksize) {
    chunksize = get_chunksize(tmem);
    uint64_t low = startIter;
    uint64_t high = maxIter < startIter + chunksize ? maxIter : startIter + chunksize;
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

#if defined(ENABLE_HEARTBEAT)
    if (update_and_has_remaining_chunksize(tmem, high - startIter, chunksize)) {
      break;
    }

#if defined(ENABLE_SOFTWARE_POLLING)
    if (unlikely(heartbeat_polling(tmem))) {
      cxts[LEVEL_ONE * CACHELINE + START_ITER] = low - 1;
      rc = loop_handler(
        cxts, constLiveIns, LEVEL_ONE, NUM_LEVELS_NEST0, tmem,
        slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0
      );
      if (rc > 0) {
        break;
      }
    }
#else
    if(unlikely(__rf_test())) {
      cxts[LEVEL_ONE * CACHELINE + START_ITER] = low - 1;
      __rf_handle_wrapper(
        rc, cxts, constLiveIns, LEVEL_ONE, NUM_LEVELS_NEST0, tmem,
        slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0
      );
      if (rc > 0) {
        break;
      }
    }
#endif
#endif
  }
#else
  for(; startIter < maxIter; startIter++) {
  
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

#if defined(ENABLE_HEARTBEAT)
#if defined(ENABLE_SOFTWARE_POLLING)
    if (unlikely(heartbeat_polling(tmem))) {
      cxts[LEVEL_ONE * CACHELINE + START_ITER] = startIter;
      rc = loop_handler(
        cxts, constLiveIns, LEVEL_ONE, NUM_LEVELS_NEST0, tmem,
        slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0
      );
      if (rc > 0) {
        break;
      }
    }
#else
    if(unlikely(__rf_test())) {
      cxts[LEVEL_ONE * CACHELINE + START_ITER] = startIter;
      __rf_handle_wrapper(
        rc, cxts, constLiveIns, LEVEL_ONE, NUM_LEVELS_NEST0, tmem,
        slice_tasks_nest0, leftover_tasks_nest0, &leftover_selector_nest0
      );
      if (rc > 0) {
        break;
      }
    }
#endif
#endif
  }
#endif

  return rc - 1;
}

// Leftover tasks
void HEARTBEAT_nest0_loop_1_0_leftover(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem) {
  int64_t rc = 0;
  rc = HEARTBEAT_nest0_loop1_slice(cxts, constLiveIns, myIndex, tmem);
  if (rc > 0) {
    return;
  }

  cxts[LEVEL_ZERO * CACHELINE + START_ITER]++;
  rc = HEARTBEAT_nest0_loop0_slice(cxts, constLiveIns, myIndex, tmem);
  if (rc > 0) {
    return;
  }

  return;
}

/* ========================================
 * Loop nest 1
 * ========================================
 */
void HEARTBEAT_nest1_loop1(int cols, float *J, float *dN, float *dS, float *dW, float *dE, float *c, int *iS, int *jE, float lambda, int i);

int64_t HEARTBEAT_nest1_loop0_slice(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem);
int64_t HEARTBEAT_nest1_loop1_slice(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem);
typedef int64_t (*sliceTasksPointer)(uint64_t *, uint64_t *, uint64_t, task_memory_t *);
sliceTasksPointer slice_tasks_nest1[2] = {
  &HEARTBEAT_nest1_loop0_slice,
  &HEARTBEAT_nest1_loop1_slice
};

void HEARTBEAT_nest1_loop_1_0_leftover(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem);
typedef void (*leftoverTasksPointer)(uint64_t *, uint64_t *, uint64_t, task_memory_t *);
leftoverTasksPointer leftover_tasks_nest1[1] = {
  &HEARTBEAT_nest1_loop_1_0_leftover
};

uint64_t leftover_selector_nest1(uint64_t receivingLevel, uint64_t splittingLevel) {
  return 0;
}

// Outlined loops
void HEARTBEAT_nest1_loop0(int rows, int cols, float *J, float *dN, float *dS, float *dW, float *dE, float *c, int *iS, int *jE, float lambda) {
  if (run_heartbeat) {
    run_heartbeat = false;

    // allocate const live-ins
    uint64_t constLiveIns[10];
    constLiveIns[0] = (uint64_t)cols;
    constLiveIns[1] = (uint64_t)J;
    constLiveIns[2] = (uint64_t)dN;
    constLiveIns[3] = (uint64_t)dS;
    constLiveIns[4] = (uint64_t)dW;
    constLiveIns[5] = (uint64_t)dE;
    constLiveIns[6] = (uint64_t)c;
    constLiveIns[7] = (uint64_t)iS;
    constLiveIns[8] = (uint64_t)jE;
    constLiveIns[9] = (uint64_t)&lambda;

    // allocate cxts
    uint64_t cxts[NUM_LEVELS_NEST1 * CACHELINE];

    // set start/max iterations for loop0
    cxts[LEVEL_ZERO * CACHELINE + START_ITER] = (uint64_t)0;
    cxts[LEVEL_ZERO * CACHELINE + MAX_ITER] = (uint64_t)rows;

    // allocate the task memory struct and initialize
    task_memory_t tmem;
    heartbeat_start(&tmem);

    // invoke nest1_loop0 in heartbeat form
    HEARTBEAT_nest1_loop0_slice(cxts, constLiveIns, 0, &tmem);

    run_heartbeat = true;
  } else {
    for (int i = 0; i < rows; i++) {
      HEARTBEAT_nest1_loop1(cols, J, dN, dS, dW, dE, c, iS, jE, lambda, i);
    }
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

// Transformed loops
int64_t HEARTBEAT_nest1_loop0_slice(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem) {
  // load start/max iterations
  uint64_t startIter = cxts[LEVEL_ZERO * CACHELINE + START_ITER];
  uint64_t maxIter = cxts[LEVEL_ZERO * CACHELINE + MAX_ITER];

  // load const live-ins
  int cols = (int)constLiveIns[0];

  int64_t rc = 0;
  // store &live-in as live-in environment for loop1
  ((uint64_t *)cxts)[LEVEL_ONE * CACHELINE + LIVE_IN_ENV] = (uint64_t)&startIter;

  for (; startIter < maxIter; startIter++) {
    // store current iteration for loop0
    cxts[LEVEL_ZERO * CACHELINE + START_ITER] = startIter;
    // set start/max iterations for loop1
    cxts[LEVEL_ONE * CACHELINE + START_ITER] = (uint64_t)0;
    cxts[LEVEL_ONE * CACHELINE + MAX_ITER] = (uint64_t)cols;
    rc = HEARTBEAT_nest1_loop1_slice(cxts, constLiveIns, 0, tmem);

    // check the status of rc because, might not need
    // to call the loop_handler in the process of returnning up
    if (rc > 0) {
      break;
    }
  }

  return rc - 1;
}

int64_t HEARTBEAT_nest1_loop1_slice(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem) {
  // load start/max iterations
  uint64_t startIter = cxts[LEVEL_ONE * CACHELINE + START_ITER];
  uint64_t maxIter = cxts[LEVEL_ONE * CACHELINE + MAX_ITER];

  // load const live-ins
  int cols = (int)constLiveIns[0];
  float *J = (float *)constLiveIns[1];
  float *dN = (float *)constLiveIns[2];
  float *dS = (float *)constLiveIns[3];
  float *dW = (float *)constLiveIns[4];
  float *dE = (float *)constLiveIns[5];
  float *c = (float *)constLiveIns[6];
  int *iS = (int *)constLiveIns[7];
  int *jE = (int *)constLiveIns[8];
  float lambda = *(float *)constLiveIns[9];

  // load live-ins
  int i = *(int *)((uint64_t *)cxts)[LEVEL_ONE * CACHELINE + LIVE_IN_ENV];

  int64_t rc = 0;
#if defined(CHUNK_LOOP_ITERATIONS)
  uint64_t chunksize;
  for (; startIter < maxIter; startIter += chunksize) {
    chunksize = get_chunksize(tmem);
    uint64_t low = startIter;
    uint64_t high = maxIter < startIter + chunksize ? maxIter : startIter + chunksize;
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

#if defined(ENABLE_HEARTBEAT)
    if (update_and_has_remaining_chunksize(tmem, high - startIter, chunksize)) {
      break;
    }

#if defined(ENABLE_SOFTWARE_POLLING)
    if (unlikely(heartbeat_polling(tmem))) {
      cxts[LEVEL_ONE * CACHELINE + START_ITER] = low - 1;
      rc = loop_handler(
        cxts, constLiveIns, LEVEL_ONE, NUM_LEVELS_NEST1, tmem,
        slice_tasks_nest1, leftover_tasks_nest1, &leftover_selector_nest1
      );
      if (rc > 0) {
        break;
      }
    }
#else
    if(unlikely(__rf_test())) {
      cxts[LEVEL_ONE * CACHELINE + START_ITER] = low - 1;
      __rf_handle_wrapper(
        rc, cxts, constLiveIns, LEVEL_ONE, NUM_LEVELS_NEST1, tmem,
        slice_tasks_nest1, leftover_tasks_nest1, &leftover_selector_nest1
      );
      if (rc > 0) {
        break;
      }
    }
#endif
#endif
  }
#else
  for(; startIter < maxIter; startIter++) {

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

#if defined(ENABLE_HEARTBEAT)
#if defined(ENABLE_SOFTWARE_POLLING)
    if (unlikely(heartbeat_polling(tmem))) {
      cxts[LEVEL_ONE * CACHELINE + START_ITER] = startIter;
      rc = loop_handler(
        cxts, constLiveIns, LEVEL_ONE, NUM_LEVELS_NEST1, tmem,
        slice_tasks_nest1, leftover_tasks_nest1, &leftover_selector_nest1
      );
      if (rc > 0) {
        break;
      }
    }
#else
    if(unlikely(__rf_test())) {
      cxts[LEVEL_ONE * CACHELINE + START_ITER] = startIter;
      __rf_handle_wrapper(
        rc, cxts, constLiveIns, LEVEL_ONE, NUM_LEVELS_NEST1, tmem,
        slice_tasks_nest1, leftover_tasks_nest1, &leftover_selector_nest1
      );
      if (rc > 0) {
        break;
      }
    }
#endif
#endif
  }
#endif

  return rc - 1;
}

// Leftover tasks
void HEARTBEAT_nest1_loop_1_0_leftover(uint64_t *cxts, uint64_t *constLiveIns, uint64_t myIndex, task_memory_t *tmem) {
  int64_t rc = 0;
  rc = HEARTBEAT_nest1_loop1_slice(cxts, constLiveIns, myIndex, tmem);
  if (rc > 0) {
    return;
  }

  cxts[LEVEL_ZERO * CACHELINE + START_ITER]++;
  rc = HEARTBEAT_nest1_loop0_slice(cxts, constLiveIns, myIndex, tmem);
  if (rc > 0) {
    return;
  }

  return;
}

} // namespace srad
