#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <iostream>
#ifdef ORIGINAL_PROGRAM
#include <chrono>
#endif
#include "loop_handler.cpp"

// // Original program
// static uint64_t a[M], b[M][N], c[M][N], d[M];

// for (i = 0; i < M; i++) { // loop0
//   a[i]++;
//   for (j = 0; j < N; j++) { // loop1
//     b[i][j]++;
//     for (k = 0; k < O; k++) { // loop2
//       array[i][j][k]++;
//     }
//     c[i][j]++;
//   }
//   d[i]++;
// }

bool heartbeat = true;

void HEARTBEAT_loop0(uint64_t ***array, uint64_t *a, uint64_t **b, uint64_t **c, uint64_t *d, uint64_t M, uint64_t N, uint64_t O);
void HEARTBEAT_loop1(uint64_t ***array, uint64_t **b, uint64_t **c, uint64_t i, uint64_t N, uint64_t O);
void HEARTBEAT_loop2(uint64_t ***array, uint64_t i, uint64_t j, uint64_t O);
void HEARTBEAT_loop0_cloned(uint64_t *startIterations, uint64_t *maxIterations, void **liveInEnvironments, uint64_t myLevel);
void HEARTBEAT_loop1_cloned(uint64_t *startIterations, uint64_t *maxIterations, void **liveInEnvironments, uint64_t myLevel);
void HEARTBEAT_loop2_cloned(uint64_t *startIterations, uint64_t *maxIterations, void **liveInEnvironments, uint64_t myLevel);
void HEARTBEAT_loop21_cloned(uint64_t *startIterations, uint64_t *maxIterations, void **liveInEnvironments, uint64_t startingLevel);
void HEARTBEAT_loop1_cloned2(uint64_t *startIterations, uint64_t *maxIterations, void **liveInEnvironments, uint64_t myLevel);
void HEARTBEAT_loop2_cloned2(uint64_t *startIterations, uint64_t *maxIterations, void **liveInEnvironments, uint64_t myLevel);

typedef void(*functionPointer)(uint64_t *, uint64_t *, void **, uint64_t);
functionPointer splittingTasks[5] = {
  &HEARTBEAT_loop0_cloned,
  &HEARTBEAT_loop1_cloned,
  &HEARTBEAT_loop2_cloned
};

functionPointer leftoverTasks[4] = {
  &HEARTBEAT_loop1_cloned2,
  &HEARTBEAT_loop2_cloned2,
  &HEARTBEAT_loop21_cloned
};

// leftover task for receiving level at 2 and splitting level at 0
void HEARTBEAT_loop21_cloned(uint64_t *startIterations, uint64_t *maxIterations, void **liveInEnvironments, uint64_t startingLevel) {
  std::cout << "Inside HEARTBEAT_loop21_cloned\n";

  std::cout << "loop2: startIterations = " << startIterations[startingLevel] << ", maxIterations = " << maxIterations[startingLevel] << std::endl;
  uint64_t *liveInEnvironmentsForLoop2[3] = { 0, 0, ((uint64_t **)liveInEnvironments)[startingLevel] };
  HEARTBEAT_loop2_cloned2(startIterations, maxIterations, (void **)liveInEnvironmentsForLoop2, startingLevel);

  startingLevel -= 1;

  std::cout << "loop1: startIterations = " << startIterations[startingLevel] << ", maxIterations = " << maxIterations[startingLevel] << std::endl;
  uint64_t *liveInEnvironmentsForLoop1[3] = { 0, ((uint64_t **)liveInEnvironments)[startingLevel], 0 };
  HEARTBEAT_loop1_cloned2(startIterations, maxIterations, (void **)liveInEnvironmentsForLoop1, startingLevel);

  return;
}

