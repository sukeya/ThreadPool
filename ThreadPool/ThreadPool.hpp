#ifndef _THREAD_POOL_THREAD_POOL_HPP_
#define _THREAD_POOL_THREAD_POOL_HPP_

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <vector>

namespace ThreadPool {
class ThreadPool {
 public:
  ThreadPool()                        = default;
  ThreadPool(ThreadPool&&)            = default;
  ThreadPool& operator=(ThreadPool&&) = default;

  ThreadPool(const ThreadPool&)            = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  ~ThreadPool();

  explicit ThreadPool(size_t);

  // add new work item to the pool
  template <class F, class... Args>
  auto enqueue(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
    using return_type = std::invoke_result_t<F, Args...>;

    auto task =
        std::packaged_task<return_type()>(std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

    auto res = task.get_future();
    {
      auto lock = std::unique_lock<std::mutex>(queue_mutex_);

      // don't allow enqueueing after stopping the pool
      if (stop_) {
        throw std::runtime_error("enqueue on stopped ThreadPool");
      }

      tasks_.emplace(std::move(task));
      condition_.notify_one();
    }
    return res;
  }

 private:
  // need to keep track of threads so we can join them
  std::vector<std::thread> workers_;
  // the task queue
  std::queue<std::packaged_task<void()>> tasks_;

  // synchronization
  std::mutex              queue_mutex_;
  std::condition_variable condition_;
  bool                    stop_;
};
}  // namespace ThreadPool

#endif
