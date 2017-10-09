/*
 * Upp_ioctl.c
 *
 * This file contains Driver Layer Interface implementation for the UPP Driver.
 * UPP Driver provides Driver Layer Interface to do operations on the UPP
 * peripheral like device initialization, channel creation, control commands for
 * peripheral specific operations etc
 *
 * Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * \file      Upp_ioctl.c
 *
 * \brief     Generic Upp Driver for DSP/BIOS Platform.It uses the internal DMA
 *            for data transfer.
 *
 *            This file implements the UPP IOM driver control commands to be
 *            used with DSP-BIOS 5 operating system.
 *
 *            (C) Copyright 2009, Texas Instruments, Inc
 *
 * \author    Platform Support Group
 *
 * \version   0.2
 *            \author Imtiaz SMA
 *            \date   Dec 14 2009
 *
 *            1. Changes for PSP coding guidelines.
 *            2. New dev and chan param structure.
 *            3. Concept of multiple independent channels introduced.
 *
 * \version   0.1
 *            \author Joe Coombs
 *            \date   Aug 05 2009
 *
 *            Initial version
 */

/*============================================================================*/
/*                             INCLUDE FILES                                  */
/*============================================================================*/
#include <xdc/std.h>
#include <string.h>
#include <assert.h>
#include "UppLocal.h"

/* ========================================================================== */
/*                           FUNCTION DEFINTIONS                              */
/* ========================================================================== */

/**
 * \brief   Implements the IOCTLS for uPP IOM mini driver.
 *
 *          This function implements the uPP device specific control operations.
 *          please refer to "Upp_Ioctl" for a comprehensive list of commands.
 *
 * \param   chanHandle  [IN]  Handle to channel
 * \param   cmd         [IN]  Control command number
 * \param   arg         [IN]  Arguments for control command
 * \param   param       [IN]  user defined data structure
 *
 * \return  IOM_COMPLETED in case of sucess.
 *          IOM_ENOTIMPL  in case the command is not supported.
 *          IOM error code in case of any error.
 */
