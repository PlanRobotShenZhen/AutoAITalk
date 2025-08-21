/**
*     Copyright (c) 2023, Nations Technologies Inc.
* 
*     All rights reserved.
*
*     This software is the exclusive property of Nations Technologies Inc. (Hereinafter 
* referred to as NATIONS). This software, and the product of NATIONS described herein 
* (Hereinafter referred to as the Product) are owned by NATIONS under the laws and treaties
* of the People's Republic of China and other applicable jurisdictions worldwide.
*
*     NATIONS does not grant any license under its patents, copyrights, trademarks, or other 
* intellectual property rights. Names and brands of third party may be mentioned or referred 
* thereto (if any) for identification purposes only.
*
*     NATIONS reserves the right to make changes, corrections, enhancements, modifications, and 
* improvements to this software at any time without notice. Please contact NATIONS and obtain 
* the latest version of this software before placing orders.

*     Although NATIONS has attempted to provide accurate and reliable information, NATIONS assumes 
* no responsibility for the accuracy and reliability of this software.
* 
*     It is the responsibility of the user of this software to properly design, program, and test 
* the functionality and safety of any application made of this information and any resulting product. 
* In no event shall NATIONS be liable for any direct, indirect, incidental, special,exemplary, or 
* consequential damages arising in any way out of the use of this software or the Product.
*
*     NATIONS Products are neither intended nor warranted for usage in systems or equipment, any
* malfunction or failure of which may cause loss of human life, bodily injury or severe property 
* damage. Such applications are deemed, "Insecure Usage".
*
*     All Insecure Usage shall be made at user's risk. User shall indemnify NATIONS and hold NATIONS 
* harmless from and against all claims, costs, damages, and other liabilities, arising from or related 
* to any customer's Insecure Usage.

*     Any express or implied warranty with regard to this software or the Product, including,but not 
* limited to, the warranties of merchantability, fitness for a particular purpose and non-infringement
* are disclaimed to the fullest extent permitted by law.

*     Unless otherwise explicitly permitted by NATIONS, anyone may not duplicate, modify, transcribe
* or otherwise distribute this software for any purposes, in whole or in part.
*
*     NATIONS products and technologies shall not be used for or incorporated into any products or systems
* whose manufacture, use, or sale is prohibited under any applicable domestic or foreign laws or regulations. 
* User shall comply with any applicable export control laws and regulations promulgated and administered by 
* the governments of any countries asserting jurisdiction over the parties or transactions.
**/
 
/**
 *\*\file n32h47x_48x_it.c
 *\*\author Nations
 *\*\version v1.0.0
 *\*\copyright Copyright (c) 2023, Nations Technologies Inc. All rights reserved.
 **/

#include "n32h47x_48x_it.h"
#include "main.h"

extern uint32_t RX_BUFFER_SIZE;
extern uint8_t RxBuf[2][1512];
extern uint8_t ActiveBuf;
extern uint8_t WS_DATA_BUF[1024];
extern uint8_t WS_OPUS_Buffer[300];
extern uint8_t SAVE_OPUS_Buffer[300];
extern uint16_t RxCnt;
extern uint16_t WsBufCnt;

extern bool AT_INIT_FLAG;
extern bool AT_OK_FLAG;
extern bool AT_ERROR_FLAG;
extern bool AT_CWSTATE_FLAG;
extern bool AT_WSOPENE_FLAG;
extern bool AT_WSCONNECTEDE_FLAG;
extern bool AT_WSSEND_FLAG;

extern bool RECEIVING_OPUS_FLAG;

extern Queue audio_queue;
extern Queue message_queue;

uint8_t AT_RESPONSE_OK[] = "\r\nOK\r\n";
uint8_t AT_RESPONSE_ERROR[] = "\r\nERROR\r\n";

bool UART7_ERROR_FLAG = false;
bool USEFUL_OPUS_FLAG = false;

uint8_t WS_OPUS_CNT[4];

uint8_t dummy;

int n=0;
uint16_t idx=0;
uint16_t cnt_opus_cnt=0;
uint16_t WsOpusBufCnt=0;
uint16_t length_opus=0;
uint8_t LastActiveBuf;

extern uint16_t cnt_frame;

/***  Cortex-M4 Processor Exceptions Handlers ***/
/**
 *\*\name   NMI_Handler.
 *\*\fun    This function handles NMI exception.
 *\*\param  none
 *\*\return none
 */
void NMI_Handler(void)
{
}

/**
 *\*\name   NMI_Handler.
 *\*\fun    This function handles Hard Fault exception.
 *\*\param  none
 *\*\return none
 */
void HardFault_Handler(void)
{
    /* Go to infinite loop when Hard Fault exception occurs */
    while (1)
    {
    }
}

/**
 *\*\name   NMI_Handler.
 *\*\fun    This function handles Memory Manage exception.
 *\*\param  none
 *\*\return none
 */