void HEARTBEAT_loop1_cloned2(uint64_t *startIterations, uint64_t *maxIterations, void **liveInEnvironments, uint64_t myLevel) {
  // load live-ins
  uint64_t *liveInEnvironment = (uint64_t *)((uint64_t **)liveInEnvironments)[myLevel];
  uint64_t ***array = (uint64_t ***)liveInEnvironment[0];
  uint64_t **b = (uint64_t **)liveInEnvironment[1];
  uint64_t **c = (uint64_t **)liveInEnvironment[2];
  uint64_t i = (uint64_t)liveInEnvironment[3];
  uint64_t O = (uint64_t)liveInEnvironment[4];

  // allocate live-in environment
  uint64_t liveInEnvironmentForLoop2[3];
  ((uint64_t **)liveInEnvironments)[myLevel + 1] = (uint64_t *)liveInEnvironmentForLoop2;

  // store live-in variables
  liveInEnvironmentForLoop2[0] = (uint64_t)array;
  liveInEnvironmentForLoop2[1] = (uint64_t)i;

  for (uint64_t j = startIterations[myLevel]; j < maxIterations[myLevel]; j++) {
    b[i][j]++;

    liveInEnvironmentForLoop2[2] = (uint64_t)j;

    // set iterations bound
    startIterations[myLevel + 1] = 0;
    maxIterations[myLevel + 1] = O;

    HEARTBEAT_loop2_cloned2(startIterations, maxIterations, (void **)liveInEnvironments, myLevel + 1);

    c[i][j]++;
  }

  return;
}

void HEARTBEAT_loop2_cloned2(uint64_t *startIterations, uint64_t *maxIterations, void **liveInEnvironments, uint64_t myLevel) {
  // load live-ins
  uint64_t *liveInEnvironment = (uint64_t *)((uint64_t **)liveInEnvironments)[myLevel];
  uint64_t ***array = (uint64_t ***)liveInEnvironment[0];
  uint64_t i = (uint64_t)liveInEnvironment[1];
  uint64_t j = (uint64_t)liveInEnvironment[2];

  for (uint64_t k = startIterations[myLevel]; k < maxIterations[myLevel]; k++) {
    array[i][j][k]++;
  }

  return;
}

void HEARTBEAT_loop0_cloned(uint64_t *startIterations, uint64_t *maxIterations, void **liveInEnvironments, uint64_t myLevel) {
  // load live-ins
  uint64_t *liveInEnvironment = (uint64_t *)((uint64_t **)liveInEnvironments)[myLevel];
  uint64_t ***array = (uint64_t ***)liveInEnvironment[0];
  uint64_t *a = (uint64_t *)liveInEnvironment[1];
  uint64_t **b = (uint64_t **)liveInEnvironment[2];
  uint64_t **c = (uint64_t **)liveInEnvironment[3];
  uint64_t *d = (uint64_t *)liveInEnvironment[4];
  uint64_t N = (uint64_t)liveInEnvironment[5];
  uint64_t O = (uint64_t)liveInEnvironment[6];

  // allocate live-in environment
  uint64_t liveInEnvironmentForLoop1[5];
  ((uint64_t **)liveInEnvironments)[myLevel + 1] = (uint64_t *)liveInEnvironmentForLoop1;

  // store live-in variables
  liveInEnvironmentForLoop1[0] = (uint64_t)array;
  liveInEnvironmentForLoop1[1] = (uint64_t)b;
  liveInEnvironmentForLoop1[2] = (uint64_t)c;
  liveInEnvironmentForLoop1[4] = (uint64_t)O;

  for (; startIterations[myLevel] < maxIterations[myLevel]; startIterations[myLevel]++) {
    int64_t rc = loop_handler(startIterations, maxIterations, liveInEnvironments, splittingTasks, leftoverTasks, myLevel);

    a[startIterations[myLevel]]++;

    liveInEnvironmentForLoop1[3] = (uint64_t)startIterations[myLevel];

    // set iterations bound
    startIterations[myLevel + 1] = 0;
    maxIterations[myLevel + 1] = N;

    HEARTBEAT_loop1_cloned(startIterations, maxIterations, (void **)liveInEnvironments, myLevel + 1);

    d[startIterations[myLevel]]++;
  }

  return;
}

