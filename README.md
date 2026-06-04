# mini-tree-example

mini_tree 框架的示例工程合集。选择一个适合你平台的版本即可。

---

## 示例列表

| 平台 | 目录 | 说明 |
|------|------|------|
| **STM32F407ZGT6** (ARM CM4F) | [`stm32_test/`](stm32_test/) | 裸机 FreeRTOS/RT-Thread 三后端切换测试，直操作寄存器，无 SDK 依赖 |
| **ESP32-S3** (Xtensa LX7) | [`esp32_test/`](esp32_test/) | ESP-IDF 组件集成，Kconfig 配置，WS2812 驱动示例 |

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
使用本仓库的完整示例工程：

```bash
cd esp32_test/
idf.py build
```

配置采用 mini_tree 自有的 Kconfig 体系（`components/mini_tree/Kconfig` + `tools/menuconfig.py` / `tools/genconfig.py`），不依赖 ESP-IDF 的 Kconfig 体系。运行 `idf.py menuconfig` 或在组件目录下执行 `tools/menuconfig.py` 即可配置。OSAL 默认 FreeRTOS，语言默认 C++。

---

详见各子目录中的说明。
