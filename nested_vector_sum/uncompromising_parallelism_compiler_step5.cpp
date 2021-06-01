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

static int tryPromoteInnermost (int v[], int low, int high, int *t, Task *k);
static int tryPromoteOutermost (
  int **m, int low, int high, int mSize, int *t, // About the outermost loop
  Task *k
  );
static int tryPromoteOutermostAndInnerLeftover (
  int **m, int low, int high, int mSize, int *t,  // About the outermost loop
  int lowInner, int highInner,                     // About the inner loop
  Task *k
  );

static void mySum_helper (int* v, int low, int high, int *t, Task *k){
  if (! (low < high)) {
    goto exit;
  }
  for (; ;) {
    (*t) += v[low];
    low++;
    if (! (low < high)) {
      break;
    }
    if (heartbeat()) {
      auto promoted = tryPromoteInnermost(v, low, high, t, k);
      if (promoted) {
	return;
      }
    }
  }
 exit:
  join(k);
}

static constexpr int D_mySum = 8;

static void myOutermostSum_helper (
  int **m, int low, int high, int mSize, int *totalT,
  Task *k
  ){
  if (! (low < high)) {
    goto exit;
  }
  for (; ;) {
    int lowInner = 0;
    int highInner = mSize;
    for (; ;) {
      int* v = m[low];
      int T = 0;
      int lowInner2 = lowInner;
      int highInner2 = std::min(lowInner + D_mySum, highInner);
      for (; lowInner2 < highInner2; lowInner2++) {
	T += v[lowInner2];
      }
      (*totalT) += T;
      lowInner = lowInner2;
      if (! (lowInner < highInner)) {
	break;
      }
      if (heartbeat()) {
	auto promoted = tryPromoteOutermostAndInnerLeftover(m, low, high, mSize, totalT, lowInner, highInner, k);
	if (promoted) {
	  return;
	}
      } 
    }
    low++;
    if (! (low < high)) {
      break;
    }
    if (heartbeat()) {
      auto promoted = tryPromoteOutermost(m, low, high, mSize, totalT, k);
      if (promoted) {
	return;
      }
    }
  }
 exit:
  join(k);
}

int myOutermostSum (int **m, int mSize){

  /*
   * Allocate the main result
   */
  int totalT = 0;

  Task *k = new Task([] {
    //    std::cout << "finished" << std::endl;
  });

  /*
   * Create the main task.
   */
  auto mainFunction = [&totalT, m, mSize, k](){
    myOutermostSum_helper(m, 0, mSize, mSize, &totalT, k);
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
  if (leftoverIterations == 0){
    return 0;
  }

  if (leftoverIterations == 1){
    auto task0 = new Task([=] {
      mySum_helper(m[low], lowInner, highInner, t, k);
    });
    release(task0);
    return 1;
  }
  //  std::cout << "promote123" << std::endl;
  /*
   * Create task join
   */
  int *t0 = new int(0);
  int *t1 = new int(0);
  int *t2 = new int(0);
  auto j = [t0, t1, t2, t, k](void) {
    *t += *t0 + *t1 + *t2;
    join(k);

    /*
     * Free the memory
     */
    //delete t1;
    //delete t2;
  };
  auto taskJoin = new Task(j);

  auto task0 = new Task([=] {
    mySum_helper(m[low], lowInner, highInner, t0, taskJoin);
  });

  low++;

  if (leftoverIterations == 2) {
    auto l1 = [taskJoin, m, low, high, mSize, t1](void){
      myOutermostSum_helper(m, low, high, mSize, t1, taskJoin);
    };
    auto task1 = new Task(l1);
    addEdge(task0, taskJoin);
    addEdge(task1, taskJoin);
    release(task0);
    release(task1);
    release(taskJoin);
    return 1;
  }

  /*
   * Create task 1
   */
  int med = (high + low)/2;
  auto l1 = [taskJoin, m, low, med, mSize, t1](void){
    myOutermostSum_helper(m, low, med, mSize, t1, taskJoin);
  };
  auto task1 = new Task(l1);
 
  /*
   * Create task 2
   */
  auto l2 = [taskJoin, m, med, high, mSize, t2](void){
    myOutermostSum_helper(m, med, high, mSize, t2, taskJoin);
  };
  auto task2 = new Task(l2);

  addEdge(task0, taskJoin);
  addEdge(task1, taskJoin);
  addEdge(task2, taskJoin);

  release(task0);
  release(task1);
  release(task2);
  release(taskJoin);

  return 1;
}

static int tryPromoteOutermost (
    int **m, int low, int high, int mSize, int *t, Task *k // About the outermost loop
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
  auto j = [t1, t2, t, high, k](void) {
    *t += (*t1 + *t2);
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
    myOutermostSum_helper(m, low, med, mSize, t1, taskJoin);
  };
  auto task1 = new Task(l1);
 
  /*
   * Create task 2
   */
  auto l2 = [taskJoin, m, med, high, mSize, t2](void){
    myOutermostSum_helper(m, med, high, mSize, t2, taskJoin);
  };
  auto task2 = new Task(l2);

  addEdge(task1, taskJoin);
  addEdge(task2, taskJoin);

  release(task2);
  release(task1);
  release(taskJoin);

  return 1;
}

static int tryPromoteInnermost (int* v, int low, int high, int *t, Task *k){
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
  auto j = [t1, t2, t, high, k](void) {
    *t += (*t1 + *t2);
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
  auto l1 = [taskJoin, v, low, med, t1](void){
    mySum_helper(v, low, med, t1, taskJoin);
  };
  auto task1 = new Task(l1);
 
  /*
   * Create task 2
   */
  auto l2 = [taskJoin, v, med, high, t2](void){
    mySum_helper(v, med, high, t2, taskJoin);
  };
  auto task2 = new Task(l2);

  addEdge(task1, taskJoin);
  addEdge(task2, taskJoin);

  release(task2);
  release(task1);
  release(taskJoin);

  return 1;
}
