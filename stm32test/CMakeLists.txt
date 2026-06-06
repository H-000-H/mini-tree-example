cmake_minimum_required(VERSION 3.16)
project(test_link C CXX ASM)

add_subdirectory(mini_tree)

add_executable(test_link
    main.c
    startup.c
    hal_stubs.c
)

target_link_libraries(test_link PRIVATE mini_tree)
target_compile_definitions(test_link PRIVATE HAL_CPU_FREQ_HZ=168000000UL)
target_compile_options(test_link PRIVATE
    -include "${CMAKE_BINARY_DIR}/generated/kconfig/config.h"
)
target_link_options(test_link PRIVATE
    -T ${CMAKE_CURRENT_SOURCE_DIR}/stm32f407zgt6.ld
    -nostartfiles
    -Wl,--gc-sections
    -specs=nano.specs
    -specs=nosys.specs
)
