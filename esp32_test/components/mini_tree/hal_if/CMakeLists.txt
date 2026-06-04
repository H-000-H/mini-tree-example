# ==============================================================================
# hal_if — 硬件抽象接口层 (Hardware Abstraction Layer Interface)
# ESP-IDF 构建: 源文件在父 CMakeLists.txt 的 idf_component_register() 中指定。
# 本文件仅用于 CMake 独立构建（非 ESP-IDF）。
# ==============================================================================

add_library(hal_if STATIC
    "src/hal_if_dummy.c"
    "src/hal_cpu_amp.c"
)

target_include_directories(hal_if
    PUBLIC
        "include"
)
