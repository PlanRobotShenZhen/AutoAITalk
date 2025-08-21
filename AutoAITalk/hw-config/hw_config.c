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
*\*\file hw_config.c
*\*\author Nations
*\*\version v1.0.0
*\*\copyright Copyright (c) 2023, Nations Technologies Inc. All rights reserved.
**/

#include "hw_config.h"
#include "usbfsd_lib.h"
#include "usbfsd_desc.h"
#include "usbfsd_pwr.h"
#include "n32h47x_48x_rcc.h"
#include "n32h47x_48x_gpio.h"
#include "n32h47x_48x_exti.h"
#include "n32h47x_48x_usart.h"
#include "misc.h"
#include "usbfsd_prop.h"

ErrorStatus HSEStartUpStatus;
USART_InitType USART_InitStructure;
EXTI_InitType EXTI_InitStructure;
uint8_t  USART_Rx_Buffer [USART_RX_DATA_SIZE]; 
uint32_t USART_Rx_ptr_in = 0;
uint32_t USART_Rx_ptr_out = 0;
uint32_t USART_Rx_length  = 0;
uint8_t  USB_Tx_State = 0;

extern LINE_CODING linecoding;

/**
*\*\name    USBFS_IO_Configure.
*\*\fun     Configures usb fs device gpio.
*\*\param   none
*\*\return  none 
**/
void USBFS_IO_Configure(void)
{
    GPIO_InitType GPIO_InitStructure;  
    RCC_EnableAHB1PeriphClk(RCC_AHB_PERIPHEN_GPIOA, ENABLE);
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_AFIO, ENABLE); 
	
    GPIO_InitStruct(&GPIO_InitStructure);
  
    GPIO_InitStructure.Pin              = GPIO_PIN_11;
    GPIO_InitStructure.GPIO_Mode        = GPIO_MODE_AF_PP;
    GPIO_InitStructure.GPIO_Alternate   = GPIO_AF_USART4_TX_PA11;
    GPIO_InitPeripheral(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.Pin              = GPIO_PIN_12;
    GPIO_InitStructure.GPIO_Mode        = GPIO_MODE_AF_PP;
    GPIO_InitStructure.GPIO_Alternate   = GPIO_AF_USART4_RX_PA12;
    GPIO_InitPeripheral(GPIOA, &GPIO_InitStructure);
    
    /* configure the USART to the default settings */
    USART_Config_Default();
}
 
/**
*\*\name    Set_System.
*\*\fun     Configures Main system clocks & power.
*\*\param   none
*\*\return  none 
**/
void Set_System(void)
{
    RCC_DeInit();

    /*Enable HSE*/
    RCC->CTRL |= RCC_CTRL_HSEEN;
    while(RCC_WaitHseStable() == ERROR)
    {
        /* if HSE clock failed, User can add here some code to deal with this error */
    }

#if defined (N32H473) || defined (N32H474)
    /*Set PLL MUL 192MHz */
    RCC_ConfigPll(RCC_PLL_SRC_HSE, RCC_PLL_PRE_1, RCC_PLL_MUL_24, RCC_PLLOUT_DIV_1);
#elif defined (N32H482) || defined (N32H487)
    /*Set PLL MUL 240MHz */
    RCC_ConfigPll(RCC_PLL_SRC_HSE, RCC_PLL_PRE_1, RCC_PLL_MUL_30, RCC_PLLOUT_DIV_1);
#endif
    
    /*Enable PLL*/
    RCC->CTRL |= RCC_CTRL_PLLEN;
    while((RCC->CTRL & RCC_CTRL_PLLRDF) != RCC_CTRL_PLLRDF)
    {
        /* if PLL clock failed, User can add here some code to deal with this error */
    }

    /*Set AHB/APB1/APB2*/
    RCC->CFG |= RCC_CFG_AHBPRES_DIV1;
    RCC->CFG |= RCC_CFG_APB1PRES_DIV1;
    RCC->CFG |= RCC_CFG_APB2PRES_DIV1;

    FLASH->AC &= (uint32_t)((uint32_t)~FLASH_AC_LATENCY);
    FLASH->AC |= (uint32_t)(FLASH_AC_LATENCY_4);

    /* Select PLL as system clock source */
    RCC->CFG &= (uint32_t)((uint32_t) ~(RCC_CFG_SCLKSW));
    RCC->CFG |= (uint32_t)RCC_CFG_SCLKSW_PLL;
    while ((RCC->CFG & RCC_CFG_SCLKSTS) != RCC_CFG_SCLKSTS_PLL) 
    {
        /* if system clock select failed , User can add here some code to deal with this error */
    }
}

