#include "speaker.h"
#include "main.h"

/* ======================= ���Ŷ��� ======================= */

// I2S3 ������������ (SD / DIN)
#define I2S_SPEAKER_DIN_PORT     GPIOC
#define I2S_SPEAKER_DIN_PIN      GPIO_PIN_1
#define I2S_SPEAKER_DIN_CLOCK    RCC_AHB_PERIPHEN_GPIOC 

// I2S3 ֡ͬ���ź� (WS / LRCLK)��ָʾ��������
#define I2S_SPEAKER_LRCLK_PORT   GPIOC
#define I2S_SPEAKER_LRCLK_PIN    GPIO_PIN_2
#define I2S_SPEAKER_LRCLK_CLOCK  RCC_AHB_PERIPHEN_GPIOC

// I2S3 λʱ�� (SCK / BCLK)
#define I2S_SPEAKER_BCLK_PORT    GPIOC
#define I2S_SPEAKER_BCLK_PIN     GPIO_PIN_3
#define I2S_SPEAKER_BCLK_CLOCK   RCC_AHB_PERIPHEN_GPIOC

// ʹ�õ� DMA ͨ����SPI3_TX ӳ�䵽 DMA2_CH2
#define I2S_SPEAK_DMA_CH         DMA2_CH2


/* ======================= ����ʱ�ӳ�ʼ�� ======================= */

/**
 * @brief  ���� I2S ģ���������������ʱ��
 *         - ʹ�� GPIOC ʱ�ӣ����� I2S ���ţ�
 *         - ʹ�� AFIO ʱ�ӣ��������Ÿ��ã�
 *         - ʹ�� SPI3��I2S��ʱ��
 *         - ʹ�� DMA2 ʱ�ӣ����� DMA ���䣩
 */
void SPEAKER_RCC_INIT(void)
{
    RCC_EnableAHB1PeriphClk(I2S_SPEAKER_LRCLK_CLOCK, ENABLE); // GPIOC
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_AFIO, ENABLE);     // ���ù���
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_SPI3, ENABLE);     // SPI3/I2S3
    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPHEN_DMA2, ENABLE);     // DMA2
}


/* ======================= GPIO ��ʼ�� ======================= */

/**
 * @brief  ���� I2S ����� GPIO ����Ϊ�����������ģʽ
 *         - DIN: �������루�����豸�����豸��
 *         - LRCLK: ��������ʱ�ӣ�֡ͬ����
 *         - BCLK: λʱ��
 *
 * @note   �������ž�λ�� GPIOC �ϣ�ʹ���ض��ĸ��ù���ӳ�䡣
 */
void SPEAKER_GPIO_INIT(void)
{
    GPIO_InitType GPIO_InitStruct;

    // ���� I2S3_DIN (SD) ����: PC1
    GPIO_InitStruct.Pin = I2S_SPEAKER_DIN_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_MODE_AF_PP;               // �����������
    GPIO_InitStruct.GPIO_Alternate = GPIO_AF_I2S3_SD_PC1;      // ӳ�䵽 I2S3_SD
    GPIO_InitStruct.GPIO_Pull = GPIO_NO_PULL;
    GPIO_InitStruct.GPIO_Slew_Rate = GPIO_SLEW_RATE_FAST;      // ����
    GPIO_InitPeripheral(I2S_SPEAKER_DIN_PORT, &GPIO_InitStruct); 	

    // ���� I2S3_LRCLK (WS) ����: PC2
    GPIO_InitStruct.Pin = I2S_SPEAKER_LRCLK_PIN;
    GPIO_InitStruct.GPIO_Alternate = GPIO_AF_I2S3_WS_PC2;      // ӳ�䵽 I2S3_WS
    GPIO_InitPeripheral(I2S_SPEAKER_LRCLK_PORT, &GPIO_InitStruct); 	

    // ���� I2S3_BCLK (SCK) ����: PC3
    GPIO_InitStruct.Pin = I2S_SPEAKER_BCLK_PIN;
    GPIO_InitStruct.GPIO_Alternate = GPIO_AF_I2S3_CK_PC3;      // ӳ�䵽 I2S3_CK
    GPIO_InitPeripheral(I2S_SPEAKER_BCLK_PORT, &GPIO_InitStruct); 	
}


/* ======================= I2S �����ʼ�� ======================= */

/**
 * @brief  ��ʼ�� SPI3 Ϊ I2S ������ģʽ
 *         - ����Ϊ������Master��
 *         - ����ģʽ��TX��
 *         - PHILLIPS ���ݱ�׼
 *         - 16λ���ݿ��
 *         - 8kHz ��Ƶ������
 *         - ʱ�Ӽ��ԣ����е�
 *         - ʹ��ϵͳʱ����Ϊ I2S ʱ��Դ
 */
