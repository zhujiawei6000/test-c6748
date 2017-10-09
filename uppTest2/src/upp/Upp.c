/*
 * Upp.c
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
 * \file      Upp.c
 *
 * \brief     Generic Upp Driver for DSP/BIOS Platform.It uses the internal DMA
 *            for data transfer.
 *
 *            This file implements the UPP IOM driver to be used with
 *            DSP-BIOS 5 operating system.
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
 *            4. support for the BIOS power management added.
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
#include <ti/sysbios/family/c64p/EventCombiner.h>
#include <ti/sysbios/family/c64p/Hwi.h>
#include <xdc/runtime/Error.h>
#include <string.h>
#include <assert.h>

#include <ti/sysbios/hal/Cache.h>
#include "Upp.h"
#include "UppLocal.h"

/*============================================================================*/
/*                        LOCAL FUNCTION PROTOTYPES                           */
/*============================================================================*/

static Int uppMdBindDev(Ptr *devp, Int devid, Ptr devParams);
static Int uppMdUnBindDev(Ptr devp);
static Int uppMdCreateChan(Ptr                *chanp,
                           Ptr                 devp,
                           String              name,
                           Int                 mode,
                           Ptr                 chanParams,
                           IOM_TiomCallback    cbFxn,
                           Ptr                 cbArg);
static Int uppMdDeleteChan(Ptr chanp);
static Int uppMdSubmitChan(Ptr chanp, IOM_Packet *const ioPacket);
static Int uppMdControlChan(Ptr chanp, Uns cmd, Ptr arg);
#ifndef PSP_DISABLE_INPUT_PARAMETER_CHECK
static Void uppValidateDevParams(const Upp_Params *devParams);
static Void uppValidateIoPkt(IOM_Packet *ioPacket);
#endif
static Void uppIntrHandler(Upp_Object *instHandle);
static Int32 uppRegisterIntrHandler(Upp_Object *instHandle,
                                    Uint32      intNum,
                                    Uint16      evt,
                                    EventCombiner_FuncPtr     intIsr);
static Int32 uppSubmitReq(Upp_Object     *instHandle,
                          Upp_ChanObj    *chanHandle,
                          IOM_Packet     *ioPacket);
static Int32 uppSubmitInterleavedReq(Upp_Object     *instHandle,
                                     Upp_ChanObj    *chanHandle,
                                     IOM_Packet     *ioPacket);
static Void uppLoadPktToDma(Upp_Object     *instHandle,
                            Upp_chanSel     chanSel,
                            IOM_Packet     *ioPacket);
static Void uppCompleteCurrentIo(Upp_ChanObj *chanHandle,Uint32 status);
static Void uppUnRegisterInt(Uint16 evt);
static Void uppCleanBufInCache(Upp_ChanObj *chanHandle,IOM_Packet *ioPacket);
#ifdef BIOS_PWRM_ENABLE
static Int32 uppRegisterNotification(Upp_Object *instHandle);
static Int32 uppUnregisterNotification(Upp_Object *instHandle);
static Int32 uppCalculateConstraints(Upp_ChanObj *chanHandle);
static Power_NotifyResponse uppNotifyCallback(Power_Event    eventType,
                                             Arg           eventArg1,
                                             Arg           eventArg2,
                                             Arg           clientArg);
static Void uppHandlePwrmEvent(Upp_Object   *instHandle);
static Power_NotifyResponse uppConfigureNewVfSettings(Upp_Object  *instHandle,
                                                     Uns          eventArg1,
                                                     Uns          eventArg2);
static Int32 uppUnregisterConstraints(Upp_ChanObj *chanHandle);
#endif

/* ========================================================================== */
/*                           GLOBAL MODULE STATE                              */
/* ========================================================================== */

/**
 * \brief  Array to store the state of the module (whether it is in use or not).
 *
 */
static Bool inUse[CSL_UPP_PER_CNT];

/**
 * \brief  UPP Module State Object
 *
 *  This structure holds the information related to the state of the instances
 *  of the UPP.
 */
static Upp_Module_State Upp_module = {&inUse[0]};

/**
 * \brief  Array of Upp instance State objects array
 */
static Upp_Object Upp_Instances[CSL_UPP_PER_CNT];

/**
 * \brief  Upp structure containing the information specific to an instance.
 */
static Upp_HwInfo Upp_deviceInstInfo[CSL_UPP_PER_CNT];

/*============================================================================*/
/*                            GLOBAL VARIABLES                                */
/*============================================================================*/

/**
 * \brief    UPP device default parameters.
 *           This structure is used to store the default parameters for the
 *           device creation.
 */
const Upp_Params Upp_PARAMS =
{
    Upp_EmuMode_SOFT_STOP,
    Upp_Loopback_DISABLE,
    Upp_DevMode_DUPLEX_0,
    FALSE,
    CSL_SYSCLK_2_FREQ,
    9,
    Upp_intHandlerSel_ECM,
    Upp_pllDomain_NONE
};

/**
 * \brief IOM driver interface table
 *
 * \note  This table holds the interface functions exposed by the IOM driver to
 *        the GIO layer.
 */
IOM_Fxns Upp_IOMFXNS =
{
    &uppMdBindDev,
    &uppMdUnBindDev,
    &uppMdControlChan,
    &uppMdCreateChan,
    &uppMdDeleteChan,
    &uppMdSubmitChan
};

/* ========================================================================== */
/*                          FUNCTION DEFINTIONS                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           MODULE FUNCTIONS                                 */
/* ========================================================================== */

/**
 * \brief   Initializes UPP IOM driver's data structures
 *
 *          This function initializes the UPP IOM driver's data structures
 *          including instance objects and channel objects.This function needs
 *          to be called at part of BIOS initialization by setting initFxn for
 *          that particular UDEV instance or by calling this function as part of
 *          user specific initFxn.
 *
 * \return  None
 */
Void Upp_init(Void)
{
    Int devId = 0x00;

    /* initialise the information for all the device instances                */
    for (devId = 0; devId < CSL_UPP_PER_CNT; devId++)
    {
        /* we have to initialize statically this information                  */
        Upp_module.inUse[devId] = FALSE;

        /* initialise the instance object                                     */
        memset((Void *)&Upp_Instances[devId], 0x00, sizeof(Upp_Object));

        if (0 == devId)
        {
            /* instance 0 initialisation                                      */
            Upp_deviceInstInfo[devId].instNum = (Uint32)devId;
            Upp_deviceInstInfo[devId].regs =
                (CSL_UppRegsOvly)CSL_UPP_0_REGS;
            Upp_deviceInstInfo[devId].cpuEventNum =
                (Uint32)CSL_INTC_EVENTID_UPPINT;
            Upp_deviceInstInfo[devId].moduleClkFreq =
                (Uint32)CSL_UPP_0_MODULE_FREQ;
#ifndef BIOS_PWRM_ENABLE
            Upp_deviceInstInfo[devId].pwrmLpscId =
                (Uint32)CSL_PSC_UPP;
#else
            Upp_deviceInstInfo[devId].pwrmLpscId =
                (Uint32)CSL_LPSC_NUMBER_UPP_0;
#endif
            Upp_deviceInstInfo[devId].pscInstance =
                (Uint32)CSL_LPSC_INST_UPP_0;
        }
    }
}

/**
 * \brief   Allocates and configures the UPP instance specified by devid.
 *
 *  Acquires the Handle of the UPP device and configure the UPP for the
 *  following things.
 *      o   validates the device creation parameters.
 *      o   configures the emulation mode.
 *      o   configures the DLB mode options.
 *      o   configures the operational mode.
 *
 * \param   devp       [OUT]  pointer to hold allocated instance object ptr
 * \param   devid      [IN]   instance number of the UPP
 * \param   devParams  [IN]   user supplied data params.
 *
 * \return  IOM_COMPLETED     if successful
 *
 *          IOM_EBADARGS      if not successful.
 *          IOM_EINUSE        if instance alrready created.
 *
 * \enter   devp        should be non NULL and valid pointer,
 *          devId       should be less than CSL_UPP_PER_CNT
 *          devParams   should be non NULL and valid pointer,
 *
 * \leave   Not implemented
 */
static Int uppMdBindDev(Ptr *devp, Int devid, Ptr devParams)
{
    Upp_Object        *instHandle = NULL;
    Upp_Params        *params     = NULL;
    volatile Uint32    timeOut    = 0;
    Int32              status     = IOM_COMPLETED;

#ifndef PSP_DISABLE_INPUT_PARAMETER_CHECK
    if ((NULL == devParams) || (NULL == devp) || (devid >= CSL_UPP_PER_CNT))
    {
        status = IOM_EBADARGS;
    }

    if (IOM_COMPLETED == status)
    {
        if (TRUE == Upp_module.inUse[devid])
        {
            status = IOM_EINUSE;
        }
    }
#endif /* PSP_DISABLE_INPUT_PARAMETER_CHECK */

    if (IOM_COMPLETED == status)
    {
        /* copy the pointer to the instance object                            */
        instHandle = &Upp_Instances[devid];

        /* set the module state as in use                                     */
        Upp_module.inUse[devid] = TRUE;

        /* get the device parameters                                          */
        params = (Upp_Params *)devParams;

#ifndef PSP_DISABLE_INPUT_PARAMETER_CHECK
        uppValidateDevParams(devParams);
#endif
        /* update the instance of the device being created                    */
        instHandle->instNum = devid;

        /* update the user supplied params to the instance object             */
        instHandle->devParams = *params;
        instHandle->retryCount = Upp_POLLED_RETRYCOUNT;
        instHandle->upicrRegVal = 0x00;

        instHandle->hwInfo = Upp_deviceInstInfo[devid];

#ifdef BIOS_PWRM_ENABLE
        /* update the power management information                            */
        instHandle->pwrmInfo.pllDomain = params->pllDomain;
#endif

        /* clear the channel objects (A&B)                                    */
        memset(&instHandle->chanA,0x00,sizeof(Upp_ChanObj));
        memset(&instHandle->chanB,0x00,sizeof(Upp_ChanObj));

        /* set the status of the channel to closed                            */
        instHandle->chanA.chanState = Upp_DriverState_CLOSED;
        instHandle->chanB.chanState = Upp_DriverState_CLOSED;

        /* create the pending and active queue for the TX channel             */
        Queue_construct(&(instHandle->chanA.queActiveList), NULL);
        Queue_construct(&(instHandle->chanA.quePendList), NULL);

        /* create the pending and active queue for the RX channel             */
        Queue_construct(&(instHandle->chanB.queActiveList), NULL);
        Queue_construct(&(instHandle->chanB.quePendList), NULL);

        /* if power management is not supported then switch on the module and *
         * keep it ON for the entire lifetime of the device                   */
        if (FALSE == instHandle->devParams.pscPwrmEnable)
        {
#ifdef BIOS_PWRM_ENABLE
            /* power on using bios PWRM API                                   */
            status = (Int32)Power_setDependency(
                                (Power_Resource)instHandle->hwInfo.pwrmLpscId);
#else
            /* power on using PSC API                                         */
            status = Psc_ModuleClkCtrl(
                         (Psc_DevId)instHandle->hwInfo.pscInstance,
                         (Uint32)instHandle->hwInfo.pwrmLpscId,
                         TRUE);
#endif /* BIOS_PWRM_ENABLE */
        }

        /* power on the module (only if the power management is enabled)      */
        status |= Upp_localLpscOn(instHandle);

        if (IOM_COMPLETED == status)
        {
            /* disable the device                                             */
            instHandle->hwInfo.regs->UPPCR &= (~CSL_UPP_UPPCR_EN_MASK);

            /* initialize the retry count                                     */
            timeOut = instHandle->retryCount;

            /* wait for the peipheral to become idle                          */
            while ((CSL_UPP_UPPCR_DB_MASK ==
                    (instHandle->hwInfo.regs->UPPCR & CSL_UPP_UPPCR_DB_MASK)) &&
                        (0 != timeOut))
            {
                timeOut--;
            }

            if (0 == timeOut)
            {
                status = IOM_ETIMEOUT;
            }

            if (IOM_COMPLETED == status)
            {
                /* place the device in software reset and then bring it out of*
                 * reset after waiting for 200CLK cycles                      */
                instHandle->hwInfo.regs->UPPCR |= CSL_UPP_UPPCR_SWRST_MASK;

                /* reinitialize the retry count                               */
                timeOut = instHandle->retryCount;

                /* wait for 200CLK cycles                                     */
                while (timeOut > 0)
                {
                    timeOut--;
                }
            }

            if (IOM_COMPLETED == status)
            {
                /* bring the device out of software reset                     */
                instHandle->hwInfo.regs->UPPCR &= (~CSL_UPP_UPPCR_SWRST_MASK);

                /* configure the emulation settings  (TBD)                    */
                instHandle->hwInfo.regs->UPPCR &=
                    (~(CSL_UPP_UPPCR_RTEMU_MASK |
                       CSL_UPP_UPPCR_SOFT_MASK |
                       CSL_UPP_UPPCR_FREE_MASK));

                instHandle->hwInfo.regs->UPPCR |= (Uint32)(params->emulationMode);

                /* reset the DLB modes                                        */
                instHandle->hwInfo.regs->UPDLB = CSL_UPP_UPDLB_RESETVAL;

                /* configure the appropriate mode for the loopback            */
                instHandle->hwInfo.regs->UPDLB =
                    (params->dlbMode << CSL_UPP_UPDLB_AB_SHIFT);

                /* configure the operating mode and number of channels        */
                instHandle->hwInfo.regs->UPCTL &= (~(CSL_UPP_UPCTL_CHN_MASK |
                                                    CSL_UPP_UPCTL_MODE_MASK));
                instHandle->hwInfo.regs->UPCTL |= (Uint32)(params->devMode);

                /* reduce the dependency count (only if PWRM enabled)         */
                status = Upp_localLpscOff(instHandle);
            }
        }
#ifdef BIOS_PWRM_ENABLE
        if ((IOM_COMPLETED == status) &&
            (TRUE == instHandle->devParams.pscPwrmEnable))
        {
            /* register the notify function for the PWRM events               */
            status = uppRegisterNotification(instHandle);
        }
#endif
        if (IOM_COMPLETED != status)
        {
            /* device creation failed                                         */
            *devp = NULL;
            Upp_module.inUse[devid] = FALSE;
        }
        else
        {
            /* update the device handle                                       */
            *devp = instHandle;

            /* set the status of the driver to created                        */
            instHandle->devState = Upp_DriverState_CREATED;
        }
    }
    return (status);
}

