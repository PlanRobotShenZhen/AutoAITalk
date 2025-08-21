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
 * @brief  UART7 中断服务函数（ISR）
 *         用于处理通过 UART7 接收到的数据，支持 AT 指令响应解析和 Opus 音频数据接收。
 *         使用 DMA 双缓冲机制 + 空闲中断（IDLE interrupt）实现高效串口接收。
 */
void UART7_IRQHandler(void)
{
    // 检查是否触发了空闲线检测中断（IDLEF）
    if (USART_GetIntStatus(UART7, USART_INT_IDLEF) != RESET)
    {
        // 步骤1：暂停 DMA 通道，防止在处理数据时继续写入
        DMA_EnableChannel(DMA2_CH6, DISABLE);

        // 读取 UART7 数据寄存器，清除 IDLE 标志（必须操作）
        USART_ReceiveData(UART7);

        // 根据当前是否处于 AT 初始化流程，执行不同逻辑
        if (AT_INIT_FLAG)
        {
            // 在 AT 初始化阶段，解析关键响应字符串
            if (!AT_OK_FLAG) {
                AT_OK_FLAG = (strstr((char*)RxBuf[ActiveBuf], "OK") != NULL); // 检测 "OK"
            }
            if (!AT_CWSTATE_FLAG) {
                AT_CWSTATE_FLAG = (strstr((char*)RxBuf[ActiveBuf], "STATE:2") != NULL); // 检测连接状态
            }
            if (!AT_WSOPENE_FLAG) {
                AT_WSOPENE_FLAG = (strstr((char*)RxBuf[ActiveBuf], "OPEN:") != NULL); // 检测 WebSocket 打开
            }
            if (!AT_WSCONNECTEDE_FLAG) {
                AT_WSCONNECTEDE_FLAG = (strstr((char*)RxBuf[ActiveBuf], "CONNECTED") != NULL); // 检测已连接
            }

            // 清空当前缓冲区，准备下一次接收
            memset(RxBuf[ActiveBuf], 0, sizeof(RxBuf[ActiveBuf]));

            // 重置 DMA 计数器为缓冲区大小（1512字节），并重新启用 DMA
            DMA_SetCurrDataCounter(DMA2_CH6, 1512); 
            DMA_EnableChannel(DMA2_CH6, ENABLE); 
        }
        else
        {
            // 非 AT 初始化阶段：处理 WebSocket 数据或 Opus 音频流

            // 切换活动缓冲区：双缓冲机制
            LastActiveBuf = ActiveBuf;
            ActiveBuf = (ActiveBuf == 0) ? 1 : 0;

            // 计算本次接收到的数据长度（总大小 - 剩余计数）
            int pcut = 1512 - DMA_GetCurrDataCounter(DMA2_CH6);

            // 重新配置 DMA，指向新的缓冲区（RxBuf[ActiveBuf]）
            UART7_DMA_Configuration(RxBuf[ActiveBuf], RX_BUFFER_SIZE);
            DMA_SetCurrDataCounter(DMA2_CH6, 1512); 
            DMA_EnableChannel(DMA2_CH6, ENABLE); 

            // 解析上一个缓冲区（LastActiveBuf）中的内容

            // 检测是否收到 '>'，表示可以发送数据
            if (!AT_WSSEND_FLAG) {
                AT_WSSEND_FLAG = (strstr((char*)RxBuf[LastActiveBuf], ">") != NULL);
            }

            // 检测是否收到 "ERROR"，用于错误处理
            if (!AT_ERROR_FLAG) {
                AT_ERROR_FLAG = (strstr((char*)RxBuf[LastActiveBuf], "ERROR") != NULL);
            }

            // 检测是否收到 "tts" 关键字，用于切换 Opus 接收状态
            if (strstr((char*)RxBuf[LastActiveBuf], "tts") != NULL) 
            {
                // 切换接收 Opus 标志
                RECEIVING_OPUS_FLAG = !RECEIVING_OPUS_FLAG;
                if (RECEIVING_OPUS_FLAG) {
                    cnt_frame = 0; // 开始新一帧，帧计数清零
                }
            }
            // 如果正在接收 Opus 数据，且数据格式为 ",X"
            else if (RECEIVING_OPUS_FLAG && (strstr((char*)RxBuf[LastActiveBuf], ",X") != NULL))
            {
                cnt_opus_cnt = 0;     // 临时用于存储长度字符串的计数器
                WsOpusBufCnt = 0;     // Opus 数据缓冲区写入指针

                int i = 0;
                while (i < pcut)
                {
                    // 查找 ",X" 模式，X 前面是长度，后面是数据
                    if (i > 2 && RxBuf[LastActiveBuf][i] == 'X' && RxBuf[LastActiveBuf][i-1] == ',')
                    {
                        int idx = i - 2;
                        // 回退查找前一个逗号，获取长度字符串起始位置
                        while (idx >= 0 && RxBuf[LastActiveBuf][idx] != ',') {
                            idx--;
                        }
                        idx++; // 跳过逗号

                        // 提取长度字符串（如 "123"）
                        int k = idx;
                        while (k < i - 1) {
                            WS_OPUS_CNT[cnt_opus_cnt++] = RxBuf[LastActiveBuf][k];
                            k++;
                        }

                        // 转换为整数，得到 Opus 数据长度
                        length_opus = atoi((const char*)WS_OPUS_CNT);

                        // 清空长度缓存
                        memset(WS_OPUS_CNT, '\0', 4);

                        // 提取 Opus 数据（从 'X' 后开始，共 length_opus 字节）
                        int m = i + 1; // 'X' 后一个字节开始
                        for (int n = 0; n < length_opus; n++) {
                            WS_OPUS_Buffer[WsOpusBufCnt++] = RxBuf[LastActiveBuf][m++];
                        }

                        // 将 Opus 数据放入音频队列
                        QueuePush(&audio_queue, WS_OPUS_Buffer, WsOpusBufCnt);

                        // 帧计数加一
                        cnt_frame++;

                        // 重置临时变量
                        WsOpusBufCnt = 0;
                        cnt_opus_cnt = 0;

                        // 跳过已处理的数据
                        i += length_opus + 1; // +1 是跳过 'X'
                    }
                    i++;
                }
            }

            // 清空已处理的缓冲区
            memset(RxBuf[LastActiveBuf], 0, sizeof(RxBuf[LastActiveBuf]));
        }
    }

    // 清除 UART7 的错误标志（溢出、噪声、帧错误、奇偶错误等）
    if (USART_GetFlagStatus(UART7, USART_FLAG_OREF | USART_FLAG_NEF | 
                                       USART_FLAG_PEF  | USART_FLAG_FEF) != RESET)
    {
        (void)UART7->STS;  // 读状态寄存器，确定错误类型
        (void)UART7->DAT;  // 读数据寄存器，清除错误标志
    }
}