/**
*\*\name    Set_USBClock.
*\*\fun     Configures USB Clock input (48MHz).
*\*\param   sysclk: system clock.
*\*\          - SYSCLK_VALUE_48MHz
*\*\          - SYSCLK_VALUE_72MHz
*\*\          - SYSCLK_VALUE_96MHz
*\*\          - SYSCLK_VALUE_144MHz
*\*\          - SYSCLK_VALUE_192MHz
*\*\          - SYSCLK_VALUE_240MHz
*\*\return  ErrorStatus 
*\*\          - SUCCESS
*\*\          - ERROR
**/
ErrorStatus Set_USBClock(uint32_t sysclk)
{
    ErrorStatus status = SUCCESS;

    switch(sysclk)
    {
    case SYSCLK_VALUE_48MHz: 
        RCC_ConfigUSBPLLPresClk(RCC_USBPLLCLK_SRC_PLL, RCC_USBPLLCLK_DIV1);
        RCC->CFG3 &= ~RCC_CFG3_USBFSTM;
        break;

    case SYSCLK_VALUE_96MHz: 
        RCC_ConfigUSBPLLPresClk(RCC_USBPLLCLK_SRC_PLL, RCC_USBPLLCLK_DIV2);
        RCC->CFG3 &= ~RCC_CFG3_USBFSTM;
        break;
    
    case SYSCLK_VALUE_144MHz: 
        RCC_ConfigUSBPLLPresClk(RCC_USBPLLCLK_SRC_PLL, RCC_USBPLLCLK_DIV3);
        RCC->CFG3 |= RCC_CFG3_USBFSTM;
        break;

    case SYSCLK_VALUE_192MHz: 
        RCC_ConfigUSBPLLPresClk(RCC_USBPLLCLK_SRC_PLL, RCC_USBPLLCLK_DIV4);
        RCC->CFG3 |= RCC_CFG3_USBFSTM;
        break;
    
    case SYSCLK_VALUE_240MHz: 
        RCC_ConfigUSBPLLPresClk(RCC_USBPLLCLK_SRC_PLL, RCC_USBPLLCLK_DIV5);
        RCC->CFG3 |= RCC_CFG3_USBFSTM;
        break;

    default:
        status = ERROR;
        break;
    }

    return status;
}

/**
*\*\name    Enter_LowPowerMode.
*\*\fun     Power-off system clocks and power while entering suspend mode.
*\*\param   none
*\*\return  none 
**/
void Enter_LowPowerMode(void)
{
    /* Set the device state to suspend */
    bDeviceState = SUSPENDED;
}

/**
*\*\name    Leave_LowPowerMode.
*\*\fun     Restores system clocks and power while exiting suspend mode.
*\*\param   none
*\*\return  none 
**/
void Leave_LowPowerMode(void)
{
    USB_DeviceMess* pInfo = &Device_Info;

    /* Set the device state to the correct state */
    if (pInfo->CurrentConfiguration != 0)
    {
        /* Device configured */
        bDeviceState = CONFIGURED;
    }
    else
    {
        bDeviceState = ATTACHED;
    }
}

/**
*\*\name    USB_Interrupts_Config.
*\*\fun     Configures the USB interrupts.
*\*\param   none
*\*\return  none 
**/
void USB_Interrupts_Config(void)
{
    NVIC_InitType NVIC_InitStructure;
    EXTI_InitType EXTI_InitStructure;

    /* 2 bit for pre-emption priority, 2 bits for subpriority */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    /* Enable the USB interrupt */
    NVIC_InitStructure.NVIC_IRQChannel                   = USB_FS_LP_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* Enable the USB Wake-up interrupt */
    NVIC_InitStructure.NVIC_IRQChannel                   = USB_FS_WKUP_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* Configure the EXTI line 18 connected internally to the USB IP */
    EXTI_ClrITPendBit(EXTI_LINE18);
    EXTI_InitStructure.EXTI_Line = EXTI_LINE18; 
    EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_InitPeripheral(&EXTI_InitStructure);
}

