int mySum (int v[], int size){
  int t=0;
  mySum_helper(v, 0, size, &t);
  return t;
}

static void mySum_helper (int v[], int low, int high, int *t){
  for (int i=low; i < high; i++){
    (*t) += v[i];
  }
}
