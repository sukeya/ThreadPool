cmake_minimum_required(VERSION 3.26.3)

set(version_major 0)
set(version ${version_major}.1.0)

project(ThreadPool VERSION ${version} LANGUAGES CXX)

# In-source build guard
if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt. ")
endif()

# Make cache variables for install destinations
include(GNUInstallDirs)

# Config
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_OPTIMIZE_DEPENDENCIES ON)

add_library(ThreadPool SHARED ThreadPool/ThreadPool.cpp)

target_compile_features(
  ThreadPool
  PUBLIC
    cxx_std_20
)

target_include_directories(
  ThreadPool
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_compile_options(
  ThreadPool
  PUBLIC
    "$<$<CONFIG:Debug>:-g;-O0;-Wall;-Wextra>"
    "$<$<CONFIG:Release>:-DNDEBUG;-O3;-march=native;-mtune=native>"
)

install(TARGETS ThreadPool
        EXPORT ThreadPoolTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ThreadPool.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ThreadPool)

install(EXPORT ThreadPoolTargets
        FILE ThreadPoolTargets.cmake
        NAMESPACE ThreadPool::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ThreadPool
)

# Create the package
include(CMakePackageConfigHelpers)

## Create a package congiguration file
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/ThreadPoolConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ThreadPool
)

install(FILES
          "${CMAKE_CURRENT_BINARY_DIR}/ThreadPoolConfig.cmake"
          "${CMAKE_CURRENT_BINARY_DIR}/ThreadPoolConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ThreadPool
)

## Create a version file
set_property(TARGET ThreadPool PROPERTY VERSION ${version})
set_property(TARGET ThreadPool PROPERTY SOVERSION ${version_major})
set_property(TARGET ThreadPool PROPERTY
  INTERFACE_ThreadPool_MAJOR_VERSION ${version_major})
set_property(TARGET ThreadPool APPEND PROPERTY
  COMPATIBLE_INTERFACE_STRING ThreadPool_MAJOR_VERSION
)

### generate the version file for the config file
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/ThreadPoolConfigVersion.cmake"
  VERSION "${version}"
  COMPATIBILITY AnyNewerVersion
)

## Export the target from the build tree
export(EXPORT ThreadPoolTargets
       FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/ThreadPoolTargets.cmake"
       NAMESPACE ThreadPool::
)

option(THREADPOOL_USE_TEST "Test thread pool" OFF)

if (THREADPOOL_USE_TEST)
  add_subdirectory(test)
endif()
