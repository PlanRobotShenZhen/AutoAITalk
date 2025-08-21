#include "microphone.h"
#include "main.h"

/* I2S���Ŷ��� */
#define I2S_SD_PORT  GPIOB                 /* �������Ŷ˿� */
#define I2S_SD_PIN   GPIO_PIN_15           /* �������� */
#define I2S_SD_CLOCK RCC_AHB_PERIPHEN_GPIOB /* ��������ʱ�� */

#define I2S_SCK_PORT  GPIOB                /* ʱ�����Ŷ˿� */
#define I2S_SCK_PIN   GPIO_PIN_13          /* ʱ������ */
#define I2S_SCK_CLOCK RCC_AHB_PERIPHEN_GPIOB /* ʱ������ʱ�� */

#define I2S_WS_PORT  GPIOB                 /* ��ѡ�����Ŷ˿� */
#define I2S_WS_PIN   GPIO_PIN_12           /* ��ѡ������ */
#define I2S_WS_CLOCK RCC_AHB_PERIPHEN_GPIOB /* ��ѡ������ʱ�� */

#define I2S_MIC_DMA_CH DMA1_CH4            /* I2Sʹ�õ�DMAͨ�� */

/**
 * @brief I2S2ʱ�ӳ�ʼ��
 * @details ��ʼ��I2S2��GPIO��DMA�����ʱ��
 */
void I2S2_RCC_INIT(void)
{
    RCC_EnableAHB1PeriphClk(RCC_AHB_PERIPHEN_GPIOB, ENABLE);  /* ʹ��GPIOBʱ�� */
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_AFIO, ENABLE);    /* ʹ��AFIOʱ�� */    
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_SPI2, ENABLE);    /* ʹ��SPI2/I2S2ʱ�� */
    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPHEN_DMA1, ENABLE);    /* ʹ��DMA1ʱ�� */
}

/**
 * @brief I2S2 GPIO��ʼ��
 * @details ����I2S2�����ݡ�ʱ�Ӻ���ѡ������
 */
void I2S2_GPIO_INIT(void)
{
    GPIO_InitType GPIO_InitStruct;
    
    /* I2S2_SD (��������) ���� */
    GPIO_InitStruct.Pin = I2S_SD_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_MODE_INPUT;              /* ����ģʽ */
    GPIO_InitStruct.GPIO_Alternate = GPIO_AF_I2S2_SD_PB15;    /* I2S2���ݸ��ù��� */
    GPIO_InitStruct.GPIO_Pull = GPIO_NO_PULL;                 /* ���������� */
    GPIO_InitPeripheral(I2S_SD_PORT, &GPIO_InitStruct);

    /* I2S2_SCK (ʱ��) ���� */
    GPIO_InitStruct.Pin = I2S_SCK_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_MODE_AF_PP;              /* ����������� */
    GPIO_InitStruct.GPIO_Alternate = GPIO_AF_I2S2_CK_PB13;    /* I2S2ʱ�Ӹ��ù��� */
    GPIO_InitStruct.GPIO_Pull = GPIO_NO_PULL;                 /* ���������� */
    GPIO_InitPeripheral(I2S_SCK_PORT, &GPIO_InitStruct);

    /* I2S2_WS (��ѡ��/��������ʱ��) ���� */
    GPIO_InitStruct.Pin = I2S_WS_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_MODE_AF_PP;              /* ����������� */
    GPIO_InitStruct.GPIO_Alternate = GPIO_AF_I2S2_WS_PB12;    /* I2S2��ѡ���ù��� */
    GPIO_InitStruct.GPIO_Pull = GPIO_NO_PULL;                 /* ���������� */
    GPIO_InitPeripheral(I2S_WS_PORT, &GPIO_InitStruct);		
}

/**
 * @brief I2S2�ӿڳ�ʼ��
 * @details ����I2S2Ϊ������ģʽ�����ڽ�����˷�����
 */
