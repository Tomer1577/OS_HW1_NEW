cmake_minimum_required(VERSION 3.16)
project(smash)

set(CMAKE_CXX_STANDARD 11)

add_executable(smash Commands.cpp signals.cpp smash.cpp)