void HEARTBEAT_loop1_cloned(uint64_t *startIterations, uint64_t *maxIterations, void **liveInEnvironments, uint64_t myLevel) {
  // load live-ins
  uint64_t *liveInEnvironment = (uint64_t *)((uint64_t **)liveInEnvironments)[myLevel];
  uint64_t ***array = (uint64_t ***)liveInEnvironment[0];
  uint64_t **b = (uint64_t **)liveInEnvironment[1];
  uint64_t **c = (uint64_t **)liveInEnvironment[2];
  uint64_t i = (uint64_t)liveInEnvironment[3];
  uint64_t O = (uint64_t)liveInEnvironment[4];

  // allocate live-in environment
  uint64_t liveInEnvironmentForLoop2[3];
  ((uint64_t **)liveInEnvironments)[myLevel + 1] = (uint64_t *)liveInEnvironmentForLoop2;

  // store live-in variables
  liveInEnvironmentForLoop2[0] = (uint64_t)array;
  liveInEnvironmentForLoop2[1] = (uint64_t)i;

  for (; startIterations[myLevel] < maxIterations[myLevel]; startIterations[myLevel]++) {
    int64_t rc = loop_handler(startIterations, maxIterations, liveInEnvironments, splittingTasks, leftoverTasks, myLevel);

    b[i][startIterations[myLevel]]++;

    liveInEnvironmentForLoop2[2] = (uint64_t)startIterations[myLevel];

    // set iterations bound
    startIterations[myLevel + 1] = 0;
    maxIterations[myLevel + 1] = O;

    HEARTBEAT_loop2_cloned(startIterations, maxIterations, (void **)liveInEnvironments, myLevel + 1);

    c[i][startIterations[myLevel]]++;
  }

  return;
}

void HEARTBEAT_loop2_cloned(uint64_t *startIterations, uint64_t *maxIterations, void **liveInEnvironments, uint64_t myLevel) {
  // load live-ins
  uint64_t *liveInEnvironment = (uint64_t *)((uint64_t **)liveInEnvironments)[myLevel];
  uint64_t ***array = (uint64_t ***)liveInEnvironment[0];
  uint64_t i = (uint64_t)liveInEnvironment[1];
  uint64_t j = (uint64_t)liveInEnvironment[2];

  for (; startIterations[myLevel] < maxIterations[myLevel]; startIterations[myLevel]++) {
    int64_t rc = loop_handler(startIterations, maxIterations, liveInEnvironments, splittingTasks, leftoverTasks, myLevel);

    array[i][j][startIterations[myLevel]]++;
  }

  return;
}

void HEARTBEAT_loop0(uint64_t ***array, uint64_t *a, uint64_t **b, uint64_t **c, uint64_t *d, uint64_t M, uint64_t N, uint64_t O) {
  if (heartbeat == true) {
    heartbeat = false;

    // allocate live-in environment
    uint64_t *liveInEnvironments[3];
    uint64_t liveInEnvironmentForLoop0[7];
    liveInEnvironments[0] = (uint64_t *)liveInEnvironmentForLoop0;
    
    // store live-in variables
    liveInEnvironmentForLoop0[0] = (uint64_t)array;
    liveInEnvironmentForLoop0[1] = (uint64_t)a;
    liveInEnvironmentForLoop0[2] = (uint64_t)b;
    liveInEnvironmentForLoop0[3] = (uint64_t)c;
    liveInEnvironmentForLoop0[4] = (uint64_t)d;
    liveInEnvironmentForLoop0[5] = (uint64_t)N;
    liveInEnvironmentForLoop0[6] = (uint64_t)O;

    // allocate iterations array
    uint64_t startIterations[3];
    uint64_t maxIterations[3];

    // set iterations bound
    startIterations[0] = 0;
    maxIterations[0] = M;

    loop_dispatcher(startIterations, maxIterations, (void **)liveInEnvironments, &HEARTBEAT_loop0_cloned);

    heartbeat = true;
  } else {
    for (uint64_t i = 0; i < M; i++) {
      a[i]++;

      HEARTBEAT_loop1(array, b, c, i, N, O);

      d[i]++;
    }
  }

  return;
}

