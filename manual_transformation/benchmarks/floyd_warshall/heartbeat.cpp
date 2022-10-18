#include "floyd_warshall.hpp"
#include "loop_handler.hpp"

#define SUB(array, row_sz, i, j) (array[i * row_sz + j])

namespace floyd_warshall {

void floyd_warshall_heartbeat(int *, int);
void HEARTBEAT_loop0(int *, int, int);
void HEARTBEAT_loop1(int *, int, int, int);
uint64_t HEARTBEAT_loop0_cloned(uint64_t *, uint64_t *, void **, uint64_t);
uint64_t HEARTBEAT_loop1_cloned(uint64_t *, uint64_t *, void **, uint64_t);

typedef uint64_t(*functionPointer)(uint64_t *, uint64_t *, void **, uint64_t);
functionPointer clonedTasks[2] = {
  &HEARTBEAT_loop0_cloned,
  &HEARTBEAT_loop1_cloned
};

// Global variable indicating whether to run loops in the heartbeat form
bool run_heartbeat = true;

void floyd_warshall_heartbeat(int *dist, int vertices) {
  for (int via = 0; via < vertices; via++) {
    HEARTBEAT_loop0(dist, vertices, via);
  }
}

// Outlined loops
void HEARTBEAT_loop0(int *dist, int vertices, int via) {
  if (run_heartbeat) {
    run_heartbeat = false;

    // allocate live-in environment for loop0
    uint64_t *liveInEnvironments[2];
    uint64_t liveInEnvironmentForLoop0[3];
    liveInEnvironments[0] = (uint64_t *)liveInEnvironmentForLoop0;

    // store into live-in environment
    liveInEnvironmentForLoop0[0] = (uint64_t)dist;
    liveInEnvironmentForLoop0[1] = (uint64_t)vertices;
    liveInEnvironmentForLoop0[2] = (uint64_t)via;

    // allocate the startIterations and maxIterations array
    uint64_t startIterations[2];
    uint64_t maxIterations[2];
    
    // set the start and max iteration for loop0
    startIterations[0] = 0;
    maxIterations[0] = vertices;

    // invoke loop0 in heartbeat form
    HEARTBEAT_loop0_cloned(startIterations, maxIterations, (void **)liveInEnvironments, 0);

    run_heartbeat = true;
  } else {
    for (int from = 0; from < vertices; from++) {
      HEARTBEAT_loop1(dist, vertices, via, from);
    }
  }

  return;
}

void HEARTBEAT_loop1(int *dist, int vertices, int via, int from) {
  for (int to = 0; to < vertices; to++) {
    if ((from != to) && (from != via) && (to != via)) {
      SUB(dist, vertices, from, to) =
        std::min(SUB(dist, vertices, from, to),
                SUB(dist, vertices, from, via) + SUB(dist, vertices, via, to));
    }
  }

  return;
}

uint64_t HEARTBEAT_loop0_cloned(uint64_t *startIterations, uint64_t *maxIterations, void **liveInEnvironments, uint64_t myLevel) {
  // load live-in environment
  uint64_t *liveInEnvironment = ((uint64_t **)liveInEnvironments)[myLevel];
  int *dist = (int *)liveInEnvironment[0];
  int vertices = (int)liveInEnvironment[1];
  int via = (int)liveInEnvironment[2];

  // allocate live-in environment for loop1
  uint64_t liveInEnvironmentForLoop1[4];
  ((uint64_t **)liveInEnvironments)[myLevel + 1] = (uint64_t *)liveInEnvironmentForLoop1;

  // store into loop1's live-in environment
  liveInEnvironmentForLoop1[0] = (uint64_t)dist;
  liveInEnvironmentForLoop1[1] = (uint64_t)vertices;
  liveInEnvironmentForLoop1[2] = (uint64_t)via;

  // allocate returnLevel variable and initialize
  uint64_t returnLevel = 1 + 1;

  for (; startIterations[myLevel] < maxIterations[myLevel]; startIterations[myLevel]++) {
    int rc = loop_handler(startIterations, maxIterations, (void **)liveInEnvironments, myLevel, returnLevel, clonedTasks);
    if (rc == 1) {
      goto reduction;
    }

    // store into loop1's live-in environment
    liveInEnvironmentForLoop1[3] = (uint64_t)startIterations[myLevel];  // from
    
    // set the start and max iteation for loop1
    startIterations[myLevel + 1] = 0;
    maxIterations[myLevel + 1] = vertices;

    returnLevel = HEARTBEAT_loop1_cloned(startIterations, maxIterations, liveInEnvironments, myLevel + 1);
  }

  if (returnLevel == myLevel) {
reduction:
    // synchronization point for the leftover task (splitting at the same level and return from a higher nesting level)
    // barrier_wait();
    return returnLevel;
  }

  // synchronization point for the splitting task
  // barrier_wait();
  return returnLevel;
}

uint64_t HEARTBEAT_loop1_cloned(uint64_t *startIterations, uint64_t *maxIterations, void **liveInEnvironments, uint64_t myLevel) {
  // load live-in environment
  uint64_t *liveInEnvironment = ((uint64_t **)liveInEnvironments)[myLevel];
  int *dist = (int *)liveInEnvironment[0];
  int vertices = (int)liveInEnvironment[1];
  int via = (int)liveInEnvironment[2];
  int from = (int)liveInEnvironment[3];

  // allocate returnLevel variable and initialize
  uint64_t returnLevel = 1 + 1;

  for (; startIterations[myLevel] < maxIterations[myLevel]; startIterations[myLevel]++) {
    int rc = loop_handler(startIterations, maxIterations, (void **)liveInEnvironments, myLevel, returnLevel, clonedTasks);
    if (rc == 1) {
      if (returnLevel < myLevel) {
        goto leftover;
      }
      goto reduction;
    }
    if ((from != startIterations[myLevel]) && (from != via) && (startIterations[myLevel] != via)) {
      SUB(dist, vertices, from, startIterations[myLevel]) =
        std::min(SUB(dist, vertices, from, startIterations[myLevel]),
                SUB(dist, vertices, from, via) + SUB(dist, vertices, via, startIterations[myLevel]));
    }
  }

  if (returnLevel == myLevel) {
reduction:
    // synchronization point for the leftover task (splitting at the same level and return from a higher nesting level)
    // barrier_wait();
    return returnLevel;
  }

  // synchronization point for the splitting task
  // barrier_wait();
  return returnLevel;

leftover:
  for (; startIterations[myLevel] < maxIterations[myLevel]; startIterations[myLevel]++) {
    if ((from != startIterations[myLevel]) && (from != via) && (startIterations[myLevel] != via)) {
      SUB(dist, vertices, from, startIterations[myLevel]) =
        std::min(SUB(dist, vertices, from, startIterations[myLevel]),
                SUB(dist, vertices, from, via) + SUB(dist, vertices, via, startIterations[myLevel]));
    }
  }
  return returnLevel;
}

}

using namespace floyd_warshall;

int main() {

  setup();

  loop_dispatcher(&floyd_warshall_heartbeat, dist, vertices);

#if defined(TEST_CORRECTNESS)
  test_correctness(dist);
#endif

  finishup();

  return 0;

}