cmake_minimum_required(VERSION 3.14)
project(cfr_solver CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -flto -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fsanitize=address,undefined")

add_executable(cfr_solver src/main.cpp)
target_include_directories(cfr_solver PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(test_cfr tests/test_cfr.cpp)
target_include_directories(test_cfr PRIVATE ${CMAKE_SOURCE_DIR}/include)

enable_testing()
add_test(NAME cfr_tests COMMAND test_cfr)
