cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 20)

find_package(GTest CONFIG REQUIRED)

enable_testing()
add_executable(
    tests
    ./examples/meta.cpp
)

target_link_libraries(
    tests
    GTest::gtest_main
)

target_include_directories(
    tests PUBLIC
    .
)