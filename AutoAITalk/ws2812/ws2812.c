#include "ws2812.h"
#include "main.h"

/* WS2812Ӳ������ */
#define LED_PB5_PORT  GPIOB                 /* LED�������Ŷ˿� */
#define LED_PB5_PIN   GPIO_PIN_5            /* LED�������� */
#define LED_PB5_CLOCK RCC_AHB_PERIPHEN_GPIOB /* GPIOBʱ�� */
#define GTIM2_CH DMA1_CH5                   /* ��ʱ��2ʹ�õ�DMAͨ�� */

/* WS2812�������� */
#define LED_NUM       1                     /* LED���� */
#define BYTES_PER_LED 24                    /* ÿ��LED��Ҫ24λ���ݣ�GRB��8λ�� */

/* PWMʱ�����������60MHzʱ�ӣ� */
#define PWM_PERIOD    119                   /* 1us PWM���ڣ�60MHz/1us = 60��ʵ��119+1=120���ڣ� */
#define PWM_HIGH      78                    /* �ߵ�ƽռ�ձ�(Լ0.8us) */
#define PWM_LOW       42                    /* �͵�ƽռ�ձ�(Լ0.4us) */

/* DMA��������С = LED���� * 24λ */
#define PWM_DMA_BUFFER_SIZE (LED_NUM * BYTES_PER_LED)

/* ȫ�ֱ��� */
uint32_t color = 0;                         /* ��ǰ��ɫֵ */
uint32_t PrescalerValue = 0;                /* Ԥ��Ƶֵ */
uint16_t PWM_DMA_BUFFER[PWM_DMA_BUFFER_SIZE]; /* DMA���仺���� */

/**
 * @brief WS2812ʱ�ӳ�ʼ��
 * @details ��ʼ����ʱ����GPIO��DMA�����ʱ��
 */
void WS2812_RCC_INIT(void)
{
    RCC_ConfigPclk1(RCC_HCLK_DIV4);         /* APB1=240MHz/4=60MHz */
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_GTIM2, ENABLE);  /* ʹ�ܶ�ʱ��2ʱ�� */
    RCC_EnableAHB1PeriphClk(LED_PB5_CLOCK, ENABLE);          /* ʹ��GPIOBʱ�� */
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_AFIO, ENABLE);   /* ʹ��AFIOʱ�� */
    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPHEN_DMA1, ENABLE);   /* ʹ��DMA1ʱ�� */
}

/**
 * @brief WS2812 GPIO��ʼ��
 * @details ����LED��������Ϊ��ʱ��PWM���ģʽ
 */
void WS2812_GPIO_INIT(void)
{
    GPIO_InitType GPIO_InitStructure;
    
    GPIO_InitStruct(&GPIO_InitStructure);
    
    GPIO_InitStructure.Pin        = LED_PB5_PIN;             /* ����5 */
    GPIO_InitStructure.GPIO_Mode  = GPIO_MODE_AF_PP;         /* ����������� */
    GPIO_InitStructure.GPIO_Slew_Rate = GPIO_SLEW_RATE_SLOW; /* ���ٰ��� */
    GPIO_InitStructure.GPIO_Alternate = GPIO_AF_GTIM2_CH2_PB5; /* ��ʱ��2ͨ��2���� */
    GPIO_InitPeripheral(LED_PB5_PORT, &GPIO_InitStructure);  /* ��ʼ��GPIO */
}

/**
 * @brief WS2812 PWM��ʼ��
 * @details ���ö�ʱ��2ΪPWMģʽ������WS2812�����ʱ���ź�
 */
