#include <assert.h>
#include <iostream>

#include "scheduler.h"

int cycleCounter = 0;
int H = 5;

bool heartbeat() {
  if (++cycleCounter == H) {
    cycleCounter = 0;
    return true;
  }
  return false;
}

int myOutermostSum (int **m, int mSize);

int main (int argc, char *argv[]){

  /*
   * Fetch the inputs
   */
  if (argc <= 1){
    std::cerr << "USAGE: " << argv[0] << " MATRIX_SIZE" << std::endl;
    return 1;
  }
  auto MSIZE = atoll(argv[1]);
  std::cout << "Matrix size = " << MSIZE << std::endl;

  /*
   * Allocate
   */
  auto m = new int*[MSIZE];
  for (auto i=0; i < MSIZE; i++){
    m[i] = new int[MSIZE];
  }

  /*
   * Initialize
   */
  for (auto i=0; i < MSIZE; i++){
    for (auto j=0; j < MSIZE; j++){
      m[i][j] = i * j;
    }
  }

  /*
   * Run
   */
  auto r = myOutermostSum(m, MSIZE);

  /*
   * Print the result
   */
  std::cout << "The result is " << r << std::endl;

  return 0;
}

static int tryPromoteInnermost (int v[], int low, int high, int *t, int outermostIndex, int totalOutermostIterations, int mSize, int **m, int *die, Task *k);
static int tryPromoteOutermost (
  int **m, int low, int high, int mSize, int *t, // About the outermost loop
  Task *k
  );
static int tryPromoteOutermostAndInnerLeftover (
  int **m, int low, int high, int mSize, int *t,  // About the outermost loop
  int lowInner, int highInner,                     // About the inner loop
  Task *k
  );

static int mySum_helper (int v[], int low, int high, int *t, int outermostIndex, int totalOutermostIterations, int mSize, int **m, Task *k){
  int die = 0;
  for (int i=low; i < high; i++){
    if (heartbeat()){
      std::cout << "promote! 2" << std::endl;
      auto promoted = tryPromoteInnermost(v, i, high, t, outermostIndex, totalOutermostIterations, mSize, m, &die, k);
      if (promoted) {
        return 1;
      }
    }

    (*t) += v[i];
  }

  return die;
}

static void myOutermostSum_helper (
  int **m, int low, int high, int mSize, int *totalT    // About the outermost loop
  , int innerIndex, int innerHigh, // About the inner loop
  Task *k
  ){
  std::cout << "Outermost: " << low << " -> " << high << " (inner " << innerIndex << " -> " << innerHigh << ")\n";
  for (int j=low; j < high; j++){
    if (heartbeat()){
      std::cout << "Outermost:  heartbeat (iteration " << j << ")\n" << std::endl;
      auto promoted = tryPromoteOutermost(m, j, high, mSize, totalT, k);
      if (promoted) {
        std::cout << "Outermost:    promoted\n";
        return ;
      }
      std::cout << "Outermost:    not promoted\n";
    }

    int *currentV = m[j];
    int die;
    if (j == low){

      /*
       * Execute the leftover of the current in-fly outermost loop iteration
       */
      die = mySum_helper(currentV, innerIndex, innerHigh, totalT, j, high, mSize, m, k);

    } else {
      die = mySum_helper(currentV, 0, mSize, totalT, j, high, mSize, m, k);
    }
    if (die) {
      return;
    }
  }
  std::cout << "Outermost: completed " << low << " -> " << high << " : " << *totalT << std::endl;
  join(k);
}

int myOutermostSum (int **m, int mSize){

  /*
   * Allocate the main result
   */
  int totalT = 0;

  Task *k = new Task([] {
    std::cout << "finished" << std::endl;
  });

  /*
   * Create the main task.
   */
  auto mainFunction = [&totalT, m, mSize, k](){
    myOutermostSum_helper(m, 0, mSize, mSize, &totalT, 0, mSize, k);
  };
  auto mainTask = new Task(mainFunction);

  /*
   * Launch
   */
  launch(mainTask);

  return totalT;
}

