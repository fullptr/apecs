cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 20)

add_executable(apecs_example
               ./examples/apecs_example.m.cpp
               ./examples/meta.cpp
               ./examples/sparse_set.cpp
               ./examples/generator.cpp)

target_include_directories(apecs_example PUBLIC .)