/**
 * \brief   Closes the UPP device instance from use.
 *          This API is called by the application when it no longer requires the
 *          device instance. Note that all the channels have to be closed for
 *          the device instance to be deleted sucessfully.
 *
 * \param   devp    [IN]  Handle to the device created.
 *
 * \return  IOM_COMPLETED     if successful
 *          IOM_ABORTED       if not successful
 *
 * \enter   devp       should be non NULL and valid pointer,
 *
 * \leave   Not implemented
 */
static Int uppMdUnBindDev(Ptr devp)
{
    Upp_Object     *instHandle = NULL;
    Int32           status     = IOM_COMPLETED;
    Uint32          hwiKey     = 0;

#ifndef PSP_DISABLE_INPUT_PARAMETER_CHECK
    if (NULL == devp)
    {
        status = IOM_EBADARGS;
    }
    else
    {
#endif /* PSP_DISABLE_INPUT_PARAMETER_CHECK */

        instHandle = (Upp_Object *)devp;

#ifndef PSP_DISABLE_INPUT_PARAMETER_CHECK
        if ((CSL_UPP_PER_CNT <= instHandle->instNum)                  ||
            (Upp_DriverState_CLOSED != instHandle->chanA.chanState)  ||
            (Upp_DriverState_CLOSED != instHandle->chanB.chanState))
        {
            status = IOM_EBADARGS;
        }
    }
#endif /* PSP_DISABLE_INPUT_PARAMETER_CHECK */

    if (IOM_COMPLETED == status)
    {
        /* critical section starts                                            */
        hwiKey = (Uint32)_disable_interrupts();

        status = Upp_localLpscOn(instHandle);

        if (IOM_COMPLETED == status)
        {
            /* disable the Upp                                                */
            instHandle->hwInfo.regs->UPPCR &= (~CSL_UPP_UPPCR_EN_MASK);

            /* power off the module                                           */
            status = Upp_localLpscOff(instHandle);
        }

        if (IOM_COMPLETED == status)
        {
            /* Mark driver state as deleted and module as not in use          */
            instHandle->devState = Upp_DriverState_DELETED;
            Upp_module.inUse[instHandle->instNum] = FALSE;

            /* critical section ends                                          */
            _restore_interrupts(hwiKey);

            if (FALSE == instHandle->devParams.pscPwrmEnable)
            {
#ifdef BIOS_PWRM_ENABLE
                /* power on using bios PWRM API                                   */
                status = (Int32)Power_releaseDependency(
                                    (Power_Resource)instHandle->hwInfo.pwrmLpscId);
#else
                /* power on using PSC API                                         */
                status = Psc_ModuleClkCtrl(
                             (Psc_DevId)instHandle->hwInfo.pscInstance,
                             (Uint32)instHandle->hwInfo.pwrmLpscId,
                             FALSE);
#endif
            }
#ifdef BIOS_PWRM_ENABLE
            else
            {
                /* unregister the notify function for the PWRM events             */
                status = uppUnregisterNotification(instHandle);
            }
#endif
        }
    }
    return status;
}

/**
 *  \brief  Creates a communication channel in specified mode to communicate
 *          data between the application and the UPP channl instance. This
 *          function sets the required hardware configurations for the data
 *          transactions.it returns configured channel handle to application.
 *          which will be used in all further transactions with the channel.
 *
 *          This function is called in response to a GIO_create call.
 *
 *          Pre-requisites:
 *          1.  Valid chanParams structure
 *              This takes much information pertaining to UPP channel
 *              configuration such as how many slots are used for this
 *              channel what are they communication parameters,clock setting etc
 *          2.  Valid device pointer
 *
 * \note    Please refer to Appendix A : IOM interface of spru616
 *          DSP/BIOS Driver Developer's Guide for details on
 *          uppMdCreateChan parameters.
 *
 * \param   chanp        [IN]     Channel Handler
 * \param   devp         [IN]     Device pointer
 * \param   name         [IN]     to the end of the Upp Instance name.
 * \param   mode         [IN]     channel  mode -> input or output
 * \param   chanParams   [IN]     channel parameters from user
 * \param   cbFxn        [IN]     callback function pointer
 * \param   cbArg        [IN]     callback function Arguments
 *
 * \return  IOM_COMPLETED     if successful
 *          IOM_EBADIO        if not successful
 *          IOM_EALLOC            "
 *          IOM_EBADARGS      if passed invalid chanParams structure
 */
static Int uppMdCreateChan(Ptr             *chanp,
                           Ptr              devp,
                           String           name,
                           Int              mode,
                           Ptr              chanParams,
                           IOM_TiomCallback cbFxn,
                           Ptr              cbArg)
{
    Upp_Object        *instHandle = NULL;
    Upp_ChanObj       *chanHandle = NULL;
    Upp_ChanParams    *chanparam  = NULL;
#ifdef BIOS_PWRM_ENABLE
    Uns                setpoint   = 0x00;
    Power_Domain        domain     = Power_CPU;
#endif
    Uint32             hwiKey     = 0;
    Uint32             clkDiv     = 0x00;
    Bool               pscPwrOn   = FALSE;
    Int32              retVal     = IOM_COMPLETED;
    Int32              status     = IOM_COMPLETED;

    /* Done to remove compiler warning of unused param                        */
    if (NULL != name)
    {
        name = name;
    }

/* Begin parameter checking                                                   */
#ifndef PSP_DISABLE_INPUT_PARAMETER_CHECK
    if (((NULL == chanParams)
        || (NULL == cbFxn)
        || (NULL == cbArg)
        || (NULL == devp)
        || (NULL == chanp))
        || ((IOM_INPUT != mode) && (IOM_OUTPUT != mode)))
    {
        status = IOM_EBADARGS;
    }
#endif  /* PSP_DISABLE_INPUT_PARAMETER_CHECK */
/* End parameter checking                                                     */

    if (IOM_COMPLETED == status)
    {
        chanparam  = (Upp_ChanParams *)chanParams;
        instHandle = (Upp_Object *)devp;

#ifndef PSP_DISABLE_INPUT_PARAMETER_CHECK
        Upp_localValidateChanParams(instHandle,chanparam);
#endif
        /* power on the module                                                */
        status = Upp_localLpscOn(instHandle);

        if (IOM_COMPLETED == status)
        {
            pscPwrOn = TRUE;
        }

        if (IOM_COMPLETED == status)
        {
            /* get the pointer to the required channel structure              */
            if (Upp_ChanSel_A == chanparam->chanSel)
            {
                chanHandle = &instHandle->chanA;
            }
            else
            {
                chanHandle = &instHandle->chanB;
            }

            /* critical section starts                                        */
            hwiKey = (Uint32)_disable_interrupts();

            /* we will check if the current requested chan is available here  *
             * protect the channel status so that multiple threads requesting *
             * same channel do not cause a corruption                         */
            if (Upp_DriverState_CLOSED == chanHandle->chanState)
            {
                chanHandle->chanState = Upp_DriverState_OPENED;
            }
            else
            {
                /* Requested channel is already taken hence we will set status*
                 * as invalid                                                 */
                status = IOM_EBADMODE;
            }
            /* critical section ends                                          */
            _restore_interrupts(hwiKey);
        }

        if (IOM_COMPLETED == status)
        {
            chanHandle->devHandle = instHandle;

            /* update the mode of the operation of the channel                */
            chanHandle->mode = (Int16)mode;

            /* Assign the respective callback function                        */
            chanHandle->cbFxn = cbFxn;
            chanHandle->cbArg = (Arg)cbArg;

            /* Counter that counts outstanding requests of this channel       */
            chanHandle->submitCount = 0;

            /* copy the user settings in to the channel object                */
            chanHandle->chanParams = *chanparam;

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

#ifdef BIOS_PWRM_ENABLE
            /* calculate contraints when the device is operating in power     *
             * saving enabled mode and is in a proper pll domain              */
            if ((TRUE == instHandle->devParams.pscPwrmEnable) &&
                (Upp_pllDomain_NONE != instHandle->pwrmInfo.pllDomain) &&
                (IOM_OUTPUT == chanHandle->mode))
            {
                status = uppCalculateConstraints(chanHandle);
            }
#endif
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

                    retVal = (Int32)Power_getCurrentSetpoint(domain,&setpoint);

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
                            if (clkDiv > 0x0Fu)
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

                    if (clkDiv > 0x0Fu)
                    {
                        status = IOM_EBADARGS;
                    }
#ifdef BIOS_PWRM_ENABLE
                }
#endif
            }
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
                    /* Note: In DLB mode both the sections require the same   *
                     * clkdiv values other wise the DLB mode does not work    *
                     * properly                                               */
                    if (Upp_Loopback_ENABLE_AB == instHandle->devParams.dlbMode)
                    {
                        /* copy the CLK div values from the channel A to B    */
                        instHandle->upicrRegVal |=
                            (((instHandle->upicrRegVal &
                               CSL_UPP_UPICR_CLKDIVA_MASK) >>
                               CSL_UPP_UPICR_CLKDIVA_SHIFT) <<
                               CSL_UPP_UPICR_CLKDIVB_SHIFT);
                    }
                    else if (Upp_Loopback_ENABLE_BA
                        == instHandle->devParams.dlbMode)
                    {
                        /* copy the CLK div values from the channel B to A    */
                        instHandle->upicrRegVal |=
                            (((instHandle->upicrRegVal &
                               CSL_UPP_UPICR_CLKDIVB_MASK) >>
                               CSL_UPP_UPICR_CLKDIVB_SHIFT) <<
                               CSL_UPP_UPICR_CLKDIVA_SHIFT);
                    }
                    /* update the value of the UPICR register                 */
                    instHandle->hwInfo.regs->UPICR = instHandle->upicrRegVal;

                    /* enable the device to make it ready for transactions    */
                    instHandle->hwInfo.regs->UPPCR |= (CSL_UPP_UPPCR_EN_MASK);
                }
            }

            /* register the interrupt handler                                 */
            hwiKey = (Uint32)_disable_interrupts();

            if (FALSE == instHandle->isrRegistered)
            {
                instHandle->isrRegistered = TRUE;
                uppRegisterIntrHandler(
                    instHandle,
                    (Uint32)instHandle->devParams.hwiNumber,
                    instHandle->hwInfo.cpuEventNum,
                    (EventCombiner_FuncPtr)&uppIntrHandler);
            }
            _restore_interrupts(hwiKey);
        }
    }

    if (TRUE == pscPwrOn)
    {
        /* power off the module                                               */
        retVal = Upp_localLpscOff(instHandle);

        if (IOM_COMPLETED == status)
        {
            status = retVal;
        }
    }

    if (IOM_COMPLETED != status )
    {
        /* enter critical section                                             */
        hwiKey = (Uint32)_disable_interrupts();

        if (NULL != chanHandle)
        {
            /* set the status of the channel to closed                        */
            chanHandle->chanState = Upp_DriverState_CLOSED;
        }

        /* Leave critical section                                             */
        _restore_interrupts(hwiKey);

        /* channel opening for transaction has failed                         */
        chanHandle = NULL;
    }
    else
    {
        *chanp = chanHandle;
    }
    return (status);
}

/**
 * \brief   frees a channel and all it's associated resources.
 *          This function undoes all the hardware and software initializations
 *          performed while the creation of the channel. After this operation
 *          the channel is no longer available for use by the application.
 *
 * \param   chanp  [IN]       Handle to the channel
 *
 * \return  IOM_COMPLETED     if successful
 *          IOM_ABORTED       if not successful
 *
 * \enter   chanp  should be non NULL and valid pointer,
 *
 * \leave   Not implemented
 */
static Int uppMdDeleteChan(Ptr chanp)
{
    Upp_Object     *instHandle = NULL;
    Upp_ChanObj    *chanHandle = NULL;
    Int32           status     = IOM_COMPLETED;

#ifndef PSP_DISABLE_INPUT_PARAMETER_CHECK
    if (NULL == chanp)
    {
        status = IOM_EBADARGS;
    }
    else
    {
#endif /* PSP_DISABLE_INPUT_PARAMETER_CHECK */
        chanHandle = (Upp_ChanObj *)chanp;
#ifndef PSP_DISABLE_INPUT_PARAMETER_CHECK
        if (Upp_DriverState_OPENED != chanHandle->chanState)
        {
            status = IOM_EBADMODE;
        }
        else
        {
            if (NULL == chanHandle->devHandle)
            {
                status = IOM_EBADARGS;
            }
            else
            {
#endif
                instHandle = (Upp_Object *)chanHandle->devHandle;
#ifndef PSP_DISABLE_INPUT_PARAMETER_CHECK
                if (NULL == instHandle)
                {
                    status = IOM_EBADARGS;
                }
            }
        }
    }
#endif /* PSP_DISABLE_INPUT_PARAMETER_CHECK */

    if (IOM_COMPLETED == status)
    {
        /* unregister all the constraints registered by this channel          */
        status = uppUnregisterConstraints(chanHandle);

        /* set the status of the channel to closed                            */
        chanHandle->chanState = Upp_DriverState_CLOSED;

        /* unregister the interrupt handler                                   */
        if ((Upp_DriverState_CLOSED == instHandle->chanA.chanState) &&
            (Upp_DriverState_CLOSED == instHandle->chanB.chanState))
        {
            uppUnRegisterInt(instHandle->hwInfo.cpuEventNum);
            instHandle->isrRegistered = FALSE;
        }
    }
    return (status);
}