void MemManage_Handler(void)
{
    /* Go to infinite loop when Memory Manage exception occurs */
    while (1)
    {
    }
}

/**
 *\*\name   NMI_Handler.
 *\*\fun    This function handles Bus Fault exception.
 *\*\param  none
 *\*\return none
 */
void BusFault_Handler(void)
{
    /* Go to infinite loop when Bus Fault exception occurs */
    while (1)
    {
    }
}

/**
 *\*\name   NMI_Handler.
 *\*\fun    This function handles Usage Fault exception.
 *\*\param  none
 *\*\return none
 */
void UsageFault_Handler(void)
{
    /* Go to infinite loop when Usage Fault exception occurs */
    while (1)
    {
    }
}

/**
 *\*\name   NMI_Handler.
 *\*\fun    This function handles SVCall exception.
 *\*\param  none
 *\*\return none
 */
void SVC_Handler(void)
{
}

/**
 *\*\name   NMI_Handler.
 *\*\fun    This function handles Debug Monitor exception.
 *\*\param  none
 *\*\return none
 */
void DebugMon_Handler(void)
{
}

/**
 *\*\name   NMI_Handler.
 *\*\fun    This function handles SysTick Handler.
 *\*\param  none
 *\*\return none
 */
void SysTick_Handler(void)
{
}

/**
*\*\name    USB_FS_LP_IRQHandler.
*\*\fun     This function handles USB_LP_IRQHandler Handler.
*\*\param   none
*\*\return  none 
**/
void USB_FS_LP_IRQHandler(void)
{
    USB_Istr();
}

/**
*\*\name    USB_FS_WKUP_IRQHandler.
*\*\fun     This function handles USB WakeUp interrupt request.
*\*\param   none
*\*\return  none 
**/
void USB_FS_WKUP_IRQHandler(void)
{
    EXTI_ClrITPendBit(EXTI_LINE18);
}

/**
*\*\name    CDC_USART_IRQHandler.
*\*\fun     This function handles EVAL_COM global interrupt request
*\*\param   none
*\*\return  none 
**/
void CDC_USART_IRQHandler(void)
{
    if (USART_GetFlagStatus(CDC_USARTx, USART_FLAG_RXDNE) != RESET)
    {
        /* Send the received data to the PC Host*/
        USART_To_USB_Send_Data();
    }

    /* If overrun condition occurs, clear the ORE flag and recover communication */
    if (USART_GetFlagStatus(CDC_USARTx, USART_FLAG_OREF) != RESET)
    {
        (void)USART_ReceiveData(CDC_USARTx);
    }
}
 

/**
 * @brief  UART7 �жϷ�������ISR��
 *         ���ڴ���ͨ�� UART7 ���յ������ݣ�֧�� AT ָ����Ӧ������ Opus ��Ƶ���ݽ��ա�
 *         ʹ�� DMA ˫������� + �����жϣ�IDLE interrupt��ʵ�ָ�Ч���ڽ��ա�
 */
