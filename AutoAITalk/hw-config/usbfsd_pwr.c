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
*\*\file usbfsd_pwr.c
*\*\author Nations
*\*\version v1.0.0
*\*\copyright Copyright (c) 2023, Nations Technologies Inc. All rights reserved.
**/

#include "usbfsd_lib.h"
#include "usbfsd_conf.h"
#include "usbfsd_pwr.h"
#include "hw_config.h"
#include "n32h47x_48x_pwr.h"

__IO uint32_t bDeviceState = UNCONNECTED; /* USB device status */
__IO bool fSuspendEnabled  = true;        /* true when suspend is possible */
__IO uint32_t EP[8];

struct
{
    __IO RESUME_STATE eState;
    __IO uint8_t bESOFcnt;
} ResumeS;

__IO uint32_t remotewakeupon = 0;


/**
*\*\name    PowerOn.
*\*\fun     USB PowerOn.
*\*\param   none
*\*\return  USB_Result
*\*\            - Success 
*\*\            - UnSupport.
**/
USB_Result PowerOn(void)
{
    uint16_t wRegVal;

    /*** CNTR_PWDN = 0 ***/
    wRegVal = CTRL_FRST;
    _SetCNTR(wRegVal);

    /*** CTRL_FRST = 0 ***/
    wInterrupt_Mask = 0;
    _SetCNTR(wInterrupt_Mask);
    /*** Clear pending interrupts ***/
    _SetISTR(0);
    /*** Set interrupt mask ***/
    wInterrupt_Mask = CTRL_RSTM | CTRL_SUSPDM | CTRL_WKUPM;
    _SetCNTR(wInterrupt_Mask);

    return Success;
}

/**
*\*\name    PowerOff.
*\*\fun     USB Poweroff.
*\*\param   none
*\*\return  USB_Result
*\*\            - Success 
*\*\            - UnSupport.
**/
USB_Result PowerOff()
{
    /* disable all interrupts and force USB reset */
    _SetCNTR(CTRL_FRST);
    /* clear interrupt status register */
    _SetISTR(0);
    /* switch-off device */
    _SetCNTR(CTRL_FRST + CTRL_PD);
    /* sw variables reset */
    /* ... */

    return Success;
}

/**
*\*\name    Suspend.
*\*\fun     USB Suspend.
*\*\param   none
*\*\return  none
**/
void Suspend(void)
{
    uint32_t i = 0;
    uint16_t wCNTR;
        
    /* suspend preparation */
    /* ... */

#if (XTALLESS == 1)
    RCC->APB1PRST |= RCC_APB1PRST_UCDRRST;
    RCC->APB1PRST &= ~RCC_APB1PRST_UCDRRST;
#endif

    /*Store CTRL value */
    wCNTR = _GetCNTR();

    /* This a sequence to apply a force RESET to handle a robustness case */

    /*Store endpoints registers status */
    for (i = 0; i < 8; i++)
        EP[i] = _GetENDPOINT(i);

    /* unmask RESET flag */
    wCNTR |= CTRL_RSTM;
    _SetCNTR(wCNTR);

    /*apply FRES */
    wCNTR |= CTRL_FRST;
    _SetCNTR(wCNTR);

    /*clear FRES*/
    wCNTR &= ~CTRL_FRST;
    _SetCNTR(wCNTR);

    /* poll for RESET flag in STS, this bit will be set by hardware, if this flag is always not set, it means maybe USB module failure */
    while ((_GetISTR() & STS_RST) == 0);

    /* clear RESET flag in STS */
    _SetISTR((uint16_t)CLR_RST);

    /*restore Enpoints*/
    for (i = 0; i < 8; i++)
        _SetENDPOINT(i, EP[i]);

    /* Now it is safe to enter macrocell in suspend mode */
    wCNTR |= CTRL_FSUSPD;
    _SetCNTR(wCNTR);

    /* force low-power mode in the macrocell */
    wCNTR = _GetCNTR();
    wCNTR |= CTRL_LP_MODE;
    _SetCNTR(wCNTR);
    
    /* switch-off the clocks */
    Enter_LowPowerMode();

#ifdef USB_LOW_PWR_MGMT_SUPPORT
    /* Request to enter SLEEP mode*/
    PWR_EnterSLEEPMode(PWR_SLEEP_NOW, PWR_SLEEPENTRY_WFI);
#endif  /* USB_LOW_PWR_MGMT_SUPPORT */
}