/**
 * \brief   Submit a I/O packet to a channel for processing
 *
 *  The GIO layer calls this function to cause the mini-driver
 *  to process the IOM_Packet for read/write/flush/abort operations.
 *
 * \param   chanp         [IN]    Pointer to channel
 * \param   ioPacket      [IN]    Pointer to packet to be submitted
 *
 * \return  IOM_COMPLETED, if packet is fully processed
 *          IOM_ABORTED,   if packet is aborted
 *          IOM_PENDING,   if packet is not fully processed
 *          IOM_EBADIO     in case of an error in processing
 *
 * \enter   chanp       should be non NULL and valid pointer,
 *          ioPacket    should be non NULL and valid pointer
 *
 * \leave   Not implemented
 */
static Int uppMdSubmitChan(Ptr chanp, IOM_Packet *const ioPacket)
{
    Upp_Object          *instHandle = NULL;
    Upp_ChanObj         *chanHandle = NULL;
    Bool                 pscPwrOn   = FALSE;
    Uint32               hwiKey     = 0x00;
    Int32                status     = IOM_COMPLETED;

#ifndef PSP_DISABLE_INPUT_PARAMETER_CHECK
    if ((NULL == chanp) || ( NULL == ioPacket))
    {
        status = IOM_EBADARGS;
    }
    else
    {
#endif
        chanHandle = (Upp_ChanObj *)chanp;

#ifndef PSP_DISABLE_INPUT_PARAMETER_CHECK

        if ((NULL == chanHandle) || (NULL == chanHandle->devHandle))
        {
            status = IOM_EBADARGS;
        }
        else
        {
#endif
            instHandle = (Upp_Object *)chanHandle->devHandle;
#ifndef PSP_DISABLE_INPUT_PARAMETER_CHECK
            if (NULL == instHandle)
            {
                status = IOM_EBADARGS;
            }
            else
            {
                if (NULL == ioPacket->addr)
                {
                    if ((IOM_READ == ioPacket->cmd) ||
                        (IOM_WRITE == ioPacket->cmd))
                    {
                        status = IOM_EBADARGS;
                    }
                }
            }

            if (IOM_COMPLETED == status)
            {
                if (((IOM_READ == ioPacket->cmd) &&
                     (IOM_INPUT != chanHandle->mode)) ||
                    ((IOM_WRITE == ioPacket->cmd) &&
                      (IOM_OUTPUT != chanHandle->mode)))
                {
                    status = IOM_EBADARGS;
                }
            }
        }
    }
#endif /* PSP_DISABLE_INPUT_PARAMETER_CHECK */
    if (IOM_COMPLETED == status)
    {
        /* process the command sent by the application                        */
        switch (ioPacket->cmd)
        {
            case IOM_READ:
            case IOM_WRITE:
                if ((TRUE != chanHandle->flushAbort) &&
                    (TRUE != instHandle->uppSmStateStop))
                {
                    status = Upp_localLpscOn(instHandle);

                    if (IOM_COMPLETED == status)
                    {
                        pscPwrOn = TRUE;

                        if ((Upp_ChanMode_DEMULTIPLEX_ENABLE !=
                                chanHandle->chanParams.chanMode) &&
                            (Upp_ChanMode_TX_INTERLEAVE_ENABLE !=
                                chanHandle->chanParams.chanMode))
                        {
                            status = uppSubmitReq(instHandle,chanHandle,ioPacket);
                        }
                        else
                        {
                            status = uppSubmitInterleavedReq(instHandle,chanHandle,ioPacket);
                        }
                    }
                }
                else
                {
                    status = IOM_EBADIO;
                }
                break;
            case IOM_FLUSH:
            case IOM_ABORT:
                /* Flush command has been issued need to abort the receive    *
                 * channel packets and complete the TX packets normally       */
                hwiKey = (Uint32)_disable_interrupts();

                if (0 != chanHandle->submitCount)
                {
                    /* set the commmand state to flush                        */
                    chanHandle->flushAbort = TRUE;
                }

                _restore_interrupts(hwiKey);
                break;
            default:
                status = IOM_EBADIO;
                break;
        }
    }

    if ((TRUE == pscPwrOn) && (IOM_PENDING != status))
    {
        /* reduce the dependency count for the module                         */
        Upp_localLpscOff(instHandle);
    }
    return status;
}

/**
 * \brief   Implements the IOCTLS for UPP IOM driver.
 *
 *          mdControlChan() implements recieves IOCTL commands from the
 *          GIO/SIO layer and executes them accordingly.
 *
 * \param   chanp  [IN]    Pointer to channel
 * \param   cmd    [IN]    IOCTL command specified by the application
 * \param   arg    [IN]    additional arguments if required by the command.
 *
 * \return  IOM_COMPLETED, if command is executed correctly
 *          IOM_ABORTED,   if command returns error during execution
 *          IOM_ENOTIMPL,  if command is not supported
 *          IOM_EBADARGS   if args are not correct
 */
static Int uppMdControlChan(Ptr chanp, Uns cmd, Ptr arg)
{
    Upp_Object        *instHandle = NULL;
    Upp_ChanObj       *chanHandle = NULL;
    Int32              status     = IOM_COMPLETED;
    Int32              retVal     = IOM_COMPLETED;

#ifndef PSP_DISABLE_INPUT_PARAMETER_CHECK
    if (NULL == chanp)
    {
        status = IOM_EBADARGS;
    }
    else
    {
#endif /* PSP_DISABLE_INPUT_PARAMETER_CHECK */

        chanHandle = (Upp_ChanObj *)chanp;

#ifndef PSP_DISABLE_INPUT_PARAMETER_CHECK
        if (NULL == chanHandle->devHandle)
        {
            status = IOM_EBADARGS;
        }
    }
#endif

    if (IOM_COMPLETED == status)
    {
        instHandle = (Upp_Object *)chanHandle->devHandle;

        /* Switch on the Module to execute the IOCTL command.This is required *
         * especially when there are no IOPs pending in the driver but an     *
         * command is issued by the application                               */
        status = Upp_localLpscOn(instHandle);

        if (IOM_COMPLETED == status)
        {
            /* call the function to process the IOCTL command                 */
            status =  Upp_localSubmitIoctl(
                          chanHandle,
                         (Upp_Ioctl)cmd,
                          arg,
                          NULL);

            /* reduce the dependency count for the module                     */
            retVal = Upp_localLpscOff(instHandle);

            if (IOM_COMPLETED == status)
            {
                status = retVal;
            }
        }
    }
    return (status);
}

/*============================================================================*/
/*                            LOCAL FUNCTIONS                                 */
/*============================================================================*/
#ifndef PSP_DISABLE_INPUT_PARAMETER_CHECK
/**
 * \brief     uppValidateDevParams
 *            This function validates all the parameters supplied by the
 *            application during the creation of the device.if the supplied
 *            params do not match then the function causes the program to abort.
 *
 * \param     devParams  [IN]  device creation parameters.
 *
 * \return    None
 *
 * \enter     devParams should be a valid Non NULL pointer.
 * \leave     None
 */
static Void uppValidateDevParams(const Upp_Params *devParams)
{
    assert(NULL != devParams);

    /* validate the emulation mode                                            */
    assert((Upp_EmuMode_FREE == devParams->emulationMode) ||
            (Upp_EmuMode_SOFT_STOP == devParams->emulationMode) ||
            (Upp_EmuMode_RTEMU == devParams->emulationMode));

    /* validate the loopback modes                                            */
    assert((Upp_Loopback_DISABLE == devParams->dlbMode) ||
            (Upp_Loopback_ENABLE_AB == devParams->dlbMode) ||
            (Upp_Loopback_ENABLE_BA == devParams->dlbMode));

    /* validate the device operational mode                                   */
    assert((Upp_DevMode_CHAN_A_RCV == devParams->devMode) ||
            (Upp_DevMode_CHAN_A_XMT ==  devParams->devMode) ||
            (Upp_DevMode_CHAN_AB_RCV == devParams->devMode) ||
            (Upp_DevMode_CHAN_AB_XMT ==  devParams->devMode) ||
            (Upp_DevMode_DUPLEX_0 == devParams->devMode) ||
            (Upp_DevMode_DUPLEX_1 == devParams->devMode));

    /* validate the pll domain                                                */
    assert((Upp_pllDomain_0 == devParams->pllDomain) ||
            (Upp_pllDomain_1 == devParams->pllDomain) ||
            (Upp_pllDomain_NONE == devParams->pllDomain));

    /* validate the different operating modes                                 */
    if (((Upp_Loopback_ENABLE_BA == devParams->dlbMode) &&
         (Upp_DevMode_DUPLEX_0 != devParams->devMode)) ||
         ((Upp_Loopback_ENABLE_AB == devParams->dlbMode) &&
         (Upp_DevMode_DUPLEX_1 != devParams->devMode)))
    {
        /* dlb modes can only be realized in respective duplex modes          */
        assert(FALSE);
    }
}

/**
 * \brief     uppValidateChanParams
 *            This function validates all the parameters supplied by the
 *            application during the creation of the channel.if the supplied
 *            params do not match then the function causes the program to abort.
 *
 * \param     devParams    [IN]  device creation parameters.
 * \param     chanParams   [IN]  channel creation parameters.
 * \return    None
 *
 * \note      This function will do the needful checking in only the DEBUG mode
 *            as RELEASE mode will result in removal of all the "assert"s
 *
 * \enter     devparam  should be a valid Non NULL pointer.
 *            chanparam should be a valid Non NULL pointer.
 *
 * \leave     None
 */
Void Upp_localValidateChanParams(const Upp_Object     *instHandle,
                                 const Upp_ChanParams *chanParams)
{
    const Upp_Params   *devParams = NULL;

    assert((NULL != instHandle) && (NULL != chanParams));
    assert(NULL != (&instHandle->devParams));

    devParams = &instHandle->devParams;
    assert(NULL != devParams);

    /* validate the channel input parameters                                  */
    assert((Upp_ChanSel_A == chanParams->chanSel) ||
            (Upp_ChanSel_B == chanParams->chanSel));

    assert((Upp_bitWidth_8 == chanParams->bitWidth) ||
            (Upp_bitWidth_9 == chanParams->bitWidth) ||
            (Upp_bitWidth_10 == chanParams->bitWidth) ||
            (Upp_bitWidth_11 == chanParams->bitWidth) ||
            (Upp_bitWidth_12 == chanParams->bitWidth) ||
            (Upp_bitWidth_13 == chanParams->bitWidth) ||
            (Upp_bitWidth_14 == chanParams->bitWidth) ||
            (Upp_bitWidth_15 == chanParams->bitWidth) ||
            (Upp_bitWidth_16 == chanParams->bitWidth));

    assert((Upp_dataRate_SINGLE == chanParams->dataRate) ||
            (Upp_dataRate_DOUBLE == chanParams->dataRate));

    assert((Upp_ChanMode_NORMAL == chanParams->chanMode) ||
            (Upp_ChanMode_TX_INTERLEAVE_ENABLE == chanParams->chanMode) ||
            (Upp_ChanMode_DEMULTIPLEX_ENABLE == chanParams->chanMode));

    assert((Upp_dataPackFmt_RJZE == chanParams->dataPackFmt) ||
            (Upp_dataPackFmt_RJSE == chanParams->dataPackFmt) ||
            (Upp_dataPackFmt_LJZF == chanParams->dataPackFmt));

    /* validate if the required output clock frequency is configurable        */
    if (((Upp_dataRate_SINGLE == chanParams->dataRate) &&
          (chanParams->opFreq > (CSL_SYSCLK_1_FREQ/4))) ||
         ((Upp_dataRate_DOUBLE == chanParams->dataRate) &&
          (chanParams->opFreq > (CSL_SYSCLK_1_FREQ/8))))
    {
        assert(FALSE);
    }

    assert((Upp_fifoThreshold_64 == chanParams->fifoThresHold) ||
            (Upp_fifoThreshold_128 == chanParams->fifoThresHold) ||
            (Upp_fifoThreshold_256 == chanParams->fifoThresHold));


    if ((Upp_ChanMode_TX_INTERLEAVE_ENABLE == chanParams->chanMode) &&
         ((Upp_DevMode_CHAN_A_XMT != devParams->devMode) ||
          (Upp_dataRate_SINGLE != chanParams->dataRate)))
    {
        /* interleave mode can work only in single channel,single data rate   *
         * transmit mode                                                      */
        assert(FALSE);
    }

    if ((Upp_ChanMode_DEMULTIPLEX_ENABLE == chanParams->chanMode) &&
         (((Upp_DevMode_CHAN_A_XMT != devParams->devMode) &&
           (Upp_DevMode_CHAN_A_RCV != devParams->devMode)) ||
          (Upp_dataRate_DOUBLE != chanParams->dataRate)))
    {
        /* demultiplex mode cna  mode can work only in double data rate       *
         * single channel receive or transmit mode only                       */
        assert(FALSE);
    }

    /* validate the mode of creation of the channels                          */
    if (((Upp_DevMode_CHAN_A_XMT == devParams->devMode) ||
          (Upp_DevMode_CHAN_A_RCV == devParams->devMode)) &&
          (Upp_ChanSel_B == chanParams->chanSel))
    {
        assert(FALSE);
    }
}

/**
 * \brief     uppValidateIoPkt
 *            This function validates all the parameters supplied by the
 *            application during the submission of the iopacket.if the supplied
 *            params do not match then the function causes the program to abort.
 *
 * \param     ioPacket     [IN]  IO packet to be verified
 *
 * \return    None
 *
 * \note      This function will do the needful checking in only the DEBUG mode
 *            as RELEASE mode will result in removal of all the "assert"s
 *
 * \enter     chanHandle  should be a valid Non NULL pointer.
 *            iopacket should be a valid Non NULL pointer.
 *
 * \leave     None
 */
