#include <stdlib.h>
#include <stdio.h>
#include "loop_handler.cpp"

int main() {
  int sz = 1000;
  int* a = (int*)calloc(sz, sizeof(int));
  for (int i = 0; i < sz; i++) {
    a[i]++;
  }
  printf("%d %d %d\n", a[0], a[sz/2], a[sz-1]);
  return 0;
}
