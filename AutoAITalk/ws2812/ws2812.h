#ifndef __WS2812_H__
#define __WS2812_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "n32h47x_48x.h"

void WS2812_RCC_INIT(void);
void WS2812_GPIO_INIT(void);
void WS2812_PWM(void);
void WS2812_DMA_INIT(void);
void WS2812_SetColor(uint8_t red, uint8_t green, uint8_t blue);
void WS2812_Send(void);


#ifdef __cplusplus
}
#endif

#endif