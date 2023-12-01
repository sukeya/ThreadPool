#include "ThreadPool.hpp"

namespace ThreadPool {
// the constructor just launches some amount of workers_
ThreadPool::ThreadPool(size_t threads) : stop_(false) {
  for (size_t i = 0; i < threads; ++i)
    workers_.emplace_back([this] {
      for (;;) {
        auto task = std::packaged_task<void()>();

        {
          auto lock = std::unique_lock<std::mutex>(queue_mutex_);
          condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
          if (stop_ && tasks_.empty()) {
            return;
          }
          task = std::move(tasks_.front());
          tasks_.pop();
        }

        task();
      }
    });
}

// the destructor joins all threads
ThreadPool::~ThreadPool() {
  {
    auto lock = std::unique_lock<std::mutex>(queue_mutex_);
    stop_     = true;
    condition_.notify_all();
  }
  for (std::thread& worker : workers_) {
    worker.join();
  }
}
}  // namespace ThreadPool
