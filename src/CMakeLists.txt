cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 20)

find_package(fmt CONFIG REQUIRED)
find_package(cppcoro CONFIG REQUIRED)

add_executable(apecs
               apecs_example.m.cpp)

target_link_libraries(apecs PUBLIC
                      fmt::fmt
                      cppcoro)

target_include_directories(apecs PUBLIC ./src)