int myNestedSum (int **m, int mSize){
  int totalT = 0;

  #pragma omp parallel for reduction(+:totalT) firstprivate(mSize) shared(m) default(none)  schedule(static, 1) num_cores(mSize)
  for (int j=0; j < mSize; j++){
    int *currentV = m[j];
    totalT += mySum(currentV, mSize);
  }
}


int mySum (int v[], int size){
  int t=0;

  #pragma omp parallel for reduction(+:t) firstprivate(size) shared(v) default(none) schedule(static, 1) num_cores(size)
  for (int i=0; i < size; i++){
    t += v[i];
  }
  
  return t;
}
