# mini-tree-example

mini_tree 框架的示例工程合集。选择一个适合你平台的版本即可。

---

## 示例列表

| 平台 | 目录 | 说明 |
|------|------|------|
| **STM32F407ZGT6** (ARM CM4F) | [`stm32_test/`](stm32_test/) | 裸机 FreeRTOS/RT-Thread 三后端切换测试，直操作寄存器，无 SDK 依赖 |
| **ESP32-S3** (Xtensa LX7) | [`esp32_test/`](esp32_test/) | ESP-IDF 组件集成，轻量模式（设备树 + OSAL），WS2812 驱动示例 |

---

## 快速开始

### STM32 版本

```bash
cd stm32_test/
cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain_arm_cm4f.cmake
cmake --build build
```

mini_tree 以 git submodule 引入，初次克隆时：

```bash
git clone --recurse-submodules https://github.com/H-000-H/mini-tree-example.git
```

### ESP32 版本

```bash
cd esp32_test/
idf.py build
```

**说明：** ESP32 版本的 `components/mini_tree/` 是 mini_tree 框架的轻量化裁剪。由于 Xtensa 架构与 ARM/RISC-V 不兼容，已移除 `lib/freeRTOS/` 和 `lib/rtthread/` 中的 RTOS 内核源码和RT-Tread源码，FreeRTOS 后端直接使用 ESP-IDF 内置版本。保留设备树、OSAL 抽象层、EventBus 等核心组件。

配置集成在 ESP-IDF 的 Kconfig 体系中，运行 `idf.py menuconfig` → Component config → mini_tree Configuration 即可设置。`components/mini_tree/Kconfig` 注册到 ESP-IDF 菜单树，配置项保存在 `sdkconfig` 中。默认 FreeRTOS 后端，支持切换 bare-metal 模式。非 ESP-IDF 环境移植时，同一套 Kconfig + `.config` + `tools/genconfig.py` 可独立使用。

> **ESP32 开发建议：**
> ESP-IDF 内置的 FreeRTOS 已针对 Xtensa 双核 SMP 深度优化，Wi-Fi、蓝牙等官方组件均依赖此 FreeRTOS。因此不推荐在 ESP32 上强行使用主仓库的裸机或 RT-Thread 后端。
>
> 若执意在 ESP32 上使用双核裸机或单 FreeRTOS 单裸机的 AMP 方案，性能下降是必然的。ESP32 的 RAM 和 Flash 容量完全足以运行 FreeRTOS，不必为节省资源强上裸机。请按实际场景按需选择。

---

详见各子目录中的说明。