Int32 Upp_localSubmitIoctl(Upp_ChanObj  *chanHandle,
                           Upp_Ioctl     cmd,
                           Ptr           arg,
                           Ptr           param)
{
    Upp_Object     *instHandle = NULL;
    Uint32          hwiKey     = 0x00;
    Uint32          clkDiv     = 0x00;
    Upp_ChanParams *chanParams = NULL;
    Power_Domain     domain     = Power_CPU;
    Uint32          setpoint   = 0x00;
    Int32           retVal     = IOM_COMPLETED;
    Int32           status     = IOM_COMPLETED;

    /* To remove the compiler warning of unused variable.Kept for future use  */
    if (NULL != param)
    {
        param = param;
    }

    assert((NULL != chanHandle) && (NULL != chanHandle->devHandle));
    instHandle = (Upp_Object *)chanHandle->devHandle;
    assert(NULL != instHandle);

    if (Upp_Ioctl_START == cmd)
    {
        hwiKey = (Uint32)_disable_interrupts();

        /* enable the state machine for the uPP device                        */
        instHandle->uppSmStateStop = FALSE;

        _restore_interrupts(hwiKey);

        /* enable the device to make it ready for transactions                */
        instHandle->hwInfo.regs->UPPCR |= (CSL_UPP_UPPCR_EN_MASK);
    }
    else if (Upp_Ioctl_STOP == cmd)
    {
        hwiKey = (Uint32)_disable_interrupts();

        /* set the state of the state machine to stopped                      */
        instHandle->uppSmStateStop = TRUE;

        /* set the state of both the channels to aborted so that no new pkts  *
         * are loaded                                                         */
        if ((Upp_DriverState_OPENED == instHandle->chanA.chanState) &&
            (0 != instHandle->chanA.submitCount))
        {
            instHandle->chanA.flushAbort = TRUE;
        }

        if ((Upp_DriverState_OPENED == instHandle->chanB.chanState) &&
            (0 != instHandle->chanB.submitCount))
        {
            instHandle->chanB.flushAbort = TRUE;
        }

        if ((FALSE == instHandle->chanA.flushAbort) &&
            (FALSE == instHandle->chanB.flushAbort))
        {
            /* disable the UPP device                                         */
            instHandle->hwInfo.regs->UPPCR &= (~CSL_UPP_UPPCR_EN_MASK);
        }

        _restore_interrupts(hwiKey);
    }
    else if (IOM_DEVICE_RESET == cmd)
    {
        hwiKey = (Uint32)_disable_interrupts();

        /* disable the UPP device                                             */
        instHandle->hwInfo.regs->UPPCR &= (~CSL_UPP_UPPCR_EN_MASK);

        /* clean up the IO packets from the channels                          */
        if (Upp_DriverState_CLOSED != instHandle->chanA.chanState)
        {
            Upp_localAbortChan(&instHandle->chanA);
            Upp_localConfigureChanA(&instHandle->chanA);

            if ((Upp_ChanMode_DEMULTIPLEX_ENABLE == chanHandle->chanParams.chanMode) ||
                (Upp_ChanMode_TX_INTERLEAVE_ENABLE == chanHandle->chanParams.chanMode))
            {
                /* configure the threshold vlaues for the DMA channel Q       */
                if (IOM_INPUT == chanHandle->mode)
                {
                    instHandle->hwInfo.regs->UPTCR &=
                       (~CSL_UPP_UPTCR_RDSIZEQ_MASK);
                    instHandle->hwInfo.regs->UPTCR |=
                       (chanHandle->chanParams.fifoThresHold
                           << CSL_UPP_UPTCR_RDSIZEQ_SHIFT);
                }
                else
                {
                    instHandle->hwInfo.regs->UPTCR &=
                       (~CSL_UPP_UPTCR_TXSIZEB_MASK);
                    instHandle->hwInfo.regs->UPTCR |=
                       (chanHandle->chanParams.fifoThresHold
                           << CSL_UPP_UPTCR_TXSIZEB_SHIFT);
                }
            }
        }

        if (Upp_DriverState_CLOSED != instHandle->chanB.chanState)
        {
            Upp_localAbortChan(&instHandle->chanB);
            Upp_localConfigureChanB(&instHandle->chanB);
        }

        /* configure the clock for this channel instance (only if output      *
         * channel)                                                           */
        if (IOM_OUTPUT == chanHandle->mode)
        {
#ifdef BIOS_PWRM_ENABLE
            if ((TRUE == instHandle->devParams.pscPwrmEnable) &&
                (Upp_pllDomain_NONE != instHandle->pwrmInfo.pllDomain))
            {
                /* check what clock domain is supported by the device     */
                if (Upp_pllDomain_0 == instHandle->pwrmInfo.pllDomain)
                {
                    domain = Power_CPU;
                }
                else
                {
                    domain = Power_PER;
                }

                retVal = (Int32)PWRM_getCurrentSetpoint(domain,&setpoint);

                if (Power_SOK == retVal)
                {
                    status = Upp_localCalculateClockSettings(
                                instHandle,
                                setpoint,
                                chanHandle->chanParams.opFreq,
                                &clkDiv);

                    if (IOM_COMPLETED == status)
                    {
                        /* check if the prescaler value is in the limits      */
                        if (clkDiv > 0x0F)
                        {
                            status = IOM_EBADARGS;
                        }
                    }
                }
            }
            else
            {
#endif
                /* No power management.calculate the clocks directly          */
                clkDiv = (instHandle->devParams.inputFreq /
                            (2u * (chanHandle->chanParams.opFreq))) - 1u;

                if (clkDiv > 0x0F)
                {
                    status = IOM_EBADARGS;
                }
#ifdef BIOS_PWRM_ENABLE
            }
#endif
        }

        if (IOM_COMPLETED == status)
        {
            if (Upp_ChanSel_A == chanHandle->chanParams.chanSel)
            {
                instHandle->upicrRegVal |=
                    (clkDiv << CSL_UPP_UPICR_CLKDIVA_SHIFT);
            }
            else
            {
                instHandle->upicrRegVal |=
                    (clkDiv << CSL_UPP_UPICR_CLKDIVB_SHIFT);
            }

            /* enable the device                                              */
            if ((Upp_DevMode_CHAN_A_RCV == instHandle->devParams.devMode) ||
                (Upp_DevMode_CHAN_A_XMT == instHandle->devParams.devMode))
            {
                /* update the value of the UPICR register                     */
                instHandle->hwInfo.regs->UPICR = instHandle->upicrRegVal;

                /* single channel mode.Hence enable the peripheral here       */
                instHandle->hwInfo.regs->UPPCR |= (CSL_UPP_UPPCR_EN_MASK);
            }
            else
            {
                /* 2 channels are configured Hence we need to check if both   *
                 * the channels are configured                                */
                if ((Upp_DriverState_OPENED == instHandle->chanA.chanState) &&
                    (Upp_DriverState_OPENED == instHandle->chanB.chanState))
                {
                    /* update the value of the UPICR register                 */
                    instHandle->hwInfo.regs->UPICR = instHandle->upicrRegVal;

                    /* enable the device to make it ready for transactions    */
                    instHandle->hwInfo.regs->UPPCR |= (CSL_UPP_UPPCR_EN_MASK);
                }
            }
        }
        _restore_interrupts(hwiKey);
    }
    else if ((IOM_CHAN_RESET == cmd) ||
              (IOM_CHAN_TIMEDOUT == cmd))
    {
        hwiKey = (Uint32)_disable_interrupts();

        Upp_localDisableInterrupts(instHandle,chanHandle);

        chanHandle->flushAbort = TRUE;

        /* disable the UPP device                                             */
        instHandle->hwInfo.regs->UPPCR &= (~CSL_UPP_UPPCR_EN_MASK);

        /* clean up the IO packets from the channels                          */
        if (Upp_DriverState_CLOSED != chanHandle->chanState)
        {
            Upp_localAbortChan(chanHandle);

            /* configure the channel                                          */
            if (Upp_ChanSel_A == chanHandle->chanParams.chanSel)
            {
                Upp_localConfigureChanA(chanHandle);

                if ((Upp_ChanMode_DEMULTIPLEX_ENABLE == chanHandle->chanParams.chanMode) ||
                    (Upp_ChanMode_TX_INTERLEAVE_ENABLE == chanHandle->chanParams.chanMode))
                {
                    /* configure the threshold vlaues for the DMA channel Q   */
                    if (IOM_INPUT == chanHandle->mode)
                    {
                        instHandle->hwInfo.regs->UPTCR &=
                           (~CSL_UPP_UPTCR_RDSIZEQ_MASK);
                        instHandle->hwInfo.regs->UPTCR |=
                           (chanHandle->chanParams.fifoThresHold
                               << CSL_UPP_UPTCR_RDSIZEQ_SHIFT);
                    }
                    else
                    {
                        instHandle->hwInfo.regs->UPTCR &=
                           (~CSL_UPP_UPTCR_TXSIZEB_MASK);
                        instHandle->hwInfo.regs->UPTCR |=
                           (chanHandle->chanParams.fifoThresHold
                               << CSL_UPP_UPTCR_TXSIZEB_SHIFT);
                    }
                }
            }
            else
            {
                Upp_localConfigureChanB(chanHandle);
            }

            /* update the value of the UPICR register                         */
            instHandle->hwInfo.regs->UPICR = instHandle->upicrRegVal;

            /* enable the device to make it ready for transactions            */
            instHandle->hwInfo.regs->UPPCR |= (CSL_UPP_UPPCR_EN_MASK);
        }

        chanHandle->flushAbort = FALSE;
        _restore_interrupts(hwiKey);
    }
#ifdef Upp_STATISTICS_ENABLED
    /*===================IOCTL TO QUERY CHANNEL STATISTICS====================*/
    else if (Upp_Ioctl_QUERY_STATS == cmd)
    {
        assert(NULL != arg);

        /* enter the critical section                                         */
        hwiKey = _disable_interrupts();

        /* copy the device statistics for this channel                        */
        memcpy((Upp_devStats *)arg,&chanHandle->stats,sizeof(Upp_devStats));

        /* exit the critical section                                          */
        _restore_interrupts();
    }
    /*===================IOCTL TO CLEAR CHANNEL STATISTICS====================*/
    else if (Upp_Ioctl_CLEAR_STATS == cmd)
    {
        /* enter the critical section                                         */
        hwiKey = _disable_interrupts();

        /* clear the device statistics for this channel                       */
        memset(&chanHandle->stats,0x00,sizeof(Upp_devStats));

        /* exit the critical section                                          */
        _restore_interrupts();
    }
#endif /* Upp_STATISTICS_ENABLED */
    /*====================IOCTL TO MODIFY TIMEOUT COUNT=======================*/
    else if (Upp_Ioctl_SET_TIMEOUT == cmd)
    {
        assert(NULL != arg);

        /* modify the retry count value to be used                            */
        instHandle->retryCount = *(Uint32 *)arg;
    }
    /*====================IOCTL TO SUSPEND THE CHANNEL========================*/
    else if (Upp_Ioctl_SUSPEND == cmd)
    {
        instHandle->uppSuspend = TRUE;

        /* similar to the pwrm implementation                                 */
        Upp_localSuspendCurrentIops(instHandle,Power_GOINGTOSTANDBY);
    }
    /*====================IOCTL TO RESUME THE CHANNEL=========================*/
    else if (Upp_Ioctl_RESUME == cmd)
    {
        /* enable the peripheral now                                          */
        instHandle->hwInfo.regs->UPPCR |= (CSL_UPP_UPPCR_EN_MASK);

        /* implementing the software resume similar to the power mode standby */
        if (Upp_DriverState_OPENED == instHandle->chanA.chanState)
        {
            Upp_localLoadPendedIops(&instHandle->chanA);
        }

        if (Upp_DriverState_OPENED == instHandle->chanB.chanState)
        {
            Upp_localLoadPendedIops(&instHandle->chanB);
        }

        instHandle->uppSuspend = FALSE;
        instHandle->devState = Upp_DriverState_CREATED;
    }
    /*=================IOCTL TO CHANGE CHANNEL CONFIGURATION==================*/
    else if (Upp_Ioctl_CHAN_CONFIG == cmd)
    {
        assert(NULL != arg);

        chanParams = (Upp_ChanParams *)arg;

        if ((chanHandle->chanParams.chanSel != chanParams->chanSel) ||
            (Upp_DriverState_CLOSED == chanHandle->chanState))
        {
            retVal = IOM_EBADARGS;
        }
        else
        {
            hwiKey = (Uint32)_disable_interrupts();

            /* disable the UPP device                                         */
            instHandle->hwInfo.regs->UPPCR &= (~CSL_UPP_UPPCR_EN_MASK);

#ifndef PSP_DISABLE_INPUT_PARAMETER_CHECK
            Upp_localValidateChanParams(instHandle,chanParams);
#endif
            /* copy the new chanparams                                        */
            chanHandle->chanParams = *chanParams;

            if (Upp_ChanSel_A == chanHandle->chanParams.chanSel)
            {
                Upp_localConfigureChanA(chanHandle);

                if ((Upp_ChanMode_DEMULTIPLEX_ENABLE == chanHandle->chanParams.chanMode) ||
                    (Upp_ChanMode_TX_INTERLEAVE_ENABLE == chanHandle->chanParams.chanMode))
                {
                    /* configure the threshold vlaues for the DMA channel Q   */
                    if (IOM_INPUT == chanHandle->mode)
                    {
                        instHandle->hwInfo.regs->UPTCR &=
                            (~CSL_UPP_UPTCR_RDSIZEQ_MASK);
                        instHandle->hwInfo.regs->UPTCR |=
                            (chanHandle->chanParams.fifoThresHold
                                << CSL_UPP_UPTCR_RDSIZEQ_SHIFT);
                    }
                    else
                    {
                        instHandle->hwInfo.regs->UPTCR &=
                            (~CSL_UPP_UPTCR_TXSIZEB_MASK);
                        instHandle->hwInfo.regs->UPTCR |=
                            (chanHandle->chanParams.fifoThresHold
                                << CSL_UPP_UPTCR_TXSIZEB_SHIFT);
                    }
                }
            }
            else
            {
                Upp_localConfigureChanB(chanHandle);
            }

            /* configure the clock for this channel instance (only if output  *
             * channel)                                                       */
            if (IOM_OUTPUT == chanHandle->mode)
            {
#ifdef BIOS_PWRM_ENABLE
                if ((TRUE == instHandle->devParams.pscPwrmEnable) &&
                    (Upp_pllDomain_NONE != instHandle->pwrmInfo.pllDomain))
                {
                    /* check what clock domain is supported by the device     */
                    if (Upp_pllDomain_0 == instHandle->pwrmInfo.pllDomain)
                    {
                        domain = Power_CPU;
                    }
                    else
                    {
                        domain = Power_PER;
                    }

                    retVal = (Int32)PWRM_getCurrentSetpoint(domain,&setpoint);

                    if (Power_SOK == retVal)
                    {
                        status = Upp_localCalculateClockSettings(
                                    instHandle,
                                    setpoint,
                                    chanHandle->chanParams.opFreq,
                                    &clkDiv);

                        if (IOM_COMPLETED == status)
                        {
                            /* check if the prescaler value is in the limits  */
                            if (clkDiv > 0x0F)
                            {
                                status = IOM_EBADARGS;
                            }
                        }
                    }
                }
                else
                {
#endif
                    /* No power management.calculate the clocks directly      */
                    clkDiv = (instHandle->devParams.inputFreq /
                                (2u * (chanHandle->chanParams.opFreq))) - 1u;

                    if (clkDiv > 0x0F)
                    {
                        status = IOM_EBADARGS;
                    }
#ifdef BIOS_PWRM_ENABLE
                }
#endif
            }

            if (IOM_COMPLETED == status)
            {
                if (Upp_ChanSel_A == chanHandle->chanParams.chanSel)
                {
                    instHandle->upicrRegVal |=
                        (clkDiv << CSL_UPP_UPICR_CLKDIVA_SHIFT);
                }
                else
                {
                    instHandle->upicrRegVal |=
                        (clkDiv << CSL_UPP_UPICR_CLKDIVB_SHIFT);
                }

                /* enable the device                                          */
                if ((Upp_DevMode_CHAN_A_RCV == instHandle->devParams.devMode) ||
                    (Upp_DevMode_CHAN_A_XMT == instHandle->devParams.devMode))
                {
                    /* update the value of the UPICR register                 */
                    instHandle->hwInfo.regs->UPICR = instHandle->upicrRegVal;

                    /* single channel mode.Hence enable the peripheral here   */
                    instHandle->hwInfo.regs->UPPCR |= (CSL_UPP_UPPCR_EN_MASK);
                }
                else
                {
                    /* 2 channels are configured Hence we need to check if    *
                     * both the channels are configured                       */
                    if ((Upp_DriverState_OPENED == instHandle->chanA.chanState) &&
                        (Upp_DriverState_OPENED == instHandle->chanB.chanState))
                    {
                        /* update the value of the UPICR register             */
                        instHandle->hwInfo.regs->UPICR = instHandle->upicrRegVal;

                        /* enable the device to make it ready for transactions*/
                        instHandle->hwInfo.regs->UPPCR |= (CSL_UPP_UPPCR_EN_MASK);
                    }
                }
            }
        }
        _restore_interrupts(hwiKey);
    }
    else
    {
        /* The requested command is not implemented                           */
        status = IOM_ENOTIMPL;
    }

    return (status);
}

