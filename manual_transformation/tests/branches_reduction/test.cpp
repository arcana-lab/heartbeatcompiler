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
// uint64_t a[sz];

// for (i = 0; i < sz; i++) {

//   b[i]++;

//   for (j = 0; j < innerIterations; j++) {
//     a[i]++;
//   }

//   c[i]++;

// }

uint64_t *b;
uint64_t *c;

void HEARTBEAT_loop0 (uint64_t *, uint64_t, uint64_t);
void HEARTBEAT_loop1 (uint64_t *, uint64_t, uint64_t);
uint64_t HEARTBEAT_loop0_cloned (uint64_t *, uint64_t *, void **, uint64_t, void *, uint64_t);
uint64_t HEARTBEAT_loop1_cloned (uint64_t *, uint64_t *, void **, uint64_t, void *, uint64_t);

typedef uint64_t(*functionPointer)(uint64_t *, uint64_t *, void **, uint64_t, void *, uint64_t);
functionPointer clonedTasks[2] = {
  &HEARTBEAT_loop0_cloned,
  &HEARTBEAT_loop1_cloned
};

bool heartbeat_running = false;

void HEARTBEAT_loop0 (uint64_t *a, uint64_t sz, uint64_t innerIterations) {
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

void HEARTBEAT_loop1 (uint64_t *a, uint64_t i, uint64_t innerIterations) {
  for (uint64_t j = 0; j < innerIterations; j++) {
    a[i]++;
  }

  return;
}

uint64_t HEARTBEAT_loop0_cloned (uint64_t *startIterations, uint64_t *maxIterations, void **liveInEnvironments, uint64_t myLevel, void *liveOutEnvironment, uint64_t myJobIndex) {
  // load live-in environment
  uint64_t *liveInEnvironment = ((uint64_t **)liveInEnvironments)[myLevel];
  uint64_t *a = (uint64_t *)liveInEnvironment[0];
  uint64_t innerIterations = (uint64_t)liveInEnvironment[1];

  // allocate the live-in environment for loop1
  // the live-ins are a, i
  uint64_t liveInEnvironmentForLoop1[2];
  ((uint64_t **)liveInEnvironments)[myLevel + 1] = (uint64_t *)liveInEnvironmentForLoop1;

  // allocate the live-out environment for loop1
  // the live-out is a[i]
  uint64_t liveOutEnvironmentForLoop1[1];

  // allocate the reduction array for a[i] 
  uint64_t reductionArray[1];

  // store reductionArray into the live-out environment
  liveOutEnvironmentForLoop1[0] = (uint64_t)reductionArray;

  // allocate returnLevel variable and initialize
  uint64_t returnLevel = 1 + 1;

  for (; startIterations[myLevel] < maxIterations[myLevel]; startIterations[myLevel]++) {
    int64_t rc = loop_handler(startIterations, maxIterations, liveInEnvironments, clonedTasks, myLevel, &returnLevel, nullptr);
    if (rc == 1) {
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
    liveInEnvironmentForLoop1[0] = (uint64_t)a; // this environmentt is actually a variant and will be optimized through LICM
    liveInEnvironmentForLoop1[1] = (uint64_t)startIterations[myLevel];

    // start and max iterations for next loop is set by the parent task
    startIterations[myLevel + 1] = 0;
    maxIterations[myLevel + 1] = innerIterations;
    returnLevel = HEARTBEAT_loop1_cloned(startIterations, maxIterations, liveInEnvironments, myLevel + 1, (void *)liveOutEnvironmentForLoop1, 0);
    
    // reduction for live-out environment of loop1
    a[startIterations[myLevel]] = ((uint64_t *)liveOutEnvironmentForLoop1[0])[0];

    c[startIterations[myLevel]]++;
  }
  if (returnLevel == myLevel) {
reduction:
    // barrier_wait();
    // reduce on children's live-out environment
    return returnLevel;
  }

  // there also won't be any left over task for the root level loop because this is the loop
  // that everyone will always trying to priortize splitting
  
  // store into live-out environment
  // barrier_wait();
  return returnLevel;
}

uint64_t HEARTBEAT_loop1_cloned (uint64_t *startIterations, uint64_t *maxIterations, void **liveInEnvironments, uint64_t myLevel, void *liveOutEnvironment, uint64_t myJobIndex) {
  // load live-in environment
  uint64_t *liveInEnvironment = ((uint64_t **)liveInEnvironments)[myLevel];
  uint64_t *a = (uint64_t *)liveInEnvironment[0];
  uint64_t i = (uint64_t)liveInEnvironment[1];

  // allocate live-out environment for childrens
  uint64_t liveOutEnvironmentForChildren[0];

  // allocate the reduction array for children
  uint64_t reductionArrayForChildren[2];
  liveOutEnvironmentForChildren[0] = (uint64_t)reductionArrayForChildren;

  // initialize reduction array to be the identity value
  uint64_t *reductionArray = (uint64_t *)(((uint64_t *)liveOutEnvironment)[0]);
  reductionArray[myJobIndex] = 0;

  // allocate returnLevel variable and initialize
  uint64_t returnLevel = 1 + 1;

  // allocate stack variable for accumulate results
  uint64_t a_i = 0;

  for (; startIterations[myLevel] < maxIterations[myLevel]; startIterations[myLevel]++) {
    int64_t rc = loop_handler(startIterations, maxIterations, liveInEnvironments, clonedTasks, myLevel, &returnLevel, (void *)liveOutEnvironmentForChildren);
    if (rc == 1) {
      if (returnLevel < myLevel) {
        goto leftover;
      }

      goto reduction;
    }

    a_i++;
  }
  if (returnLevel == myLevel) {
reduction:
    // barrier_wait();
    // reduce on children's live-out environment
    reductionArray[myJobIndex] += a_i + reductionArrayForChildren[0] + reductionArrayForChildren[1];
    return returnLevel;
  }
  // store into live-out environment
  reductionArray[myJobIndex] = a_i;
  // barrier_wait();
  return returnLevel;

leftover:
  for (; startIterations[myLevel] < maxIterations[myLevel]; startIterations[myLevel]++) {
    a_i++;
  }
  // store into live-out environment
  reductionArray[myJobIndex] = a_i;
  return returnLevel;
}

int main (int argc, char *argv[]) {
  if (argc < 3) {
    std::cerr << "USAGE: " << argv[0] << " OUTER_ITERATIONS INNER_ITERATIONS" << std::endl;
    return 1;
  }
  uint64_t sz = atoll(argv[1]);
  uint64_t inner = atoi(argv[2]);
  uint64_t *a = (uint64_t *)calloc(sz, sizeof(uint64_t));
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
    std::cout << i << ": " << a[i] << std::endl;
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
