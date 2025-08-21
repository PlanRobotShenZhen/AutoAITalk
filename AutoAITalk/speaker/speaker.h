#ifndef __SPEAKER_H__
#define __SPEAKER_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "n32h47x_48x.h"

void SPEAKER_RCC_INIT(void);
void SPEAKER_GPIO_INIT(void);
void SPEAKER_I2S_INIT(void);
void SPEAKER_DMA_INIT(int16_t *buffer,int BUFFER_SIZE);
void SPEAKER_Start(void);

#ifdef __cplusplus
}
#endif

#endif