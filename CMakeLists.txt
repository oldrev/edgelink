cmake_minimum_required(VERSION 3.22)
project(EdgeLinkApp)

set(CMAKE_GENERATOR "Ninja")

# 设置C++标准
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置 C++ 编译器为 ccache
set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
set(CMAKE_C_COMPILER_LAUNCHER ccache)

# 按代码尺寸优化
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Os")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os")
endif()


# 查找Boost库
find_package(Boost REQUIRED COMPONENTS system url json)
set(Boost_MINIMUM_REQUIRED 1.81.0)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

find_library(LIBSPDLOG NAMES spdlog REQUIRED)
find_library(LIBFMT NAMES fmt REQUIRED)
find_library(LIBRTTR_CORE NAMES rttr_core REQUIRED)
find_library(LIBDUKTAPE NAMES duktape REQUIRED)

include_directories(
    ./include
    ./libs/duktape-cpp/src
    ./libs/mustache/include
)

file(GLOB_RECURSE SOURCES 
    "src/*.c" 
    "src/*.cpp"
    "./libs/mustache/src/renderer.cpp"
)



#include(FetchContent) # 引入该CMake模块

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# 添加可执行文件
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_executable(EdgeLinkApp ${SOURCES})

# target_compile_options(${PROJECT_NAME} PRIVATE -fno-rtti), 不要关掉 RTTI

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE ${Boost_LIBRARIES}
    PRIVATE ${LIBSPDLOG}
    PRIVATE ${LIBFMT}
    PRIVATE ${LIBRTTR_CORE}
    PRIVATE ${LIBDUKTAPE}
)
add_definitions(-DBOOST_LOG_DYN_LINK)


target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.hpp)

# 添加复制操作，将配置文件复制到输出目录
configure_file("./bin/edgelink" ${EXECUTABLE_OUTPUT_PATH}/edgelink COPYONLY)
configure_file(./edgelink-conf.json ${EXECUTABLE_OUTPUT_PATH}/edgelink-conf.json COPYONLY)
configure_file(./flows.json ${EXECUTABLE_OUTPUT_PATH}/flows.json COPYONLY)


# 包含插件子项目

set(PLUGINS_OUTPUT_DIR "${EXECUTABLE_OUTPUT_PATH}/plugins" CACHE INTERNAL "Plugins directory")
file(MAKE_DIRECTORY ${PLUGINS_OUTPUT_DIR})

# modbus 插件
add_subdirectory(./plugins/modbus)
add_dependencies(${PROJECT_NAME} "edgelink.plugins.modbus")

# mqtt 插件
add_subdirectory(./plugins/mqtt)
add_dependencies(${PROJECT_NAME} "edgelink.plugins.mqtt")