cmake_minimum_required(VERSION 3.22)
project(EdgeLink)

set(CMAKE_GENERATOR "Ninja")

# 设置C++标准
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置 C++ 编译器为 ccache
set(CMAKE_CXX_COMPILER_LAUNCHER ccache)

# 设置 C 编译器为 ccache
set(CMAKE_C_COMPILER_LAUNCHER ccache)


# 查找Boost库
find_package(Boost REQUIRED COMPONENTS system log regex log_setup url  thread)
set(Boost_MINIMUM_REQUIRED 1.81.0)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()


find_library(LIBSPDLOG NAMES spdlog REQUIRED)
find_library(LIBFMT NAMES fmt REQUIRED)
find_library(LIBRTTR_CORE NAMES rttr_core REQUIRED)


include_directories(
    ./include
    ./libs/async_mqtt-1.0.9/include
)

file(GLOB_RECURSE SOURCES "src/*.c" "src/*.cpp")


#include(FetchContent) # 引入该CMake模块

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# 添加可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

target_compile_options(${PROJECT_NAME} PRIVATE -fno-rtti)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE ${Boost_LIBRARIES}
    PRIVATE ${LIBSPDLOG}
    PRIVATE ${LIBFMT}
    PRIVATE ${LIBRTTR_CORE}
)
add_definitions(-DBOOST_LOG_DYN_LINK)


target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.hpp)

# 添加复制操作，将配置文件复制到输出目录
configure_file(./edgelink-conf.json ${EXECUTABLE_OUTPUT_PATH}/edgelink-conf.json COPYONLY)


# 包含插件子项目

set(PLUGINS_OUTPUT_DIR "${EXECUTABLE_OUTPUT_PATH}/plugins" CACHE INTERNAL "Plugins directory")
file(MAKE_DIRECTORY ${PLUGINS_OUTPUT_DIR})

# modbus 插件
add_subdirectory(./plugins/modbus)
add_dependencies(${PROJECT_NAME} "edgelink.plugins.modbus")

# mqtt 插件
add_subdirectory(./plugins/mqtt)
add_dependencies(${PROJECT_NAME} "edgelink.plugins.mqtt")