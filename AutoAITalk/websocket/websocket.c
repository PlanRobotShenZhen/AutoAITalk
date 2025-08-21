#include "websocket.h"
#include "main.h"

#define UART7_TX_PORT  GPIOB
#define UART7_TX_PIN   GPIO_PIN_11
#define UART7_TX_CLOCK RCC_AHB_PERIPHEN_GPIOB

#define UART7_RX_PORT  GPIOB
#define UART7_RX_PIN   GPIO_PIN_10
#define UART7_RX_CLOCK RCC_AHB_PERIPHEN_GPIOB

#define UART7_RX_DMA_CH DMA2_CH6
#define UART7_TX_DMA_CH DMA2_CH7


/**
 * @brief  UART7 ʱ�ӳ�ʼ������
 * @note   ���� UART7 ģ�鼰��������裨GPIO��AFIO��DMA����ʱ��
 *         Ϊ�����������š����ں� DMA �ṩʱ��֧��
 * @param  ��
 * @retval ��
 */
void UART7_RCC_INIT(void)
{
    // ���� UART7 �� TX �� RX �������� GPIO �˿ڵ� AHB1 ����ʱ��
    RCC_EnableAHB1PeriphClk(UART7_TX_CLOCK | UART7_RX_CLOCK, ENABLE); 

    // ʹ�ܸ��ù��� I/O (AFIO) �� APB2 ����ʱ�ӣ������������ŵĸ��ù��ܣ��� UART ���ܣ�
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_AFIO, ENABLE);    

    // ʹ�� UART7 ģ������� APB2 ����ʱ�ӣ����������Ĵ���
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_UART7, ENABLE);

    // ʹ�� DMA2 ģ��� AHB ����ʱ�ӣ�Ϊ����ʹ�� DMA ����������׼��
    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPHEN_DMA2, ENABLE);    
}

/**
 * @brief  UART7 GPIO ���ų�ʼ������
 * @note   ���� UART7 �ķ��ͣ�TX���ͽ��գ�RX������Ϊ��������ģʽ
 *         �����������������л����ʵȲ���
 * @param  ��
 * @retval ��
 */
void UART7_GPIO_INIT(void)
{
    GPIO_InitType GPIO_InitStruct;

    // ���� UART7 TX ����
    GPIO_InitStruct.Pin = UART7_TX_PIN;                          // ָ�� TX ���ű��
    GPIO_InitStruct.GPIO_Mode = GPIO_MODE_AF_PP;                // ����Ϊ�����������ģʽ
    GPIO_InitStruct.GPIO_Alternate = GPIO_AF_UART7_TX_PB11;      // ӳ�䵽 UART7 �� TX ����
    GPIO_InitStruct.GPIO_Pull = GPIO_PULL_UP;                   // �����������裬����ź��ȶ���
    GPIO_InitStruct.GPIO_Slew_Rate = GPIO_SLEW_RATE_FAST;       // ���������л�����Ϊ�����١�
    GPIO_InitPeripheral(UART7_TX_PORT, &GPIO_InitStruct);       // Ӧ�����õ�ָ���˿�

    // ���� UART7 RX ����
    GPIO_InitStruct.Pin = UART7_RX_PIN;                          // ָ�� RX ���ű��
    GPIO_InitStruct.GPIO_Mode = GPIO_MODE_AF_PP;                // ͬ��Ϊ��������ģʽ��������Ӳ���Զ�����
    GPIO_InitStruct.GPIO_Alternate = GPIO_AF_UART7_RX_PB10;      // ӳ�䵽 UART7 �� RX ����
    GPIO_InitStruct.GPIO_Pull = GPIO_PULL_UP;                   // ������������ǿ����������
    GPIO_InitStruct.GPIO_Slew_Rate = GPIO_SLEW_RATE_FAST;       // �����л�����
    GPIO_InitPeripheral(UART7_RX_PORT, &GPIO_InitStruct);       // Ӧ�����õ� RX �˿�
}

