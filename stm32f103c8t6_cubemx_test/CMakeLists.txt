cmake_minimum_required(VERSION 3.22)

# ==============================================================================
# STM32F103C8T6 + CubeMX HAL + mini_tree 示例工程
# ==============================================================================
# 用法:
#   cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/gcc.cmake
#   cmake --build build
#
# 前置条件:
#   1. ARM GCC 工具链 (arm-none-eabi-)
#   2. Python3 （设备树生成需要）
#   3. mini_tree 子模块已初始化: git submodule update --init
# ==============================================================================

# ---- 项目定义 ----
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

project(stm32f103c8t6_cubemx_test C CXX ASM)
message("Build type: " ${CMAKE_BUILD_TYPE})

# ---- 全局编译选项 ----
add_compile_options(-ffunction-sections -fdata-sections)

# ---- 配置 mini_tree Kconfig ----
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/mini_tree/.config
    "# Kconfig 配置 — ARM Cortex-M3 裸机\n"
    "CONFIG_PLATFORM_ARM_CM3=y\n"
    "CONFIG_CPU_CORES=1\n"
    "CONFIG_OSAL_NULL=y\n"
    "CONFIG_SYSTEM_CPP=y\n"
    "CONFIG_SYS_LOG_USE_PRINTF=y\n"
    "CONFIG_BUILD_DISASM=n\n"
)

# ---- 注入自定义 board 文件到 mini_tree 子模块 ----
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/board/board.dts
     DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/mini_tree/board/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/board/soc/stm32f103c8t6.dtsi
     DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/mini_tree/board/soc/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/board/CMakeLists.txt
     DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/mini_tree/board/)

# ---- CubeMX 集成 ----
add_executable(${CMAKE_PROJECT_NAME})
add_subdirectory(cmake/stm32cubemx)

# ---- mini_tree 框架 ----
add_subdirectory(mini_tree)

# ---- soc_port: STM32F103C8T6 HAL 实现 ----
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
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
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/test_mqtt.cpp

    # LFS
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/fs/lfs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/fs/lfs_util.c
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/fs/flash_lfs.c

    # MQTT
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
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/fs
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/mqtt/MQTTPacket/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/soc_port_stm32f103c8t6_hal/mqtt/MQTTClient
)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    _MQTT_
    MQTT_USE_TLS
    MQTTCLIENT_PLATFORM_HEADER=MQTTBareMetal.h
    LFS_NO_ASSERT
)

# ---- 链接 ----
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    mini_tree
)

target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
    -Wl,-Map=${CMAKE_PROJECT_NAME}.map
    -Wl,--print-memory-usage
)
