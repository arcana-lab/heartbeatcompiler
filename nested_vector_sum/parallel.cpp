#include <assert.h>
#include <iostream>
#include "cmdline.hpp"
#include "mcsl_fjnative.hpp"

int cycleCounter = 0;
int H = 5;

bool heartbeat() {
  if (++cycleCounter == H) {
    cycleCounter = 0;
    return true;
  }
  return false;
}

int tryPromote (int v[], int low, int high, int* dst);

void mySum (int v[], int low, int high, int* dst){
  int t=0;

  if (! (low < high)) {
    goto exit;
  }
  for (; ;) {
    t += v[low];
    low++;
    if (! (low < high)) {
      break;
    }
    if (heartbeat()) {
      if (tryPromote(v, low, high, dst)) {
	break;
      }
    } 
  }
 exit:
  *dst += t;
}

using namespace mcsl;

int tryPromote (int v[], int low, int high, int* dst){
  if ((high - low) <= 1) {
    return 0;
  }
  int dst1 = 0; int dst2 = 0;
  int mid = (low + high) / 2;
  auto task1 = fjnative_of_function([&] {
   mySum(v, low, mid, &dst1);
  });
  auto task2 = fjnative_of_function([&] {
   mySum(v, mid, high, &dst2);
  });
  auto join = fjnative_of_function([&] {
    *dst = dst1 + dst2;
  });
  auto t1 = (fjnative*)&task1; auto t2 = (fjnative*)&task2; auto tj = (fjnative*)&join;
  auto current = (fjnative*)current_fiber.mine();
  current->status = fiber_status_pause;
  mcsl::fjnative::add_edge(t2, tj);
  mcsl::fjnative::add_edge(t1, tj);
  mcsl::fjnative::add_edge(tj, current);
  tj->release();
  t2->release();
  t1->release();
  if (context::capture<fjnative*>(context::addr(current->ctx))) {
    return 1;
  }
  t1->stack = mcsl::fjnative::notownstackptr;
  t1->swap_with_scheduler();
  t1->run();
  auto f = fjnative_scheduler::take<fiber>();
  if (f == nullptr) {
    current->status = fiber_status_finish;
    current->exit_to_scheduler();
    return 1; // unreachable
  }
  t2->stack = mcsl::fjnative::notownstackptr;
  t2->swap_with_scheduler();
  t2->run();
  current->status = fiber_status_finish;
  current->swap_with_scheduler();
  return 1;
}

int main (int argc, char *argv[]){

  auto MSIZE = deepsea::cmdline::parse_or_default_int("n", 32);
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

  int dst = 0;

  mcsl::launch([&] {
  }, [&] {
    printf("dst=%d\n",dst);
  }, [&] {
    mySum(m[1], 0, MSIZE, &dst);
  });

  return 0;
}
