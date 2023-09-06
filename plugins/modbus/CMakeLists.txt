
set(PLUGIN_MODBUS "edgelink.plugins.modbus")

file(GLOB_RECURSE SOURCES "src/*.c" "src/*.cpp")

find_library(LIBMODBUS NAMES modbus REQUIRED)

add_library(${PLUGIN_MODBUS} SHARED ${SOURCES})

# 包含Boost头文件
target_include_directories(${PLUGIN_MODBUS} 
    PRIVATE ${Boost_INCLUDE_DIRS}
    PRIVATE ${../include}
    PRIVATE ${../libs/signals/src}
)

target_link_libraries(
    ${PLUGIN_MODBUS}
    PRIVATE ${Boost_LIBRARIES}
    PRIVATE ${LIBMODBUS}
    PRIVATE ${LIBSPDLOG}
    PRIVATE ${LIBFMT}
    PRIVATE ${LIBRTTR_CORE}
)

target_precompile_headers(${PLUGIN_MODBUS} PRIVATE src/pch.hpp)


add_custom_command(
    TARGET ${PLUGIN_MODBUS}
    POST_BUILD
    COMMAND cp $<TARGET_FILE:${PLUGIN_MODBUS}> ${PLUGINS_OUTPUT_DIR}/
)


#file(COPY "${CMAKE_BINARY_DIR}/${PLUGIN_MODBUS}.so" DESTINATION "${PLUGINS_OUTPUT_DIR}")