/**
*\*\name    USB_Config.
*\*\fun     Configures USB Clock input (48MHz).
*\*\param   sysclk: system clock.
*\*\          - SYSCLK_VALUE_48MHz
*\*\          - SYSCLK_VALUE_72MHz
*\*\          - SYSCLK_VALUE_96MHz
*\*\          - SYSCLK_VALUE_144MHz
*\*\          - SYSCLK_VALUE_192MHz
*\*\          - SYSCLK_VALUE_240MHz
*\*\return  ErrorStatus 
*\*\          - SUCCESS
*\*\          - ERROR
**/
ErrorStatus USB_Config(uint32_t sysclk)
{
    ErrorStatus status = SUCCESS;
    
    RCC_ConfigUSBFSClk(RCC_USBFS_CLKSRC_PLLPRES);

    if(Set_USBClock(sysclk) == SUCCESS)
    {
        RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_USBFS, ENABLE);
        status = SUCCESS;
    }
    else
    {
        status = ERROR;
    }
    return status;
}

/**
*\*\name    USART_COM_Init.
*\*\fun     Configures USART COM port
*\*\param   USART_InitStruct: pointer to a USART_InitType structure that
*\*\        contains the configuration information for the specified USART peripheral.
*\*\return  none
**/
void USART_COM_Init(USART_InitType* USART_InitStruct)
{
    GPIO_InitType GPIO_InitStructure;
    NVIC_InitType NVIC_InitStructure;
    
    /* Enable CDC USART GPIO clock */
    RCC_EnableAHB1PeriphClk(CDC_PERIPH_GPIO, ENABLE);
    
    /* Enable CDC USART Clock */
    RCC_EnableAPB2PeriphClk(CDC_PERIPH | RCC_APB2_PERIPH_AFIO, ENABLE);
    
    /* 2 bit for pre-emption priority, 2 bits for subpriority */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    GPIO_InitStruct(&GPIO_InitStructure);
    /* Configure CDC USART Tx as alternate function push-pull */
    GPIO_InitStructure.Pin            = CDC_TX_PIN;
    GPIO_InitStructure.GPIO_Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStructure.GPIO_Alternate = CDC_TX_AF;
    GPIO_InitPeripheral(CDC_GPIO, &GPIO_InitStructure);
    
    GPIO_InitStructure.Pin            = CDC_RX_PIN;
    GPIO_InitStructure.GPIO_Alternate = CDC_RX_AF;
    GPIO_InitPeripheral(CDC_GPIO, &GPIO_InitStructure);

    /* Enable CDC USART Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel            = CDC_USART_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd         = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* CDC USART configuration */
    USART_Init(CDC_USARTx, USART_InitStruct);
    /* Enable the CDC USART Receive interrupt */
    USART_ConfigInt(CDC_USARTx, USART_INT_RXDNE, ENABLE);
    /* Enable CDC USART */
    USART_Enable(CDC_USARTx, ENABLE);
}

/**
*\*\name    USART_Config_Default.
*\*\fun     configure the EVAL_COM1 with default values.
*\*\param   none
*\*\return  none
**/
void USART_Config_Default(void)
{
    /* USART default configuration */
    USART_InitStructure.BaudRate            = 115200;
    USART_InitStructure.WordLength          = USART_WL_8B;
    USART_InitStructure.StopBits            = USART_STPB_1;
    USART_InitStructure.Parity              = USART_PE_NO;
    USART_InitStructure.OverSampling        = USART_16OVER;
    USART_InitStructure.HardwareFlowControl = USART_HFCTRL_NONE;
    USART_InitStructure.Mode                = USART_MODE_RX | USART_MODE_TX;

    /* Configure and enable the USART */
    USART_COM_Init(&USART_InitStructure);
}

