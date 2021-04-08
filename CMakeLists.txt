cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 20)

find_package(cppcoro CONFIG REQUIRED)

add_executable(apecs_example
               ./example/apecs_example.m.cpp)

target_link_libraries(apecs_example PUBLIC
                      cppcoro)

target_include_directories(apecs_example PUBLIC .)