cmake_minimum_required(VERSION 3.8)

project(signals-demo CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(signals-demo "")
target_sources(signals-demo
               PRIVATE
               "demo.h"
               "demo.cpp"
               "basic_demo.cpp"
               "track_objects.cpp"
               "cross_thread_calls.cpp")
target_link_libraries(signals-demo signals)