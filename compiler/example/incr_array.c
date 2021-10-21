#include <stdlib.h>
#include <stdio.h>

int heartbeat ;
int loop_handler (long long int startIteration, long long int maxItereation, void *env) {
   return 0;
}

int NOELLE_DOALLDispatcher (void){
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