static Void uppValidateIoPkt(IOM_Packet *ioPacket)
{
    Upp_transParam *transParam = NULL;

    assert(NULL != ioPacket);

    transParam = ioPacket->addr;

    /* validate the ioPacket memory address                                   */
    assert((NULL != transParam) && (NULL != transParam->windowAddr));


    /* validate the window address                                            */
    if (0 != ((Uint32)transParam->windowAddr & CSL_UPP_UPID0_ADDR_MASK))
    {
        /* window address is not aligned to multiple of 8 bytes               */
        assert(FALSE);
    }

    /* vallidate the line count                                               */
    assert(0 != transParam->lineCount);

    /* validate the byte count                                                */
    if (0 != (transParam->bytesPerLine & CSL_UPP_UPID1_BCNT_MASK))
    {
        /* byte count is not an even multiple                                 */
        assert(FALSE);
    }

    /* validate the line offset value                                         */
    if (0 != (transParam->lineOffset & CSL_UPP_UPID2_LNOFFSET_MASK))
    {
        /* line count is not aligned to multiple of 8 bytes                   */
        assert(FALSE);
    }

    /* validate that the byte count and the line count is not zero            */
    assert((0 != transParam->bytesPerLine) && (0 != transParam->lineCount));

}
#endif

/**
 * \brief   Configures the hardware channel A
 *          This function configure the channel according to the settings
 *          provided by the application.
 *
 * \param   chanHandle   [IN]  Handle to the channel
 *
 * \return  None
 */
Void Upp_localConfigureChanA(Upp_ChanObj *chanHandle)
{
    Upp_Object   *instHandle = NULL;

    assert((NULL != chanHandle) && (NULL != chanHandle->devHandle));

    instHandle = (Upp_Object *)chanHandle->devHandle;
    assert(NULL != instHandle);

    /* configure the START,WAIT,ENABLE pin polarities                         */
    instHandle->upicrRegVal |=
        (((Uint32)chanHandle->chanParams.signalCfg.waitEnable
            << CSL_UPP_UPICR_WAITA_SHIFT) |
         ((Uint32)chanHandle->chanParams.signalCfg.enaEnable
            << CSL_UPP_UPICR_ENAA_SHIFT) |
         ((Uint32)chanHandle->chanParams.signalCfg.startEnable
            << CSL_UPP_UPICR_STARTA_SHIFT) |
         ((Uint32)chanHandle->chanParams.signalCfg.waitPinPol
            << CSL_UPP_UPICR_WAITPOLA_SHIFT) |
         ((Uint32)chanHandle->chanParams.signalCfg.enablePinPol
            << CSL_UPP_UPICR_ENAPOLA_SHIFT) |
         ((Uint32)chanHandle->chanParams.signalCfg.startPinPol
            << CSL_UPP_UPICR_STARTPOLA_SHIFT) |
         ((Uint32)chanHandle->chanParams.signalCfg.clkPol
            << CSL_UPP_UPICR_CLKINVA_SHIFT) |
         ((Uint32)chanHandle->chanParams.signalCfg.pinIdleState
            << CSL_UPP_UPICR_TRISA_SHIFT));

    /* update the data rate                                                   */
    instHandle->hwInfo.regs->UPCTL &= (~CSL_UPP_UPCTL_DRA_MASK);
    instHandle->hwInfo.regs->UPCTL |=
        (chanHandle->chanParams.dataRate << CSL_UPP_UPCTL_DRA_SHIFT);

    /* update the interface width                                             */
    instHandle->hwInfo.regs->UPCTL &= (~CSL_UPP_UPCTL_IWA_MASK);

    if (Upp_bitWidth_8 != chanHandle->chanParams.bitWidth)
    {
        instHandle->hwInfo.regs->UPCTL |= CSL_UPP_UPCTL_IWA_MASK;
    }

    /* update the channel width                                               */
    instHandle->hwInfo.regs->UPCTL &= (~CSL_UPP_UPCTL_DPWA_MASK);

    if ((Upp_bitWidth_8 != chanHandle->chanParams.bitWidth) &&
        (Upp_bitWidth_16 != chanHandle->chanParams.bitWidth))
    {
        instHandle->hwInfo.regs->UPCTL |=
            (chanHandle->chanParams.bitWidth << CSL_UPP_UPCTL_DPWA_SHIFT);
    }

    /* update the channel mode (if any)                                       */
    instHandle->hwInfo.regs->UPCTL &= ~(CSL_UPP_UPCTL_DDRDEMUX_MASK |
                                        CSL_UPP_UPCTL_SDRTXIL_MASK);
    instHandle->hwInfo.regs->UPCTL |= (chanHandle->chanParams.chanMode
                                        << CSL_UPP_UPCTL_SDRTXIL_SHIFT);


    /* update the data packing format                                         */
    if ((Upp_bitWidth_8 != chanHandle->chanParams.bitWidth) &&
        (Upp_bitWidth_16 != chanHandle->chanParams.bitWidth))
    {
        instHandle->hwInfo.regs->UPCTL &= (~CSL_UPP_UPCTL_DPFA_MASK);
        instHandle->hwInfo.regs->UPCTL |=
            (chanHandle->chanParams.dataPackFmt << CSL_UPP_UPCTL_DPFA_SHIFT);
    }

    if (CSL_UPP_UPICR_TRISA_MASK !=
        (instHandle->upicrRegVal & CSL_UPP_UPICR_TRISA_MASK))
    {
        /* configure the idle value                                           */
        instHandle->hwInfo.regs->UPIVR &= (~CSL_UPP_UPIVR_VALA_MASK);

        instHandle->hwInfo.regs->UPIVR |=
        ((Uint32)chanHandle->chanParams.idleVal << CSL_UPP_UPIVR_VALA_SHIFT);
    }

    if (IOM_INPUT == chanHandle->mode)
    {
        instHandle->hwInfo.regs->UPTCR &= (~CSL_UPP_UPTCR_RDSIZEI_MASK);
        instHandle->hwInfo.regs->UPTCR |=
           (chanHandle->chanParams.fifoThresHold << CSL_UPP_UPTCR_RDSIZEI_SHIFT);
    }
    else
    {
        instHandle->hwInfo.regs->UPTCR &= (~CSL_UPP_UPTCR_TXSIZEA_MASK);
        instHandle->hwInfo.regs->UPTCR |=
           (chanHandle->chanParams.fifoThresHold << CSL_UPP_UPTCR_TXSIZEA_SHIFT);
    }
}

/**
 * \brief   Configures the hardware channel B
 *          This function configure the channel according to the settings
 *          provided by the application.
 *
 * \param   chanHandle   [IN]  Handle to the channel
 *
 * \return  None
 */
Void Upp_localConfigureChanB(Upp_ChanObj *chanHandle)
{
    Upp_Object   *instHandle = NULL;

    assert((NULL != chanHandle) && (NULL != chanHandle->devHandle));

    instHandle = (Upp_Object *)chanHandle->devHandle;
    assert(NULL != instHandle);

    /* configure the START,WAIT,ENABLE pin polarities                         */
    instHandle->upicrRegVal |=
        (((Uint32)chanHandle->chanParams.signalCfg.waitEnable
            << CSL_UPP_UPICR_WAITB_SHIFT) |
         ((Uint32)chanHandle->chanParams.signalCfg.enaEnable
            << CSL_UPP_UPICR_ENAB_SHIFT) |
         ((Uint32)chanHandle->chanParams.signalCfg.startEnable
            << CSL_UPP_UPICR_STARTB_SHIFT) |
         ((Uint32)chanHandle->chanParams.signalCfg.waitPinPol
            << CSL_UPP_UPICR_WAITPOLB_SHIFT) |
         ((Uint32)chanHandle->chanParams.signalCfg.enablePinPol
            << CSL_UPP_UPICR_ENAPOLB_SHIFT) |
         ((Uint32)chanHandle->chanParams.signalCfg.startPinPol
            << CSL_UPP_UPICR_STARTPOLB_SHIFT) |
         ((Uint32)chanHandle->chanParams.signalCfg.clkPol
            << CSL_UPP_UPICR_CLKINVB_SHIFT) |
         ((Uint32)chanHandle->chanParams.signalCfg.pinIdleState
            << CSL_UPP_UPICR_TRISB_SHIFT));

    /* update the data rate                                                   */
    instHandle->hwInfo.regs->UPCTL &= (~CSL_UPP_UPCTL_DRB_MASK);
    instHandle->hwInfo.regs->UPCTL |=
        (chanHandle->chanParams.dataRate << CSL_UPP_UPCTL_DRB_SHIFT);

    /* update the interface width                                             */
    instHandle->hwInfo.regs->UPCTL &= (~CSL_UPP_UPCTL_IWB_MASK);

    if (Upp_bitWidth_8 != chanHandle->chanParams.bitWidth)
    {
        instHandle->hwInfo.regs->UPCTL |= CSL_UPP_UPCTL_IWB_MASK;
    }

    /* update the channel width                                               */
    instHandle->hwInfo.regs->UPCTL &= (~CSL_UPP_UPCTL_DPWB_MASK);

    if ((Upp_bitWidth_8 != chanHandle->chanParams.bitWidth) &&
        (Upp_bitWidth_16 != chanHandle->chanParams.bitWidth))
    {
        instHandle->hwInfo.regs->UPCTL |=
            (chanHandle->chanParams.bitWidth << CSL_UPP_UPCTL_DPWB_SHIFT);
    }

    /* Note : No need to configure the operational mode for interleaved mode  *
     * as the channel B will not be used in the interleaved mode,only chan A  *
     * is required when operating in this special mode                        */

    /* update the data packing format                                         */
    if ((Upp_bitWidth_8 != chanHandle->chanParams.bitWidth) &&
        (Upp_bitWidth_16 != chanHandle->chanParams.bitWidth))
    {
        instHandle->hwInfo.regs->UPCTL &= (~CSL_UPP_UPCTL_DPFB_MASK);
        instHandle->hwInfo.regs->UPCTL |=
            (chanHandle->chanParams.dataPackFmt << CSL_UPP_UPCTL_DPFB_SHIFT);
    }

    /* configure the high impedance state                                     */
    if (CSL_UPP_UPICR_TRISB_MASK !=
        (instHandle->upicrRegVal & CSL_UPP_UPICR_TRISB_MASK))
    {
        /* configure the idle value                                           */
        instHandle->hwInfo.regs->UPIVR &= (~CSL_UPP_UPIVR_VALB_MASK);

        instHandle->hwInfo.regs->UPIVR |=
        ((Uint32)chanHandle->chanParams.idleVal << CSL_UPP_UPIVR_VALB_SHIFT);
    }

    if (IOM_INPUT == chanHandle->mode)
    {
        instHandle->hwInfo.regs->UPTCR &= (~CSL_UPP_UPTCR_RDSIZEQ_MASK);
        instHandle->hwInfo.regs->UPTCR |=
           (chanHandle->chanParams.fifoThresHold << CSL_UPP_UPTCR_RDSIZEQ_SHIFT);
    }
    else
    {
        instHandle->hwInfo.regs->UPTCR &= (~CSL_UPP_UPTCR_TXSIZEB_MASK);
        instHandle->hwInfo.regs->UPTCR |=
           (chanHandle->chanParams.fifoThresHold << CSL_UPP_UPTCR_TXSIZEB_SHIFT);
    }
}

/**
 * \brief   Function to disable the interrupts for the channel
 *
 * \param   instHandle   [IN]    device Handle
 * \param   chanHandle   [IN]    channel handle
 *
 * \return  None
 */
Void Upp_localDisableInterrupts(Upp_Object  *instHandle,
                                Upp_ChanObj *chanHandle)
{
    assert((NULL != chanHandle) && (NULL != instHandle));

    if (Upp_ChanSel_A == chanHandle->chanParams.chanSel)
    {
       /* disable the interrupts for the channel I                            */
       instHandle->hwInfo.regs->UPIEC =
           (CSL_UPP_UPIEC_DPEI_MASK |
            CSL_UPP_UPIEC_UORI_MASK |
            CSL_UPP_UPIEC_ERRI_MASK |
            CSL_UPP_UPIEC_EOWI_MASK |
            CSL_UPP_UPIEC_EOLI_MASK);

       if ((Upp_ChanMode_DEMULTIPLEX_ENABLE ==
               chanHandle->chanParams.chanMode) ||
           (Upp_ChanMode_TX_INTERLEAVE_ENABLE ==
               chanHandle->chanParams.chanMode))
       {
           /* disable the interrupts for the channel Q                        */
           instHandle->hwInfo.regs->UPIEC =
               (CSL_UPP_UPIEC_DPEQ_MASK |
                CSL_UPP_UPIEC_UORQ_MASK |
                CSL_UPP_UPIEC_ERRQ_MASK |
                CSL_UPP_UPIEC_EOWQ_MASK |
                CSL_UPP_UPIEC_EOLQ_MASK);
       }
    }
    else
    {
       instHandle->hwInfo.regs->UPIEC =
           (CSL_UPP_UPIEC_DPEQ_MASK |
            CSL_UPP_UPIEC_UORQ_MASK |
            CSL_UPP_UPIEC_ERRQ_MASK |
            CSL_UPP_UPIEC_EOWQ_MASK |
            CSL_UPP_UPIEC_EOLQ_MASK);
    }
}

/**
 * \brief   Function to enable the interrupts for the channel
 *
 * \param   instHandle   [IN]    device Handle
 * \param   chanHandle   [IN]    channel handle
 *
 * \return  None
 */
Void Upp_localEnableInterrupts(Upp_Object  *instHandle,
                               Upp_ChanObj *chanHandle)
{
    assert((NULL != chanHandle) && (NULL != instHandle));

    if (Upp_ChanSel_A == chanHandle->chanParams.chanSel)
    {
        /* enable the interrupts for the channel I                            */
        instHandle->hwInfo.regs->UPIES =
            (CSL_UPP_UPIES_DPEI_MASK |
             CSL_UPP_UPIES_UORI_MASK |
             CSL_UPP_UPIES_ERRI_MASK |
             CSL_UPP_UPIES_EOLI_MASK |
             CSL_UPP_UPIES_EOWI_MASK);

          if ((Upp_ChanMode_DEMULTIPLEX_ENABLE ==
                   chanHandle->chanParams.chanMode) ||
               (Upp_ChanMode_TX_INTERLEAVE_ENABLE ==
                   chanHandle->chanParams.chanMode))
           {
               /* disable the interrupts for the channel Q                    */
               instHandle->hwInfo.regs->UPIES =
                   (CSL_UPP_UPIES_DPEQ_MASK |
                    CSL_UPP_UPIES_UORQ_MASK |
                    CSL_UPP_UPIES_ERRQ_MASK |
                    CSL_UPP_UPIES_EOWQ_MASK |
                    CSL_UPP_UPIES_EOLQ_MASK);
           }
    }
    else
    {
        instHandle->hwInfo.regs->UPIES =
            (CSL_UPP_UPIES_DPEQ_MASK |
             CSL_UPP_UPIES_UORQ_MASK |
             CSL_UPP_UPIES_ERRQ_MASK |
             CSL_UPP_UPIES_EOLQ_MASK |
             CSL_UPP_UPIES_EOWQ_MASK);
    }
}


