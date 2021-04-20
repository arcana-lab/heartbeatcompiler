int myOutermostSum (int **m, int mSize){
  int totalT = 0;
  myOutermostSum(m, 0, mSize, mSize, &totalT);
  return totalT;
}

static void myOutermostSum_helper (int **m, int low, high, int mSize, int *totalT){
  for (int j=low; j < high; j++){
    if (heartbeat){
      auto promoted = tryPromoteOutermost(m, j, high, totalT);
      if (promoted) {
        return ;
      }
    }

    int *currentV = m[j];
    mySum_helper(currentV, 0, mSize, totalT);
  }
}

static void mySum_helper (int v[], int low, int high, int *t){
  for (int i=low; i < high; i++){
    auto promoted = tryPromoteInnermost(v, i, high, t);
    if (promoted) {
      return ;
    }

    (*t) += v[i];
  }
}
