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
*\*\file hw_config.h
*\*\author Nations
*\*\version v1.0.0
*\*\copyright Copyright (c) 2023, Nations Technologies Inc. All rights reserved.
**/

#ifndef __HW_CONFIG_H__
#define __HW_CONFIG_H__

#include "n32h47x_48x.h"
#include "usbfsd_type.h"
#include "n32h47x_48x_gpio.h"

//#define USB_LOW_PWR_MGMT_SUPPORT

#define SYSCLK_VALUE_48MHz            ((uint32_t)48000000)
#define SYSCLK_VALUE_72MHz            ((uint32_t)72000000)
#define SYSCLK_VALUE_96MHz            ((uint32_t)96000000)
#define SYSCLK_VALUE_108MHz           ((uint32_t)108000000)
#define SYSCLK_VALUE_144MHz           ((uint32_t)144000000)
#define SYSCLK_VALUE_192MHz           ((uint32_t)192000000)
#define SYSCLK_VALUE_240MHz           ((uint32_t)240000000)

#define CDC_USARTx              USART4
#define CDC_PERIPH              RCC_APB2_PERIPH_USART4
#define CDC_GPIO                GPIOA
#define CDC_PERIPH_GPIO         RCC_AHB_PERIPHEN_GPIOA 
#define CDC_USART_IRQn          USART4_IRQn
#define CDC_USART_IRQHandler    USART4_IRQHandler
#define CDC_TX_PIN              GPIO_PIN_11
#define CDC_RX_PIN              GPIO_PIN_12
#define CDC_TX_AF               GPIO_AF_USART4_TX_PA11
#define CDC_RX_AF               GPIO_AF_USART4_RX_PA12

#define USART_RX_DATA_SIZE   512

void Set_System(void);
ErrorStatus Set_USBClock(uint32_t sysclk);
void Enter_LowPowerMode(void);
void Leave_LowPowerMode(void);
void USB_Interrupts_Config(void);
void TimingDelay_Decrement(void);
void USBFS_IO_Configure(void);
ErrorStatus USB_Config(uint32_t sysclk);
void USART_Config_Default(void);
bool USART_Config(void);
void USB_To_USART_Send_Data(uint8_t* data_buffer, uint8_t data_length);
void USART_To_USB_Send_Data(void);
void Handle_USBAsynchXfer (void);


#endif  /*__HW_CONFIG_H__ */