/**
 * \brief   registers interrupt handler for the device.
 *
 * \param   instHandle   [IN]    device handle
 * \param   intNum       [IN]    Hwi number for this event
 * \param   evt          [IN]    event number
 * \param   intIsr       [IN]    function pointer
 *
 * \return  None
 */
static Int32 uppRegisterIntrHandler(Upp_Object *instHandle,
                                    Uint32      intNum,
                                    Uint16      evt,
                                    EventCombiner_FuncPtr     intIsr)
{
//    ECM_Attrs            ecmAttrs   = ECM_ATTRS;

    Uint32               hwiKey     = 0;
    Int32                status     = IOM_COMPLETED;

    assert((NULL != instHandle) && (NULL != intIsr));

    hwiKey = (Uint32)_disable_interrupts();

    if (Upp_intHandlerSel_ECM == instHandle->devParams.instHandlerSel)
    {
//        /*Enable the interrupt later                                          */
//        ecmAttrs.unmask = 0u;
//
//        /* setup arg such that isr will get 'chan' as arg                     */
//        ecmAttrs.arg    = (Arg)instHandle;

        /* Plug the ISR with interrupt (event) number                         */
        EventCombiner_dispatchPlug(evt,intIsr, (UArg)instHandle, TRUE);

        /* Enabling the HWI_ID where corresponding ECM group is configured    */
        Hwi_enableIER(1u << intNum);

        EventCombiner_enableEvent(evt);
    }
    else
    {
        Hwi_Params            hwiAttrs;
        Hwi_Params_init(&hwiAttrs);
        hwiAttrs.arg = (Arg)instHandle;

        Error_Block eb;
        Error_init(&eb);

        /* map the uPP event to the Hwi number                                */
        Hwi_eventMap(instHandle->devParams.hwiNumber,evt);

        /* plug the interrupt handler directly in to the HWI itself           */
        Hwi_create(instHandle->devParams.hwiNumber,
            intIsr,
            &hwiAttrs,
            &eb);

        /* Enabling the HWI_ID where corresponding event is configured        */
        Hwi_enableIER(1u << instHandle->devParams.hwiNumber);
    }

    /* Enabling the interrupts                                                */
    _restore_interrupts(hwiKey);

    return status;
}

/**
 * \brief   Function to unregister interrupt handler for channel
 *
 *          This function unregisters the interrupt handler for the channel by
 *          disabling the evnt for the channel.
 *
 * \param   evt          [IN]  interrupt event number
 *
 * \return  None
 */
static Void uppUnRegisterInt(Uint16 evt)
{
    Uint32   hwiKey = 0;

    /* disable the interrupts                                                 */
    hwiKey = (Uint32)_disable_interrupts();

    /* Disable the event                                                      */
    EventCombiner_disableEvent(evt);

    /* Enabling the interrupts                                                */
    _restore_interrupts(hwiKey);
}

/**
 * \brief   Interrupt Handler for the UPP device
 *          The function handles all the interrupts generated by the uPP device.
 *          it handles the interrupts for both the channels, also when a END-OF-
 *          WINDOW interrupt occurs it loads a new packet to the internal DMA
 *          and closes the current IOP and calls the approriate callback.
 *
 * \param   instHandle   [IN]    Handle to the channel
 *
 * \return  None
 *
 * \enter   instHandle should be a valid non NULL pointer.
 * \leave   None
 */
static Void uppIntrHandler(Upp_Object *instHandle)
{
    Upp_ChanObj *chanHandle = NULL;
    Uint32       status     = 0x00;
    IOM_Packet  *ioPacket   = NULL;
#ifdef BIOS_PWRM_ENABLE
    Uint32       count      = 0x00;
    Int32        retVal     = Power_SOK;
#endif

    assert(NULL != instHandle);

    /* read the interrupt status register                                     */
    status = instHandle->hwInfo.regs->UPIER;

    while (status)
    {
        /*================Channel I interrupt processing======================*/
        chanHandle = &instHandle->chanA;

        if (0 != (Upp_MASK_CHAN_A & status))
        {
            instHandle->hwInfo.regs->UPIER = (status & Upp_MASK_CHAN_A);
        }

#ifndef Upp_STATISTICS_ENABLE
        if (CSL_UPP_UPIER_EOWI_MASK == (CSL_UPP_UPIER_EOWI_MASK & status))
        {
            /* enable the end of line interrupt                               */
            instHandle->hwInfo.regs->UPIES |= (CSL_UPP_UPIES_EOLI_MASK);
        }

        if (CSL_UPP_UPIER_EOLI_MASK == (CSL_UPP_UPIER_EOLI_MASK & status))
        {
            if (NULL != chanHandle->chanParams.userCbFxnStatus)
            {
                (chanHandle->chanParams.userCbFxnStatus)
                    (chanHandle->chanParams.userCbArgStatus,status,NULL);
            }
        }
#else
        if (CSL_UPP_UPIER_EOLI_MASK == (CSL_UPP_UPIER_EOLI_MASK & status))
        {
            chanHandle->stats.lineCnt++;
            if (NULL != chanHandle->chanParams.userCbFxnStatus)
            {
                (chanHandle->chanParams.userCbFxnStatus)
                    (chanHandle->chanParams.userCbArgStatus,status,NULL);
            }
        }

        if (CSL_UPP_UPIER_ERRI_MASK == (CSL_UPP_UPIER_ERRI_MASK & status))
        {
            chanHandle->stats.cbaErrCnt++;
        }

        if (CSL_UPP_UPIER_UORI_MASK == (CSL_UPP_UPIER_UORI_MASK & status))
        {
            chanHandle->stats.dataErrCnt++;
        }

        if (CSL_UPP_UPIER_DPEI_MASK == (CSL_UPP_UPIER_DPEI_MASK & status))
        {
            chanHandle->stats.dmaErrCnt++;
        }
#endif
        if ((0 != (Upp_PKT_LOAD_MASK_CHAN_A & status)) &&
            (TRUE != chanHandle->flushAbort))
        {
            if ((CSL_UPP_UPIS2_PEND_MASK !=
                (instHandle->hwInfo.regs->UPIS2 & CSL_UPP_UPIS2_PEND_MASK)) &&
                (FALSE == instHandle->pwrmInfo.ioSuspend))
            {
                /* check if there is any packet to load or not                */
                if (FALSE == Queue_empty(&(chanHandle->quePendList)))
                {
                    ioPacket = Queue_get(&(chanHandle->quePendList));

                    /* put the packet in to the active list                   */
                    Queue_put(&(chanHandle->queActiveList),ioPacket);

                    /* load the packet in to the DMA                          */
                    uppLoadPktToDma(instHandle,Upp_ChanSel_A,ioPacket);
                }

#ifndef Upp_STATISTICS_ENABLE
                /* disable the End of line interrupt                          */
                instHandle->hwInfo.regs->UPIEC = CSL_UPP_UPIEC_EOLI_MASK;
#endif
            }
        }

        if ((0 != (Upp_PKT_COMPLETION_MASK_CHAN_A & status))
            && (0 != chanHandle->submitCount))
        {
            chanHandle->submitCount--;

            /* complete the processing of the current packet                  */
            uppCompleteCurrentIo(chanHandle,status);
        }

        if (TRUE == chanHandle->flushAbort)
        {
            Upp_localAbortChan(chanHandle);

            /* reset the channel abort mode                                   */
            chanHandle->flushAbort = FALSE;
        }

        /*================Channel Q interrupt processing======================*/
        if ((Upp_ChanMode_DEMULTIPLEX_ENABLE !=
                instHandle->chanA.chanParams.chanMode) &&
            (Upp_ChanMode_TX_INTERLEAVE_ENABLE !=
                instHandle->chanA.chanParams.chanMode))
        {
            /* Individual DMA channels are mapped to individual channels      */
            chanHandle = &instHandle->chanB;
        }

        if (0 != (Upp_MASK_CHAN_B & status))
        {
            instHandle->hwInfo.regs->UPIER = (status & Upp_MASK_CHAN_B);
        }

#ifndef Upp_STATISTICS_ENABLE
        if (CSL_UPP_UPIER_EOWQ_MASK == (CSL_UPP_UPIER_EOWQ_MASK & status))
        {
            /* enable the end of line interrupt                               */
            instHandle->hwInfo.regs->UPIES |= (CSL_UPP_UPIES_EOLQ_MASK);
        }

        if (CSL_UPP_UPIER_EOLQ_MASK == (CSL_UPP_UPIER_EOLQ_MASK & status))
        {
            if (NULL != chanHandle->chanParams.userCbFxnStatus)
            {
                (chanHandle->chanParams.userCbFxnStatus)
                    (chanHandle->chanParams.userCbArgStatus,status,NULL);
            }
        }
#else
        if (CSL_UPP_UPIER_EOLQ_MASK == (CSL_UPP_UPIER_EOLQ_MASK & status))
        {
            chanHandle->stats.lineCnt++;

            if (NULL != chanHandle->chanParams.userCbFxnStatus)
            {
                (chanHandle->chanParams.userCbFxnStatus)
                    (chanHandle->chanParams.userCbArgStatus,status,NULL);
            }
        }

        if (CSL_UPP_UPIER_ERRQ_MASK == (CSL_UPP_UPIER_ERRQ_MASK & status))
        {
            chanHandle->stats.cbaErrCnt++;
        }

        if (CSL_UPP_UPIER_UORQ_MASK == (CSL_UPP_UPIER_UORQ_MASK & status))
        {
            chanHandle->stats.dataErrCnt++;
        }

        if (CSL_UPP_UPIER_DPEQ_MASK == (CSL_UPP_UPIER_DPEQ_MASK & status))
        {
            chanHandle->stats.dmaErrCnt++;
        }
#endif
        if ((0 != (Upp_PKT_LOAD_MASK_CHAN_B & status)) &&
            (TRUE != chanHandle->flushAbort))
        {
            if ((CSL_UPP_UPQS2_PEND_MASK !=
                  (instHandle->hwInfo.regs->UPQS2 & CSL_UPP_UPQS2_PEND_MASK)) &&
                  (FALSE == instHandle->pwrmInfo.ioSuspend))
            {
                /* check if there is any packet to load or not                */
                if (FALSE == Queue_empty(&(chanHandle->quePendList)))
                {
                    ioPacket = Queue_get(&(chanHandle->quePendList));

                    /* put the packet in to the active list                   */
                    Queue_put(&(chanHandle->queActiveList),ioPacket);

                    /* load the packet in to the DMA                          */
                    uppLoadPktToDma(instHandle,Upp_ChanSel_B,ioPacket);
                }

#ifndef Upp_STATISTICS_ENABLE
                /* disable the End of line interrupt                          */
                instHandle->hwInfo.regs->UPIEC = CSL_UPP_UPIEC_EOLQ_MASK;
#endif
            }
        }

        if ((0 != (Upp_PKT_COMPLETION_MASK_CHAN_B & status))
            && (0 != chanHandle->submitCount))
        {
            chanHandle->submitCount--;

            /* complete the processing of the current packet                  */
            uppCompleteCurrentIo(chanHandle,status);
        }

#ifdef BIOS_PWRM_ENABLE
        /* check if the driver is waiting to go to sleep or process DVFS      *
         * event (only if the application has requested PWRM support).        */
        if ((TRUE == instHandle->devParams.pscPwrmEnable) ||
             (TRUE == instHandle->pwrmInfo.ioSuspend))
        {
            uppHandlePwrmEvent(instHandle);
        }
#endif

        /* check if the channel is to be aborted                              */
        if (TRUE == chanHandle->flushAbort)
        {
            Upp_localAbortChan(chanHandle);
            chanHandle->flushAbort = FALSE;
        }

        if ((TRUE == instHandle->uppSmStateStop) &&
            (FALSE == instHandle->chanA.flushAbort) &&
            (FALSE == instHandle->chanB.flushAbort))
        {
            /* disable the UPP device                                         */
            instHandle->hwInfo.regs->UPPCR &= (~CSL_UPP_UPPCR_EN_MASK);
        }

#ifdef BIOS_PWRM_ENABLE
        /* reduce the dependency count                                        */
        retVal = (Int32)Power_getDependencyCount(
                        (Power_Resource)instHandle->hwInfo.pwrmLpscId,
                         &count);

        /* re read the interrupt status register                              */
        if ((Power_SOK == retVal) && (0 != count))
        {
#endif
            /* if atleast one packet is there we should read the interrupt    *
             * status otherwise the module might be OFF in PSC                */
            status = instHandle->hwInfo.regs->UPIER;

#ifdef BIOS_PWRM_ENABLE
        }
        else
        {
            /* set status to 0x00 to exit from the interrupt Handler          */
            status = 0x00;
        }
#endif
    }

#ifdef BIOS_PWRM_ENABLE
    /* reduce the dependency count                                            */
    retVal = (Int32)Power_getDependencyCount(
                    (Power_Resource)instHandle->hwInfo.pwrmLpscId,
                     &count);

    /* set the UPEOI register to 0x00 to regenerate the interrupts            */
    if ((Power_SOK == retVal) && (0 != count))
    {
#endif
        instHandle->hwInfo.regs->UPEOI = 0x00;
#ifdef BIOS_PWRM_ENABLE
    }
#endif
}

