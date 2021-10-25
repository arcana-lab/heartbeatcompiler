#include <stdlib.h>
#include <stdio.h>

int loop_handler (
    long long int startIteration, 
    long long int maxIteration, 
    void *env, 
    void (*f)(int64_t, int64_t, void *)
    ) {
  static long long int currentIter = 0;
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

int main() {
  int sz = 1000;
  int* a = (int*)calloc(sz, sizeof(int));
  for (int i = 0; i < sz; i++) {
    a[i]++;
  }
  printf("%d %d %d\n", a[0], a[sz/2], a[sz-1]);
  return 0;
}
