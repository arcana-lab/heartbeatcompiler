int mySum (int v[], int size){
  int t=0;

  #pragma omp parallel for reduction(+:t) firstprivate(size) shared(v) default(none) default(static, 1) num_cores(size)
  for (int i=0; i < size; i++){
    t += v[i];
  }
  
  return t;
}
