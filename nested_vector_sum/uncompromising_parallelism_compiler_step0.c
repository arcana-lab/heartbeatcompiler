int myNestedSum (int **m, int mSize){
  int totalT = 0;
  myNestedSum(m, 0, mSize, mSize, &totalT);
  return totalT;
}

int mySum (int v[], int size){
  int t=0;

  for (int i=0; i < size; i++){
    t += v[i];
  }
  
  return t;
}

static void myNestedSum_helper (int **m, int low, high, int mSize, int *totalT){
  for (int j=low; j < high; j++){
    int *currentV = m[j];
    (*totalT) += mySum(currentV, mSize);
  }
}