void SPEAKER_I2S_INIT(void)
{
    // ��λ SPI3 ��Ĭ��״̬
    SPI_I2S_DeInit(SPI3);

    // ѡ�� I2S3 ��ʱ��ԴΪϵͳʱ�ӣ�Sysclk��������ʹ�� PLLI2S
    RCC_ConfigI2S3Clk(RCC_I2S3_CLKSRC_SYSCLK);

    I2S_InitType I2S_InitStructure;
    I2S_InitStruct(&I2S_InitStructure); // ��ʼ���ṹ��Ĭ��ֵ

    I2S_InitStructure.I2sMode          = I2S_MODE_MASTER_TX;     // ������ģʽ
    I2S_InitStructure.Standard         = I2S_STD_PHILLIPS;       // PHILLIPS ��׼
    I2S_InitStructure.DataFormat       = I2S_DATA_FMT_16BITS;    // 16λ����
    I2S_InitStructure.MCLKEnable       = I2S_MCLK_DISABLE;       // ��ʹ�� MCLK
    I2S_InitStructure.AudioFrequency   = I2S_AUDIO_FREQ_8K;      // 8kHz ������
    I2S_InitStructure.CLKPOL           = I2S_CLKPOL_LOW;         // ʱ�ӿ���״̬Ϊ��

    // ��ȡϵͳʱ��Ƶ�ʣ����� I2S ��Ƶ����
    RCC_ClocksType RCC_Clocks;
    RCC_GetClocksFreqValue(&RCC_Clocks);
    I2S_InitStructure.ClkSrcFrequency = RCC_Clocks.SysclkFreq;  // ����ʱ��ԴƵ��

    // Ӧ�����ò���ʼ�� I2S
    I2S_Init(SPI3, &I2S_InitStructure);
}


/* ======================= DMA ��ʼ�� ======================= */

/**
 * @brief  ���� DMA ���� I2S ���ݴ��䣨�ڴ� �� ���裩
 *
 * @param buffer      ָ����Ƶ���ݻ�������int16_t ���ͣ�
 * @param BUFFER_SIZE ��������С���� half-word Ϊ��λ��
 *
 * @note  ʹ�� DMA2_CH2��ͨ��ӳ��Ϊ SPI3_TX��
 */
void SPEAKER_DMA_INIT(int16_t *buffer, int BUFFER_SIZE)
{	
    // ��λ DMA ͨ��
    DMA_DeInit(I2S_SPEAK_DMA_CH);

    DMA_InitType DMA_InitStructure;
    DMA_StructInit(&DMA_InitStructure); // ���Ĭ��ֵ

    DMA_InitStructure.PeriphAddr       = (uint32_t)&(SPI3->DAT);         // �����ַ��SPI3 ���ݼĴ���
    DMA_InitStructure.MemAddr          = (uint32_t)buffer;               // �ڴ��ַ����Ƶ������
    DMA_InitStructure.Direction        = DMA_DIR_PERIPH_DST;             // ���䷽���ڴ� �� ����
    DMA_InitStructure.BufSize          = BUFFER_SIZE;                    // ��������������������
    DMA_InitStructure.PeriphInc        = DMA_PERIPH_INC_DISABLE;         // �����ַ������
    DMA_InitStructure.MemoryInc        = DMA_MEM_INC_ENABLE;             // �ڴ��ַ����
    DMA_InitStructure.PeriphDataSize   = DMA_PERIPH_DATA_WIDTH_HALFWORD;// �������ݿ�ȣ�16λ
    DMA_InitStructure.MemDataSize      = DMA_MEM_DATA_WIDTH_HALFWORD;   // �ڴ����ݿ�ȣ�16λ
    DMA_InitStructure.CircularMode     = DMA_MODE_NORMAL;                // ��ѭ��ģʽ
    DMA_InitStructure.Priority         = DMA_PRIORITY_HIGH;              // �����ȼ�

    // ��ʼ�� DMA ͨ��
    DMA_Init(I2S_SPEAK_DMA_CH, &DMA_InitStructure);

    // �� SPI3_TX ����ӳ�䵽 DMA2_CH2
    DMA_RequestRemap(DMA_REMAP_SPI3_I2S3_TX, I2S_SPEAK_DMA_CH, ENABLE);
}


/* ======================= �������� ======================= */

/**
 * @brief  ���� I2S ����
 *         - ʹ�� SPI3 �� TX DMA ����
 *         - ʹ�� I2S ����
 *         - **ע�⣺DMA ͨ��δ��������**������׼�������ݺ��ֶ�����
 *
 * @note  ʵ�ʿ�ʼ��������� DMA_EnableChannel(...) ������ DMA��
 */
void SPEAKER_Start(void)
{
    // ʹ�� SPI3 �ķ��� DMA ����
    SPI_I2S_EnableDma(SPI3, SPI_I2S_DMA_TX, ENABLE);

    // ���� I2S ����
    I2S_Enable(SPI3, ENABLE);

    // ����Ҫ���˴� DMA �����ã�����������ݺ��ֶ�����
    DMA_EnableChannel(DMA2_CH2, DISABLE);
}


/* ======================= �������� ======================= */

/**
 * @brief  ������Ƶ������������������������ƣ�
 *
 * @param buffer  ָ�� int16_t ���͵���Ƶ���ݻ�����
 * @param size    ������������������Ԫ�ظ�����
 * @param volume  �����ٷֱȣ�0~100��
 *
 * @note  ʹ�ø���˷�ʵ�������������ڣ���������ʹ���-32768 ~ 32767��
 */
void SPEAKER_Set_Vol(int16_t *buffer, uint32_t size, uint8_t volume)
{
    // ������ת��Ϊ 0.0 ~ 1.0 �ĸ�������
    float factor = (float)volume / 100.0f;

    for (uint32_t i = 0; i < size; i++)
    {
        // ����Ŵ���ֵ��int32_t ��ֹ�����
        int32_t scaled = (int32_t)(buffer[i]) * factor;

        // ���ʹ�����ֹ��� int16_t ��Χ
        if (scaled > 32767)
            buffer[i] = 32767;
        else if (scaled < -32768)
            buffer[i] = -32768;
        else
            buffer[i] = (int16_t)scaled;
    }
}