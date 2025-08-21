#ifndef __WEBSOCKET_H__
#define __WEBSOCKET_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "n32h47x_48x.h"

void UART7_RCC_INIT(void);
void UART7_GPIO_INIT(void);
void UART7_INIT(void);
void UART7_NVIC_Configuration(void);
bool UART7_SendATCommand(const char* cmd);
void USART7_SendBuffer(const uint8_t* pData, uint32_t Size);
void UART7_DMA_Configuration(uint8_t *buffer,int BUFFER_SIZE);
#ifdef __cplusplus
}
#endif

#endif