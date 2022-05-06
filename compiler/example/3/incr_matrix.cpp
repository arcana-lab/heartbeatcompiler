#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <iostream>
#include "loop_handler.cpp"

/*
for (uint64_t i = 0; i < rows; i++) {
  for (uint64_t j = 0; j < cols; j++){
    a[i][j]++;
  }
 }
*/

// need new signature for the general purpose handler and its semantics
// e.g., what's the meaning of the return value
/* handler for non nested case:

int loop_handler (
       long long int startIteration,
       long long int maxIteration,
       void *env,
       void (*f)(int64_t, int64_t, void *)
       ) {
  ... return 0 if no promotion, 1 if yes
}

For nested, we need a handler that takes something like a vector of
all arguments, corresponding to the nesting structure of the place
where the handler is called.

Potential convention: if handler returns 1, then...
Convention: introduce param for each handler function indicating its nesting depth. Level 1 represents outermost, 2 one level deeper, etc.
*/

void HEARTBEAT_loop_inner (int
  for (uint64_t j = 0; j < cols; j++){
    a[i][j]++;
  }
}

void HEARTBEAT_loop_outer (int32_t[][] a, uint64_t rows, uint64_t cols){
  for (uint64_t i = 0; i < rows; i++) {
    HEARTBEAT_loop_inner(a, i, cols);
  }
}

int main (int argc, char *argv[]) {
  if (argc < 3){
    std::cerr << "USAGE: " << argv[0] << " OUTER_ITERATIONS INNER_ITERATIONS" << std::endl;
    return 1;
  }
  auto sz = atoll(argv[1]);
  auto inner = atoi(argv[2]);
  auto a = (int32_t*)calloc(sz, sizeof(int32_t));
  
  HEARTBEAT_loop0(sz, a, inner);

  HEARTBEAT_loop1(sz, a, inner);

  printf("%lld %d %d %d\n", sz, a[0], a[sz/2], a[sz-1]);

  return 0;
}
