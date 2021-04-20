#include <functional>
#include <vector>

class Task {
public:
  int in;
  std::function<void()> body;

  Task(std::function<void()> body)
    : body(body), in(1) { }
};

std::vector<Task*> ready;

auto join(Task* t) -> void {
  auto in = --t->in;
  if (in == 0) {
    ready.push_back(t);
  }
}

auto release(Task* t) -> void {
  join(t);
}

auto addEdge(Task* t, Task* k) -> void {
  k->in++;
}

auto loop() -> void {
  while (! ready.empty()) {
    auto t = ready.back();
    ready.pop_back();
    t->body();
    delete t;
  }
}

auto launch(Task* t) -> void {
  release(t);
  loop();
}
