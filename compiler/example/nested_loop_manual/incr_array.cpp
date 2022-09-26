#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <iostream>
#ifdef ORIGINAL_PROGRAM
#include <chrono>
#endif
#include "loop_handler.cpp"


// // Original program
// static uint64_t b[sz], c[sz];
// uint64_t a[sz][innerIterations];

// for (i = 0; i < sz; i++) {

//   b[i]++;

//   for (j = 0; j < innerIterations; j++) {
//     a[i][j]++;
//   }

//   c[i]++;

// }

uint64_t *b;
uint64_t *c;

void HEARTBEAT_loop0 (uint64_t **, uint64_t, uint64_t);
void HEARTBEAT_loop1 (uint64_t **, uint64_t, uint64_t);
uint64_t HEARTBEAT_loop0_cloned (uint64_t *, uint64_t *, void **, uint64_t);
uint64_t HEARTBEAT_loop1_cloned (uint64_t *, uint64_t *, void **, uint64_t);

typedef uint64_t(*functionPointer)(uint64_t *, uint64_t *, void **, uint64_t);
functionPointer clonedTasks[2] = {
  &HEARTBEAT_loop0_cloned,
  &HEARTBEAT_loop1_cloned
};

bool heartbeat_running = false;

void HEARTBEAT_loop0 (uint64_t **a, uint64_t sz, uint64_t innerIterations) {
  // either execute the whole loop in the heartbeat form or not
  if (heartbeat_running == false) {
    heartbeat_running = true;

    // allocate the live-in environments
    // for loop0, the live-ins are a, innerIterations
    uint64_t *liveInEnvironments[2];
    uint64_t liveInEnvironment[2];
    liveInEnvironments[0] = liveInEnvironment;
    liveInEnvironment[0] = (uint64_t)a;
    liveInEnvironment[1] = (uint64_t)innerIterations;

    // allocate the startIteration array
    uint64_t startIterations[2];
    startIterations[0] = 0;

    // allocate the maxIteration array
    uint64_t maxIterations[2];
    maxIterations[0] = sz;

    // invoking loop_dispatcher to execute the loop0 in its heartbeat format
    loop_dispatcher(startIterations, maxIterations, (void **)liveInEnvironments, &HEARTBEAT_loop0_cloned);
    
    heartbeat_running = false;
    goto ret;
  }

  for (uint64_t i = 0; i < sz; i++) {
    b[i]++;
    HEARTBEAT_loop1(a, i, innerIterations);
    c[i]++;
  }

ret:
  return;
}

void HEARTBEAT_loop1 (uint64_t **a, uint64_t i, uint64_t innerIterations) {
  for (uint64_t j = 0; j < innerIterations; j++) {
    a[i][j]++;
  }

  return;
}

uint64_t HEARTBEAT_loop0_cloned (uint64_t *startIterations, uint64_t *maxIterations, void **liveInEnvironments, uint64_t myLevel) {
  // load live-in environment
  uint64_t *liveInEnvironment = ((uint64_t **)liveInEnvironments)[myLevel];
  uint64_t **a = (uint64_t **)liveInEnvironment[0];
  uint64_t innerIterations = (uint64_t)liveInEnvironment[1];

  // allocate the live-in environment for loop1
  // the live-ins are a, i
  uint64_t liveInEnvironmentForLoop1[2];
  ((uint64_t **)liveInEnvironments)[myLevel + 1] = (uint64_t *)liveInEnvironmentForLoop1;

  // allocate returnLevel variable and initialize
  uint64_t returnLevel = 1 + 1;

  for (; startIterations[myLevel] < maxIterations[myLevel]; startIterations[myLevel]++) {
    uint64_t rc = loop_handler(startIterations, maxIterations, liveInEnvironments, clonedTasks, myLevel, &returnLevel);
    if (rc != -1) {
      // the return level checking at the root level loop is redundant because the lowest 
      // returnLevel can not be smaller than 0, which will always return false
      // 2 < 0 --> false
      // 0 < 0 --> false
      // therfore we don't need to generate code here and jump to the leftover task
      
      // barrier_wait();
      goto reduction;
    }

    b[startIterations[myLevel]]++;

    // prepare loop environment for loop1
    liveInEnvironmentForLoop1[0] = (uint64_t)a;
    liveInEnvironmentForLoop1[1] = (uint64_t)startIterations[myLevel];

    // start and max iterations for next loop is set by the parent task
    startIterations[myLevel + 1] = 0;
    maxIterations[myLevel + 1] = innerIterations;
    returnLevel = HEARTBEAT_loop1_cloned(startIterations, maxIterations, liveInEnvironments, myLevel + 1);

    c[startIterations[myLevel]]++;
  }
reduction:
    // barrier_wait();
    // reduce on children's live-out environment
    return returnLevel;

  // there also won't be any left over task for the root level loop because this is the loop
  // that everyone will always trying to priortize splitting
}

