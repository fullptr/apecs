cmake_minimum_required(VERSION 3.13)
project(apecs)
set(CMAKE_CXX_STANDARD 20)

add_library(apecs INTERFACE)
target_include_directories(apecs INTERFACE include)

if (APECS_BUILD_TESTS)
    find_package(GTest CONFIG REQUIRED)
    enable_testing()

    add_executable(
        tests
        tests/meta.cpp
        tests/sparse_set.cpp
        tests/registry.cpp
    )

    target_link_libraries(
        tests apecs GTest::gtest_main
    )
endif()