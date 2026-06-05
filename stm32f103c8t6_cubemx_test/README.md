# STM32F103C8T6 + CubeMX HAL + mini_tree 示例

展示 mini_tree 框架与 **STM32CubeMX HAL** 的集成方式。**本示例为内存极限测试**——大量外设（GPIO、SPI、I2C、ADC、TIM、PWM、CAN、RTC、OLED、WiFi、MQTT、LFS 文件系统）的 HAL 封装和测试代码仅是展示框架接入模式，外设驱动未经充分验证，不保证实际运行。

## 目录结构

```
stm32f103c8t6_cubemx_test/
├── CMakeLists.txt                  # 顶层 CMake 构建
├── cmake/
│   ├── gcc.cmake                   # ARM GCC 工具链配置
│   └── stm32cubemx/CMakeLists.txt  # CubeMX 构建集成
├── Core/Inc/                       # CubeMX 生成的外设头文件
├── Core/Src/                       # CubeMX 生成的初始化代码 + main.cpp
├── startup_stm32f103xb.s           # 启动文件
├── STM32F103XX_FLASH.ld            # 链接脚本
├── stm32f103c8t6-mini-tree.ioc     # CubeMX 工程文件
├── Drivers/                        # ST HAL + CMSIS（精简版）
├── board/
│   ├── board.dts                   # 板级设备树（LED、按键、外设配置）
│   ├── CMakeLists.txt              # 覆盖 mini_tree 默认的 board 构建
│   └── soc/stm32f103c8t6.dtsi      # SoC 级外设地址描述
├── mini_tree/                      # mini_tree 框架（git submodule）
└── soc_port_stm32f103c8t6_hal/     # STM32F103C8T6 HAL 实现
    ├── hal_gpio.cpp / hal_spi.cpp / hal_i2c.cpp / hal_adc.cpp
    ├── hal_tim.cpp / hal_pwm.cpp / hal_can.cpp / hal_rtc.cpp
    ├── hal_stubs.c                 # 未实现 HAL 接口的弱符号桩
    ├── w25q64.c                    # SPI Flash 驱动
    ├── oled_drv.cpp                # SSD1306 OLED 驱动
    ├── wifi_esp32.cpp              # ESP32 AT 命令 WiFi 驱动
    ├── wifi_manager.cpp            # WiFi 连接管理器
    ├── fs/                         # LittleFS 文件系统
    ├── mqtt/                       # MQTT Client (Eclipse Paho 移植)
    └── test_*.cpp                  # 各外设测试用例
```

## 构建方法

### 前置条件

1. **ARM GCC 工具链** — `arm-none-eabi-gcc` 需在 PATH 中
2. **Python 3** — 设备树代码生成需要
3. **mini_tree 子模块**：

```bash
git submodule update --init
```

### 编译

```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/gcc.cmake
cmake --build build
```

编译完成后会在终端输出固件大小（RAM/Flash 占用）。

## 说明

| 要点 | 说明 |
|------|------|
| CubeMX 生成 Core/ 和 Drivers/ | Core/Src/main.cpp 为应用入口，在 USER CODE 段中插入 mini_tree 调用 |
| cmake/stm32cubemx/CMakeLists.txt | 统一管理 CubeMX HAL 源文件、头文件路径和编译宏 |
| 外设初始化由 CubeMX 生成 | MX_GPIO_Init() 等在各 .c 中实现，mini_tree 的 hal_* 层基于 HAL 驱动封装 |
| mini_tree 负责系统层 | 设备树、EventBus、安全关断、生命周期管理等 |
| soc_port 承担适配层 | 实现 hal_if 接口，调用 STM32 HAL API 完成硬件操作 |
| **本示例定位** | **内存极限测试**——大量外设接入后验证 RAM/Flash 占用，外设代码仅为接入示范，未经充分验证 |
