#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* 在 CubeMX main() 中调用, 紧接在 MX_GPIO_Init() 之后 */
void run_gpio_test(void);

#ifdef __cplusplus
}
#endif