/**
 * \brief   This function completes the current pending request and then
 *          invokes the application registered callback.
 *
 * \param   chanHandle   [IN]  Handle to the channel
 * \param   status       [IN]  actual status of completion of the packet
 *
 * \return  None
 *
 * \enter   chanHandle  is a valid non null pointer
 * \leave   Not implemented
 */
static Void uppCompleteCurrentIo(Upp_ChanObj *chanHandle,
                                 Uint32       status)
{
    Upp_Object      *instHandle = NULL;
    IOM_Packet      *ioPacket   = NULL;

    assert((NULL != chanHandle) && (NULL != chanHandle->devHandle));

    instHandle = chanHandle->devHandle;
    assert(NULL != instHandle);

    if (FALSE == Queue_empty(&(chanHandle->queActiveList)))
    {
        ioPacket = Queue_get(&(chanHandle->queActiveList));

        if (NULL != ioPacket)
        {
            /* check if the packet completed with any errors if so then we    *
             * will update the status of the packet as error                  */
            if (0 != ((CSL_UPP_UPIER_EOWI_MASK | CSL_UPP_UPIER_EOWQ_MASK) & status))
            {
                /* packet is completing on End of window interrupt            */
                ioPacket->status = IOM_COMPLETED;
                ioPacket->size = 0;
            }
            else
            {
                /* error has occured                                          */
                ioPacket->status = IOM_EBADIO;

                /* calculate the actual number of bytes transferred           */
                if ((CSL_UPP_UPIER_DPEI_MASK |
                     CSL_UPP_UPIER_UORI_MASK |
                     CSL_UPP_UPIER_ERRI_MASK) ==
                        (status & (CSL_UPP_UPIER_DPEI_MASK |
                                   CSL_UPP_UPIER_UORI_MASK |
                                   CSL_UPP_UPIER_ERRI_MASK)))
                {
                    ioPacket->size  -=
                        ((instHandle->hwInfo.regs->UPIS1
                              & CSL_UPP_UPIS1_LNCNT_MASK) *
                         (instHandle->hwInfo.regs->UPIS1
                              & CSL_UPP_UPIS1_BCNT_MASK));
                }
                else
                {
                    ioPacket->size  -=
                        ((instHandle->hwInfo.regs->UPQS1
                              & CSL_UPP_UPQS1_LNCNT_MASK) *
                         (instHandle->hwInfo.regs->UPQS1
                              & CSL_UPP_UPQS1_BCNT_MASK));
                }
            }
        }
#ifdef Upp_STATISTICS_ENABLE
        chanHandle->stats.pktCnt++;
        chanHandle->stats.byteCnt = ioPacket->size;
#endif
        if (0 == chanHandle->submitCount)
        {
            Upp_localDisableInterrupts(instHandle,chanHandle);
        }

        /* reduce the dependency count                                        */
        Upp_localLpscOff((Upp_Object *)chanHandle->devHandle);

        /* ioPacket cannot be NULL becuase the queue is already checked for   *
         * empty status                                                       */
        if ((NULL != chanHandle->chanParams.userCbFxnError) &&
            (IOM_EBADIO == ioPacket->status))
        {
            /* call this function only in case of an error                    */
            chanHandle->chanParams.userCbFxnError(
                chanHandle->chanParams.userCbArgError,
                status,(Uint32)ioPacket);
        }

        /* call the completion callback function registered with us           *
         * during channel creation                                            */
        if ((NULL != chanHandle->cbFxn) && (NULL != chanHandle->cbArg))
        {
            /*  Application callback                                          */
            (*chanHandle->cbFxn)((Ptr)chanHandle->cbArg,ioPacket);
        }
    }
}

/**
 * \brief    Function to handle the IO packet submission.
 *           This function handles the packet queuing and loading mechanism for
 *           the UPP driver. initial two packet submitted by the driver are
 *           loaded directly in to the DMA desciptors.The other packets (after
 *           two packets) will be queued in to the pending queue and loaded
 *           as and when a packet is completed in the DMA.
 *
 * \param    instHandle   [IN]  Handle to the device instance.
 * \param    chanHandle   [IN]  handle to the channel
 * \param    ioPacket     [IN]  IO packet to be processed.
 *
 * \return   IOM_PENDING  if the packet is loaded successfully.
 *           IOM error code in case of an error.
 */
static Int32 uppSubmitReq(Upp_Object     *instHandle,
                          Upp_ChanObj    *chanHandle,
                          IOM_Packet     *ioPacket)
{
    Uint32           hwiKey     = 0;
    Bool             pend       = FALSE;
    Int32            status     = IOM_PENDING;

    assert((NULL != chanHandle) && (NULL != ioPacket) && (NULL != instHandle));

#ifndef PSP_DISABLE_INPUT_PARAMETER_CHECK
    uppValidateIoPkt(ioPacket);
#endif

    /* clean the buffers if the cache operation mode is enabled               */
    if (TRUE == chanHandle->chanParams.enableCache)
    {
        uppCleanBufInCache(chanHandle,ioPacket);
    }

    /* We are entering the crictical section because the submit count can     *
     * change at any point of time due to completion of the IOP               */
    hwiKey = (Uint32)_disable_interrupts();

    if ((Upp_MAXLINKCNT <= chanHandle->submitCount) ||
        (Upp_DriverState_PWRM_SUSPEND == instHandle->devState))
    {
        /* There are enough packets programmed in the DMA                     */
        chanHandle->submitCount++;

        Queue_put(&chanHandle->quePendList,(Ptr)ioPacket);

        /* critical section ends                                              */
        _restore_interrupts(hwiKey);
    }
    else
    {
        /* Either one of the paramset or Both the paramsets are free          */
        /* increment the submit count                                         */
        chanHandle->submitCount++;

        if (Upp_ChanSel_A == chanHandle->chanParams.chanSel)
        {
            if (CSL_UPP_UPIS2_PEND_MASK ==
                    (instHandle->hwInfo.regs->UPIS2 & CSL_UPP_UPIS2_PEND_MASK))
            {
                pend = TRUE;
            }
        }
        else
        {
            if (CSL_UPP_UPQS2_PEND_MASK ==
                (instHandle->hwInfo.regs->UPQS2 & CSL_UPP_UPQS2_PEND_MASK))
            {
                pend = TRUE;
            }
        }

        if (FALSE == pend)
        {
            /* descriptors need to be loaded in to the DMA                    */
            Queue_put(&(chanHandle->queActiveList),ioPacket);

            uppLoadPktToDma(
                instHandle,
                chanHandle->chanParams.chanSel,
                ioPacket);
        }
        else
        {
            Queue_put(&chanHandle->quePendList,(Ptr)ioPacket);
        }

        if (1u == chanHandle->submitCount)
        {
            Upp_localEnableInterrupts(instHandle,chanHandle);
        }

        /* exit the critical section code                                     */
        _restore_interrupts(hwiKey);
    } /* Pending queue empty or not */
    return (status);
}

/**
 * \brief    Function to handle the IO packet submission in interleaved mode.
 *           This function handles the packet queuing and loading mechanism for
 *           the UPP driver in interleaved mode.
 *
 * \param    instHandle   [IN]  Handle to the device instance.
 * \param    chanHandle   [IN]  handle to the channel
 * \param    ioPacket     [IN]  IO packet to be processed.
 *
 * \return   IOM_PENDING  if the packet is loaded successfully.
 *           IOM error code in case of an error.
 */
static Int32 uppSubmitInterleavedReq(Upp_Object     *instHandle,
                                     Upp_ChanObj    *chanHandle,
                                     IOM_Packet     *ioPacket)
{
    Uint32           hwiKey     = 0;
    Bool             pend       = FALSE;
    Upp_chanSel      chanSel    = Upp_ChanSel_A;
    Int32            status     = IOM_PENDING;

    assert((NULL != chanHandle) && (NULL != ioPacket) && (NULL != instHandle));

#ifndef PSP_DISABLE_INPUT_PARAMETER_CHECK
    uppValidateIoPkt(ioPacket);
#endif

    /* clean the buffers if the cache operation mode is enabled               */
    if (TRUE == chanHandle->chanParams.enableCache)
    {
        uppCleanBufInCache(chanHandle,ioPacket);
    }

    /* We are entering the crictical section because the submit count can     *
     * change at any point of time due to completion of the IOP               */
    hwiKey = (Uint32)_disable_interrupts();

    if ((Upp_INTERLEAVED_MAXLINKCNT <= chanHandle->submitCount) ||
        (Upp_DriverState_PWRM_SUSPEND == instHandle->devState))
    {
        /* There are enough packets programmed in the DMA                     */
        chanHandle->submitCount++;

        Queue_put(&chanHandle->quePendList,(Ptr)ioPacket);

        /* critical section ends                                              */
        _restore_interrupts(hwiKey);
    }
    else
    {
        /* Either one of the paramset or Both the paramsets are free          */
        /* increment the submit count                                         */
        chanHandle->submitCount++;

        if (1 == (chanHandle->submitCount%2))
        {
            chanSel = Upp_ChanSel_A;

            /* Load to the channel I                                          */
            if (CSL_UPP_UPIS2_PEND_MASK ==
                    (instHandle->hwInfo.regs->UPIS2 & CSL_UPP_UPIS2_PEND_MASK))
            {
                pend = TRUE;
            }
        }
        else
        {
            chanSel = Upp_ChanSel_B;

            if (CSL_UPP_UPQS2_PEND_MASK ==
                (instHandle->hwInfo.regs->UPQS2 & CSL_UPP_UPQS2_PEND_MASK))
            {
                pend = TRUE;
            }
        }

        if (FALSE == pend)
        {
            /* descriptors need to be loaded in to the DMA                    */
            Queue_put(&(chanHandle->queActiveList),ioPacket);

            uppLoadPktToDma(instHandle,chanSel,ioPacket);
        }
        else
        {
            Queue_put(&chanHandle->quePendList,(Ptr)ioPacket);
        }

        if (2u == chanHandle->submitCount)
        {
            Upp_localEnableInterrupts(instHandle,chanHandle);
        }

        /* exit the critical section code                                     */
        _restore_interrupts(hwiKey);
    } /* Pending queue empty or not */
    return (status);
}


/**
 * \brief     This function cleans the buffers from the cache.
 *            If the data lines are in continous memory location then the cache
 *            operation is done on the whole buffer at once. if the lines
 *            are located in the memeory with some offset between them then
 *            each of the data line is cleaned seperately.
 *
 * \param     chanHandle  [IN]   handle to the channel
 * \param     ioPacket    [IN]   Io packet to be cleaned from cache.
 *
 * \return    None
 */
static Void uppCleanBufInCache(Upp_ChanObj *chanHandle,IOM_Packet *ioPacket)
{
    Uint32            lnCnt      = 0x00;
    Upp_transParam   *transParam = NULL;

    assert((NULL != chanHandle) && (NULL != ioPacket));

    transParam = (Upp_transParam *)ioPacket->addr;
    assert(NULL != transParam);

    if (IOM_INPUT == chanHandle->mode)
    {
        /* Cleans and invalidate the D Cache in MVA way                       */
        if (transParam->bytesPerLine == transParam->lineOffset)
        {
            /* memory location is continous                                   */
            Cache_wbInv(transParam->windowAddr,ioPacket->size,Cache_Type_ALL,TRUE);
        }
        else
        {
            for (lnCnt = 0; lnCnt < transParam->lineCount; lnCnt++)
            {
                Cache_wbInv(
                    (Ptr)((Uint32)transParam->windowAddr + (lnCnt * transParam->lineOffset)),
                    transParam->bytesPerLine,
                    Cache_Type_ALL,
                    TRUE);
            }
        }
    }
    else
    {
        /* clean the D Cache in MVA way                                       */
        if (transParam->bytesPerLine == transParam->lineOffset)
        {
            /* memory location is continous                                   */
            Cache_wb(transParam->windowAddr,ioPacket->size,Cache_Type_ALL,TRUE);
        }
        else
        {
            for (lnCnt = 0; lnCnt < transParam->lineCount; lnCnt++)
            {
                Cache_wb(
                    (Ptr)((Uint32)transParam->windowAddr + (lnCnt * transParam->lineOffset)),
                    transParam->bytesPerLine,
                    Cache_Type_ALL,
                    TRUE);
            }
        }
    }
}

/**
 *  \brief  This function loads the packets to the actual DMA paramset.
 *
 * \param    instHandle   [IN]  Handle to the device instance.
 * \param    chanHandle   [IN]  handle to the channel
 * \param    ioPacket     [IN]  IO packet to be processed.
 *
 *  \return None
 *
 *  \enter  Not implemented
 *
 *  \leave  Not implemented
 */
static Void uppLoadPktToDma(Upp_Object     *instHandle,
                            Upp_chanSel     chanSel,
                            IOM_Packet     *ioPacket)
{
    Upp_transParam   *transParam = NULL;

    assert((NULL != instHandle) && (NULL != ioPacket));

    transParam = (Upp_transParam *)ioPacket->addr;
    assert(NULL != transParam);

    /* load the packet to the DMA descriptors                                 */
    if (Upp_ChanSel_A == chanSel)
    {
        instHandle->hwInfo.regs->UPID0 = (Uint32)transParam->windowAddr;
        instHandle->hwInfo.regs->UPID1 =
            (((Uint32)transParam->lineCount << CSL_UPP_UPID1_LNCNT_SHIFT) |
             (transParam->bytesPerLine));
        instHandle->hwInfo.regs->UPID2 = transParam->lineOffset;
    }
    else
    {
        instHandle->hwInfo.regs->UPQD0 = (Uint32)transParam->windowAddr;
        instHandle->hwInfo.regs->UPQD1 =
            (((Uint32)transParam->lineCount << CSL_UPP_UPQD1_LNCNT_SHIFT) |
             (transParam->bytesPerLine));
        instHandle->hwInfo.regs->UPQD2 = transParam->lineOffset;
    }
}

/*============================================================================*/
/*                             PWRM FUNCTIONS                                 */
/*============================================================================*/

