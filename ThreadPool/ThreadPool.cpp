#include "ThreadPool/ThreadPool.hpp"

namespace ThreadPool {
ThreadPool::ThreadPool()
    : workers_(),
      tasks_(),
      queue_mutex_ptr_(std::make_unique<std::mutex>()),
      condition_ptr_(std::make_unique<std::condition_variable>()),
      stop_(false) {}

ThreadPool::ThreadPool(ThreadPool&& thread_pool)
    : workers_(std::move(thread_pool.workers_)),
      tasks_(std::move(thread_pool.tasks_)),
      queue_mutex_ptr_(std::move(queue_mutex_ptr_)),
      condition_ptr_(std::move(thread_pool.condition_ptr_)),
      stop_(thread_pool.stop_) {}

ThreadPool& ThreadPool::operator=(ThreadPool&& thread_pool) {
  workers_         = std::move(thread_pool.workers_);
  tasks_           = std::move(thread_pool.tasks_);
  queue_mutex_ptr_ = std::move(thread_pool.queue_mutex_ptr_);
  condition_ptr_   = std::move(thread_pool.condition_ptr_);
  stop_            = thread_pool.stop_;
  return *this;
}

// the constructor just launches some amount of workers_
ThreadPool::ThreadPool(size_t threads_size) : ThreadPool() {
  for (size_t i = 0; i < threads_size; ++i)
    workers_.emplace_back([this] {
      for (;;) {
        auto task = std::packaged_task<void()>();

        {
          auto lock = std::unique_lock<std::mutex>(*queue_mutex_ptr_);
          condition_ptr_->wait(lock, [this] { return stop_ || !tasks_.empty(); });
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
    auto lock = std::unique_lock<std::mutex>(*queue_mutex_ptr_);
    stop_     = true;
    condition_ptr_->notify_all();
  }
  for (std::thread& worker : workers_) {
    worker.join();
  }
}
}  // namespace ThreadPool
