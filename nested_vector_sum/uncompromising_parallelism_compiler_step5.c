int myOutermostSum (int **m, int mSize){
  int totalT = 0;
  myOutermostSum_helper(m, 0, mSize, mSize, &totalT);
  return totalT;
}

static void myOutermostSum_helper (
  int **m, int low, high, int mSize, int *totalT    // About the outermost loop
  , int innerIndex, int innerHigh // About the inner loop
  ){
  for (int j=low; j < high; j++){
    if (heartbeat){
      auto promoted = tryPromoteOutermost(m, j, high, mSize, totalT);
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
      exit(0) ;
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

static int tryPromoteOutermostAndInnerLeftover (
  int **m, int low, int high, int mSize, int *t,  // About the outermost loop
  int lowInner, int highInner                     // About the inner loop
  ){

  /*
   * Profitability guard.
   */
  int leftoverIterations = high - low;
  if (leftoverIterations < 2){
    return 0;
  }

  /*
   * Create task join
   */
  int *t1 = new int(0);
  int *t2 = new int(0);
  auto j = [t1, t2, t](void) {
    *t += *t1 + *t2;

    /*
     * Free the memory
     */
    delete t1;
    delete t2;
  };
  auto taskJoin = new Task(j);

  /*
   * Create task 1
   */
  int med = (high + low)/2;
  auto l1 = [taskJoin, m, low, med, mSize, t1](void){
    myOutermostSum_helper(m, low, med, mSize, t1, 0, mSize);
    join(taskJoin);
  };
  auto task1 = new Task(l1);
 
  /*
   * Create task 2
   */
  auto l2 = [taskJoin, m, med, high, mSize, t2](void){
    myOutermostSum_helper(m, med, high, mSize, t2, 0, mSize);
    join(taskJoin);
  };
  auto task2 = new Task(l2);

  /*
   * Create task 3: the task that will execute the leftover of the inner loop of the current outermost loop.
   */
  auto leftoverInnerCode = [taskJoin, v, lowInner, highInner, t, mSize](void){
    myOutermostSum_helper(m, low-1, low, mSize, t, lowInner, highInner);
    join(taskJoin);
  };
  auto task3 = new Task(leftoverInnerCode);

  addEdge(task1, taskJoin);
  addEdge(task2, taskJoin);
  addEdge(task3, taskJoin);

  task1->release();
  task2->release();
  task3->release();
  taskJoin->release();

  return 1;
}

static int tryPromoteOutermost (
  int **m, int low, int high, int mSize, int *t // About the outermost loop
  ){

  /*
   * Profitability guard.
   */
  int leftoverIterations = high - low;
  if (leftoverIterations < 2){
    return 0;
  }

  /*
   * Create task join
   */
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
  auto taskJoin = new Task(j);

  /*
   * Create task 1
   */
  int med = (high + low)/2;
  auto l1 = [taskJoin, m, low, med, mSize, t1](void){
    myOutermostSum_helper(m, low, med, mSize, t1, 0, mSize);
    join(taskJoin);
  };
  auto task1 = new Task(l1);
 
  /*
   * Create task 2
   */
  auto l2 = [taskJoin, m, med, high, mSize, t2](void){
    myOutermostSum_helper(m, med, high, mSize, t2, 0, mSize);
    join(taskJoin);
  };
  auto task2 = new Task(l2);

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
     *
     * Promote the remaining outermost iterations of our outer-loop slice as well as the remaining inner iterations to complete the current outer loop iteration.
     */
    (*die) = tryPromoteOutermostAndInnerLeftover(m, outermostIndex + 1, totalOutermostIterations, mSize, t);
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

  /*
   * Create task join
   */
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
  auto taskJoin = new Task(j);

  /*
   * Create task 1
   */
  int med = (high + low)/2;
  auto l1 = [taskJoin, v, low, med, t1](void){
    mySum_helper(v, low, med, t1, mSize);
    join(taskJoin);
  };
  auto task1 = new Task(l1);
 
  /*
   * Create task 2
   */
  auto l2 = [taskJoin, v, med, high, t2](void){
    mySum_helper(v, med, high, t2, mSize);
    join(taskJoin);
  };
  auto task2 = new Task(l2);

  addEdge(task1, taskJoin);
  addEdge(task2, taskJoin);

  task1->release();
  task2->release();
  taskJoin->release();

  return 1;
}