/**
 *  \brief    Upp_localLpscOn
 *
 *            This function is invoked to power ON the Upp peripheral and
 *            configure it according to the operating mode. Note that this
 *            function uses different functions to power ON the module,
 *            depending on whether PWRM is enabled in this driver or NOT
 *
 *  \param    instHandle  [IN] Handle to the device instance
 *
 *  \return   IOM_COMPLETED in case of success, an ERROR code in case of error.
 *
 *  \enter    instHandle    must be a valid pointer
 *
 *  \leave    Not Implemented.
 */
Int32 Upp_localLpscOn(Upp_Object *instHandle)
{
    Int32  status = IOM_COMPLETED;

    assert(NULL != instHandle);

    if (TRUE == instHandle->devParams.pscPwrmEnable)
    {
#ifdef BIOS_PWRM_ENABLE
        /* power on using bios PWRM API                                       */
        status = (Int32)Power_setDependency(
                            (Power_Resource)instHandle->hwInfo.pwrmLpscId);
#else
        /* power on using the PSC API                                         */
        status = Psc_ModuleClkCtrl(
                    (Psc_DevId)instHandle->hwInfo.pscInstance,
                    (Uint32)instHandle->hwInfo.pwrmLpscId,
                    TRUE);
#endif
    }
    return (status);
}

/**
 * \brief    Upp_localLpscOff
 *
 *           This function is invoked to power OFF the module.
 *
 * \param    instHandle  [IN] Handle to the device instance
 *
 * \return   IOM_COMPLETED in case of success, an ERROR code in case of error.
 *
 * \enter    instHandle must be a valid channel object
 *           No pending IO
 *
 * \leave    Not Implemented.
 */
Int32 Upp_localLpscOff(Upp_Object *instHandle)
{
    Int32  status = IOM_COMPLETED;

    assert(NULL != instHandle);

    if (TRUE == instHandle->devParams.pscPwrmEnable)
    {
#ifdef BIOS_PWRM_ENABLE
        /* power off  using PWRM API                                          */
        status = (Int32)Power_releaseDependency(
                    (Power_Resource)instHandle->hwInfo.pwrmLpscId);
#else
        /* power off using PSC API only if it is an instance other than "0"   */
        status = Psc_ModuleClkCtrl(
                    (Psc_DevId)instHandle->hwInfo.pscInstance,
                    (Uint32)instHandle->hwInfo.pwrmLpscId,
                    FALSE);
#endif
    }
    return (status);
}

#ifdef BIOS_PWRM_ENABLE
/**
 * \brief     This function registers with the BIOS PWRM module for all the
 *            possible power management events.
 *
 * \param     instHandle [IN] Handle to the driver instance object
 *
 * \return    None
 *
 * \enter     instHandle is a NON null valid pointer
 *
 * \leave     Not implemented
 */
static Int32 uppRegisterNotification(Upp_Object *instHandle)
{
    Uint32       eventCnt = 0x00;
    Power_Status  retVal   = Power_SOK;
    Int32        status   = IOM_COMPLETED;

    assert(NULL != instHandle);

    /* register for the events to be notified by the driver                   */
    for (eventCnt = 0; eventCnt < Upp_MAX_PWRM_EVENTS; eventCnt++)
    {
        /* register the notify function for the PWRM event                    */
        retVal = Power_registerNotify(
                    (Power_Event)eventCnt,
                    NULL,
                    &uppNotifyCallback,
                    (Arg)instHandle,
                    &(instHandle->pwrmInfo.notifyHandle[eventCnt]),
                    &(instHandle->pwrmInfo.delayedCompletionFxn[eventCnt]));

        if (Power_SOK != retVal)
        {
            status = IOM_EBADARGS;
            break;
        }
    }
    return (status);
}

/**
 * \brief     This function unregisters all the power management events
 *            which were previously registered by the driver instance.
 *
 * \param     instHandle [IN] Handle to the driver instance object
 *
 * \return    IOM_COMPLETED in case of success
 *            IOM_EBADARGS  in case of error
 *
 * \enter     instHandle is a NON null valid pointer
 *
 * \leave     Not implemented
 */
static Int32 uppUnregisterNotification(Upp_Object *instHandle)
{
    Uint32       eventCnt = 0x00;
    Power_Status  retVal   = Power_SOK;
    Int32        status   = IOM_COMPLETED;

    assert(NULL != instHandle);

    for (eventCnt = 0; eventCnt < Upp_MAX_PWRM_EVENTS; eventCnt++)
    {
        /* register the notify function for the PWRM event                    */
        if (NULL != (instHandle->pwrmInfo.notifyHandle[eventCnt]))
        {
            retVal = Power_unregisterNotify(
                        (instHandle->pwrmInfo.notifyHandle[eventCnt]));

            if (Power_SOK != retVal)
            {
                status = IOM_EBADARGS;
                break;
            }
            else
            {
                instHandle->pwrmInfo.notifyHandle[eventCnt] = 0x00;
            }
        }
    }
    return (status);
}

/**
 * \brief     This function is the notify function called by the BIOS whenever
 *            a power event occurs in the system.This function handles the
 *            power events and calls the appropriate functions to process the
 *            same.
 *
 * \param     eventType [IN] power event type
 * \param     eventArg1 [IN] event-specific argument
 * \param     eventArg2 [IN] event-specific argument
 * \param     clientArg [IN] argument passed by the PWRM module(usually
 *                          (supplied during the registration of the notifyFxn)
 *
 * \return    None
 *
 * \enter     eventType is a valid event
 *
 * \leave     Not implemented
 */
static Power_NotifyResponse uppNotifyCallback(Power_Event    eventType,
                                             Arg           eventArg1,
                                             Arg           eventArg2,
                                             Arg           clientArg)
{
    Upp_Object            *instHandle = NULL;
    Power_NotifyResponse    pwrmStatus = Power_NOTIFYDONE;
    Power_Status            status     = Power_SOK;

    assert(NULL != clientArg);
    instHandle = (Upp_Object *)clientArg;
    assert(NULL != instHandle);

    /* check the event notified by the PWRM module and process accordingly    */
    switch (eventType)
    {
        case Power_GOINGTOSLEEP:
        case Power_GOINGTOSTANDBY:
        case Power_GOINGTODEEPSLEEP:
            pwrmStatus = Upp_localSuspendCurrentIops(instHandle,eventType);
            break;
        case Power_PENDING_CPU_SETPOINTCHANGE:
        case Power_PENDING_PER_SETPOINTCHANGE:
            /* CPU or PERIPHERAL domain V/F setpoint or a sleep command is    *
             * issued hence we will complete the current IO and suspend all   *
             * the pending IOs                                                */
            if (((Power_PENDING_CPU_SETPOINTCHANGE == eventType) &&
                     (Upp_pllDomain_0 == instHandle->pwrmInfo.pllDomain)) ||
                ((Power_PENDING_PER_SETPOINTCHANGE == eventType) &&
                     (Upp_pllDomain_1 == instHandle->pwrmInfo.pllDomain)))
            {
                pwrmStatus = Upp_localSuspendCurrentIops(instHandle,eventType);
            }
            break;
        case Power_DONE_CPU_SETPOINTCHANGE:
        case Power_DONE_PER_SETPOINTCHANGE:
            /* The pending CPU setpoint change has now been made.Hence now we *
             * will recalculate the new settings for this instance and then   *
             * restore the pending state of the driver. please note that we   *
             * may need to recalculate for both the channels or none depending*
             * on the */
            if (((Power_DONE_CPU_SETPOINTCHANGE == eventType) &&
                     (Upp_pllDomain_0 == instHandle->pwrmInfo.pllDomain)) ||
               ((Power_DONE_PER_SETPOINTCHANGE == eventType) &&
                      (Upp_pllDomain_1 == instHandle->pwrmInfo.pllDomain)))
            {
                pwrmStatus = uppConfigureNewVfSettings(
                                 instHandle,
                                 (Uns)eventArg1,
                                 (Uns)eventArg2);
            }

            if (Power_NOTIFYDONE == pwrmStatus)
            {
                if (Upp_DriverState_OPENED == instHandle->chanA.chanState)
                {
                    Upp_localLoadPendedIops(&instHandle->chanA);
                }

                if (Upp_DriverState_OPENED == instHandle->chanB.chanState)
                {
                    Upp_localLoadPendedIops(&instHandle->chanB);
                }

                instHandle->devState = Upp_DriverState_CREATED;
            }
            break;
        case Power_AWAKEFROMSLEEP:
        case Power_AWAKEFROMDEEPSLEEP:
            /* restore the dependency count                                   */
            while (instHandle->pwrmInfo.dependencyCount > 0)
            {
                status = Power_setDependency(
                             (Power_Resource)instHandle->hwInfo.pwrmLpscId);

                if (Power_SOK != status)
                {
                    break;
                }
                instHandle->pwrmInfo.dependencyCount--;
            }

            if (Power_SOK == status)
            {
                /* enable the peripheral now                                  */
                instHandle->hwInfo.regs->UPPCR |= (CSL_UPP_UPPCR_EN_MASK);

                if (Upp_DriverState_OPENED == instHandle->chanA.chanState)
                {
                    Upp_localLoadPendedIops(&instHandle->chanA);
                }

                if (Upp_DriverState_OPENED == instHandle->chanB.chanState)
                {
                    Upp_localLoadPendedIops(&instHandle->chanB);
                }

                instHandle->devState = Upp_DriverState_CREATED;
            }
            else
            {
                pwrmStatus = Power_NOTIFYNOTDONE;
            }
            break;
        case Power_AWAKEFROMSTANDBY:
            /* enable the peripheral now                                      */
            instHandle->hwInfo.regs->UPPCR |= (CSL_UPP_UPPCR_EN_MASK);

            if (Upp_DriverState_OPENED == instHandle->chanA.chanState)
            {
                Upp_localLoadPendedIops(&instHandle->chanA);
            }

            if (Upp_DriverState_OPENED == instHandle->chanB.chanState)
            {
                Upp_localLoadPendedIops(&instHandle->chanB);
            }

            instHandle->devState = Upp_DriverState_CREATED;
            break;
        default:
            break;
    }
    return (pwrmStatus);
}

/**
 * \brief     This function calculates the constraints for a device at all
 *            the setpoints supported for the SOC. it then registers the
 *            appropriate contraints for the setpoints not supported.
 *
 * \param     chanHandle [IN] Handle to the channel.
 *
 * \return    IOM_COMPLETED in case of success.
 *            IOM_EBADMODE  in case of error.
 *
 * \enter     instHandle is a NON null valid pointer
 */
static Int32 uppCalculateConstraints(Upp_ChanObj *chanHandle)
{
    Upp_Object     *instHandle      = NULL;
    Uns             numberSetpoints = 0x00;
    Uns             setpoint        = 0x00;
    Uint32          prescale        = 0x00;
    Uint32          mask            = 0x00;
    Int32           status          = IOM_COMPLETED;
    Power_Constraint constraint      = Power_DISALLOWEDSLEEPSTATE_MASK;
    Power_Status     retVal          = Power_SOK;
    Power_Domain     domain          = Power_CPU;

    assert(NULL != chanHandle);
    assert(NULL != chanHandle->devHandle);

    instHandle = (Upp_Object *)chanHandle->devHandle;
    assert(NULL != instHandle);

    /* check what clock domain is supported by the device                     */
    if (Upp_pllDomain_0 == instHandle->pwrmInfo.pllDomain)
    {
        domain = Power_CPU;
        constraint = Power_DISALLOWED_CPU_SETPOINT_MASK;
    }
    else
    {
        domain = Power_PER;
        constraint = Power_DISALLOWED_PER_SETPOINT_MASK;
    }

    /* get the number of supported setpoint info for this instance            */
    retVal = Power_getNumSetpoints(domain, &numberSetpoints);

    if (Power_SOK == retVal)
    {
        /* calculate the constraints for the required bus frequency           */
        for (setpoint = 0x00; setpoint < numberSetpoints; setpoint++)
        {
            status = Upp_localCalculateClockSettings(
                         instHandle,
                         setpoint,
                         chanHandle->chanParams.opFreq,
                         &prescale);

            if (IOM_COMPLETED == status)
            {
                /* successfully calculated now check if the prescaler is OK   */
                if (prescale > 0x0F)
                {
                    /* prepare the constraint mask  for this setpoint         */
                    mask |= (1 << setpoint);
                }
            }
            else
            {
                retVal = (Uint32)Power_EINVALIDVALUE;
                break;
            }
        }

        /* check if this instance has already registered some constraint      *
         * if so then unregister those before registering the new             *
         * contraints(for CPU or PER event's unsupported DVFS only)           */
        if (Power_SOK == retVal)
        {
            if (NULL != instHandle->pwrmInfo.constraintHandle[
                            chanHandle->chanParams.chanSel][constraint - 1])
            {
                retVal = Power_unregisterConstraint(
                              instHandle->pwrmInfo.constraintHandle[
                                chanHandle->chanParams.chanSel][constraint - 1]);
            }

            /* register the new set of constraints (only if any contraints    *
             * are present)                                                   */
            if ((Power_SOK == retVal) && (0 != mask))
            {
                /* store the value of the constraint mask being registered    */
                instHandle->pwrmInfo.constraintMask[
                    chanHandle->chanParams.chanSel] = mask;

                /* register the constraint with the PWRM module               */
                retVal = Power_registerConstraint(
                             constraint,
                             mask,
                             &instHandle->pwrmInfo.constraintHandle[
                                chanHandle->chanParams.chanSel][constraint - 1]);
            }
        }
    }

    if (Power_SOK != retVal)
    {
        /* failed in calculating the constraints                              */
        status = IOM_EBADMODE;
    }
    return (status);
}


/**
 * \brief     This function unregisters all the constraints for the channel
 *            instance.
 *
 * \param     chanHandle [IN]  Handle to the channel
 *
 * \return    IOM_COMPLETED in case of success
 *            IOM_EBADMODE  in case of failure
 *
 * \enter     chanHandle is a valid non null pointer
 *
 * \leave     Not implemented
 */
