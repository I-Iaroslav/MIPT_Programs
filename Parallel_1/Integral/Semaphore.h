#pragma once

#include <mutex>
#include <condition_variable>

class Semaphore {
private:
    std::mutex mtx;
    std::condition_variable cv;
    int count;
public:
    Semaphore(int count_ = 0) : count(count_) {}

    inline void notify() {
        std::unique_lock<std::mutex> lock(mtx);
        count++;
        //notify the waiting thread
        cv.notify_one();
    }

    inline void wait() {
        std::unique_lock<std::mutex> lock(mtx);
        while (count == 0) {
            //wait on the mutex until notify is called
            cv.wait(lock);
        }
        count--;
    }

    inline bool try_wait() {
        std::unique_lock<std::mutex> lock(mtx);
        if (count != 0) {
            count--;
            return true;
        }
        return false;
    }
};
