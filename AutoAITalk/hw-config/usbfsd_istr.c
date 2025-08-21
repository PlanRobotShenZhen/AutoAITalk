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
*\*\file usbfsd_istr.c
*\*\author Nations
*\*\version v1.0.0
*\*\copyright Copyright (c) 2023, Nations Technologies Inc. All rights reserved.
**/

#include "usbfsd_lib.h"
#include "usbfsd_prop.h"
#include "usbfsd_pwr.h"
#include "usbfsd_istr.h"
#include "usbfsd_conf.h"

__IO uint16_t wIstr;            /* STS register last read value */
__IO uint8_t bIntPackSOF   = 0; /* SOFs received between 2 consecutive packets */
__IO uint32_t esof_counter = 0; /* expected SOF counter */
__IO uint32_t wCNTR        = 0;

/* function pointers to non-control endpoints service routines */
void (*pEpInt_IN[7])(void) = 
{
    EP1_IN_Callback,
    EP2_IN_Callback,
    EP3_IN_Callback,
    EP4_IN_Callback,
    EP5_IN_Callback,
    EP6_IN_Callback,
    EP7_IN_Callback,
};

void (*pEpInt_OUT[7])(void) = 
{
    EP1_OUT_Callback,
    EP2_OUT_Callback,
    EP3_OUT_Callback,
    EP4_OUT_Callback,
    EP5_OUT_Callback,
    EP6_OUT_Callback,
    EP7_OUT_Callback,
};

/**
*\*\name    USB_Istr.
*\*\fun     STS events interrupt service routine.
*\*\param   none
*\*\return  none
**/
void USB_Istr(void)
{
    __IO uint32_t EP[8];

    wIstr = _GetISTR();

#if (IMR_MSK & STS_CTRS)
    if (wIstr & STS_CTRS & wInterrupt_Mask)
    {
        /* servicing of the endpoint correct transfer interrupt */
        /* clear of the CTR flag into the sub */
        USB_CorrectTransferLp();
#ifdef CTR_CALLBACK
        CTR_Callback();
#endif
    }
#endif
    /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#if (IMR_MSK & STS_RST)
    if (wIstr & STS_RST & wInterrupt_Mask)
    {
        _SetISTR((uint16_t)CLR_RST);
        Device_Property.Init();
        Device_Property.Reset();
#ifdef RESET_CALLBACK
        RESET_Callback();
#endif
    }
#endif
    /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#if (IMR_MSK & STS_DOVR)
    if (wIstr & STS_DOVR & wInterrupt_Mask)
    {
        _SetISTR((uint16_t)CLR_DOVR);
#ifdef DOVR_CALLBACK
        DOVR_Callback();
#endif
    }
#endif
    /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#if (IMR_MSK & STS_ERROR)
    if (wIstr & STS_ERROR & wInterrupt_Mask)
    {
        _SetISTR((uint16_t)CLR_ERROR);
#ifdef ERR_CALLBACK
        ERR_Callback();
#endif
    }
#endif
    /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#if (IMR_MSK & STS_WKUP)
    if (wIstr & STS_WKUP & wInterrupt_Mask)
    {
        _SetISTR((uint16_t)CLR_WKUP);
        Resume(RESUME_EXTERNAL);
#ifdef WKUP_CALLBACK
        WKUP_Callback();
#endif
    }
#endif
    /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#if (IMR_MSK & STS_SUSPD)
    if (wIstr & STS_SUSPD & wInterrupt_Mask)
    {
        /* check if SUSPEND is possible */
        if (fSuspendEnabled)
        {
            Suspend();
        }
        else
        {
            /* if not possible then resume after xx ms */
            Resume(RESUME_LATER);
        }
        /* clear of the STS bit must be done after setting of CTRL_FSUSPD */
        _SetISTR((uint16_t)CLR_SUSPD);
#ifdef SUSP_CALLBACK
        SUSP_Callback();
#endif
    }
#endif
    /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#if (IMR_MSK & STS_SOF)
    if (wIstr & STS_SOF & wInterrupt_Mask)
    {
        _SetISTR((uint16_t)CLR_SOF);
        bIntPackSOF++;

#ifdef SOF_CALLBACK
        SOF_Callback();
#endif
    }
#endif
    /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#if (IMR_MSK & STS_ESOF)
    if (wIstr & STS_ESOF & wInterrupt_Mask)
    {
        uint32_t i;
        /* clear ESOF flag in STS */
        _SetISTR((uint16_t)CLR_ESOF);

        if ((_GetFNR() & FN_RXDP) != 0)
        {
            /* increment ESOF counter */
            esof_counter++;

            /* test if we enter in ESOF more than 3 times with FSUSP =0 and RXDP =1=>> possible missing SUSP flag*/
            if ((esof_counter > 3) && ((_GetCNTR() & CTRL_FSUSPD) == 0))
            {
                /* this a sequence to apply a force RESET*/

                /*Store CTRL value */
                wCNTR = _GetCNTR();

                /*Store endpoints registers status */
                for (i = 0; i < 8; i++)
                    EP[i] = _GetENDPOINT(i);

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

                esof_counter = 0;
            }
        }
        else
        {
            esof_counter = 0;
        }

        /* resume handling timing is made with ESOFs */
        Resume(RESUME_ESOF); /* request without change of the machine state */

#ifdef ESOF_CALLBACK
        ESOF_Callback();
#endif
    }
#endif
} /* USB_Istr */

