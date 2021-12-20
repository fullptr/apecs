cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 20)

find_package(GTest CONFIG REQUIRED)
enable_testing()

add_executable(
    tests
    meta.cpp
    sparse_set.cpp
    registry.cpp
)

target_link_libraries(
    tests
    apecs
    GTest::gtest_main
)