/*
 * UppLocal.h
 *
 * UPP Driver internal header file
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
 *  \file     UppLocal.h
 *
 *  \brief    Header file conataining the driver internal definitions and data
 *            structures.
 *
 *            (C) Copyright 2009, Texas Instruments, Inc
 *
 *  \author   Platform Support Group
 *
 *  \version  0.2
 *            \author Imtiaz SMA
 *            \date   Dec 14 2009
 *
 *            created by moving local definitions to a new file from original
 *            Upp_md.h
 *
 *  \version  0.1
 *            \author Joe Coombs
 *            \date   Aug 05 2009
 *
 *            Initial version
 */

#ifndef _UPPLOCAL_H_
#define _UPPLOCAL_H_

#if !defined(CHIP_C6748) && !defined(CHIP_OMAPL138)
#error "No chip type defined! (Must use -DCHIP_C6748 or -DCHIP_OMAPL138)"
#endif

/*============================================================================*/
/*                            INCLUDE FILES                                   */
/*============================================================================*/

#include <xdc/std.h>
#include <ti/sysbios/io/iom.h>
#include <ti/sysbios/knl/Queue.h>
#include "Upp.h"
#include "cslr/cslr_upp.h"

#ifdef CHIP_C6748
    #include "cslr/soc_C6748.h"
#endif

#ifdef CHIP_OMAPL138
    #include "ti/pspiom/cslr/soc_OMAPL138.h"
#endif

//#if ((defined(CHIP_C6748) || defined(CHIP_OMAPL138)) && (!defined(BIOS_PWRM_ENABLE)))
//#error "Use Bios PWRM module only for the SoC"
//#else
//    #include <pwrm.h>
//#endif
#ifdef BIOS_PWRM_ENABLE
#include <ti/sysbios/family/c674/Power.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*                            MACRO DEFINTIONS                                */
/*============================================================================*/

#define Upp_MAXLINKCNT                 (2u)
/**< Maximum number of DMA jobs loaded at a time (Must be 2).                 */

#define Upp_INTERLEAVED_MAXLINKCNT     (4u)
/**< Maximum number of DMA jobs loaded at a time (Must be 4).                 */


#define Upp_MAX_PWRM_EVENTS            (Power_INVALIDEVENT)
/**< Max number of PWRM events for which the mcasp will register              */

#define Upp_MAX_PWRM_CONSTRAINTS       (Power_DISALLOWEDSLEEPSTATE_MASK)
/**< Max number of constraints for which a moudule can register               */

#define Upp_MASK_CHAN_A                (CSL_UPP_UPIER_EOLI_MASK | \
                                        CSL_UPP_UPIER_EOWI_MASK | \
                                        CSL_UPP_UPIER_ERRI_MASK | \
                                        CSL_UPP_UPIER_UORI_MASK | \
                                        CSL_UPP_UPIER_DPEI_MASK)
/**< Mask of all interrupts for the channel A                                 */


#define Upp_PKT_COMPLETION_MASK_CHAN_A (CSL_UPP_UPIER_EOWI_MASK | \
                                        CSL_UPP_UPIER_ERRI_MASK | \
                                        CSL_UPP_UPIER_UORI_MASK | \
                                        CSL_UPP_UPIER_DPEI_MASK)
/**< Mask used to compare and complete the processing of IOP                  */

#define Upp_PKT_LOAD_MASK_CHAN_A       (CSL_UPP_UPIER_EOWI_MASK | \
                                        CSL_UPP_UPIER_EOLI_MASK)
/**< mask used to compare and load a new packet to DMA                        */


#define Upp_MASK_CHAN_B                (CSL_UPP_UPIER_EOLQ_MASK | \
                                        CSL_UPP_UPIER_EOWQ_MASK | \
                                        CSL_UPP_UPIER_ERRQ_MASK | \
                                        CSL_UPP_UPIER_UORQ_MASK | \
                                        CSL_UPP_UPIER_DPEQ_MASK)
/**< Mask of all interrupts for the channel B                                 */



#define Upp_PKT_COMPLETION_MASK_CHAN_B (CSL_UPP_UPIER_EOWQ_MASK | \
                                        CSL_UPP_UPIER_ERRQ_MASK | \
                                        CSL_UPP_UPIER_UORQ_MASK | \
                                        CSL_UPP_UPIER_DPEQ_MASK)
