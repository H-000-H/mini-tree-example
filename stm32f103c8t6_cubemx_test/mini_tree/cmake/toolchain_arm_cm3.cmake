# ARM Cortex-M3 交叉编译工具链 (GCC)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(TOOLCHAIN_PREFIX arm-none-eabi-)

set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_AR           ${TOOLCHAIN_PREFIX}ar)

# -std= 由 CMakeLists.txt 的 CMAKE_C_STANDARD 控制
set(CMAKE_C_FLAGS   "-mcpu=cortex-m3 -mthumb -Wall -Wextra -Os -g" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "-mcpu=cortex-m3 -mthumb -Wall -Wextra -Os -g -fno-rtti -fno-exceptions" CACHE STRING "" FORCE)
set(CMAKE_ASM_FLAGS "-mcpu=cortex-m3 -mthumb -x assembler-with-cpp" CACHE STRING "" FORCE)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