/**
*\*\name    Resume_Init.
*\*\fun     Handles wake-up restoring normal operations.
*\*\param   none
*\*\return  none
**/
void Resume_Init(void)
{
    uint16_t wCNTR;
    
    /* CTRL_LP_MODE = 0 */
    wCNTR = _GetCNTR();
    wCNTR &= (~CTRL_LP_MODE);
    _SetCNTR(wCNTR);

#ifdef USB_LOW_PWR_MGMT_SUPPORT      
    /* restore full power */
    /* ... on connected devices */
    Leave_LowPowerMode();
#endif /* USB_LOW_PWR_MGMT_SUPPORT */

    /* reset FSUSP bit */
    _SetCNTR(IMR_MSK);
    /*Pull up DP*/
    _EnPortPullup();
    /* reverse suspend preparation */
    /* ... */
}

/**
*\*\name    Resume.
*\*\fun     USB Resume,This is the state machine handling resume operations and
*\*\        timing sequence. The control is based on the Resume structure
*\*\        variables and on the ESOF interrupt calling this subroutine
*\*\        without changing machine state.
*\*\param   eResumeSetVal
*\*\            - RESUME_EXTERNAL
*\*\            - RESUME_INTERNAL
*\*\            - RESUME_LATER
*\*\            - RESUME_WAIT
*\*\            - RESUME_START
*\*\            - RESUME_ON
*\*\            - RESUME_OFF
*\*\            - RESUME_ESOF
*\*\return  none
**/
void Resume(RESUME_STATE eResumeSetVal)
{
    uint16_t wCNTR;

    if (eResumeSetVal != RESUME_ESOF)
        ResumeS.eState = eResumeSetVal;
    switch (ResumeS.eState)
    {
    case RESUME_EXTERNAL:
        if (remotewakeupon == 0)
        {
            Resume_Init();
            ResumeS.eState = RESUME_OFF;
        }
        else /* RESUME detected during the RemoteWAkeup signalling => keep RemoteWakeup handling*/
        {
            ResumeS.eState = RESUME_ON;
        }
        break;
    case RESUME_INTERNAL:
        Resume_Init();
        ResumeS.eState = RESUME_START;
        remotewakeupon = 1;
        break;
    case RESUME_LATER:
        ResumeS.bESOFcnt = 2;
        ResumeS.eState   = RESUME_WAIT;
        break;
    case RESUME_WAIT:
        ResumeS.bESOFcnt--;
        if (ResumeS.bESOFcnt == 0)
            ResumeS.eState = RESUME_START;
        break;
    case RESUME_START:
        wCNTR = _GetCNTR();
        wCNTR |= CTRL_RESUM;
        _SetCNTR(wCNTR);
        ResumeS.eState   = RESUME_ON;
        ResumeS.bESOFcnt = 10;
        break;
    case RESUME_ON:
        ResumeS.bESOFcnt--;
        if (ResumeS.bESOFcnt == 0)
        {
            wCNTR = _GetCNTR();
            wCNTR &= (~CTRL_RESUM);
            _SetCNTR(wCNTR);
            ResumeS.eState = RESUME_OFF;
            remotewakeupon = 0;
        }
        break;
    case RESUME_OFF:
    case RESUME_ESOF:
    default:
        ResumeS.eState = RESUME_OFF;
        break;
    }
}