/**< Mask used to compare and complete the processing of IOP                  */

#define Upp_PKT_LOAD_MASK_CHAN_B       (CSL_UPP_UPIER_EOWQ_MASK | \
                                        CSL_UPP_UPIER_EOLQ_MASK)
/**< mask used to compare and load a new packet to DMA                        */

/*============================================================================*/
/*                         ENUMERATED DATA TYPES                              */
/*============================================================================*/

/**
 *  \brief Upp driver state
 *
 *   Upp driver state enums used to track the driver and channel state.
 */
typedef enum Upp_DriverState_t
{
    Upp_DriverState_DELETED,
    Upp_DriverState_CREATED,
    Upp_DriverState_INITIALIZED,
    Upp_DriverState_OPENED,
    Upp_DriverState_CLOSED,
    Upp_DriverState_DEINITIALIZED,
    Upp_DriverState_POWERED_DOWN,
    Upp_DriverState_PWRM_SUSPEND
}Upp_DriverState;
/**< Upp driver and channel states                                            */

/*============================================================================*/
/*                              DATA STRUCTURES                               */
/*============================================================================*/

/**
 * \brief Upp Hardware specific information Object
 *
 *  This structure maintains the information specific to the hardware instance
 *  of the UPP. information like the base address and the cpu event numbers
 *  and DMA events is specific to the instance. This structure is a collection
 *  of such information.
 */
typedef struct Upp_HwInfo_t
{
    Uint32               instNum;
    /**< Instance of MCBSP being referred by this object                      */

    CSL_UppRegsOvly      regs;
    /**< Pointer to the register overlay structure of the MCBSP               */

    Uint16               cpuEventNum;
    /**< UPP interrupt number                                                 */

    Uint32               moduleClkFreq;
    /**< Module Clock Frequency                                               */

    Uint32               pwrmLpscId;
    /**< LPSC number for this instance                                        */

    Uint32               pscInstance;
    /**< Instance of the PSC which controls this module's instance            */

}Upp_HwInfo;
/**< Upp Hardware specific information Object                                 */

#ifdef BIOS_PWRM_ENABLE
/**
 * \brief   Structure to hold the information related to the PWRM.
 *
 * This data structure holds the data related to the power management like the
 * constraint mask,notification Handles etc.
 */
typedef struct Upp_pwrmInfo_t
{
    Upp_pllDomain          pllDomain;
    /**< Pll domain to be used for the device                                 */

    Bool                   ioSuspend;
    /**< whether the driver is in IO suspend mode (PWRM change is in progress)*/

    Power_Event             pwrmEvent;
    /**< Current pwrm event being processed                                   */

    Uint32                 constraintMask[2];
    /**< Constraint mask (for information only)                               */

    Bool                   constraintRegistered;
    /**< whether the contraint is registered for this module                  */

    Uint32                 dependencyCount;
    /**< Current dependency count when the driver is going for sleep or
     * deepsleep                                                              */

    Power_NotifyHandle      notifyHandle[Upp_MAX_PWRM_EVENTS];
    /**< Handles required for unregistering of the events with PWRM           */

    Fxn                    delayedCompletionFxn[Upp_MAX_PWRM_EVENTS];
    /**< delayed completion callback function pointer                         */

    Power_ConstraintHandle  constraintHandle[2][Upp_MAX_PWRM_CONSTRAINTS];
    /**< Handles required for unregistering of the constraints with PWRM      */

}Upp_pwrmInfo;
/**< Power management object structure                                        */
#endif

/**
 * \brief   Upp channel object
 *
 *  This structure defines the channel object for the UPP driver.each channel
 *  object refers to a physical channel on the UPP device.it holds all the
 *  channel specific information like bit width,threshold etc
 */
typedef struct Upp_ChanObj_t
{
    Int16                      mode;
    /**< mode of operation of the channel (IOM_INPUT/IOM_OUTPUT)              */

    Upp_DriverState            chanState;
    /**< operational state of the channel (opened/closed)                     */

    volatile Bool              flushAbort;
    /**< Flag to indicate if the channel is currently aborting/flushing
     * requests                                                               */

    Ptr                        devHandle;
    /**< Pointer to McBSP device driver instance object                       */

    IOM_TiomCallback           cbFxn;
    /**< IOM call back function to be called once a packet is processed       */

    Arg                        cbArg;
    /**< Callback Function argument                                           */

    Uint32                     submitCount;
    /**< No of packet currently held by the channel                           */

    Queue_Struct                    queActiveList;
    /**< Queue to handle the active IOPs                                      */

    Queue_Struct                    quePendList;
    /**< Queue to handle the pending IOPs                                     */

    Upp_ChanParams             chanParams;
    /**<  Channel params supplied by the user                                 */

#ifdef Upp_STATISTICS_ENABLE
    Upp_devStats               stats;
    /* channel error /status statistics                                       */
#endif
}Upp_ChanObj;
/**< Upp channel object                                                       */

