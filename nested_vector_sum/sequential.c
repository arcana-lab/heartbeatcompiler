int myNestedSum (int **m, int mSize){
  int totalT = 0;
  for (int j=0; j < mSize; j++){
    int *currentV = m[j];
    totalT += mySum(currentV, mSize);
  }
}

int mySum (int v[], int size){
  int t=0;

  for (int i=0; i < size; i++){
    t += v[i];
  }
  
  return t;
}