/**
 * @brief  UART7 ����ģ���ʼ������
 * @note   ���� UART7 �Ļ���ͨ�Ų��������������ʡ�����λ��ֹͣλ��У�鷽ʽ��
 *         �����ý���/���͹��ܡ�DMA ��������Ϳ����ж�
 * @param  ��
 * @retval ��
 */
void UART7_INIT(void)
{
    USART_InitType USART_InitStructure;
    USART_StructInit(&USART_InitStructure); // ʹ��Ĭ��ֵ���ṹ�壬ȷ��δ�����ֶΰ�ȫ

    // ���ô���ͨ�Ų���
    USART_InitStructure.BaudRate = 43000;                       // ���ò�����Ϊ 43000 bps
    USART_InitStructure.WordLength = USART_WL_8B;               // ����λ���ȣ�8 λ
    USART_InitStructure.StopBits = USART_STPB_1;                // ֹͣλ��1 λ
    USART_InitStructure.Parity = USART_PE_NO;                   // ����żУ��
    USART_InitStructure.HardwareFlowControl = USART_HFCTRL_NONE;// ��ʹ��Ӳ�����أ�RTS/CTS��
    USART_InitStructure.OverSampling = USART_16OVER;            // ���� 16 ��������������ȶ���
    USART_InitStructure.Mode = USART_MODE_RX | USART_MODE_TX;   // ���ý��պͷ���˫��ģʽ

    // ������Ӧ�õ� UART7 ����
    USART_Init(UART7, &USART_InitStructure);

    // ���� UART7 �� DMA ��������ʵ�������Զ����˵��ڴ�
    USART_EnableDMA(UART7, USART_DMAREQ_RX, ENABLE);

    // ʹ�ܿ����߼���жϣ�IDLE Interrupt���������ж�һ֡���ݽ������
    USART_ConfigInt(UART7, USART_INT_IDLEF, ENABLE);

    // ���ʹ�� UART7 ģ�飬��ʼ����
    USART_Enable(UART7, ENABLE);
}

/**
 * @brief  UART7 DMA ����ͨ�����ú���
 * @note   ���� DMA ͨ�����ڴ� UART7 ���ݼĴ����Զ����˽��յ������ݵ�ָ��������
 *         ֧��ͨ�������ж� + DMA ʵ�֡����������ݽ��ա�
 * @param  buffer        - [in] �û��ṩ�����ݽ��ջ������׵�ַ
 * @param  BUFFER_SIZE   - [in] �������ܴ�С���ֽڣ�
 * @retval ��
 */
void UART7_DMA_Configuration(uint8_t *buffer, int BUFFER_SIZE)
{
    DMA_InitType DMA_InitStructure;

    // ��λ������ָ���� DMA ͨ����ȷ����ʼ״̬�ɾ�
    DMA_DeInit(UART7_RX_DMA_CH);

    // ʹ��Ĭ��ֵ��ʼ�� DMA ���ýṹ��
    DMA_StructInit(&DMA_InitStructure);

    // ���� DMA �������
    DMA_InitStructure.PeriphAddr = (uint32_t)&UART7->DAT;           // �����ַ��UART7 ���ݼĴ���
    DMA_InitStructure.MemAddr = (uint32_t)buffer;                   // �ڴ��ַ���û�������
    DMA_InitStructure.Direction = DMA_DIR_PERIPH_SRC;               // ���䷽������ΪԴ����ȡ���ݣ�
    DMA_InitStructure.BufSize = BUFFER_SIZE;                        // ���������������ֽڣ�
    DMA_InitStructure.PeriphInc = DMA_PERIPH_INC_DISABLE;           // �����ַ��������ʼ�ն� DAT �Ĵ�����
    DMA_InitStructure.MemoryInc = DMA_MEM_INC_ENABLE;               // �ڴ��ַ���������δ��뻺����
    DMA_InitStructure.PeriphDataSize = DMA_PERIPH_DATA_WIDTH_BYTE;  // �������ݿ�ȣ��ֽ�
    DMA_InitStructure.MemDataSize = DMA_MEM_DATA_WIDTH_BYTE;        // �ڴ����ݿ�ȣ��ֽ�
    DMA_InitStructure.CircularMode = DMA_MODE_NORMAL;               // ����ģʽ����ѭ�������ʺ�һ֡����
    DMA_InitStructure.Priority = DMA_PRIORITY_VERY_HIGH;            // ����Ϊ������ȼ������ⶪ����
    DMA_InitStructure.Mem2Mem = DMA_M2M_DISABLE;                    // �����ڴ浽�ڴ洫��

    // ��ʼ�� DMA ͨ����Ӧ����������
    DMA_Init(UART7_RX_DMA_CH, &DMA_InitStructure);

    // ����ӳ�� DMA ����ͨ����ȷ�� UART7_RX ��ȷ���ӵ�ָ�� DMA ͨ��
    DMA_RequestRemap(DMA_REMAP_UART7_RX, UART7_RX_DMA_CH, ENABLE);

    // ���� DMA ͨ������ʼ���� UART7 �����ݽ�������
    DMA_EnableChannel(UART7_RX_DMA_CH, ENABLE);
}

