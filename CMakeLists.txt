cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 20)

add_library(apecs INTERFACE)
target_include_directories(apecs INTERFACE include)