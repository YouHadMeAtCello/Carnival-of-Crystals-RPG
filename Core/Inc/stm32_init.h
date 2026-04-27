#ifndef STM32_INIT_H
#define STM32_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_DMA_Init(void);
void MX_USART2_UART_Init(void);

#ifdef __cplusplus
}
#endif

#endif