void WS2812_PWM(void)
{
    TIM_TimeBaseInitType TIM_TimeBaseStructure;
    OCInitType TIM_OCInitStructure;
    
    /* ��ʱ���������� */
    TIM_InitTimBaseStruct(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.Period    = PWM_PERIOD;            /* �Զ���װ��ֵ */
    TIM_TimeBaseStructure.Prescaler = 0;                     /* Ԥ��Ƶ��������Ƶ�� */
    TIM_TimeBaseStructure.ClkDiv    = TIM_CLK_DIV1;          /* ʱ�ӷ�Ƶ */
    TIM_TimeBaseStructure.CounterMode = TIM_CNT_MODE_UP;     /* ���ϼ���ģʽ */
    TIM_InitTimeBase(GTIM2, &TIM_TimeBaseStructure);         /* ��ʼ����ʱ�� */
    
    /* PWMģʽ���ã�ͨ��2 */
    TIM_InitOcStruct(&TIM_OCInitStructure);
    TIM_OCInitStructure.OCMode      = TIM_OCMODE_PWM1;       /* PWMģʽ1 */
    TIM_OCInitStructure.OutputState = TIM_OUTPUT_STATE_ENABLE; /* ���ʹ�� */
    TIM_OCInitStructure.Pulse       = 0;                     /* ��ʼռ�ձ�Ϊ0 */
    TIM_OCInitStructure.OCPolarity  = TIM_OC_POLARITY_HIGH;  /* ������Ը� */
    TIM_OCInitStructure.OCIdleState = TIM_OC_IDLE_STATE_RESET; /* ����״̬�� */
    TIM_InitOc2(GTIM2, &TIM_OCInitStructure);                /* ��ʼ������Ƚ�ͨ��2 */
    
    /* ����Ԥװ�ؼĴ��� */
    TIM_ConfigOc2Preload(GTIM2, TIM_OC_PRE_LOAD_ENABLE);
    TIM_ConfigArPreload(GTIM2, ENABLE);
}

/**
 * @brief WS2812 DMA��ʼ��
 * @details ����DMA�����Զ�����PWM���ݵ���ʱ��
 */
void WS2812_DMA_INIT(void)
{
    DMA_DeInit(GTIM2_CH);                    /* ��λDMAͨ�� */
    DMA_InitType DMA_InitStruct;
    DMA_StructInit(&DMA_InitStruct);         /* ��ʼ��DMA�ṹ�� */
    
    DMA_InitStruct.PeriphAddr = (uint32_t)&(GTIM2->CCDAT2);  /* Ŀ���ַ��TIM2 CCR2�Ĵ��� */
    DMA_InitStruct.MemAddr = (uint32_t)PWM_DMA_BUFFER;       /* Դ��ַ��DMA������ */
    DMA_InitStruct.BufSize = PWM_DMA_BUFFER_SIZE;            /* ���������� */
    DMA_InitStruct.Direction = DMA_DIR_PERIPH_DST;           /* ���䷽���ڴ浽���� */
    DMA_InitStruct.PeriphInc = DMA_PERIPH_INC_DISABLE;       /* �����ַ������ */
    DMA_InitStruct.MemoryInc = DMA_MEM_INC_ENABLE;           /* �ڴ��ַ���� */
    DMA_InitStruct.PeriphDataSize = DMA_PERIPH_DATA_WIDTH_HALFWORD; /* �������ݿ�ȣ�16λ */
    DMA_InitStruct.MemDataSize = DMA_MEM_DATA_WIDTH_HALFWORD;       /* �ڴ����ݿ�ȣ�16λ */
    DMA_InitStruct.CircularMode  = DMA_MODE_NORMAL;          /* ��ͨģʽ����ѭ���� */
    DMA_InitStruct.Mem2Mem = DMA_M2M_DISABLE;                /* �����ڴ浽�ڴ�ģʽ */
    DMA_InitStruct.Priority = DMA_PRIORITY_HIGH;             /* �����ȼ� */
    
    DMA_Init(GTIM2_CH, &DMA_InitStruct);     /* ��ʼ��DMA */
    
    /* ����DMA����ӳ�� */
    DMA_RequestRemap(DMA_REMAP_GTIM2_CH2, GTIM2_CH, ENABLE);
    
    /* ���ö�ʱ��DMA���� */
    TIM_EnableDma(GTIM2, TIM_DMA_CC2, ENABLE);
}

/**
 * @brief ����WS2812��ɫ
 * @param red ��ɫ������0-255��
 * @param green ��ɫ������0-255��
 * @param blue ��ɫ������0-255��
 * @details ��RGB��ɫת��ΪWS2812��GRB��ʽ�����DMA������
 */
void WS2812_SetColor(uint8_t red, uint8_t green, uint8_t blue)
{
    /* ���ȵ��ڣ����͵�25%���ȣ� */
    red = (uint8_t)(red * 0.25);
    green = (uint8_t)(green * 0.25);
    blue = (uint8_t)(blue * 0.25);
    
    /* WS2812ʹ��GRB˳����ɫ��ǰ�� */
    color = ((uint32_t)green << 16) | ((uint32_t)red << 8) | blue;
    
    /* ���DMA������������ɫλӳ��ΪPWMռ�ձ� */
    for(int i = 0; i < BYTES_PER_LED; i++) {
        /* �����λ��MSB����ʼ���� */
        if(color & (1 << (23 - i))) {
            PWM_DMA_BUFFER[i] = PWM_HIGH;    /* 1�룺�ߵ�ƽʱ��ϳ� */
        } else {
            PWM_DMA_BUFFER[i] = PWM_LOW;     /* 0�룺�ߵ�ƽʱ��϶� */
        }
    }
}

/**
 * @brief �������ݵ�WS2812
 * @details ͨ��DMA������ɫ���ݲ�������λ�ź�
 */
void WS2812_Send(void)
{
    /* ����DMA���������� */
    DMA_SetCurrDataCounter(GTIM2_CH, PWM_DMA_BUFFER_SIZE); 
    
    /* ������ʱ����DMA */
    TIM_Enable(GTIM2, ENABLE);
    TIM_EnableDma(GTIM2, TIM_DMA_CC2, ENABLE);
    DMA_EnableChannel(GTIM2_CH, ENABLE);
    
    /* �ȴ�DMA������� */
    while (!DMA_GetFlagStatus(DMA_FLAG_TC5, DMA1));
    DMA_ClearFlag(DMA_FLAG_TC5, DMA1);

    /* �رն�ʱ����DMA */
    DMA_EnableChannel(GTIM2_CH, DISABLE);
    TIM_Enable(GTIM2, DISABLE);
    TIM_EnableDma(GTIM2, TIM_DMA_CC2, DISABLE);
    
    /* ע�⣺��������>50us�ĵ͵�ƽ��λ�źţ��ɹرն�ʱ�������� */
}