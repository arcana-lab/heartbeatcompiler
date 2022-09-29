#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <iostream>
#ifdef ORIGINAL_PROGRAM
#include <chrono>
#endif
#include "loop_handler.cpp"

/*
 * Task for Federico Sossai
 * 1. Loop outlining
 * 2. Loop cloning
 * 3. Loop environment for live-in and live-out
 * 4. Heartbeat transformation for the cloning task
 * 5. Modify loop_handler logic
 * 6. Ensure correct output
 */

uint64_t *b;
uint64_t *c;

int main (int argc, char *argv[]) {
  if (argc < 3){
    std::cerr << "USAGE: " << argv[0] << " OUTER_ITERATIONS INNER_ITERATIONS" << std::endl;
    return 1;
  }

  uint64_t M = atoll(argv[1]);
  uint64_t N = atoll(argv[2]);
  uint64_t O = atoll(argv[3]);
  uint64_t **A = (uint64_t **)calloc(M, sizeof(uint64_t *));
  uint64_t **B = (uint64_t **)calloc(M, sizeof(uint64_t *));
  for (uint64_t i = 0; i < M; i++) {
    A[i] = (uint64_t *)calloc(N, sizeof(uint64_t));
    B[i] = (uint64_t *)calloc(N, sizeof(uint64_t));
  }
  b = (uint64_t *)calloc(M, sizeof(uint64_t));
  c = (uint64_t *)calloc(M, sizeof(uint64_t));

  
#ifdef ORIGINAL_PROGRAM
  using clock = std::chrono::system_clock;
  using sec = std::chrono::duration<double>;
  const auto before = clock::now();
#endif

///////////////////////////////////////
// Modification starts within in this area
// Following is the target loop, comment this part out
// after making the heartbeat transformation
  for (uint64_t i = 0; i < M; i++) {
    b[i]++;

    for (uint64_t j = 0; j < N; j++) {
      
      for (uint64_t k = 0; k < O; k++) {
        A[i][j]++;
        B[i][j]++;
      }
    
    }

    c[i]++;
  }
/////////////////////////////////////

  uint64_t accum_b = 0;
  uint64_t accum_c = 0;
  for (uint64_t i = 0; i < M; i++) {
    uint64_t accum_A_i = 0;
    uint64_t accum_B_i = 0;
    for (uint64_t j = 0; j < N; j++) {
      accum_A_i += A[i][j];
      accum_B_i += B[i][j];
    }
    std::cout << "sum of A[" << i << "] + B[" << i << "] = " << accum_A_i + accum_B_i << std::endl;
  
    accum_b += b[i];
    accum_c += c[i];
  }
  std::cout << "sum of b = " << accum_b << std::endl;
  std::cout << "sum of c = " << accum_c << std::endl;


#ifdef ORIGINAL_PROGRAM
  const sec duration = clock::now() - before;
  printf("[{\"exectime\":  %f}]\n", duration.count());
#endif
  

  return 0;
}
