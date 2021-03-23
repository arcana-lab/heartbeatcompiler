int myNestedSum (int **m, int mSize){
  int totalT = 0;
  myNestedSum(m, 0, mSize, mSize, &totalT);
  return totalT;
}

static void myNestedSum_helper (
  int **m, int low, high, int mSize, int *totalT    // About the outermost loop
  , int innerIndex, int innerHigh // About the inner loop
  ){
  for (int j=low; j < high; j++){
    if (heartbeat){
      auto promoted = tryPromoteOutermost(m, j, high, totalT, 0, mSize); //TODO Ask Mike
      if (promoted) {
        return ;
      }
    }

    int *currentV = m[j];
    int die;
    if (j == low){

      /*
       * Execute the leftover of the current in-fly outermost loop iteration
       */
      die = mySum_helper(currentV, innerIndex, innerHigh, totalT, j, high, mSize);
    } else {
      die = mySum_helper(currentV, 0, mSize, totalT, j, high, mSize);
    }
    if (die) {
      return ;
    }
  }
}

static int mySum_helper (int v[], int low, int high, int *t, int outermostIndex, int totalOutermostIterations, int mSize){
  int die = 0;
  for (int i=low; i < high; i++){
    if (heartbeat){
      auto promoted = tryPromoteInnermost(v, i, high, t, outermostIndex, totalOutermostIterations, mSize, &die);
      if (promoted) {
        return ;
      }
    }

    (*t) += v[i];
  }
  return die;
}

static int tryPromoteOutermost (
  int **m, int low, int high, int mSize, int *t // About the outermost loop
  , int innerIndex, int innerHigh // About the inner loop
  ){

  /*
   * Profitability guard.
   */
  int leftoverIterations = high - low;
  if (leftoverIterations < 2){
    return 0;
  }

  int *t1 = new int(*t);
  int *t2 = new int(0);
  auto j = [t1, t2, t](void) {
    *t = *t1 + *t2;

    /*
     * Free the memory
     */
    delete t1;
    delete t2;
  };

  /*
   * Create task 1
   */
  int med = (high + low)/2;
  auto l1 = [j, m, low, med, mSize, t1](void){
    myNestedSum_helper(m, low, med, mSize, t1, innerIndex, innerHigh);
    join(j);
  };
  auto task1 = new Task(l1);
 
  /*
   * Create task 2
   */
  auto l2 = [j, m, med, high, mSize, t2](void){
    myNestedSum_helper(m, med, high, mSize, t2, 0, mSize);
    join(j);
  };
  auto task2 = new Task(l2);

  /*
   * Create task join
   */
  auto taskJoin = new Task(j);

  addEdge(task1, taskJoin);
  addEdge(task2, taskJoin);

  task1->release();
  task2->release();
  taskJoin->release();

  return 1;
}

static int tryPromoteInnermost (int v[], int low, int high, int *t, int outermostIndex, int totalOutermostIterations, int mSize, int *die){

  /*
   * Am I running the last iteration of my slice of the parent loop?
   */
  if ((outermostIndex + 1) < totalOutermostIterations){

    /*
     * I'm not running the last iteration of the outermost loop of my slice.
     * Promote the remaining outermost iterations of our outer-loop slice
     */
    (*die) = tryPromoteOutermost(m, outermostIndex, totalOutermostIterations, mSize, totalT, low);
    assert(*die);
    return 0;
  }

  /*
   * Profitability guard.
   */
  int leftoverIterations = high - low;
  if (leftoverIterations < 2){
    return 0;
  }

  int *t1 = new int(*t);
  int *t2 = new int(0);
  auto j = [t1, t2, t](void) {
    *t = *t1 + *t2;

    /*
     * Free the memory
     */
    delete t1;
    delete t2;
  };

  /*
   * Create task 1
   */
  int med = (high + low)/2;
  auto l1 = [j, v, low, med, t1](void){
    mySum_helper(v, low, med, t1, mSize);
    join(j);
  };
  auto task1 = new Task(l1);
 
  /*
   * Create task 2
   */
  auto l2 = [j, v, med, high, t2](void){
    mySum_helper(v, med, high, t2, mSize);
    join(j);
  };
  auto task2 = new Task(l2);

  /*
   * Create task join
   */
  auto taskJoin = new Task(j);

  addEdge(task1, taskJoin);
  addEdge(task2, taskJoin);

  task1->release();
  task2->release();
  taskJoin->release();

  return 1;
}
