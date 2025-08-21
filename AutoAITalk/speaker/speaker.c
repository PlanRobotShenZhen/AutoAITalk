#include "speaker.h"
#include "main.h"

/* ======================= 引脚定义 ======================= */

// I2S3 数据输入引脚 (SD / DIN)
#define I2S_SPEAKER_DIN_PORT     GPIOC
#define I2S_SPEAKER_DIN_PIN      GPIO_PIN_1
#define I2S_SPEAKER_DIN_CLOCK    RCC_AHB_PERIPHEN_GPIOC 

// I2S3 帧同步信号 (WS / LRCLK)，指示左右声道
#define I2S_SPEAKER_LRCLK_PORT   GPIOC
#define I2S_SPEAKER_LRCLK_PIN    GPIO_PIN_2
#define I2S_SPEAKER_LRCLK_CLOCK  RCC_AHB_PERIPHEN_GPIOC

// I2S3 位时钟 (SCK / BCLK)
#define I2S_SPEAKER_BCLK_PORT    GPIOC
#define I2S_SPEAKER_BCLK_PIN     GPIO_PIN_3
#define I2S_SPEAKER_BCLK_CLOCK   RCC_AHB_PERIPHEN_GPIOC

// 使用的 DMA 通道：SPI3_TX 映射到 DMA2_CH2
#define I2S_SPEAK_DMA_CH         DMA2_CH2


/* ======================= 外设时钟初始化 ======================= */

/**
 * @brief  配置 I2S 模块所需的所有外设时钟
 *         - 使能 GPIOC 时钟（用于 I2S 引脚）
 *         - 使能 AFIO 时钟（用于引脚复用）
 *         - 使能 SPI3（I2S）时钟
 *         - 使能 DMA2 时钟（用于 DMA 传输）
 */
void SPEAKER_RCC_INIT(void)
{
    RCC_EnableAHB1PeriphClk(I2S_SPEAKER_LRCLK_CLOCK, ENABLE); // GPIOC
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_AFIO, ENABLE);     // 复用功能
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_SPI3, ENABLE);     // SPI3/I2S3
    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPHEN_DMA2, ENABLE);     // DMA2
}


/* ======================= GPIO 初始化 ======================= */

/**
 * @brief  配置 I2S 所需的 GPIO 引脚为复用推挽输出模式
 *         - DIN: 数据输入（从主设备到从设备）
 *         - LRCLK: 左右声道时钟（帧同步）
 *         - BCLK: 位时钟
 *
 * @note   所有引脚均位于 GPIOC 上，使用特定的复用功能映射。
 */
void SPEAKER_GPIO_INIT(void)
{
    GPIO_InitType GPIO_InitStruct;

    // 配置 I2S3_DIN (SD) 引脚: PC1
    GPIO_InitStruct.Pin = I2S_SPEAKER_DIN_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_MODE_AF_PP;               // 复用推挽输出
    GPIO_InitStruct.GPIO_Alternate = GPIO_AF_I2S3_SD_PC1;      // 映射到 I2S3_SD
    GPIO_InitStruct.GPIO_Pull = GPIO_NO_PULL;
    GPIO_InitStruct.GPIO_Slew_Rate = GPIO_SLEW_RATE_FAST;      // 高速
    GPIO_InitPeripheral(I2S_SPEAKER_DIN_PORT, &GPIO_InitStruct); 	

    // 配置 I2S3_LRCLK (WS) 引脚: PC2
    GPIO_InitStruct.Pin = I2S_SPEAKER_LRCLK_PIN;
    GPIO_InitStruct.GPIO_Alternate = GPIO_AF_I2S3_WS_PC2;      // 映射到 I2S3_WS
    GPIO_InitPeripheral(I2S_SPEAKER_LRCLK_PORT, &GPIO_InitStruct); 	

    // 配置 I2S3_BCLK (SCK) 引脚: PC3
    GPIO_InitStruct.Pin = I2S_SPEAKER_BCLK_PIN;
    GPIO_InitStruct.GPIO_Alternate = GPIO_AF_I2S3_CK_PC3;      // 映射到 I2S3_CK
    GPIO_InitPeripheral(I2S_SPEAKER_BCLK_PORT, &GPIO_InitStruct); 	
}


/* ======================= I2S 外设初始化 ======================= */

/**
 * @brief  初始化 SPI3 为 I2S 主发送模式
 *         - 设置为主机（Master）
 *         - 发送模式（TX）
 *         - PHILLIPS 数据标准
 *         - 16位数据宽度
 *         - 8kHz 音频采样率
 *         - 时钟极性：空闲低
 *         - 使用系统时钟作为 I2S 时钟源
 */
