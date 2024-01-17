#include "catch2/catch_test_macros.hpp"

#include <iostream>
#include <random>
#include <set>

#include "ThreadPool/ThreadPool.hpp"

TEST_CASE("Add 2 tasks simultaneously") {
  auto ids = std::array<int, 2>{};

  ThreadPool::ThreadPool pool(1);

  const auto time_to_sleep = std::chrono::milliseconds(100);
  const auto start         = std::chrono::system_clock::now();
  auto       t1            = std::thread([&pool, &ids, &time_to_sleep]() {
    std::this_thread::sleep_for(time_to_sleep);
    auto task = pool.enqueue([&ids]() { ids.at(0) = 1; });
    task.wait();
  });

  auto t2 = std::thread([&pool, &ids, &start, &time_to_sleep]() {
    std::this_thread::sleep_for(start + time_to_sleep - std::chrono::system_clock::now());
    auto task = pool.enqueue([&ids]() { ids.at(1) = 2; });
    task.wait();
  });

  t1.join();
  t2.join();

  CHECK(ids == std::array<int, 2>{1, 2});
}

TEST_CASE("Does 2 threads takes the same task?") {
  ThreadPool::ThreadPool pool(2);

  auto random_ints = std::set<std::uint_fast32_t>{};
  auto task        = pool.enqueue([&random_ints]() {
    std::mt19937 engine(std::random_device{}());
    random_ints.insert(engine());
  });

  task.wait();

  CHECK(random_ints.size() == 1);
}

TEST_CASE("Stop the pool when a thread is running task") {
  auto id_set = std::set<int>{};

  auto task = std::future<void>{};
  {
    ThreadPool::ThreadPool pool(1);
    task = pool.enqueue([&id_set]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      id_set.insert(1);
    });
  }
  task.wait();
  CHECK(id_set.size() == 1);
  CHECK(id_set == std::set<int>{1});
}

TEST_CASE("Stop the pool when a thread has some tasks") {
  auto ids = std::array<int, 3>{};

  auto tasks = std::vector<std::future<void>>{};
  {
    ThreadPool::ThreadPool pool(1);
    tasks.push_back(pool.enqueue([&ids]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      ids.at(0) = 1;
    }));
    tasks.push_back(pool.enqueue([&ids]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      ids.at(1) = 2;
    }));
    tasks.push_back(pool.enqueue([&ids]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      ids.at(2) = 3;
    }));
  }
  for (auto& task : tasks) {
    task.wait();
  }
  CHECK(ids == std::array<int, 3>{1, 2, 3});
}
