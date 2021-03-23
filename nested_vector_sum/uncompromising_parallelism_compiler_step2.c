int myNestedSum (int **m, int mSize){
  int totalT = 0;
  myNestedSum(m, 0, mSize, mSize, &totalT);
  return totalT;
}

static void myNestedSum_helper (int **m, int low, high, int mSize, int *totalT){
  for (int j=low; j < high; j++){
    int *currentV = m[j];
    mySum_helper(currentV, 0, mSize, totalT);
  }
}

static void mySum_helper (int v[], int low, int high, int *t){
  for (int i=low; i < high; i++){
    (*t) += v[i];
  }
}
