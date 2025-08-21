#include "microphone.h"
#include "main.h"

/* I2S引脚定义 */
#define I2S_SD_PORT  GPIOB                 /* 数据引脚端口 */
#define I2S_SD_PIN   GPIO_PIN_15           /* 数据引脚 */
#define I2S_SD_CLOCK RCC_AHB_PERIPHEN_GPIOB /* 数据引脚时钟 */

#define I2S_SCK_PORT  GPIOB                /* 时钟引脚端口 */
#define I2S_SCK_PIN   GPIO_PIN_13          /* 时钟引脚 */
#define I2S_SCK_CLOCK RCC_AHB_PERIPHEN_GPIOB /* 时钟引脚时钟 */

#define I2S_WS_PORT  GPIOB                 /* 字选择引脚端口 */
#define I2S_WS_PIN   GPIO_PIN_12           /* 字选择引脚 */
#define I2S_WS_CLOCK RCC_AHB_PERIPHEN_GPIOB /* 字选择引脚时钟 */

#define I2S_MIC_DMA_CH DMA1_CH4            /* I2S使用的DMA通道 */

/**
 * @brief I2S2时钟初始化
 * @details 初始化I2S2、GPIO和DMA所需的时钟
 */
void I2S2_RCC_INIT(void)
{
    RCC_EnableAHB1PeriphClk(RCC_AHB_PERIPHEN_GPIOB, ENABLE);  /* 使能GPIOB时钟 */
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_AFIO, ENABLE);    /* 使能AFIO时钟 */    
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_SPI2, ENABLE);    /* 使能SPI2/I2S2时钟 */
    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPHEN_DMA1, ENABLE);    /* 使能DMA1时钟 */
}

/**
 * @brief I2S2 GPIO初始化
 * @details 配置I2S2的数据、时钟和字选择引脚
 */
void I2S2_GPIO_INIT(void)
{
    GPIO_InitType GPIO_InitStruct;
    
    /* I2S2_SD (数据输入) 配置 */
    GPIO_InitStruct.Pin = I2S_SD_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_MODE_INPUT;              /* 输入模式 */
    GPIO_InitStruct.GPIO_Alternate = GPIO_AF_I2S2_SD_PB15;    /* I2S2数据复用功能 */
    GPIO_InitStruct.GPIO_Pull = GPIO_NO_PULL;                 /* 无上拉下拉 */
    GPIO_InitPeripheral(I2S_SD_PORT, &GPIO_InitStruct);

    /* I2S2_SCK (时钟) 配置 */
    GPIO_InitStruct.Pin = I2S_SCK_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_MODE_AF_PP;              /* 复用推挽输出 */
    GPIO_InitStruct.GPIO_Alternate = GPIO_AF_I2S2_CK_PB13;    /* I2S2时钟复用功能 */
    GPIO_InitStruct.GPIO_Pull = GPIO_NO_PULL;                 /* 无上拉下拉 */
    GPIO_InitPeripheral(I2S_SCK_PORT, &GPIO_InitStruct);

    /* I2S2_WS (字选择/左右声道时钟) 配置 */
    GPIO_InitStruct.Pin = I2S_WS_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_MODE_AF_PP;              /* 复用推挽输出 */
    GPIO_InitStruct.GPIO_Alternate = GPIO_AF_I2S2_WS_PB12;    /* I2S2字选择复用功能 */
    GPIO_InitStruct.GPIO_Pull = GPIO_NO_PULL;                 /* 无上拉下拉 */
    GPIO_InitPeripheral(I2S_WS_PORT, &GPIO_InitStruct);		
}

/**
 * @brief I2S2接口初始化
 * @details 配置I2S2为主接收模式，用于接收麦克风数据
 */
