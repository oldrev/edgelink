cmake_minimum_required(VERSION 3.22)

include(FetchContent)

option(EL_WITH_MQTT "Include MQTT support" YES)
#option(EL_WITH_MODBUS "Include ModBus support" YES)

set(CMAKE_GENERATOR "Ninja")

# 按代码尺寸优化
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    string(REPLACE "-O3" "-Os" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "-O3" "-Os" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
endif()

set(CMAKE_DEBUG_POSTFIX d)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置C语言编译器选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")


# 设置 C++ 编译器为 ccache
set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
set(CMAKE_C_COMPILER_LAUNCHER ccache)

project(EdgeLink)

add_custom_target(git_versioning
  ${CMAKE_COMMAND} -D SRC=${CMAKE_SOURCE_DIR}/version-config.h.in
                   -D DST=${CMAKE_BINARY_DIR}/version-config.h
                   -P ${CMAKE_SOURCE_DIR}/cmake/GenerateGitVersionHeader.cmake
)

# 查找Boost库
find_package(Boost REQUIRED COMPONENTS system url json program_options)
set(Boost_MINIMUM_REQUIRED 1.81.0)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

find_library(LIBSPDLOG NAMES spdlog REQUIRED)
find_library(LIBFMT NAMES fmt REQUIRED)
find_library(LIBRTTR_CORE NAMES rttr_core REQUIRED)

# 通过 cmake 引入 ASYNC_MQTT 库
add_subdirectory(./libs/async_mqtt)

add_subdirectory(./libs/quickjspp)

include_directories(
    ${CMAKE_BINARY_DIR}
    ./include
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/mustache/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/async_mqtt/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/rva/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/boost-di/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/croncpp/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/quickjspp"
)

file(GLOB_RECURSE EL_SOURCES 
    "src/*.c"
    "src/*.cpp"
    "./libs/mustache/src/renderer.cpp"
)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

add_subdirectory(./abstractions)

############################################## Main App #################################################
# 添加可执行文件
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_executable(${PROJECT_NAME} ${EL_SOURCES})

add_dependencies(${PROJECT_NAME} git_versioning)

add_dependencies(${PROJECT_NAME} EdgeLinkAbstractions)

add_dependencies(${PROJECT_NAME} quickjspp)
add_dependencies(${PROJECT_NAME} quickjs)

# target_compile_options(${PROJECT_NAME} PRIVATE -fno-rtti), 不要关掉 RTTI

set(EL_DEP_LIBS
    PRIVATE ${Boost_LIBRARIES}
    PRIVATE ${LIBSPDLOG}
    PRIVATE ${LIBFMT}
    PRIVATE ${LIBRTTR_CORE}
    PRIVATE quickjspp
    PRIVATE quickjs
    PRIVATE EdgeLinkAbstractions
)

target_link_libraries(
    ${PROJECT_NAME}
    ${EL_DEP_LIBS}
)

add_definitions(-DBOOST_LOG_DYN_LINK)

target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.hpp)

# 测试 ---------------------------------------------------------------------------

# 启用测试
include(CTest)
if(BUILD_TESTING)
    enable_testing()

    add_subdirectory(libs/catch2)

    set(TESTS_EL_SOURCES ${EL_SOURCES})
    list(REMOVE_ITEM TESTS_EL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

    file(GLOB_RECURSE TESTS_SOURCES 
        "tests/*.c"
        "tests/*.cpp"
    )

    # 指定测试可执行文件的名称
    add_executable(EdgeLinkTests
        ${TESTS_EL_SOURCES}
        ${TESTS_SOURCES}
    )

    target_precompile_headers(EdgeLinkTests PRIVATE tests/pch.hpp)

    # 链接cache2和其他项目依赖项
    target_link_libraries(EdgeLinkTests PRIVATE
        ${EL_DEP_LIBS}
        Catch2
        Catch2WithMain
    )

    # 添加测试
    add_test(NAME EdgeLinkTests COMMAND EdgeLinkTests)
    add_dependencies(EdgeLinkTests EdgeLinkAbstractions)

    list(APPEND CMAKE_MODULE_PATH ${CMAKE_INSTALL_PREFIX}/lib/cmake/Catch2)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    include(CTest)
    include(Catch)
    catch_discover_tests(EdgeLinkTests)

endif()

# 配置文件 ---------------------------------------------------------


# 添加复制操作，将配置文件复制到输出目录
configure_file(./edgelink.ini ${EXECUTABLE_OUTPUT_PATH}/edgelink.ini COPYONLY)
configure_file(./flows.json ${EXECUTABLE_OUTPUT_PATH}/flows.json COPYONLY)


# 插件 ---------------------------------------------------------------------------

# 包含插件子项目

set(PLUGINS_OUTPUT_DIR "${EXECUTABLE_OUTPUT_PATH}/plugins" CACHE INTERNAL "Plugins directory")
file(MAKE_DIRECTORY ${PLUGINS_OUTPUT_DIR})



macro(edgelink_plugin PLUGIN_NAME)
    set(THIS_PLUGIN "edgelink.plugins.${PLUGIN_NAME}")

    file(GLOB_RECURSE SOURCES "src/*.c" "src/*.cpp")
    get_filename_component(CURRENT_CMAKE_SOURCE_DIR "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PLUGINS_OUTPUT_DIR})

    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    add_library(${THIS_PLUGIN} SHARED ${SOURCES})

    add_dependencies(${THIS_PLUGIN} EdgeLinkAbstractions)
    add_dependencies(${THIS_PLUGIN} git_versioning)
    add_dependencies(${PROJECT_NAME} quickjspp)
    add_dependencies(${PROJECT_NAME} quickjs)

    target_link_libraries(
        ${THIS_PLUGIN}
        PRIVATE ${Boost_LIBRARIES}
        PRIVATE ${LIBSPDLOG}
        PRIVATE ${LIBFMT}
        PRIVATE ${LIBRTTR_CORE}
        PRIVATE EdgeLinkAbstractions
    )
endmacro()


# 获取所有子目录
file(GLOB PLUGIN_SUBDIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/plugins/*/)

# 遍历子目录并添加进来
foreach(CHILD ${PLUGIN_SUBDIRS})
    set(PLUGIN_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${CHILD})
    if(IS_DIRECTORY ${PLUGIN_SOURCE_DIR})
        MESSAGE(STATUS "Plugin found: ${PLUGIN_SOURCE_DIR}")
        add_subdirectory("${PLUGIN_SOURCE_DIR}")
    endif()
endforeach()