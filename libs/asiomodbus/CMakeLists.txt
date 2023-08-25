cmake_minimum_required(VERSION 3.22)
project(asiomodbus)

set(CMAKE_GENERATOR "Ninja")

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找Boost库
find_package(Boost REQUIRED COMPONENTS system asio)
set(Boost_MINIMUM_REQUIRED 1.81.0)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()


include_directories(
    ./include
)

file(GLOB_RECURSE SOURCES "src/*.c" "src/*.cpp")


set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# 添加可执行文件
add_library(${PROJECT_NAME} ${SOURCES})

target_link_libraries( ${PROJECT_NAME} PRIVATE ${Boost_LIBRARIES})
#add_definitions(-DBOOST_LOG_DYN_LINK)

target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)