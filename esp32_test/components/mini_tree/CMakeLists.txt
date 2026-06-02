# mini_tree component for ESP-IDF (ESP32-S3)
# Full framework build: board + core + system_c + drivers

# ═══════════════════════════════════════════════════════════════
#  OSAL 后端选择 (改这一行即可切换)
#  可选值: freertos | rtthread | null
# ═══════════════════════════════════════════════════════════════
set(MINI_TREE_OSAL "freertos" CACHE STRING "OSAL backend: freertos, rtthread, null")

# ── RT-Thread 内核源文件 (仅在 rtthread 模式下编译) ──
set(RTT_SRCS
    "lib/rtthread/src/clock.c"
    "lib/rtthread/src/cpu_up.c"
    "lib/rtthread/src/defunct.c"
    "lib/rtthread/src/idle.c"
    "lib/rtthread/src/ipc.c"
    "lib/rtthread/src/irq.c"
    "lib/rtthread/src/kservice.c"
    "lib/rtthread/src/mem.c"
    "lib/rtthread/src/object.c"
    "lib/rtthread/src/scheduler_comm.c"
    "lib/rtthread/src/scheduler_up.c"
    "lib/rtthread/src/thread.c"
    "lib/rtthread/src/timer.c"
    "lib/rtthread/src/klibc/kerrno.c"
    "lib/rtthread/src/klibc/kstdio.c"
    "lib/rtthread/src/klibc/kstring.c"
    "lib/rtthread/libcpu/xtensa/cpuport.c"
    "lib/rtthread/libcpu/xtensa/context_gcc.S"
)

# ── 按 OSAL 选择源文件 & 编译宏 ──
if(MINI_TREE_OSAL STREQUAL "freertos")
    set(OSAL_SRCS       "osal/src/osal_freertos.c")
    set(OSAL_RTT_KERNEL "")   # 不编 RT-Thread 内核
    set(OSAL_DEFINE     CONFIG_OSAL_FREERTOS)

elseif(MINI_TREE_OSAL STREQUAL "rtthread")
    set(OSAL_SRCS       "osal/src/osal_rtthread.c")
    set(OSAL_RTT_KERNEL ${RTT_SRCS})   # 编入 RT-Thread 内核
    set(OSAL_DEFINE     CONFIG_OSAL_RTTHREAD)
    # 警告: ESP-IDF 自带的 FreeRTOS 仍在链接中, 两个内核并存需自行处理冲突

elseif(MINI_TREE_OSAL STREQUAL "null")
    set(OSAL_SRCS       "osal/src/osal_null.c")
    set(OSAL_RTT_KERNEL "")
    set(OSAL_DEFINE     CONFIG_OSAL_NULL)

else()
    message(FATAL_ERROR "MINI_TREE_OSAL must be one of: freertos, rtthread, null")
endif()

# ── HAL (ESP32-S3 real implementation) ──
set(HAL_SRCS
    "hal_if/src/hal_esp32s3.c"
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

# ── System layer (C runtime) ──
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

idf_component_register(
    SRCS
        ${OSAL_SRCS}
        ${OSAL_RTT_KERNEL}
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
        "lib/rtthread/include"
        "lib/rtthread"
        "lib/rtthread/libcpu/xtensa"
    REQUIRES
        freertos
        esp_driver_rmt
        esp_driver_gpio
)

# Compile definitions (OSAL 宏由 ${OSAL_DEFINE} 传入, config.h 不再硬编码)
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    ${OSAL_DEFINE}
    __RT_KERNEL_SOURCE__
    RT_KLIBC_USING_LIBC_VSNPRINTF
    RT_KLIBC_USING_LIBC_VSSCANF
)