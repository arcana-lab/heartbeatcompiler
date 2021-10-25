#pragma once

#include <pthread.h>
#include <iostream>
#include <atomic>
#include <unordered_map>
#include <vector>

class HeartBeatsManager {
  public:
    HeartBeatsManager();

    void addThread (void);

    ~HeartBeatsManager();

  private:
    pthread_spinlock_t lock;
    std::vector<std::atomic_bool *> heartbeats;
    std::unordered_map<uint64_t, uint32_t> fromThreadIDToIndex;
    uint32_t globalThreadID;
};

extern HeartBeatsManager hbm;
