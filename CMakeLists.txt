cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 20)
project(apecs)

find_package(GTest CONFIG REQUIRED)
enable_testing()

add_executable(
    tests
    ./tests/meta.cpp
    ./tests/generator.cpp
    ./tests/sparse_set.cpp
    ./tests/registry.cpp
    ./tests/handle.cpp
)

target_link_libraries(
    tests
    GTest::gtest_main
)

target_include_directories(
    tests PUBLIC
    .
)