/**
 * \brief   Upp instance Object
 *
 * \note    This data structure holds the information pertaining to an instance
 *          of the Upp device.it holds information like the current device
 *          state, handle to the UPP channels. The data structure is
 *          initialized during "mdBindDev", which is called during DSP-BIOS
 *          initialization, and is persistent till it is invalidated by
 *          "mdUnBindDev".
 */
typedef struct Upp_Object_t
{
    Int32                     instNum;
    /**< Instance number of the current instance                              */

    Upp_DriverState           devState;
    /**< operational state of the driver (created/deleted)                    */

    Upp_Params                devParams;
    /**< structure to store the device creation parameters                    */

    Upp_HwInfo                hwInfo;
    /**< UPP handle for initial port configuration                            */

    Uint32                    retryCount;
    /**< Retry count to be used by the driver for timeout operations          */

    Upp_ChanObj               chanA;
    /**< channel object for the hardware channel A                            */

    Upp_ChanObj               chanB;
    /**< channel object for the hardware channel B                            */

    Uint32                    upicrRegVal;
    /**< value to be written to the UPICR register                            */

    Bool                      isrRegistered;
    /**< variable to indicate if the interrupt handler is registered          */

    volatile Bool             uppSmStateStop;
    /**< Flag to indicate if the upp state machine is stopped or enabled.
     *   (TRUE = stopped, FALSE = state machine running properly)             */

    volatile Bool             uppSuspend;
    /**< Variable to indicate if the upp is in suspend mode                   */

#ifdef BIOS_PWRM_ENABLE
    Upp_pwrmInfo              pwrmInfo;
    /**< structure to hold the PWMR related information                       */
#endif

}Upp_Object;
/**< Upp instance Object                                                      */

/* ---------------------------- module-wide state --------------------------- */
/**
 * \brief  Module-wide state
 *
 *         inUse in module state points to an array whose size is soc specific.
 *         Pointer used to match IDriver codebase.
 *
 *         This variable indicates if an instance is already in use or can be
 *         used by the application
 */
typedef struct Upp_Module_State_t
{
    Bool             *inUse;
    /**< Variable to indicate if the instance is in use or not                */
}Upp_Module_State;
/**< Module-wide state structure                                              */

/*============================================================================*/
/*                          MODULE WIDE FUNCTIONS                             */
/*============================================================================*/

Int32 Upp_localLpscOn(Upp_Object *instHandle);
Int32 Upp_localLpscOff(Upp_Object *instHandle);
Int32 Upp_localSubmitIoctl(Upp_ChanObj  *chanHandle,
                           Upp_Ioctl     cmd,
                           Ptr           arg,
                           Ptr           param);
Void Upp_localAbortChan(Upp_ChanObj *chanHandle);
Void Upp_localConfigureChanA(Upp_ChanObj *chanHandle);
Void Upp_localConfigureChanB(Upp_ChanObj *chanHandle);
Int32 Upp_localCalculateClockSettings(Upp_Object *instHandle,
                                      Uint32      setpoint,
                                      Uint32      opFreq,
                                      Uint32     *clkdiv);
Power_NotifyResponse Upp_localSuspendCurrentIops(Upp_Object  *instHandle,
                                                Power_Event   eventType);
Int32 Upp_localLoadPendedIops(Upp_ChanObj *chanHandle);
Void Upp_localDisableInterrupts(Upp_Object  *instHandle,
                                Upp_ChanObj *chanHandle);
Void Upp_localEnableInterrupts(Upp_Object  *instHandle,
                               Upp_ChanObj *chanHandle);
Void Upp_localValidateChanParams(const Upp_Object     *instHandle,
                                 const Upp_ChanParams *chanParams);

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif /* _UPPLOCAL_H_ */
/*============================================================================*/
/*                         END OF FILE                                        */
/*============================================================================*/