void I2S2_I2S2_INIT(void)
{
    /* ����I2S2ʱ��ԴΪϵͳʱ�� */
    RCC_ConfigI2S2Clk(RCC_I2S2_CLKSRC_SYSCLK);
    
    I2S_InitType I2S_InitStructure;
    I2S_InitStructure.I2sMode = I2S_MODE_MASTER_RX;          /* ��ģʽ���� */
    I2S_InitStructure.Standard = I2S_STD_PHILLIPS;           /* Philips��׼ */
    I2S_InitStructure.DataFormat = I2S_DATA_FMT_16BITS;      /* 16λ���ݸ�ʽ */
    I2S_InitStructure.MCLKEnable = I2S_MCLK_DISABLE;         /* ����MCLK��� */
    I2S_InitStructure.AudioFrequency = I2S_AUDIO_FREQ_16K;   /* 16kHz������ */
    I2S_InitStructure.CLKPOL = I2S_CLKPOL_LOW;               /* ʱ�ӿ��е͵�ƽ */
    
    /* ��ȡϵͳʱ��Ƶ�ʲ�����I2S */
    RCC_ClocksType RCC_Clocks;
    RCC_GetClocksFreqValue(&RCC_Clocks);
    I2S_InitStructure.ClkSrcFrequency = RCC_Clocks.SysclkFreq;
    I2S_Init(SPI2, &I2S_InitStructure); 	
}

/**
 * @brief I2S2 DMA��ʼ��
 * @param buffer DMA������ָ��
 * @param BUFFER_SIZE ��������С
 * @details ����DMA����I2S���ݽ���
 */
void I2S2_DMA_INIT(int16_t *buffer, int BUFFER_SIZE) 
{
    DMA_DeInit(I2S_MIC_DMA_CH);                              /* ��λDMAͨ�� */
    DMA_InitType DMA_InitStruct;
    DMA_StructInit(&DMA_InitStruct);                         /* ��ʼ��DMA�ṹ�� */
    
    DMA_InitStruct.PeriphAddr = (uint32_t)&(SPI2->DAT);      /* �����ַ��SPI���ݼĴ��� */
    DMA_InitStruct.MemAddr = (uint32_t)buffer;               /* �ڴ��ַ�����ջ����� */
    DMA_InitStruct.BufSize = BUFFER_SIZE;                    /* ���������� */
    DMA_InitStruct.Direction = DMA_DIR_PERIPH_SRC;           /* ���䷽�����赽�ڴ� */
    DMA_InitStruct.PeriphInc = DMA_PERIPH_INC_DISABLE;       /* �����ַ�̶� */
    DMA_InitStruct.MemoryInc = DMA_MEM_INC_ENABLE;           /* �ڴ��ַ���� */
    DMA_InitStruct.Priority = DMA_PRIORITY_HIGH;             /* �����ȼ� */
    DMA_InitStruct.PeriphDataSize = DMA_PERIPH_DATA_WIDTH_HALFWORD; /* �������ݿ�ȣ�16λ */
    DMA_InitStruct.MemDataSize = DMA_MEM_DATA_WIDTH_HALFWORD;       /* �ڴ����ݿ�ȣ�16λ */
    DMA_InitStruct.CircularMode = DMA_MODE_CIRCULAR;         /* ѭ��ģʽ */
    DMA_Init(I2S_MIC_DMA_CH, &DMA_InitStruct);               /* ��ʼ��DMA */
    
    /* ����DMA����ӳ�� */
    DMA_RequestRemap(DMA_REMAP_SPI2_I2S2_RX, I2S_MIC_DMA_CH, ENABLE);
    
    /* ʹ��DMA��������ж� */
    DMA_ConfigInt(I2S_MIC_DMA_CH, DMA_INT_TXC | DMA_INT_TXC, ENABLE);
}

/**
 * @brief ��ʼ��˷�¼��
 * @details ����I2S��DMA����ʼ������˷�����
 */
void Mic_Start_Record(void) 
{
    /* ��DMA��I2S���� */
    SPI_I2S_EnableDma(SPI2, SPI_I2S_DMA_RX, ENABLE);
    
    /* ʹ��I2S2 */
    I2S_Enable(SPI2, ENABLE);
    
    /* ����DMA���� */
    DMA_EnableChannel(I2S_MIC_DMA_CH, ENABLE);
}