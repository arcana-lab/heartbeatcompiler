#include <stdlib.h>
#include <stdio.h>

#if 0
int loop_handler(int* a, int i, int sz) {
  if (sz - i < 2) {
    return 0; // no promotion
  }
  int mid = (i + sz) / 2;
  // fork in parallel (call tpal runtime)
  incr_array(a, i, mid); 
  incr_array(a, mid, sz);
  // block until both branches finish
  return 1;
}
void incr_array(int* a, int i, int sz) {
  for (int i = 0; i < sz; i++) {
    if (heartbeat) {
      loop_handler(a, i, sz);
    }
    a[i]++;
  }
}
#endif

int main() {
  int sz = 1000;
  int* a = (int*)malloc(sizeof(int) * sz);
  for (int i = 0; i < sz; i++) {
    a[i]++;
  }  
  return 0;
}
