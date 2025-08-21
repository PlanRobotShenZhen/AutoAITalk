#ifndef __MICROPHONE_H__
#define __MICROPHONE_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "n32h47x_48x.h"

void I2S2_RCC_INIT(void);
void I2S2_GPIO_INIT(void);
void I2S2_I2S2_INIT(void);
void I2S2_DMA_INIT(int16_t *buffer,int BUFFER_SIZE);
void Mic_Start_Record(void);
//void DMA1_Channel4_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif