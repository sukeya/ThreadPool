#ifndef THREAD_POOL_THREAD_POOL_HPP_
#define THREAD_POOL_THREAD_POOL_HPP_

#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <vector>

namespace ThreadPool {
template <class F, class URBG, class Int>
requires std::invocable<F, std::unique_lock<std::mutex>&> && std::integral<Int>
static void AbsolutelyLock(
    std::mutex& mutex, URBG& engine, std::uniform_int_distribution<Int>& dist, F&& f
) {
  for (auto lock = std::unique_lock<std::mutex>(mutex, std::defer_lock);;) {
    if (lock.try_lock()) {
      f(lock);
      break;
    } else {
      std::this_thread::sleep_for(std::chrono::microseconds(dist(engine)));
    }
  }
}

class ThreadPool {
 public:
  ThreadPool() = default;

  ThreadPool(const ThreadPool&)            = delete;
  ThreadPool(ThreadPool&&)                 = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;
  ThreadPool& operator=(ThreadPool&&)      = delete;

  virtual ~ThreadPool();

  explicit ThreadPool(size_t);

  // add new work item to the pool
  template <class F, class... Args>
  auto enqueue(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
    using return_type = std::invoke_result_t<F, Args...>;
    using Task        = std::packaged_task<return_type()>;

    auto task = Task(std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    auto res = task.get_future();

    AbsolutelyLock(
        queue_mutex_,
        engine_,
        dist_,
        [this, moved_task = std::move(task)](std::unique_lock<std::mutex>&) mutable {
          // don't allow enqueueing after stopping the pool
          if (stop_) {
            throw std::runtime_error("enqueue on stopped ThreadPool");
          }

          tasks_.emplace(std::move(moved_task));
          condition_.notify_one();
        }
    );

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
  bool                    stop_{false};

  // for determining time to sleep.
  std::mt19937                       engine_{std::random_device{}()};
  std::uniform_int_distribution<int> dist_{1, 999};
};
}  // namespace ThreadPool

#endif