void I2S2_I2S2_INIT(void)
{
    /* 配置I2S2时钟源为系统时钟 */
    RCC_ConfigI2S2Clk(RCC_I2S2_CLKSRC_SYSCLK);
    
    I2S_InitType I2S_InitStructure;
    I2S_InitStructure.I2sMode = I2S_MODE_MASTER_RX;          /* 主模式接收 */
    I2S_InitStructure.Standard = I2S_STD_PHILLIPS;           /* Philips标准 */
    I2S_InitStructure.DataFormat = I2S_DATA_FMT_16BITS;      /* 16位数据格式 */
    I2S_InitStructure.MCLKEnable = I2S_MCLK_DISABLE;         /* 禁用MCLK输出 */
    I2S_InitStructure.AudioFrequency = I2S_AUDIO_FREQ_16K;   /* 16kHz采样率 */
    I2S_InitStructure.CLKPOL = I2S_CLKPOL_LOW;               /* 时钟空闲低电平 */
    
    /* 获取系统时钟频率并配置I2S */
    RCC_ClocksType RCC_Clocks;
    RCC_GetClocksFreqValue(&RCC_Clocks);
    I2S_InitStructure.ClkSrcFrequency = RCC_Clocks.SysclkFreq;
    I2S_Init(SPI2, &I2S_InitStructure); 	
}

/**
 * @brief I2S2 DMA初始化
 * @param buffer DMA缓冲区指针
 * @param BUFFER_SIZE 缓冲区大小
 * @details 配置DMA用于I2S数据接收
 */
void I2S2_DMA_INIT(int16_t *buffer, int BUFFER_SIZE) 
{
    DMA_DeInit(I2S_MIC_DMA_CH);                              /* 复位DMA通道 */
    DMA_InitType DMA_InitStruct;
    DMA_StructInit(&DMA_InitStruct);                         /* 初始化DMA结构体 */
    
    DMA_InitStruct.PeriphAddr = (uint32_t)&(SPI2->DAT);      /* 外设地址：SPI数据寄存器 */
    DMA_InitStruct.MemAddr = (uint32_t)buffer;               /* 内存地址：接收缓冲区 */
    DMA_InitStruct.BufSize = BUFFER_SIZE;                    /* 传输数据量 */
    DMA_InitStruct.Direction = DMA_DIR_PERIPH_SRC;           /* 传输方向：外设到内存 */
    DMA_InitStruct.PeriphInc = DMA_PERIPH_INC_DISABLE;       /* 外设地址固定 */
    DMA_InitStruct.MemoryInc = DMA_MEM_INC_ENABLE;           /* 内存地址递增 */
    DMA_InitStruct.Priority = DMA_PRIORITY_HIGH;             /* 高优先级 */
    DMA_InitStruct.PeriphDataSize = DMA_PERIPH_DATA_WIDTH_HALFWORD; /* 外设数据宽度：16位 */
    DMA_InitStruct.MemDataSize = DMA_MEM_DATA_WIDTH_HALFWORD;       /* 内存数据宽度：16位 */
    DMA_InitStruct.CircularMode = DMA_MODE_CIRCULAR;         /* 循环模式 */
    DMA_Init(I2S_MIC_DMA_CH, &DMA_InitStruct);               /* 初始化DMA */
    
    /* 配置DMA请求映射 */
    DMA_RequestRemap(DMA_REMAP_SPI2_I2S2_RX, I2S_MIC_DMA_CH, ENABLE);
    
    /* 使能DMA传输完成中断 */
    DMA_ConfigInt(I2S_MIC_DMA_CH, DMA_INT_TXC | DMA_INT_TXC, ENABLE);
}

/**
 * @brief 开始麦克风录音
 * @details 启动I2S和DMA，开始接收麦克风数据
 */
void Mic_Start_Record(void) 
{
    /* 绑定DMA到I2S接收 */
    SPI_I2S_EnableDma(SPI2, SPI_I2S_DMA_RX, ENABLE);
    
    /* 使能I2S2 */
    I2S_Enable(SPI2, ENABLE);
    
    /* 启动DMA传输 */
    DMA_EnableChannel(I2S_MIC_DMA_CH, ENABLE);
}