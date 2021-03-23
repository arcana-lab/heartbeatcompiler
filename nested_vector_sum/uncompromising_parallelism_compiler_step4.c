int myNestedSum (int **m, int mSize){
  int totalT = 0;
  myNestedSum(m, 0, mSize, mSize, &totalT);
  return totalT;
}

static void myNestedSum_helper (int **m, int low, high, int mSize, int *totalT){
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

static int tryPromoteOutermost (int **m, int low, int high, int mSize, int *t){

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
    myNestedSum_helper(m, low, med, mSize, t1);
    join(j);
  };
  auto task1 = new Task(l1);
 
  /*
   * Create task 2
   */
  auto l2 = [j, m, med, high, mSize, t2](void){
    myNestedSum_helper(m, med, high, mSize, t2);
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

static int tryPromoteInnermost (int v[], int low, int high, int *t){

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
    mySum_helper(v, low, med, t1);
    join(j);
  };
  auto task1 = new Task(l1);
 
  /*
   * Create task 2
   */
  auto l2 = [j, v, med, high, t2](void){
    mySum_helper(v, med, high, t2);
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
