#include "ws2812.h"
#include "main.h"

/* WS2812硬件定义 */
#define LED_PB5_PORT  GPIOB                 /* LED数据引脚端口 */
#define LED_PB5_PIN   GPIO_PIN_5            /* LED数据引脚 */
#define LED_PB5_CLOCK RCC_AHB_PERIPHEN_GPIOB /* GPIOB时钟 */
#define GTIM2_CH DMA1_CH5                   /* 定时器2使用的DMA通道 */

/* WS2812参数定义 */
#define LED_NUM       1                     /* LED数量 */
#define BYTES_PER_LED 24                    /* 每个LED需要24位数据（GRB各8位） */

/* PWM时序参数（基于60MHz时钟） */
#define PWM_PERIOD    119                   /* 1us PWM周期（60MHz/1us = 60，实际119+1=120周期） */
#define PWM_HIGH      78                    /* 高电平占空比(约0.8us) */
#define PWM_LOW       42                    /* 低电平占空比(约0.4us) */

/* DMA缓冲区大小 = LED数量 * 24位 */
#define PWM_DMA_BUFFER_SIZE (LED_NUM * BYTES_PER_LED)

/* 全局变量 */
uint32_t color = 0;                         /* 当前颜色值 */
uint32_t PrescalerValue = 0;                /* 预分频值 */
uint16_t PWM_DMA_BUFFER[PWM_DMA_BUFFER_SIZE]; /* DMA传输缓冲区 */

/**
 * @brief WS2812时钟初始化
 * @details 初始化定时器、GPIO和DMA所需的时钟
 */
void WS2812_RCC_INIT(void)
{
    RCC_ConfigPclk1(RCC_HCLK_DIV4);         /* APB1=240MHz/4=60MHz */
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_GTIM2, ENABLE);  /* 使能定时器2时钟 */
    RCC_EnableAHB1PeriphClk(LED_PB5_CLOCK, ENABLE);          /* 使能GPIOB时钟 */
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_AFIO, ENABLE);   /* 使能AFIO时钟 */
    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPHEN_DMA1, ENABLE);   /* 使能DMA1时钟 */
}

/**
 * @brief WS2812 GPIO初始化
 * @details 配置LED数据引脚为定时器PWM输出模式
 */
void WS2812_GPIO_INIT(void)
{
    GPIO_InitType GPIO_InitStructure;
    
    GPIO_InitStruct(&GPIO_InitStructure);
    
    GPIO_InitStructure.Pin        = LED_PB5_PIN;             /* 引脚5 */
    GPIO_InitStructure.GPIO_Mode  = GPIO_MODE_AF_PP;         /* 复用推挽输出 */
    GPIO_InitStructure.GPIO_Slew_Rate = GPIO_SLEW_RATE_SLOW; /* 慢速摆率 */
    GPIO_InitStructure.GPIO_Alternate = GPIO_AF_GTIM2_CH2_PB5; /* 定时器2通道2复用 */
    GPIO_InitPeripheral(LED_PB5_PORT, &GPIO_InitStructure);  /* 初始化GPIO */
}

/**
 * @brief WS2812 PWM初始化
 * @details 配置定时器2为PWM模式，产生WS2812所需的时序信号
 */