void HEARTBEAT_loop1(uint64_t ***array, uint64_t **b, uint64_t **c, uint64_t i, uint64_t N, uint64_t O) {
  for (uint64_t j = 0; j < N; j++) {
    b[i][j]++;

    HEARTBEAT_loop2(array, i, j, O);

    c[i][j]++;
  }

  return;
}

void HEARTBEAT_loop2(uint64_t ***array, uint64_t i, uint64_t j, uint64_t O) {
  for (uint64_t k = 0; k < O; k++) {
    array[i][j][k]++;
  }

  return;
}

int main (int argc, char *argv[]) {
  if (argc < 3) {
    std::cerr << "USAGE: " << argv[0] << " OUTER_ITERATIONS INNER_ITERATIONS" << std::endl;
    return 1;
  }
  uint64_t M = atoll(argv[1]);
  uint64_t N = atoll(argv[2]);
  uint64_t O = atoll(argv[3]);

  uint64_t ***array = (uint64_t ***)calloc(M, sizeof(uint64_t **));
  for (uint64_t i = 0; i < M; i++) {
    array[i] = (uint64_t **)calloc(N, sizeof(uint64_t *));
    for (uint64_t j = 0; j < N; j++) {
      array[i][j] = (uint64_t *)calloc(O, sizeof(uint64_t));
    }
  }

  uint64_t *a = (uint64_t *)calloc(M, sizeof(uint64_t));
  uint64_t **b = (uint64_t **)calloc(M, sizeof(uint64_t *));
  uint64_t **c = (uint64_t **)calloc(M, sizeof(uint64_t *));
  uint64_t *d = (uint64_t *)calloc(M, sizeof(uint64_t));
  for (uint64_t i = 0; i < M; i++) {
    b[i] = (uint64_t *)calloc(N, sizeof(uint64_t));
    c[i] = (uint64_t *)calloc(N, sizeof(uint64_t));
  }

#ifdef ORIGINAL_PROGRAM
  using clock = std::chrono::system_clock;
  using sec = std::chrono::duration<double>;
  const auto before = clock::now();
#endif


  HEARTBEAT_loop0(array, a, b, c, d, M, N, O);

  uint64_t accum_a = 0;
  uint64_t accum_b = 0;
  uint64_t accum_c = 0;
  uint64_t accum_d = 0;

  for (uint64_t i = 0; i < M; i++) {
    uint64_t accum_array_i = 0;
    for (uint64_t j = 0; j < N; j++) {
      for (uint64_t k = 0; k < O; k++) {
        accum_array_i += array[i][j][k];
      }
    }
    std::cout << "Sum of array[" << i << "] = " << accum_array_i << std::endl;
  }

  for (uint64_t i = 0; i < M; i++) {
    accum_a += a[i];
    for (uint64_t j = 0; j < N; j++) {
      accum_b += b[i][j];
      accum_c += c[i][j];
    }
    accum_d += d[i];
  }

  std::cout << "Sum of a array = " << accum_a << std::endl;
  std::cout << "Sum of b array = " << accum_b << std::endl;
  std::cout << "Sum of c array = " << accum_c << std::endl;
  std::cout << "Sum of d array = " << accum_d << std::endl;


#ifdef ORIGINAL_PROGRAM
  const sec duration = clock::now() - before;
  printf("[{\"exectime\":  %f}]\n", duration.count());
#endif
  

  return 0;
}