/**
*\*\name    USART_Config.
*\*\fun     Configure the EVAL_COM1 according to the line coding structure.
*\*\param   none
*\*\return  Configuration status
*\*\            TRUE : configuration done with success
*\*\            FALSE : configuration aborted.
**/
bool USART_Config(void)
{
    /* set the Stop bit*/
    switch (linecoding.format)
    {
        case 0:
            USART_InitStructure.StopBits = USART_STPB_1;
            break;
        case 1:
            USART_InitStructure.StopBits = USART_STPB_1_5;
            break;
        case 2:
            USART_InitStructure.StopBits = USART_STPB_2;
            break;
        default :
            USART_Config_Default();
            return (false);
    }

    /* set the parity bit*/
    switch (linecoding.paritytype)
    {
        case 0:
          USART_InitStructure.Parity = USART_PE_NO;
          break;
        case 1:
          USART_InitStructure.Parity = USART_PE_EVEN;
          break;
        case 2:
          USART_InitStructure.Parity = USART_PE_ODD;
          break;
        default :
        {
          USART_Config_Default();
          return (false);
        }
    }

    /*set the data type : only 8bits and 9bits is supported */
    switch (linecoding.datatype)
    {
        case 0x07:
          /* With this configuration a parity (Even or Odd) should be set */
          USART_InitStructure.WordLength = USART_WL_8B;
            break;
        case 0x08:
          if (USART_InitStructure.Parity == USART_PE_NO)
          {
                USART_InitStructure.WordLength = USART_WL_8B;
          }
          else 
          {
                USART_InitStructure.WordLength = USART_WL_9B;
          }
            break;
        default :
            USART_Config_Default();
            return (false);
    }

    USART_InitStructure.BaudRate = linecoding.bitrate;
    USART_InitStructure.OverSampling        = USART_16OVER;
    USART_InitStructure.HardwareFlowControl = USART_HFCTRL_NONE;
    USART_InitStructure.Mode = USART_MODE_RX | USART_MODE_TX;

    /* Configure and enable the USART */
    USART_COM_Init(&USART_InitStructure);

    return (true);
}

/**
*\*\name    USB_To_USART_Send_Data.
*\*\fun     send the received data from USB to the USART.
*\*\param   data_buffer: data address.
*\*\param   data_length: the length of bytes to send.
*\*\return  none
**/
void USB_To_USART_Send_Data(uint8_t* data_buffer, uint8_t data_length)
{
    uint32_t i;

    for (i = 0; i < data_length; i++)
    {
        USART_SendData(CDC_USARTx, *(data_buffer + i));
        while(USART_GetFlagStatus(CDC_USARTx, USART_FLAG_TXDE) == RESET); 
    }
}

/**
*\*\name    Handle_USBAsynchXfer.
*\*\fun     send data to USB.
*\*\param   none
*\*\return  none
**/
void Handle_USBAsynchXfer (void)
{
    uint16_t USB_Tx_ptr;
    uint16_t USB_Tx_length;

    if(USB_Tx_State != 1)
    {
        if (USART_Rx_ptr_out == USART_RX_DATA_SIZE)
        {
            USART_Rx_ptr_out = 0;
        }

        if(USART_Rx_ptr_out == USART_Rx_ptr_in) 
        {
            USB_Tx_State = 0; 
            return;
        }

        if(USART_Rx_ptr_out > USART_Rx_ptr_in) /* rollback */
        { 
            USART_Rx_length = USART_RX_DATA_SIZE - USART_Rx_ptr_out;
        }
        else 
        {
            USART_Rx_length = USART_Rx_ptr_in - USART_Rx_ptr_out;
        }
    
        if (USART_Rx_length > VIRTUAL_COM_PORT_DATA_SIZE)
        {
            USB_Tx_ptr = USART_Rx_ptr_out;
            USB_Tx_length = VIRTUAL_COM_PORT_DATA_SIZE;

            USART_Rx_ptr_out += VIRTUAL_COM_PORT_DATA_SIZE;   
            USART_Rx_length -= VIRTUAL_COM_PORT_DATA_SIZE;    
        }
        else
        {
            USB_Tx_ptr = USART_Rx_ptr_out;
            USB_Tx_length = USART_Rx_length;

            USART_Rx_ptr_out += USART_Rx_length;
            USART_Rx_length = 0;
        }
        USB_Tx_State = 1; 
        USB_CopyUserToPMABuf(&USART_Rx_Buffer[USB_Tx_ptr], ENDP1_TXADDR, USB_Tx_length);
        USB_SetEpTxCnt(ENDP1, USB_Tx_length);
        USB_SetEpTxValid(ENDP1); 
    }
}

/**
*\*\name    USART_To_USB_Send_Data.
*\*\fun     send the received data from USART to USB.
*\*\param   none
*\*\return  none
**/
void USART_To_USB_Send_Data(void)
{
    if (linecoding.datatype == 7)
    {
        USART_Rx_Buffer[USART_Rx_ptr_in] = USART_ReceiveData(CDC_USARTx) & 0x7F;
    }
    else if (linecoding.datatype == 8)
    {
        USART_Rx_Buffer[USART_Rx_ptr_in] = USART_ReceiveData(CDC_USARTx);
    }

    USART_Rx_ptr_in++;

    /* To avoid buffer overflow */
    if(USART_Rx_ptr_in == USART_RX_DATA_SIZE)
    {
        USART_Rx_ptr_in = 0;
    }
}
