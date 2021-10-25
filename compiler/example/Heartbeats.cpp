#include "Heartbeats.hpp"

HeartBeatsManager::HeartBeatsManager() {
  pthread_spin_init(&this->lock, PTHREAD_PROCESS_PRIVATE);
  this->globalThreadID = 0;

  return ;
}

void HeartBeatsManager::addThread (void){
  uint64_t threadID = pthread_self();

  pthread_spin_lock(&this->lock);
  fromThreadIDToIndex[threadID] = (this->globalThreadID)++;
  heartbeats.push_back(new std::atomic_bool(false));
  pthread_spin_unlock(&this->lock);

  return ;
}

HeartBeatsManager::~HeartBeatsManager() {
  for (auto &p : fromThreadIDToIndex){
    std::cout << p.first << " " << p.second << " " << *heartbeats[p.second] << std::endl;
  }

  return ;
}

HeartBeatsManager hbm{};