uint64_t HEARTBEAT_loop1_cloned (uint64_t *startIterations, uint64_t *maxIterations, void **liveInEnvironments, uint64_t myLevel) {
  // load live-in environment
  uint64_t *liveInEnvironment = ((uint64_t **)liveInEnvironments)[myLevel];
  uint64_t **a = (uint64_t **)liveInEnvironment[0];
  uint64_t i = (uint64_t)liveInEnvironment[1];

  // allocate returnLevel variable and initialize
  uint64_t returnLevel = 1 + 1;

  for (; startIterations[myLevel] < maxIterations[myLevel]; startIterations[myLevel]++) {
    uint64_t rc = loop_handler(startIterations, maxIterations, liveInEnvironments, clonedTasks, myLevel, &returnLevel);
    if (rc != -1) {
      if (returnLevel < myLevel) {
        goto leftover;
      }

      goto reduction;
    }

    a[i][startIterations[myLevel]]++;
  }
reduction:
  // barrier_wait();
  // // reduce on children's live-out environment
  return returnLevel;

leftover:
  for (; startIterations[myLevel] < maxIterations[myLevel]; startIterations[myLevel]++) {
    a[i][startIterations[myLevel]]++;
  }
  return returnLevel;
}

int main (int argc, char *argv[]) {
  if (argc < 3) {
    std::cerr << "USAGE: " << argv[0] << " OUTER_ITERATIONS INNER_ITERATIONS" << std::endl;
    return 1;
  }
  uint64_t sz = atoll(argv[1]);
  uint64_t inner = atoi(argv[2]);
  uint64_t **a = (uint64_t **)calloc(sz, sizeof(uint64_t));
  for (uint64_t i = 0; i < sz; i++) {
    a[i] = (uint64_t *)calloc(inner, sizeof(uint64_t));
  }
  b = (uint64_t *)calloc(sz, sizeof(uint64_t));
  c = (uint64_t *)calloc(sz, sizeof(uint64_t));



#ifdef ORIGINAL_PROGRAM
  using clock = std::chrono::system_clock;
  using sec = std::chrono::duration<double>;
  const auto before = clock::now();
#endif


  HEARTBEAT_loop0(a, sz, inner);

  uint64_t accum_b = 0;
  uint64_t accum_c = 0;
  for (uint64_t i = 0; i < sz; i++) {
    uint64_t accum_a_i = 0;
    for (uint64_t j = 0; j < inner; j++) {
      accum_a_i += a[i][j];
    }
    std::cout << i << ": " << accum_a_i << std::endl;
    accum_b += b[i];
    accum_c += c[i];
  }
  std::cout << "Sum of b array = " << accum_b << std::endl;
  std::cout << "Sum of c array = " << accum_c << std::endl;


#ifdef ORIGINAL_PROGRAM
  const sec duration = clock::now() - before;
  printf("[{\"exectime\":  %f}]\n", duration.count());
#endif
  

  return 0;
}
