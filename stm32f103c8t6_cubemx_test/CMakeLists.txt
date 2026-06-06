cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME stm32f103c8t6-mini-tree)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM, C and CXX languages
enable_language(C CXX ASM)

# 全局编译选项 (在 add_subdirectory 前设置, 对所有目标生效)
add_compile_options(-ffunction-sections -fdata-sections)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add mini_tree framework
add_subdirectory(mini_tree)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/hal_stubs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/hal_gpio.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/hal_spi.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/hal_tim.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/hal_i2c.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/hal_adc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/hal_can.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/hal_pwm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/hal_rtc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/w25q64.c
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/test_gpio.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/test_spi.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/test_tim.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/test_i2c.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/test_adc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/font.c
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/oled_drv.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/test_oled.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/wifi_esp32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/wifi_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/test_wifi.cpp

    # ── LFS ──
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/fs/lfs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/fs/lfs_util.c
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/fs/flash_lfs.c

    # ── MQTT ──
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/mqtt/MQTTPacket/src/MQTTPacket.c
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/mqtt/MQTTPacket/src/MQTTConnectClient.c
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/mqtt/MQTTPacket/src/MQTTDeserializePublish.c
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/mqtt/MQTTPacket/src/MQTTSerializePublish.c
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/mqtt/MQTTPacket/src/MQTTFormat.c
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/mqtt/MQTTPacket/src/MQTTSubscribeClient.c
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/mqtt/MQTTPacket/src/MQTTUnsubscribeClient.c
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/mqtt/MQTTClient/MQTTClient.c
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/mqtt/MQTTClient/MQTTBareMetal.c
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/mqtt/mqtt_transport.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/test_mqtt.cpp
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/fs
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/mqtt/MQTTPacket/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/mqtt/MQTTClient
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
    _MQTT_
    MQTT_USE_TLS
    MQTTCLIENT_PLATFORM_HEADER=MQTTBareMetal.h
    LFS_NO_ASSERT
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
    mini_tree
)

# 链接脚本 & map 生成
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
    -Wl,-Map=${CMAKE_PROJECT_NAME}.map
    -Wl,--print-memory-usage
)

# ── fix_cc_for_clangd 已废弃 ──
# 改为在 .clangd 中设置 Compiler: clang++，clangd 自动使用系统 LLVM clang
# 查询内置头文件路径，不再需要替换 compile_commands.json。
