# mini-tree-example

mini_tree 框架的示例工程合集。选择一个适合你平台的版本即可。

---

## 示例列表

| 平台 | 目录 | 说明 |
|------|------|------|
| **STM32F407ZGT6** (ARM CM4F) | [`stm32_test/`](stm32_test/) | 裸机 FreeRTOS/RT-Thread 三后端切换测试，直操作寄存器，无 SDK 依赖 |
| **ESP32-S3** (Xtensa LX7) | [`esp32_test/`](esp32_test/) | ESP-IDF 组件集成，config.h 配置，WS2812 驱动示例 |

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

**注意**: ESP32 版本使用 `components/mini_tree/config.h` 作为配置入口（ESP-IDF 已有 Kconfig，不再引入第二套 Kconfig），运行前修改 `config.h` 即可。OSAL 默认 FreeRTOS，语言默认 C++。

---

详见各子目录中的说明。
