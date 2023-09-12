cmake_minimum_required(VERSION 3.22)
project(EdgeLinkApp)

set(CMAKE_GENERATOR "Ninja")

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置 C++ 编译器为 ccache
set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
set(CMAKE_C_COMPILER_LAUNCHER ccache)

# 按代码尺寸优化
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    string(REPLACE "-O3" "-Os" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "-O3" "-Os" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
endif()

set(CMAKE_DEBUG_POSTFIX d)

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
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/mustache/include"
)

file(GLOB_RECURSE EL_SOURCES 
    "src/*.c" 
    "src/*.cpp"
    "./libs/mustache/src/renderer.cpp"
)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# 添加可执行文件
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_executable(${PROJECT_NAME} ${EL_SOURCES})

add_subdirectory(./abstractions)
add_dependencies(${PROJECT_NAME} "edgelink-abstractions")

# target_compile_options(${PROJECT_NAME} PRIVATE -fno-rtti), 不要关掉 RTTI

set(EL_DEP_LIBS 
    PRIVATE ${Boost_LIBRARIES}
    PRIVATE ${LIBSPDLOG}
    PRIVATE ${LIBFMT}
    PRIVATE ${LIBRTTR_CORE}
    PRIVATE ${LIBDUKTAPE}
    PRIVATE "edgelink-abstractions"
)

target_link_libraries(
    ${PROJECT_NAME}
    ${EL_DEP_LIBS}
)

add_definitions(-DBOOST_LOG_DYN_LINK)

target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.hpp)

# 测试 ---------------------------------------------------------------------------

# 启用测试
enable_testing()

set(TESTS_EL_SOURCES ${EL_SOURCES})
list(REMOVE_ITEM TESTS_EL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

file(GLOB_RECURSE TESTS_SOURCES 
    "tests/*.c" 
    "tests/*.cpp"
)

# 指定测试可执行文件的名称
add_executable("edgelink.tests"
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/catch2/src/catch_amalgamated.cpp
    ${TESTS_EL_SOURCES}
    ${TESTS_SOURCES}
)

target_precompile_headers("edgelink.tests" PRIVATE tests/pch.hpp)

target_include_directories("edgelink.tests" PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/catch2/include
)

# 链接cache2和其他项目依赖项
target_link_libraries("edgelink.tests" PRIVATE
    ${EL_DEP_LIBS}
 )

# 添加测试
add_test(NAME "edgelink.tests" COMMAND "edgelink.tests")
add_dependencies("edgelink.tests" "edgelink-abstractions")



# 添加复制操作，将配置文件复制到输出目录
configure_file(./edgelink-conf.json ${EXECUTABLE_OUTPUT_PATH}/edgelink-conf.json COPYONLY)
configure_file(./flows.json ${EXECUTABLE_OUTPUT_PATH}/flows.json COPYONLY)


# 插件 ---------------------------------------------------------------------------

# 包含插件子项目

set(PLUGINS_OUTPUT_DIR "${EXECUTABLE_OUTPUT_PATH}/plugins" CACHE INTERNAL "Plugins directory")
file(MAKE_DIRECTORY ${PLUGINS_OUTPUT_DIR})

# modbus 插件
add_subdirectory(./plugins/modbus)

# mqtt 插件
add_subdirectory(./plugins/mqtt)