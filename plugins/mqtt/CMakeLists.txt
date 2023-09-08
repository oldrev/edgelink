
set(PLUGIN_MQTT "edgelink.plugins.mqtt")

file(GLOB_RECURSE SOURCES "src/*.c" "src/*.cpp")


get_filename_component(CURRENT_CMAKE_SOURCE_DIR "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)

add_subdirectory(${CURRENT_CMAKE_SOURCE_DIR}/libs/async_mqtt)

add_library(${PLUGIN_MQTT} SHARED ${SOURCES})

# 包含Boost头文件
target_include_directories(${PLUGIN_MQTT} 
    PRIVATE ${Boost_INCLUDE_DIRS}
    PRIVATE ${../include}
    ./libs/async_mqtt/include
)

target_link_libraries(
    ${PLUGIN_MQTT}
    PRIVATE ${Boost_LIBRARIES}
    PRIVATE ${LIBSPDLOG}
    PRIVATE ${LIBFMT}
    PRIVATE ${LIBRTTR_CORE}
    PRIVATE async_mqtt_iface
)

target_precompile_headers(${PLUGIN_MQTT} PRIVATE src/pch.hpp)


add_custom_command(
    TARGET ${PLUGIN_MQTT}
    POST_BUILD
    COMMAND cp $<TARGET_FILE:${PLUGIN_MQTT}> ${PLUGINS_OUTPUT_DIR}/
)


#file(COPY "${CMAKE_BINARY_DIR}/${PLUGIN_MQTT}.so" DESTINATION "${PLUGINS_OUTPUT_DIR}")