static int tryPromoteOutermostAndInnerLeftover (
  int **m, int low, int high, int mSize, int *t,  // About the outermost loop
  int lowInner, int highInner,                     // About the inner loop
  Task *k
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
  auto j = [t1, t2, t, k](void) {
    *t += *t1 + *t2;
    join(k);

    /*
     * Free the memory
     */
    //delete t1;
    //delete t2;
  };
  auto taskJoin = new Task(j);

  /*
   * Create task 1
   */
  int med = (high + low)/2;
  auto l1 = [taskJoin, m, low, med, mSize, t1](void){
    myOutermostSum_helper(m, low, med, mSize, t1, 0, mSize, taskJoin);
  };
  auto task1 = new Task(l1);
 
  /*
   * Create task 2
   */
  auto l2 = [taskJoin, m, med, high, mSize, t2](void){
    myOutermostSum_helper(m, med, high, mSize, t2, 0, mSize, taskJoin);
  };
  auto task2 = new Task(l2);

  /*
   * Create task 3: the task that will execute the leftover of the inner loop of the current outermost loop.
   */
  auto leftoverInnerCode = [taskJoin, m, low, lowInner, highInner, t, mSize](void){
    myOutermostSum_helper(m, low-1, low, mSize, t, lowInner, highInner, taskJoin);
  };
  auto task3 = new Task(leftoverInnerCode);

  addEdge(task1, taskJoin);
  addEdge(task2, taskJoin);
  addEdge(task3, taskJoin);

  release(task1);
  release(task2);
  release(task3);
  release(taskJoin);

  return 1;
}

static int tryPromoteOutermost (
    int **m, int low, int high, int mSize, int *t, Task *k // About the outermost loop
  ){
  static int done = 0;

  /*
   * Profitability guard.
   */
  int leftoverIterations = high - low;
  if (leftoverIterations < 2){
    return 0;
  }
  if (done > 3) {
    return 0;
  }
  done++;

  /*
   * Create task join
   */
  std::cout << "At iteration " << low - 1 << " (potentially incomplete): " << *t << std::endl;
  int *t1 = new int(0);
  int *t2 = new int(0);
  auto j = [t1, t2, t, high, k](void) {
    *t += (*t1 + *t2);
    std::cout << "At iteration " << high - 1 << ": " << *t << std::endl;
    join(k);

    /*
     * Free the memory
     */
    //delete t1;
    //delete t2;
  };
  auto taskJoin = new Task(j);

  /*
   * Create task 1
   */
  int med = (high + low)/2;
  std::cout << "Outermost:    Low = " << low << std::endl;
  std::cout << "Outermost:    Mid = " << med << std::endl;
  std::cout << "Outermost:    High = " << high << std::endl;
  auto l1 = [taskJoin, m, low, med, mSize, t1](void){
    myOutermostSum_helper(m, low, med, mSize, t1, 0, mSize, taskJoin);
  };
  auto task1 = new Task(l1);
 
  /*
   * Create task 2
   */
  auto l2 = [taskJoin, m, med, high, mSize, t2](void){
    myOutermostSum_helper(m, med, high, mSize, t2, 0, mSize, taskJoin);
  };
  auto task2 = new Task(l2);

  addEdge(task1, taskJoin);
  addEdge(task2, taskJoin);

  release(task2);
  release(task1);
  release(taskJoin);

  return 1;
}

static int tryPromoteInnermost (int v[], int low, int high, int *t, int outermostIndex, int totalOutermostIterations, int mSize, int **m, int *die, Task *k){

  /*
   * Am I running the last iteration of my slice of the parent loop?
   */
  if ((outermostIndex + 1) < totalOutermostIterations){

    /*
     * I'm not running the last iteration of the outermost loop of my slice.
     *
     * Promote the remaining outermost iterations of our outer-loop slice as well as the remaining inner iterations to complete the current outer loop iteration.
     */
    (*die) = tryPromoteOutermostAndInnerLeftover(m, outermostIndex + 1, totalOutermostIterations, mSize, t, low, high, k);
    return *die;
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
  auto j = [t1, t2, t, k](void) {
    *t = *t1 + *t2;
    join(k);

    /*
     * Free the memory
     */
    //delete t1;
    //delete t2;
  };
  auto taskJoin = new Task(j);

  /*
   * Create task 1
   */
  int med = (high + low)/2;
  auto l1 = [taskJoin, v, low, med, t1, m, mSize, outermostIndex, totalOutermostIterations](void){
    mySum_helper(v, low, med, t1, mSize, outermostIndex, totalOutermostIterations, m, taskJoin);
  };
  auto task1 = new Task(l1);
 
  /*
   * Create task 2
   */
  auto l2 = [taskJoin, v, med, high, t2, m, outermostIndex, totalOutermostIterations, mSize](void){
    mySum_helper(v, med, high, t2, mSize, outermostIndex, totalOutermostIterations, m, taskJoin);
  };
  auto task2 = new Task(l2);

  addEdge(task1, taskJoin);
  addEdge(task2, taskJoin);

  release(task1);
  release(task2);
  release(taskJoin);

  return 1;
}