static Int32 uppUnregisterConstraints(Upp_ChanObj *chanHandle)
{
    Upp_Object  *instHandle = NULL;
    Int32        status     = IOM_COMPLETED;
    Power_Status  retVal     = Power_SOK;
    Uint32       count      = 0x00;

    assert((NULL != chanHandle) && (NULL != chanHandle->devHandle));
    instHandle = (Upp_Object *)chanHandle->devHandle;
    assert(NULL != instHandle);

    for (count = 0; count < Upp_MAX_PWRM_CONSTRAINTS; count++)
    {
        if (NULL != instHandle->pwrmInfo.constraintHandle[
                        chanHandle->chanParams.chanSel][count])
        {
            retVal = Power_unregisterConstraint(
                         instHandle->pwrmInfo.constraintHandle[
                            chanHandle->chanParams.chanSel][count]);

            if (Power_SOK != retVal)
            {
                status = IOM_EBADMODE;
                break;
            }
            else
            {
                instHandle->pwrmInfo.constraintHandle[
                    chanHandle->chanParams.chanSel][count] = 0x00;
            }
        }
    }
    return (status);
}


/**
 * \brief     This function calculates the prescaler values for the required
 *            output frequency for a given setpoint.
 *
 * \param     instHandle [IN]  Handle to the driver instance object
 * \param     setpoint   [IN]  setpoint for which the prescaler is calculated.
 * \param     opFreq     [IN]  desired module IO frequency
 * \param     clkdiv     [OUT] calculated divisor value.
 *
 * \return    IOM_COMPLETED if the calculation is completed.
 *            IOM_EBADMODE  if the calculation failed for some reason.
 *
 * \enter     instHandle is a valid non null pointer
 *            setpoint   is a valid setpoint for this SoC.
 *            prescalar  is a non null pointer.
 *
 * \leave     Not implemented
 *
 * \note      1. Please note that the calculated divisor values are not checked
 *               in this function.
 *            2. Please note that this function should be called when the
 *               pllDomain is NOT configured as "Upp_pllDomain_NONE"
 */
Int32 Upp_localCalculateClockSettings(Upp_Object *instHandle,
                                      Uint32      setpoint,
                                      Uint32      opFreq,
                                      Uint32     *clkdiv)
{
    Power_Status     retVal          = Power_SOK;
    Power_Domain     domain          = Power_CPU;
    Uns             frequency       = 0x00;
    Uns             voltage         = 0x00;
    Uint32          sysclkDiv       = 0x00;
    Int32           status          = IOM_COMPLETED;

    assert((NULL != instHandle) && (NULL != clkdiv));

    /* check what clock domain is supported by the device                     */
    if (Upp_pllDomain_0 == instHandle->pwrmInfo.pllDomain)
    {
        domain = Power_CPU;
    }
    else if (Upp_pllDomain_1 == instHandle->pwrmInfo.pllDomain)
    {
        domain = Power_PER;
    }
    else
    {
        status = IOM_EBADMODE;
    }

    /* calculate the sysclk divisor value                                     */
    sysclkDiv = (CSL_SYSCLK_1_FREQ / instHandle->devParams.inputFreq);

    /* get the information about the current set point                        */
    retVal = Power_getSetpointInfo(
                 domain,
                 setpoint,
                 &frequency,
                 &voltage);

    if (Power_SOK == retVal)
    {
        /* calculate the new prescaler values                                 */
        (*clkdiv) = (((frequency * 1000u)/sysclkDiv)/ (2u * (opFreq))) - 1u;
    }

    if (Power_SOK != retVal)
    {
        status = IOM_EBADMODE;
    }
    return (status);
}

/**
 * \brief   This function handles the power events related to the Upp.it
 *          reduces the dependency count and stops the further IO from being
 *          programmed.
 *
 * \param   instHandle [IN] Handle to the device instance.
 * \param   chanHandle [IN] Handle to the channel
 *
 * \return  None
 */
static Void uppHandlePwrmEvent(Upp_Object   *instHandle)
{
    Int32   retVal = IOM_COMPLETED;
    Uint32  count  = 0x00;

    assert(NULL != instHandle);

    /* check if the driver is waiting to go to sleep                          */
    if (TRUE == instHandle->pwrmInfo.ioSuspend)
    {
        /* Set the driver state to PWRM suspend so that no more IO can be     *
         * programmed                                                         */
        instHandle->devState = Upp_DriverState_PWRM_SUSPEND;

        if ((TRUE == Queue_empty(&instHandle->chanA.queActiveList)) &&
            (TRUE == Queue_empty(&instHandle->chanB.queActiveList)))
        {
            /* if both the channels are inactive then reset the io suspend    */
            instHandle->pwrmInfo.ioSuspend = FALSE;

            if ((Power_GOINGTOSLEEP == instHandle->pwrmInfo.pwrmEvent) ||
                 (Power_GOINGTODEEPSLEEP == instHandle->pwrmInfo.pwrmEvent))
            {
                 /* reduce the dependency count                               */
                 retVal = (Int32)Power_getDependencyCount(
                                 (Power_Resource)instHandle->hwInfo.pwrmLpscId,
                                  &count);

                 instHandle->pwrmInfo.dependencyCount = count;

                 if (Power_SOK == retVal)
                 {
                     while (count > 0)
                     {
                         retVal = (Int32)Power_releaseDependency(
                             (Power_Resource)instHandle->hwInfo.pwrmLpscId);

                         if (Power_SOK != retVal)
                         {
                             break;
                         }
                         count--;
                     }
                 }
            }

            /* disable the UPP device                                         */
            instHandle->hwInfo.regs->UPPCR &= (~CSL_UPP_UPPCR_EN_MASK);

            if (TRUE != instHandle->uppSuspend)
            {
                /* call the delayed completion callback only if the device is *
                 * not in suspend due to IOCTL                                */
                /* call the delayed completion function                       */
                (instHandle->pwrmInfo.delayedCompletionFxn  \
                    [instHandle->pwrmInfo.pwrmEvent])();
            }
        }
    }
}

/**
 * \brief      This function calculates clock settings for the new DVFS set
 *             point.
 *
 * \param      instHandle [IN] Handle to the device instance object
 * \param      eventArg1  [IN] previous set point.
 * \param      eventArg2  [IN] current set point
 *
 * \return     Power_SOK incase that the prescaler values for the new set point
 *             are calculated successfully else an appropriate error code is
 *             returned
 *
 * \enter      instHandle is a valid non NULL pointer
 *
 * \leave      Not imeplemented
 */
static Power_NotifyResponse uppConfigureNewVfSettings(Upp_Object  *instHandle,
                                                     Uns          eventArg1,
                                                     Uns          eventArg2)
{
    Power_NotifyResponse  retVal      = Power_NOTIFYDONE;
    Uint32               clkDivA     = 0x00;
    Uint32               clkDivB     = 0x00;
    volatile Uint32      timeOut     = 0x00;
    Int32                status      = IOM_COMPLETED;

    /* This function will calculate the settings for the new set point and    *
     * also indicate if this is a valid setpoint                              */
    assert(NULL != instHandle);

    /* check if both the set points are not the same                          */
    if (eventArg1 != eventArg2)
    {
        /* The DVFS set point change has happened successfully.calculate the  *
         * new prescaler value for both the channels(only if they are in TX   *
         * mode)                                                              */
        if (IOM_OUTPUT == instHandle->chanA.mode)
        {
            status = Upp_localCalculateClockSettings(
                         instHandle,
                         eventArg2,
                         instHandle->chanA.chanParams.opFreq,
                         &clkDivA);

            if ((IOM_COMPLETED == status)  && (clkDivA <= 0xF))
            {
                instHandle->upicrRegVal &= (~CSL_UPP_UPICR_CLKDIVA_MASK);
                instHandle->upicrRegVal |=
                    (clkDivA << CSL_UPP_UPICR_CLKDIVA_SHIFT);
            }
            else
            {
                status = IOM_EBADARGS;
            }
        }

        /* calculate the clkdiv values for the channel B                      */
        if (IOM_COMPLETED == status)
        {
            if (IOM_OUTPUT == instHandle->chanB.mode)
            {
                status = Upp_localCalculateClockSettings(
                             instHandle,
                             eventArg2,
                             instHandle->chanB.chanParams.opFreq,
                             &clkDivB);

                if ((IOM_COMPLETED == status)  && (clkDivB <= 0xF))
                {
                    instHandle->upicrRegVal &= (~CSL_UPP_UPICR_CLKDIVB_MASK);
                    instHandle->upicrRegVal |=
                        (clkDivB << CSL_UPP_UPICR_CLKDIVB_SHIFT);
                }
                else
                {
                    status = IOM_EBADARGS;
                }
            }
        }

        if (IOM_COMPLETED == status)
        {
            /* reprogram the clkdiv values to the UPICR register              *
             * place the device in software reset and then bring it out of    *
             * reset after waiting for 200CLK cycles                          */
            instHandle->hwInfo.regs->UPPCR |= CSL_UPP_UPPCR_SWRST_MASK;

            /* reinitialize the retry count                                   */
            timeOut = instHandle->retryCount;

            /* wait for 200CLK cycles                                         */
            while (timeOut > 0)
            {
                timeOut--;
            }

            /* bring the device out of reset                                  */
            instHandle->hwInfo.regs->UPPCR &= (~CSL_UPP_UPPCR_SWRST_MASK);

            /* update the UPICR register                                      */
            instHandle->hwInfo.regs->UPICR = instHandle->upicrRegVal;
        }
    }

    /* enable the peripheral now                                              */
    instHandle->hwInfo.regs->UPPCR |= (CSL_UPP_UPPCR_EN_MASK);

    if (IOM_COMPLETED != status)
    {
        retVal = Power_NOTIFYERROR;
    }
    return (retVal);
}
#endif /* ifdef BIOS_PWRM_ENABLE */

/**
 * \brief    This function configures the driver so that the current IOP can be
 *           completed and then supends all the remaining IOPs.
 *
 * \param    instHandle [IN] Handle to the device instance object
 * \param    eventType  [IN] event which has caused this notification call to
 *                           driver
 *
 * \return   Power_NOTIFYDONE    if no IOPs are pending
 *           Power_NOTIFYNOTDONE if IOPs are pending in the driver.
 */
Power_NotifyResponse Upp_localSuspendCurrentIops(Upp_Object  *instHandle,
                                                Power_Event   eventType)
{
    Uint32               hwiKey = 0x00;
    Power_NotifyResponse  status = Power_NOTIFYDONE;

    assert(NULL != instHandle);

    hwiKey = (Uint32)_disable_interrupts();

    if ((TRUE == Queue_empty(&instHandle->chanA.queActiveList)) &&
        (TRUE == Queue_empty(&instHandle->chanB.queActiveList)))
    {
        /* No IOP is pending currently. we can return the status of the       *
         * notify call as sucess                                              */
        instHandle->devState = Upp_DriverState_PWRM_SUSPEND;

        /* disable the UPP device                                             */
        instHandle->hwInfo.regs->UPPCR &= (~CSL_UPP_UPPCR_EN_MASK);

        status = Power_NOTIFYDONE;
    }
    else
    {
        /* There are some IOPs pending,hence we will complete processing      *
         * of this request in the ISR context                                 *
         * by calling the delayed completion function                         */
        status = Power_NOTIFYNOTDONE;
        instHandle->devState = Upp_DriverState_PWRM_SUSPEND;
        instHandle->pwrmInfo.ioSuspend = TRUE;
        instHandle->pwrmInfo.pwrmEvent = eventType;
    }

    _restore_interrupts(hwiKey);
    return (status);
}

/**
 * \brief     This function restores the driver to the original state that is it
 *            resumes the normal operation of the driver by picking the IOPs
 *            from the pending queue and putting it to the active IOP.
 *
 * \param     instHandle [IN] Handle to the device instance object
 *
 * \enter     instHandle is a valid non NULL pointer
 *
 * \leave     Not impplemented
 */
Int32 Upp_localLoadPendedIops(Upp_ChanObj *chanHandle)
{
    Upp_Object           *instHandle      = NULL;
    IOM_Packet           *ioPacket        = NULL;
    Bool                  pend            = FALSE;
    Int32                 status          = IOM_COMPLETED;
    Uint32                count           = 0x00;
    Uint32                hwiKey          = 0x00;

    assert((NULL != chanHandle) && (NULL != chanHandle->devHandle));
    instHandle = (Upp_Object *)chanHandle->devHandle;
    assert(NULL != instHandle);

    hwiKey = (Uint32)_disable_interrupts();

    /* disable the interrupts for the uPP device                              */
    Upp_localDisableInterrupts(instHandle,chanHandle);

    for (count = 0; count < (Upp_MAXLINKCNT); count++)
    {
        if (FALSE == Queue_empty(&chanHandle->quePendList))
        {
            if (Upp_ChanSel_A == chanHandle->chanParams.chanSel)
            {
                if (CSL_UPP_UPIS2_PEND_MASK ==
                        (instHandle->hwInfo.regs->UPIS2 & CSL_UPP_UPIS2_PEND_MASK))
                {
                    pend = TRUE;
                }
            }
            else
            {
                if (CSL_UPP_UPQS2_PEND_MASK ==
                    (instHandle->hwInfo.regs->UPQS2 & CSL_UPP_UPQS2_PEND_MASK))
                {
                    pend = TRUE;
                }
            }

            if (FALSE == pend)
            {
                ioPacket = Queue_get(&chanHandle->quePendList);

                /* descriptors need to be loaded in to the DMA                */
                Queue_put(&(chanHandle->queActiveList),ioPacket);

                uppLoadPktToDma(
                    instHandle,
                    chanHandle->chanParams.chanSel,
                    ioPacket);
            }
        }
    }

    Upp_localEnableInterrupts(instHandle,chanHandle);

    /* exit the critical section code                                         */
    _restore_interrupts(hwiKey);

    return (status);
}

/* ========================================================================== */
/*                             END OF FILE                                    */
/* ========================================================================== */

