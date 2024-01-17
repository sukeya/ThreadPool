find_package(Catch2 CONFIG REQUIRED)

include(CTest)

add_executable(ThreadPoolTest test.cpp)
target_link_libraries(
  ThreadPoolTest
  PRIVATE
    ThreadPool
    Catch2::Catch2WithMain
)
add_test(NAME ThreadPoolTest COMMAND ThreadPoolTest)
