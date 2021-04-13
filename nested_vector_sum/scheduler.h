#include <functional>
#include <vector>

class task {
public:
  int in;
  std::function<void()> body;

  task(std::function<void()> body)
    : body(body), in(1) { }
};

namespace scheduler {

  std::vector<task*> ready;

  auto join(task* t) -> void {
    auto in = --t->in;
    if (in == 0) {
      ready.push_back(t);
    }
  }

  auto release(task* t) -> void {
    join(t);
  }
  
  auto fork(task* t, task* k) -> void {
    k->in++;
    release(t);    
  }

  auto loop() -> void {
    while (! ready.empty()) {
      auto t = ready.back();
      ready.pop_back();
      t->body();
      delete t;
    }
  }

  auto launch(task* t) -> void {
    release(t);
    loop();
  }
  
} // end namespace
