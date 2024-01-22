#include "ThreadPool/ThreadPool.hpp"

namespace ThreadPool {
// the constructor just launches some amount of workers_
ThreadPool::ThreadPool(size_t threads_size) {
  for (size_t i = 0; i < threads_size; ++i)
    workers_.emplace_back([this]() {
      // for determining time to sleep.
      auto engine = std::mt19937(std::random_device{}());
      auto dist   = std::uniform_int_distribution<int>(dist_.param());
      for (auto task = std::packaged_task<void()>();;) {
        // TODO: Why doesn't the following code work?
        /*
        AbsolutelyLock(queue_mutex_, [this, &task](std::unique_lock<std::mutex>& lock) {
          condition_.wait(lock, [this]() { return stop_ || !tasks_.empty(); });
          if (stop_ && tasks_.empty()) {
            return;
          }
          task = std::move(tasks_.front());
          tasks_.pop();
        });
        */

        for (auto lock = std::unique_lock<std::mutex>(queue_mutex_, std::defer_lock);;) {
          if (lock.try_lock()) {
            condition_.wait(lock, [this]() { return stop_ || !tasks_.empty(); });
            if (stop_ && tasks_.empty()) {
              return;
            }
            task = std::move(tasks_.front());
            tasks_.pop();
            break;
          } else {
            std::this_thread::sleep_for(std::chrono::microseconds(dist(engine)));
          }
        }

        task();
      }
    });
}

// the destructor joins all threads
ThreadPool::~ThreadPool() {
  AbsolutelyLock(queue_mutex_, engine_, dist_, [this](std::unique_lock<std::mutex>&) {
    stop_ = true;
    condition_.notify_all();
  });
  for (std::thread& worker : workers_) {
    worker.join();
  }
}
}  // namespace ThreadPool
