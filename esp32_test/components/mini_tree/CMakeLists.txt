# mini_tree component for ESP-IDF (ESP32-S3)
# Lightweight mode: device tree + OSAL + bare metal only.
# Configuration via Kconfig (.config)
#
# NOTE: Use CMAKE_CURRENT_LIST_DIR for paths — ESP-IDF's two-phase
# processing means CMAKE_CURRENT_SOURCE_DIR is NOT the component dir
# during the requirements-gathering phase.

# ═══════════════════════════════════════════════════════════════
# OSAL source selection from .config (kconfiglib format)
# ═══════════════════════════════════════════════════════════════
set(KCONFIG_DOT "${CMAKE_CURRENT_LIST_DIR}/.config")

file(STRINGS "${KCONFIG_DOT}" CONFIG_OSAL_ENTRY
     REGEX "^CONFIG_OSAL_[A-Z_]+=y$")

# Force cmake reconfigure when .config changes
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${KCONFIG_DOT}")

# ── OSAL 后端选择 ──
if(CONFIG_OSAL_ENTRY STREQUAL "CONFIG_OSAL_FREERTOS=y")
    set(OSAL_SRCS   "osal/src/osal_freertos.c")
    set(OSAL_DEFINE CONFIG_OSAL_FREERTOS)

elseif(CONFIG_OSAL_ENTRY STREQUAL "CONFIG_OSAL_RTTHREAD=y")
    set(OSAL_SRCS   "osal/src/osal_rtthread.c")
    set(OSAL_DEFINE CONFIG_OSAL_RTTHREAD)

else()
    # Default: NULL (bare metal)
    set(OSAL_SRCS   "osal/src/osal_null.c")
    set(OSAL_DEFINE CONFIG_OSAL_NULL)
endif()

# ── HAL (ESP32-S3 real implementation) ──
set(HAL_SRCS
    "hal_if/src/hal_esp32s3.c"
    "hal_if/src/hal_cpu_amp.c"    # AMP 弱符号 (CPU_CORES=1 时无影响)
)

# ── Board layer (device tree, driver, config store, tasks) ──
set(BOARD_SRCS
    "board/src/board_device.c"
    "board/src/board_driver.c"
    "board/src/config_store.c"
    "board/src/task_config.c"
    "board/src/task_utils.c"
)

# ── Generated board files (dtc-lite output) ──
set(GEN_SRCS
    "board/generated/board_devtable.c"
    "board/generated/board_probe.c"
    "board/generated/board_force_link.c"
)

# ── Core layer (event bus, buffer pool, production log) ──
set(CORE_SRCS
    "core/src/event_bus.cpp"
    "core/src/buffer_pool.c"
    "core/src/production_log.c"
)

# ── System layer ──
set(SYSTEM_SRCS
    "system_c/src/system_init.c"
    "system_c/src/system_scrubber.c"
    "system_c/src/system_wdt.c"
    "system_c/src/task_manager.c"
    "system_cpp/src/safe_state.c"
)

# ── WS2812 LED driver ──
set(DRIVER_SRCS
    "drivers/ws2812/ws2812_drv.c"
)

# ── Algorithm ──
set(ALGO_SRCS
    "algorithm/buffer/circle_fifo_buffer.c"
)

# ═══════════════════════════════════════════════════════════════
idf_component_register(
    SRCS
        ${OSAL_SRCS}
        ${HAL_SRCS}
        ${BOARD_SRCS}
        ${GEN_SRCS}
        ${CORE_SRCS}
        ${SYSTEM_SRCS}
        ${DRIVER_SRCS}
        ${ALGO_SRCS}
    INCLUDE_DIRS
        "."
        "board/include"
        "board/generated"
        "compat"
        "core/include"
        "hal_if/include"
        "osal/include"
        "system_c/include"
        "system_cpp/include"
        "drivers/ws2812"
    REQUIRES
        esp_driver_rmt
        esp_driver_gpio
)

# ═══════════════════════════════════════════════════════════════
# Generate config.h from Kconfig at build time
# Use CMAKE_BINARY_DIR based location (stable across both phases)
# ═══════════════════════════════════════════════════════════════
set(KCONFIG_GEN_DIR "${CMAKE_BINARY_DIR}/generated/kconfig/mini_tree")
set(KCONFIG_OUT     "${KCONFIG_GEN_DIR}/config.h")

add_custom_command(
    OUTPUT  ${KCONFIG_OUT}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${KCONFIG_GEN_DIR}
    COMMAND python "${CMAKE_CURRENT_LIST_DIR}/tools/genconfig.py"
                "${CMAKE_CURRENT_LIST_DIR}/Kconfig"
                ${KCONFIG_GEN_DIR}
                --config "${KCONFIG_DOT}"
    DEPENDS
        "${CMAKE_CURRENT_LIST_DIR}/Kconfig"
        "${KCONFIG_DOT}"
        "${CMAKE_CURRENT_LIST_DIR}/tools/genconfig.py"
    COMMENT "Generating config.h from Kconfig"
)

add_custom_target(kconfig_gen DEPENDS ${KCONFIG_OUT})
add_dependencies(${COMPONENT_LIB} kconfig_gen)

# Make generated config.h available to this component
target_include_directories(${COMPONENT_LIB} PRIVATE ${KCONFIG_GEN_DIR})

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    ${OSAL_DEFINE}
)
