#include <stdlib.h>
#include <stdio.h>
#include "Heartbeats.hpp"

extern "C" {
  int loop_handler (
      long long int startIteration, 
      long long int maxIteration, 
      void *env, 
      void (*f)(int64_t, int64_t, void *)
      ) {
    static std::atomic_bool * me = hbm.addThread();
    static long long int currentIter = 0;

    /*
     * Check if an heartbeat happened.
     */
    if (!(*me)){
      return 0;
    }

    /*
     * A heartbeat happened.
     *
     * Step 1: reset the thread-specific heartbeat
     */
    (*me) = false;

    /*
     * Step 2: check if there is enough work
     */
    if ((maxIteration - startIteration) < 2){
      return 0;
    }

    /*
     * Step 3: split the remaining work
     */
    //TODO

    printf("Loop_handler: Start\n");
    printf("Loop_handler:   startIteration = %lld\n", startIteration);
    if (  0
          || ((startIteration % 2) == 0)
          || (startIteration == currentIter)
       ){
      printf("Loop_handler: Exit\n");
      return 0;
    }
    currentIter = startIteration;
    printf("Loop_handler:   Promotion\n");
    printf("Loop_handler:   maxIteration = %lld\n", maxIteration);
    

    (*f)(startIteration, maxIteration, env);

    printf("Loop_handler: Exit\n");
    return 1;
  }
}

int main() {
  int sz = 1000;
  int* a = (int*)calloc(sz, sizeof(int));
  for (int i = 0; i < sz; i++) {
    a[i]++;
  }
  printf("%d %d %d\n", a[0], a[sz/2], a[sz-1]);
  return 0;
}