void UART7_IRQHandler(void)
{
    // ����Ƿ񴥷��˿����߼���жϣ�IDLEF��
    if (USART_GetIntStatus(UART7, USART_INT_IDLEF) != RESET)
    {
        // ����1����ͣ DMA ͨ������ֹ�ڴ�������ʱ����д��
        DMA_EnableChannel(DMA2_CH6, DISABLE);

        // ��ȡ UART7 ���ݼĴ�������� IDLE ��־�����������
        USART_ReceiveData(UART7);

        // ���ݵ�ǰ�Ƿ��� AT ��ʼ�����̣�ִ�в�ͬ�߼�
        if (AT_INIT_FLAG)
        {
            // �� AT ��ʼ���׶Σ������ؼ���Ӧ�ַ���
            if (!AT_OK_FLAG) {
                AT_OK_FLAG = (strstr((char*)RxBuf[ActiveBuf], "OK") != NULL); // ��� "OK"
            }
            if (!AT_CWSTATE_FLAG) {
                AT_CWSTATE_FLAG = (strstr((char*)RxBuf[ActiveBuf], "STATE:2") != NULL); // �������״̬
            }
            if (!AT_WSOPENE_FLAG) {
                AT_WSOPENE_FLAG = (strstr((char*)RxBuf[ActiveBuf], "OPEN:") != NULL); // ��� WebSocket ��
            }
            if (!AT_WSCONNECTEDE_FLAG) {
                AT_WSCONNECTEDE_FLAG = (strstr((char*)RxBuf[ActiveBuf], "CONNECTED") != NULL); // ���������
            }

            // ��յ�ǰ��������׼����һ�ν���
            memset(RxBuf[ActiveBuf], 0, sizeof(RxBuf[ActiveBuf]));

            // ���� DMA ������Ϊ��������С��1512�ֽڣ������������� DMA
            DMA_SetCurrDataCounter(DMA2_CH6, 1512); 
            DMA_EnableChannel(DMA2_CH6, ENABLE); 
        }
        else
        {
            // �� AT ��ʼ���׶Σ����� WebSocket ���ݻ� Opus ��Ƶ��

            // �л����������˫�������
            LastActiveBuf = ActiveBuf;
            ActiveBuf = (ActiveBuf == 0) ? 1 : 0;

            // ���㱾�ν��յ������ݳ��ȣ��ܴ�С - ʣ�������
            int pcut = 1512 - DMA_GetCurrDataCounter(DMA2_CH6);

            // �������� DMA��ָ���µĻ�������RxBuf[ActiveBuf]��
            UART7_DMA_Configuration(RxBuf[ActiveBuf], RX_BUFFER_SIZE);
            DMA_SetCurrDataCounter(DMA2_CH6, 1512); 
            DMA_EnableChannel(DMA2_CH6, ENABLE); 

            // ������һ����������LastActiveBuf���е�����

            // ����Ƿ��յ� '>'����ʾ���Է�������
            if (!AT_WSSEND_FLAG) {
                AT_WSSEND_FLAG = (strstr((char*)RxBuf[LastActiveBuf], ">") != NULL);
            }

            // ����Ƿ��յ� "ERROR"�����ڴ�����
            if (!AT_ERROR_FLAG) {
                AT_ERROR_FLAG = (strstr((char*)RxBuf[LastActiveBuf], "ERROR") != NULL);
            }

            // ����Ƿ��յ� "tts" �ؼ��֣������л� Opus ����״̬
            if (strstr((char*)RxBuf[LastActiveBuf], "tts") != NULL) 
            {
                // �л����� Opus ��־
                RECEIVING_OPUS_FLAG = !RECEIVING_OPUS_FLAG;
                if (RECEIVING_OPUS_FLAG) {
                    cnt_frame = 0; // ��ʼ��һ֡��֡��������
                }
            }
            // ������ڽ��� Opus ���ݣ������ݸ�ʽΪ ",X"
            else if (RECEIVING_OPUS_FLAG && (strstr((char*)RxBuf[LastActiveBuf], ",X") != NULL))
            {
                cnt_opus_cnt = 0;     // ��ʱ���ڴ洢�����ַ����ļ�����
                WsOpusBufCnt = 0;     // Opus ���ݻ�����д��ָ��

                int i = 0;
                while (i < pcut)
                {
                    // ���� ",X" ģʽ��X ǰ���ǳ��ȣ�����������
                    if (i > 2 && RxBuf[LastActiveBuf][i] == 'X' && RxBuf[LastActiveBuf][i-1] == ',')
                    {
                        int idx = i - 2;
                        // ���˲���ǰһ�����ţ���ȡ�����ַ�����ʼλ��
                        while (idx >= 0 && RxBuf[LastActiveBuf][idx] != ',') {
                            idx--;
                        }
                        idx++; // ��������

                        // ��ȡ�����ַ������� "123"��
                        int k = idx;
                        while (k < i - 1) {
                            WS_OPUS_CNT[cnt_opus_cnt++] = RxBuf[LastActiveBuf][k];
                            k++;
                        }

                        // ת��Ϊ�������õ� Opus ���ݳ���
                        length_opus = atoi((const char*)WS_OPUS_CNT);

                        // ��ճ��Ȼ���
                        memset(WS_OPUS_CNT, '\0', 4);

                        // ��ȡ Opus ���ݣ��� 'X' ��ʼ���� length_opus �ֽڣ�
                        int m = i + 1; // 'X' ��һ���ֽڿ�ʼ
                        for (int n = 0; n < length_opus; n++) {
                            WS_OPUS_Buffer[WsOpusBufCnt++] = RxBuf[LastActiveBuf][m++];
                        }

                        // �� Opus ���ݷ�����Ƶ����
                        QueuePush(&audio_queue, WS_OPUS_Buffer, WsOpusBufCnt);

                        // ֡������һ
                        cnt_frame++;

                        // ������ʱ����
                        WsOpusBufCnt = 0;
                        cnt_opus_cnt = 0;

                        // �����Ѵ��������
                        i += length_opus + 1; // +1 ������ 'X'
                    }
                    i++;
                }
            }

            // ����Ѵ���Ļ�����
            memset(RxBuf[LastActiveBuf], 0, sizeof(RxBuf[LastActiveBuf]));
        }
    }

    // ��� UART7 �Ĵ����־�������������֡������ż����ȣ�
    if (USART_GetFlagStatus(UART7, USART_FLAG_OREF | USART_FLAG_NEF | 
                                       USART_FLAG_PEF  | USART_FLAG_FEF) != RESET)
    {
        (void)UART7->STS;  // ��״̬�Ĵ�����ȷ����������
        (void)UART7->DAT;  // �����ݼĴ�������������־
    }
}