void WS2812_PWM(void)
{
    TIM_TimeBaseInitType TIM_TimeBaseStructure;
    OCInitType TIM_OCInitStructure;
    
    /* 定时器基础配置 */
    TIM_InitTimBaseStruct(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.Period    = PWM_PERIOD;            /* 自动重装载值 */
    TIM_TimeBaseStructure.Prescaler = 0;                     /* 预分频器（不分频） */
    TIM_TimeBaseStructure.ClkDiv    = TIM_CLK_DIV1;          /* 时钟分频 */
    TIM_TimeBaseStructure.CounterMode = TIM_CNT_MODE_UP;     /* 向上计数模式 */
    TIM_InitTimeBase(GTIM2, &TIM_TimeBaseStructure);         /* 初始化定时器 */
    
    /* PWM模式配置：通道2 */
    TIM_InitOcStruct(&TIM_OCInitStructure);
    TIM_OCInitStructure.OCMode      = TIM_OCMODE_PWM1;       /* PWM模式1 */
    TIM_OCInitStructure.OutputState = TIM_OUTPUT_STATE_ENABLE; /* 输出使能 */
    TIM_OCInitStructure.Pulse       = 0;                     /* 初始占空比为0 */
    TIM_OCInitStructure.OCPolarity  = TIM_OC_POLARITY_HIGH;  /* 输出极性高 */
    TIM_OCInitStructure.OCIdleState = TIM_OC_IDLE_STATE_RESET; /* 空闲状态低 */
    TIM_InitOc2(GTIM2, &TIM_OCInitStructure);                /* 初始化输出比较通道2 */
    
    /* 启用预装载寄存器 */
    TIM_ConfigOc2Preload(GTIM2, TIM_OC_PRE_LOAD_ENABLE);
    TIM_ConfigArPreload(GTIM2, ENABLE);
}

/**
 * @brief WS2812 DMA初始化
 * @details 配置DMA用于自动传输PWM数据到定时器
 */
void WS2812_DMA_INIT(void)
{
    DMA_DeInit(GTIM2_CH);                    /* 复位DMA通道 */
    DMA_InitType DMA_InitStruct;
    DMA_StructInit(&DMA_InitStruct);         /* 初始化DMA结构体 */
    
    DMA_InitStruct.PeriphAddr = (uint32_t)&(GTIM2->CCDAT2);  /* 目标地址：TIM2 CCR2寄存器 */
    DMA_InitStruct.MemAddr = (uint32_t)PWM_DMA_BUFFER;       /* 源地址：DMA缓冲区 */
    DMA_InitStruct.BufSize = PWM_DMA_BUFFER_SIZE;            /* 传输数据量 */
    DMA_InitStruct.Direction = DMA_DIR_PERIPH_DST;           /* 传输方向：内存到外设 */
    DMA_InitStruct.PeriphInc = DMA_PERIPH_INC_DISABLE;       /* 外设地址不递增 */
    DMA_InitStruct.MemoryInc = DMA_MEM_INC_ENABLE;           /* 内存地址递增 */
    DMA_InitStruct.PeriphDataSize = DMA_PERIPH_DATA_WIDTH_HALFWORD; /* 外设数据宽度：16位 */
    DMA_InitStruct.MemDataSize = DMA_MEM_DATA_WIDTH_HALFWORD;       /* 内存数据宽度：16位 */
    DMA_InitStruct.CircularMode  = DMA_MODE_NORMAL;          /* 普通模式（非循环） */
    DMA_InitStruct.Mem2Mem = DMA_M2M_DISABLE;                /* 禁用内存到内存模式 */
    DMA_InitStruct.Priority = DMA_PRIORITY_HIGH;             /* 高优先级 */
    
    DMA_Init(GTIM2_CH, &DMA_InitStruct);     /* 初始化DMA */
    
    /* 配置DMA请求映射 */
    DMA_RequestRemap(DMA_REMAP_GTIM2_CH2, GTIM2_CH, ENABLE);
    
    /* 启用定时器DMA请求 */
    TIM_EnableDma(GTIM2, TIM_DMA_CC2, ENABLE);
}

/**
 * @brief 设置WS2812颜色
 * @param red 红色分量（0-255）
 * @param green 绿色分量（0-255）
 * @param blue 蓝色分量（0-255）
 * @details 将RGB颜色转换为WS2812的GRB格式并填充DMA缓冲区
 */
void WS2812_SetColor(uint8_t red, uint8_t green, uint8_t blue)
{
    /* 亮度调节（降低到25%亮度） */
    red = (uint8_t)(red * 0.25);
    green = (uint8_t)(green * 0.25);
    blue = (uint8_t)(blue * 0.25);
    
    /* WS2812使用GRB顺序（绿色在前） */
    color = ((uint32_t)green << 16) | ((uint32_t)red << 8) | blue;
    
    /* 填充DMA缓冲区，将颜色位映射为PWM占空比 */
    for(int i = 0; i < BYTES_PER_LED; i++) {
        /* 从最高位（MSB）开始处理 */
        if(color & (1 << (23 - i))) {
            PWM_DMA_BUFFER[i] = PWM_HIGH;    /* 1码：高电平时间较长 */
        } else {
            PWM_DMA_BUFFER[i] = PWM_LOW;     /* 0码：高电平时间较短 */
        }
    }
}

/**
 * @brief 发送数据到WS2812
 * @details 通过DMA传输颜色数据并产生复位信号
 */
void WS2812_Send(void)
{
    /* 设置DMA传输数据量 */
    DMA_SetCurrDataCounter(GTIM2_CH, PWM_DMA_BUFFER_SIZE); 
    
    /* 启动定时器和DMA */
    TIM_Enable(GTIM2, ENABLE);
    TIM_EnableDma(GTIM2, TIM_DMA_CC2, ENABLE);
    DMA_EnableChannel(GTIM2_CH, ENABLE);
    
    /* 等待DMA传输完成 */
    while (!DMA_GetFlagStatus(DMA_FLAG_TC5, DMA1));
    DMA_ClearFlag(DMA_FLAG_TC5, DMA1);

    /* 关闭定时器和DMA */
    DMA_EnableChannel(GTIM2_CH, DISABLE);
    TIM_Enable(GTIM2, DISABLE);
    TIM_EnableDma(GTIM2, TIM_DMA_CC2, DISABLE);
    
    /* 注意：这里会产生>50us的低电平复位信号（由关闭定时器产生） */
}