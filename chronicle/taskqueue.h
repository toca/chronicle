#pragma once
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

template<typename T>
class TaskQueue {
public:
    TaskQueue() {};
    ~TaskQueue() {};

    void enqueue(T task) {
        std::unique_lock<std::mutex> lock(this->mutex);
        this->queue.push(task);
        lock.unlock();
        this->cond.notify_one();
    }

    void processTasks() {
        while (!this->stop) {
            std::unique_lock<std::mutex> lock(this->mutex);
            this->cond.wait(lock, [this] { return !this->queue.empty() || stop; });
            if (stop) break;
            T task = queue.front();
            this->queue.pop();
            lock.unlock();
            task();
        }
    }

    void Stop() {
        this->stop = true;
        this->cond.notify_all();
    }

private:
    std::mutex mutex;
    std::condition_variable cond;
    std::queue<T> queue;
    bool stop = false;
};