/**
 * \brief    Function to abort/flush a channel.
 *           This function will abort all the requests in the channel.it will
 *           first check the active queue and then clear the packets in the
 *           pending queue.
 *
 * \param    chanHandle [IN] handle to the channel
 *
 * \return   IOM_COMPLETED if successful
 *           IOM error code in case of failure
 *
 * \enter    chanHandle should be a valid non NULL pointer
 * \leave    None
 */
Void Upp_localAbortChan(Upp_ChanObj *chanHandle)
{
    Upp_Object *instHandle = NULL;
    IOM_Packet *ioPacket   = NULL;

    assert((NULL != chanHandle) && (NULL != chanHandle->devHandle));
    instHandle = (Upp_Object *)chanHandle->devHandle;
    assert(NULL != instHandle);

    while (FALSE == QUE_empty(&chanHandle->queActiveList))
    {
        ioPacket = QUE_get(&chanHandle->queActiveList);

        if (NULL != ioPacket)
        {
            /* update the status of the packet as ABORTED                     */
            ioPacket->status = IOM_EABORT;
            ioPacket->size = 0;
        }

        /* reduce the submit count                                            */
        chanHandle->submitCount--;

        /* call the completion callback function registered with us           *
         * during channel creation                                            */
        if ((NULL != chanHandle->cbFxn) && (NULL != chanHandle->cbArg))
        {
            /*  Application callback                                          */
            (chanHandle->cbFxn)((Ptr)chanHandle->cbArg,ioPacket);
        }

        /* reduce the dependency count                                        */
        Upp_localLpscOff(instHandle);
    }


    while (FALSE == QUE_empty(&chanHandle->quePendList))
    {
        ioPacket = QUE_get(&chanHandle->quePendList);

        if (NULL != ioPacket)
        {
            /* update the status of the packet as ABORTED                     */
            ioPacket->status = IOM_EABORT;
            ioPacket->size = 0;
        }

        /* reduce the submit count                                            */
        chanHandle->submitCount--;

        /* call the completion callback function registered with us           *
         * during channel creation                                            */
        if ((NULL != chanHandle->cbFxn) && (NULL != chanHandle->cbArg))
        {
            /*  Application callback                                          */
            (chanHandle->cbFxn)((Ptr)chanHandle->cbArg,ioPacket);
        }

        if (0 == chanHandle->submitCount)
        {
            Upp_localDisableInterrupts(instHandle,chanHandle);
        }

        /* reduce the dependency count                                        */
        Upp_localLpscOff(instHandle);
    }

}

/* ========================================================================== */
/*                             END OF FILE                                    */
/* ========================================================================== */

