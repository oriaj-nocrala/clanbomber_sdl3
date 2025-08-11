#ifndef MUTEX_H
#define MUTEX_H

#include <mutex>

class Mutex {
    std::mutex m;
public:
    void lock() { m.lock(); }
    void unlock() { m.unlock(); }
};

#endif