/**
 * @brief  UART7 NVIC �ж����ȼ����ú���
 * @note   ���� UART7 �жϵ���ռ���ȼ��������ȼ�
 *         ���ڴ�������жϣ�IDLE�����¼�
 * @param  ��
 * @retval ��
 */
void UART7_NVIC_Configuration(void)
{
    NVIC_InitType NVIC_InitStructure;

    // �����ж����ȼ�����Ϊ Group 2��2 λ��ռ���ȼ���2 λ�����ȼ�
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    // ���� UART7 �ж�ͨ��
    NVIC_InitStructure.NVIC_IRQChannel = UART7_IRQn;                  // ָ���ж�ԴΪ UART7
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;         // ��ռ���ȼ���Ϊ 1
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;                // �����ȼ���Ϊ 1
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;                   // ʹ�ܸ��ж�ͨ��

    // Ӧ�����õ� NVIC��Ƕ�������жϿ�������
    NVIC_Init(&NVIC_InitStructure);
}

// ���� AT ָ��ȴ���Ӧ
// AT�� ����ģ���Ƿ��Ѿ��ϵ�������ͨѶ�Ƿ�����
// AT+RST������ģ��
// AT+GMR���鿴�汾��Ϣ
// ATE��������ر�AT���Թ���
// AT+RESTORE���ָ���������
// AT+CWMODE=1��Station ģʽ����ͬ��AT+CWMODE=1,1 �Զ������ȵ㣩��������AT+CWMODE=1,0���ֹ�Զ������ȵ㣨���ӳɹ�֮������������
// AT+CWJAP=���ȵ�����,�����롱
// AT+CWQAP���Ͽ���AP������
// AT+CWAUTOCONN=1���ϵ��Զ�����AP��Ĭ�ϣ�0���ϵ粻�Զ�����AP

// AT+WSCFG=0,30,600,4096 ����WebSocket����
// AT+WSOPEN=0,"ws://5.181.225.4:23530//" ����
// AT+WSOPEN? ��ѯ����
// AT+WSSEND=0,n,2 //����n�ֽ�����
// AT+WSCLOSE=0 �ر�link_idΪ0������
bool UART7_SendATCommand(const char* cmd) 
{
  // ����ָ��
  for (uint8_t i = 0; i < strlen(cmd); i++) 
	{
		USART_SendData(UART7, cmd[i]);
		while (USART_GetFlagStatus(UART7, USART_FLAG_TXC) == RESET); // �ȴ��������
  }
	return true;
}

void USART7_SendBuffer(const uint8_t* pData, uint32_t Size)
{
    for (uint32_t i = 0; i < Size; i++)
    {
        USART_SendData(UART7, pData[i]);  // ���ֽڷ���
        while (USART_GetFlagStatus(UART7, USART_FLAG_TXC) == RESET); // �ȴ��������
    }
}