void SPEAKER_I2S_INIT(void)
{
    // 复位 SPI3 到默认状态
    SPI_I2S_DeInit(SPI3);

    // 选择 I2S3 的时钟源为系统时钟（Sysclk），避免使用 PLLI2S
    RCC_ConfigI2S3Clk(RCC_I2S3_CLKSRC_SYSCLK);

    I2S_InitType I2S_InitStructure;
    I2S_InitStruct(&I2S_InitStructure); // 初始化结构体默认值

    I2S_InitStructure.I2sMode          = I2S_MODE_MASTER_TX;     // 主发送模式
    I2S_InitStructure.Standard         = I2S_STD_PHILLIPS;       // PHILLIPS 标准
    I2S_InitStructure.DataFormat       = I2S_DATA_FMT_16BITS;    // 16位数据
    I2S_InitStructure.MCLKEnable       = I2S_MCLK_DISABLE;       // 不使用 MCLK
    I2S_InitStructure.AudioFrequency   = I2S_AUDIO_FREQ_8K;      // 8kHz 采样率
    I2S_InitStructure.CLKPOL           = I2S_CLKPOL_LOW;         // 时钟空闲状态为低

    // 获取系统时钟频率，用于 I2S 分频计算
    RCC_ClocksType RCC_Clocks;
    RCC_GetClocksFreqValue(&RCC_Clocks);
    I2S_InitStructure.ClkSrcFrequency = RCC_Clocks.SysclkFreq;  // 设置时钟源频率

    // 应用配置并初始化 I2S
    I2S_Init(SPI3, &I2S_InitStructure);
}


/* ======================= DMA 初始化 ======================= */

/**
 * @brief  配置 DMA 用于 I2S 数据传输（内存 → 外设）
 *
 * @param buffer      指向音频数据缓冲区（int16_t 类型）
 * @param BUFFER_SIZE 缓冲区大小（以 half-word 为单位）
 *
 * @note  使用 DMA2_CH2，通道映射为 SPI3_TX。
 */
void SPEAKER_DMA_INIT(int16_t *buffer, int BUFFER_SIZE)
{	
    // 复位 DMA 通道
    DMA_DeInit(I2S_SPEAK_DMA_CH);

    DMA_InitType DMA_InitStructure;
    DMA_StructInit(&DMA_InitStructure); // 填充默认值

    DMA_InitStructure.PeriphAddr       = (uint32_t)&(SPI3->DAT);         // 外设地址：SPI3 数据寄存器
    DMA_InitStructure.MemAddr          = (uint32_t)buffer;               // 内存地址：音频缓冲区
    DMA_InitStructure.Direction        = DMA_DIR_PERIPH_DST;             // 传输方向：内存 → 外设
    DMA_InitStructure.BufSize          = BUFFER_SIZE;                    // 传输数据量（半字数）
    DMA_InitStructure.PeriphInc        = DMA_PERIPH_INC_DISABLE;         // 外设地址不递增
    DMA_InitStructure.MemoryInc        = DMA_MEM_INC_ENABLE;             // 内存地址递增
    DMA_InitStructure.PeriphDataSize   = DMA_PERIPH_DATA_WIDTH_HALFWORD;// 外设数据宽度：16位
    DMA_InitStructure.MemDataSize      = DMA_MEM_DATA_WIDTH_HALFWORD;   // 内存数据宽度：16位
    DMA_InitStructure.CircularMode     = DMA_MODE_NORMAL;                // 非循环模式
    DMA_InitStructure.Priority         = DMA_PRIORITY_HIGH;              // 高优先级

    // 初始化 DMA 通道
    DMA_Init(I2S_SPEAK_DMA_CH, &DMA_InitStructure);

    // 将 SPI3_TX 请求映射到 DMA2_CH2
    DMA_RequestRemap(DMA_REMAP_SPI3_I2S3_TX, I2S_SPEAK_DMA_CH, ENABLE);
}


/* ======================= 启动播放 ======================= */

/**
 * @brief  启动 I2S 播放
 *         - 使能 SPI3 的 TX DMA 请求
 *         - 使能 I2S 外设
 *         - **注意：DMA 通道未立即开启**，需在准备好数据后手动启动
 *
 * @note  实际开始传输需调用 DMA_EnableChannel(...) 并启动 DMA。
 */
void SPEAKER_Start(void)
{
    // 使能 SPI3 的发送 DMA 功能
    SPI_I2S_EnableDma(SPI3, SPI_I2S_DMA_TX, ENABLE);

    // 启动 I2S 外设
    I2S_Enable(SPI3, ENABLE);

    // 【重要】此处 DMA 被禁用，需在填充数据后手动开启
    DMA_EnableChannel(DMA2_CH2, DISABLE);
}


/* ======================= 音量控制 ======================= */

/**
 * @brief  调整音频缓冲区的音量（软件音量控制）
 *
 * @param buffer  指向 int16_t 类型的音频数据缓冲区
 * @param size    缓冲区中样本数量（元素个数）
 * @param volume  音量百分比（0~100）
 *
 * @note  使用浮点乘法实现线性音量调节，结果做饱和处理（-32768 ~ 32767）
 */
void SPEAKER_Set_Vol(int16_t *buffer, uint32_t size, uint8_t volume)
{
    // 将音量转换为 0.0 ~ 1.0 的浮点因子
    float factor = (float)volume / 100.0f;

    for (uint32_t i = 0; i < size; i++)
    {
        // 计算放大后的值（int32_t 防止溢出）
        int32_t scaled = (int32_t)(buffer[i]) * factor;

        // 饱和处理，防止溢出 int16_t 范围
        if (scaled > 32767)
            buffer[i] = 32767;
        else if (scaled < -32768)
            buffer[i] = -32768;
        else
            buffer[i] = (int16_t)scaled;
    }
}