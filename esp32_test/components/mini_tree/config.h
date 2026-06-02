/*
 * Generated configuration header for mini-tree on ESP32-S3
 * Manual version - replace with Kconfig-generated version if using Kconfig
 */
#ifndef MINI_TREE_CONFIG_H
#define MINI_TREE_CONFIG_H

/* Platform: ESP32-S3 Xtensa LX7 */
#ifndef CONFIG_PLATFORM_XTENSA
#define CONFIG_PLATFORM_XTENSA
#endif

/* OSAL Backend: 由 CMakeLists.txt 中 MINI_TREE_OSAL 变量控制, 编译定义自动传入
 *   freertos  → CONFIG_OSAL_FREERTOS
 *   rtthread  → CONFIG_OSAL_RTTHREAD
 *   null      → CONFIG_OSAL_NULL
 */
#if !defined(CONFIG_OSAL_FREERTOS) && !defined(CONFIG_OSAL_RTTHREAD) && !defined(CONFIG_OSAL_NULL)
#define CONFIG_OSAL_FREERTOS
#endif

/* System Log: Use printf */
#ifndef CONFIG_SYS_LOG_USE_PRINTF
#define CONFIG_SYS_LOG_USE_PRINTF
#endif

/* System Backend: C */
#ifndef CONFIG_SYSTEM_C
#define CONFIG_SYSTEM_C
#endif

/* Toolchain: GCC */
#ifndef CONFIG_TOOLCHAIN_GCC
#define CONFIG_TOOLCHAIN_GCC
#endif

/* Build Options */
/* #undef CONFIG_BUILD_DISASM */

#endif /* MINI_TREE_CONFIG_H */