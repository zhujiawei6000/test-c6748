/*
 * Upp.h
 *
 * This file contains Application programming interface for the UPP driver and
 * command/macro definitions used by the UPP driver.
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
 * \file        Upp.h
 *
 * \brief       UPP driver interface definition file
 *
 *              This file contains the interfaces, data types and symbolic
 *              definitions that are needed by the application to utilize the
 *              services of the UPP IOM driver.
 *
 *              (C) Copyright 2009, Texas Instruments, Inc
 *
 *  \author     Platform Support Group
 *
 *  \version    0.2
 *              \author Imtiaz SMA
 *              \date   Dec 14 2009
 *
 *              1. Changes for PSP coding guidelines.
 *              2. New dev and chan param structure.
 *              3. Concept of multiple independent channels introduced.
 *              4. power management support added.
 *
 *  \version    0.1
 *              \author Joe Coombs
 *              \date   Aug 05 2009
 *
 *              Initial version
 *
 */

#ifndef _UPP_H_
#define _UPP_H_

/*============================================================================*/
/*                               INCLUDE FILES                                */
/*============================================================================*/

#include <xdc/std.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*                               MACRO DEFINITIONS                            */
/*============================================================================*/

/**
 * \def    Upp_POLLED_RETRYCOUNT
 *         This macro specifies the default retry count to be used by the UPP
 *         driver when polling for any hardware bit to be set or reset.
 *
 * \note   This default value used by the driver can be changed by an IOCTL
 *         "Upp_Ioctl_SET_TIMEOUT".
 */
#define Upp_POLLED_RETRYCOUNT   (0xFFFFu)


/*============================================================================*/
/*                           ENUMERATED DATA TYPES                            */
/*============================================================================*/

/**
 * \brief   Upp Emulation mode settings
 *          Use this symbol to set the Emulation Mode to FREE/SOFT mode or any
 *          other emulation mode.
 *
 * \note    The SOFT mode settings are applicable only is the FREE mode is
 *          disabled.Hence if the soft mode is selected (both enable or disable)
 *          the driver will internally disable the FREE mode.DO NOT CHANGE
 *          THESE VALUES as they directly correspond to the bit values to be
 *          written in to the register.
 */
typedef enum Upp_EmuMode_t /* TBD: review these settings */
{
    Upp_EmuMode_FREE        = (1u),
    /**< FREE mode is enabled              */

    Upp_EmuMode_SOFT_STOP   = (2u),
    /**< SOFT mode is enabled              */

    Upp_EmuMode_RTEMU       = (7u)
    /**< Real time emulation mode enabled  */

}Upp_EmuMode;
/**< UPP device emulation mode settings                                       */

/**
 * \brief  UPP Digital loopback mode selection
 *         Enum to select the enable/disable of Loopback mode of the UPP.
 *         Application can use this enum in the devParams to specify if the
 *         device to be created is to be operated in the loopback mode.
 *
 * \note   Please note that the channels have to be created appropriately for
 *         the loopback mode to work properly.
 */
typedef enum Upp_Loopback_t
{
    Upp_Loopback_DISABLE   = (0u),
    /**< loopback mode off                 */

    Upp_Loopback_ENABLE_AB = (1u),
    /**< A-to-B digital loopback enabled   */

    Upp_Loopback_ENABLE_BA = (2u)
    /**< B-to-A digital loopback enabled   */
}Upp_Loopback;
/**< UPP digital loopback mode selection   */

/**
 * \brief   UPP device operational mode.
 *          This enum is used to define the operational mode of the Upp device
 *          like all receive,all transmit or duplex mode selection.
 *
 * \note    it is important to note that the operational mode is used combined
 *          with other options like data rate etc.Hence any mismatch in the
 *          options specified during the creation of the channel will result
 *          in failure of channel creation.DO NOT CHANGE THESE VALUES as they
 *          directly correspond to the bit values to be written in to the
 *          register.
 */
typedef enum Upp_DevMode_t
{
    Upp_DevMode_CHAN_A_RCV      = (0u),
    /**< Option to operate one channels in receive mode                       */

    Upp_DevMode_CHAN_A_XMT      = (1u),
    /**< Option to operate one channels in transmit mode                      */

    Upp_DevMode_CHAN_AB_RCV     = (4u),
    /**< Option to operate both channels in receive mode                      */

    Upp_DevMode_CHAN_AB_XMT     = (5u),
    /**< Option to operate both channels in transmit mode                     */

    Upp_DevMode_DUPLEX_0        = (6u),
    /**< Option to operate in Duplex 0 mode Chan A = RCV, chan B = XMT        */

    Upp_DevMode_DUPLEX_1        = (7u)
    /**< Option to operate in Duplex 1 mode Chan A = XMT, chan B = RCV        */

}Upp_DevMode;
/**< Upp device operational mode.                                             */

/**
 * \brief   The mode of operation of the channel.
 *          This enum is used to specify if the channel is to be operated in
 *          special mode (interleaved mode) or in the normal mode of operation.
 *
 * \note    please note that the special modes required  proper configuration of
 *          the channels and also impose certain limit on the channel creation.
 *          Refer to the SPRUJ5A for more details.
 */
typedef enum Upp_ChanMode_t
{
    Upp_ChanMode_NORMAL               = (0u),
    /**< Normal mode of operation                    */

    Upp_ChanMode_TX_INTERLEAVE_ENABLE = (1u),
    /**< Single data rate transmit interleave mode   */

    Upp_ChanMode_DEMULTIPLEX_ENABLE   = (2u)
    /**< Double data rate demultiplexing mode        */

}Upp_ChanMode;
/**< channel operational mode                        */


/**
 * \brief   Threshold values for the FIFO.
 *          This enum is used to specify the FIFO threshold values for the UPP.
 *
 * \note    Please note that all the threshold values may not be configurable,
 *          and also there might be restriction depending on the mode of
 *          operation.
 */
typedef enum Upp_fifoThreshold_t
{
    Upp_fifoThreshold_64   = (0u),
    /**< FIFO threshold of 64                       */

    Upp_fifoThreshold_128  = (1u),
    /**< FIFO threshold of 128                      */

    Upp_fifoThreshold_256  = (3u)
    /**< FIFO threshold of 256                      */

}Upp_fifoThreshold;
/**< Threshold values for the FIFO                  */

/**
 * \brief  PLL domain to be used by the device.
 *         This enum is used by the application to indicate to which PLL domain
 *         the UPP device is configured to. The pll domain will be required
 *         to handle the DVFS changes and other power management events.
 */
typedef enum Upp_pllDomain_t
{
    Upp_pllDomain_0    = (0u),
    /**< PLL domain 0                    */

    Upp_pllDomain_1    = (1u),
    /**< PLL domain 1                    */

    Upp_pllDomain_NONE = (2u)
    /**< Not in any pll domain           */

}Upp_pllDomain;
/**< PLL domain to be used by the device */

/**
 * \brief  Enum for the channel selection
 *
 * \Note   This enum is used as a part of the chanparams to indicate which
 *         physical channel is being requested by the application.
 */
typedef enum Upp_chanSel_t
{
    Upp_ChanSel_A  = (0u),
    /**< Channel selected is A           */

    Upp_ChanSel_B  = (1u)
    /**< Channel selected is B           */

}Upp_chanSel;
/**< Enum for the channel selection      */

/**
 * \brief Enum for the selection of the bit width
 */
typedef enum Upp_bitWidth_t
{
    Upp_bitWidth_8     =  (0u),
    /**< data width is 8 bit                */

    Upp_bitWidth_9     =  (1u),
    /**< data width is 9 bit                */

    Upp_bitWidth_10    =  (2u),
    /**< data width is 10 bit               */

    Upp_bitWidth_11    =  (3u),
    /**< data width is 11 bit               */

    Upp_bitWidth_12    =  (4u),
    /**< data width is 12 bit               */

    Upp_bitWidth_13    =  (5u),
    /**< data width is 13 bit               */

    Upp_bitWidth_14    =  (6u),
    /**< data width is 14 bit               */

    Upp_bitWidth_15    =  (7u),
    /**< data width is 15 bit               */

    Upp_bitWidth_16    =  (8u)
    /**< data width is 16 bit               */

}Upp_bitWidth;
/**<  Enum for the selection of the bit width                                 */

/**
 * \brief  Data rate to be used by the channel.
 */
typedef enum Upp_dataRate_t
{
    Upp_dataRate_SINGLE = (0u),
    /**< Normal data transfer rate          */

    Upp_dataRate_DOUBLE = (1u)
    /**< Double data transfer rate          */

}Upp_dataRate;
/**< Data rate to be used by the channel.   */

/**
 * \brief  Upp interrupt handler selction HWI or ECM
 */
typedef enum Upp_intHandlerSel_t
{
    Upp_intHandlerSel_ECM = (0u),
    /**< plug the interrupt handler in to a ECM */

    Upp_intHandlerSel_HWI = (1u)
    /**< plug the interrupt handler in to a HWI */
}Upp_intHandlerSel;

/**
 * \brief  Data packing format to be used.
 */
typedef enum Upp_dataPackFmt_t
{
    Upp_dataPackFmt_RJZE = (0u),
    /**< Right justified zero extended          */

    Upp_dataPackFmt_RJSE = (1u),
    /**< Right justified sign extended          */

    Upp_dataPackFmt_LJZF = (2u)
    /**< Left justified zero extended           */
}Upp_dataPackFmt;
/**< Data packing format to be used.            */

/**
 * \brief  UPP control signal polarity
 */
typedef enum Upp_polarity_t
{
    Upp_polarity_ACTIVE_HIGH  =  (0u),
    /**< Upp signal polarity is active HIGH         */

    Upp_polarity_ACTIVE_LOW   =  (1u)
    /**< Upp signal polarity is active LOW          */

}Upp_polarity;
/**< UPP control signal polarity                    */

/**
 * \brief  UPP interface clock polarity
 */
typedef enum Upp_clkPol_t
{
    Upp_clkPol_RISING_EDGE_SYNC  = (0u),
    /**< Channel interface is synchronized to rising edge of clock            */

    Upp_clkPol_FALLING_EDGE_SYNC = (1u)
    /**< Channel interface is synchronized to falling edge of clock           */

}Upp_clkPol;
/**< UPP interface clock polarity                                             */

/**
 * \brief   Idle state of the pin.
 */
typedef enum Upp_PinIdleState_t
{
    Upp_PinIdleState_IDLE_VAL       = (0u),
    /**< Idle value set in the register to be driven out                      */

    Upp_PinIdleState_HIGH_IMPEDANCE = (1u)
    /**< Pins to be placed in the high impedance state                        */

}Upp_PinIdleState;
/**< Idle state of the pin                                                    */

/*============================================================================*/
/*                              DATA STRUCTURES                               */
/*============================================================================*/
#ifdef Upp_STATISTICS_ENABLE
/**
 * \brief  Structure to hold the statistics for the UPP channel.
 *
 */
typedef struct Upp_devStats_t
{
    Uint32      byteCnt;
    /* no of bytes processed                 */

    Uint32      lineCnt;
    /* No of lines processed                 */

    Uint32      dataErrCnt;
    /**< Overrun/underrun error count        */

    Uint32      pktCnt;
    /**< No of packets processed             */

    Uint32      cbaErrCnt;
    /**< Number of internal errors           */

    Uint32      dmaErrCnt;
    /**< Number of DMA errors                */
}Upp_devStats;
/**< Upp device statistics                   */
#endif

/**
 * \brief  Structure format for the application to submit data.
 *
 *         This is the format of the structure to be supplied by the application
 *         while submitting a request to the Upp IOM driver.
 *
 * \see    uppMdSubmitChan()
 */
typedef struct Upp_tranParam_t
{
    Ptr         windowAddr;
    /**< Pointer to the start of the transfer data                            */

    Uint16      bytesPerLine;
    /**< Number of data bytes per line                                        */

    Uint16      lineCount;
    /**< Number of lines                                                      */

    Uint16      lineOffset;
    /**< Offset address between data lines                                    */

}Upp_transParam;
/**< Upp IO transactions data strcuture.                                      */

/**
 * \brief Error callback function protype
 *
 *        This is the global error callback function for the Upp driver.
 *        This function is called directly called from ISR context in case of
 *        error.The application can register a function of this type to be
 *        called in case of any uPP errors.
 *
 * \param Arg1  [IN]  use supplied argument.
 * \param Arg2  [IN]  Error event
 * \param Arg3  [IN]  iopacket currently being processed.
 *
 * \note  Since this function is called from an ISR context,care should be taken
 *        that this function conforms to ISR coding guidelines.please refer to
 *        the DSP-BIOS application programming reference guide for the functions
 *        that are allowed to be called in the HWI context.
 */
typedef Void (*Upp_ErrCallback)(Uint32 Arg1,Uint32 Arg2,Uint32 Arg3);

/**
 * \brief Configuration structure for the UPP control signals.
 *
 *        This structure is used to configure the WAIT,ENABLE and START signals
 *        for a given UPP channel.Signal that need to be enabled have to be
 *        passed using this structure.
 *
 * \see   Upp_Params
 */
typedef struct Upp_signalConfig_t
{
    Bool              startEnable;
    /**< whether the START signal is to be enabled                            */

    Upp_polarity      startPinPol;
    /**< START signal polarity                                                */

    Bool              enaEnable;
    /**< whether the ENABLE signal is to be enabled                           */

    Upp_polarity      enablePinPol;
    /**< ENABLE signal polarity                                               */

    Bool              waitEnable;
    /**< whether the ENABLE signal is to be enabled                           */

    Upp_polarity      waitPinPol;
    /**< WAIT signal polarity                                                 */

    Upp_clkPol        clkPol;
    /**< clock inversion. Controls clock signal polarity for interface Channel*/

    Upp_PinIdleState  pinIdleState;
    /**< The idle state of the pin.(high impedance or idle value to be driven)*/

}Upp_signalConfig;
/**< UPP control signal configuration data strcuture                          */

/**
 * \brief   UPP device create params
 *
 *          This structure holds the configuration to be used for the
 *          creation of the Upp device instance.This configuration need to be
 *          supplied during the creation of the device instance.
 *
 * \see     uppMdBindDev()
 */
typedef struct Upp_Params_t
{
    Upp_EmuMode              emulationMode;
    /**< Emulation mode settings for the Upp                                  */

    Upp_Loopback             dlbMode;
    /**< Digital loop back mode (ENABLE/DISABLE)                              */

    Upp_DevMode              devMode;
    /**< Mode in which upp instance needs to be created                       */

    Bool                     pscPwrmEnable;
    /**< Option to enable or disable the PSC control in the driver            */

    Uint32                   inputFreq;
    /**< Input clock source frequency                                         */

    Uint16                   hwiNumber;
    /**< hwi number configured for UPP event                                  */

    Upp_intHandlerSel        instHandlerSel;
    /**< option to select where the interrupt handler is to be plugged        */

    Upp_pllDomain            pllDomain;
    /**< Pll domain where the device is                                       */

}Upp_Params;
/**< UPP device setup params                                                  */

/**
 * \brief  Upp channel setup params
 *
 *         This structure holds configuration to be used for creating a
 *         channel of the Upp. These parameters need to be specified during
 *         the creation of the channel.
 *
 * \see    uppMdCreateChan()
 */
typedef struct Upp_ChanParams_t
{
    Bool                     enableCache;
    /**< whether the cache operations are to be performed on the application
     *   buffers or not                                                       */

    Upp_chanSel              chanSel;
    /**< channel to be configured                                             */

    Upp_bitWidth             bitWidth;
    /**< Data width for this channel                                          */

    Upp_dataRate             dataRate;
    /**< Data rate specification for the channel                              */

    Upp_ChanMode             chanMode;
    /**< Option to specify if the demultiplexing mode or tx interleave mode is
     * to be enable.Used in combination with "dataRate"                       */

    Upp_dataPackFmt          dataPackFmt;
    /**< Data packing format                                                  */

    Uint32                   opFreq;
    /**< output frequency                                                     */

    Uint16                   idleVal;
    /**< Idle value to be sent out. Only valid when the channel is configured
     * as Transmit                                                            */

    Upp_ErrCallback          userCbFxnStatus;
    /**< User callback function to be called in case of an EOL interrput      */

    Arg                      userCbArgStatus;
    /**< Optional user registered callback argument                           */

    Upp_ErrCallback          userCbFxnError;
    /**< User callback function to be called only in case of an errors in uPP */

    Arg                      userCbArgError;
    /**< Optional user registered callback argument                           */


    Upp_fifoThreshold        fifoThresHold;
    /**< Fifo threshold settings                                              */

    Upp_signalConfig         signalCfg;
    /**< UPP start,enable and wait signal configurations                      */

}Upp_ChanParams;
/**< Upp Channel setup parameters                                             */

/*============================================================================*/
/*                           IOCTL COMMANDS                                   */
/*============================================================================*/

/**
 *  \brief  UPP Ioctl commands
 *
 *          List of all the control commands supported by the Upp driver.
 *
 */
typedef enum Upp_Ioctl_t
{
    Upp_Ioctl_START = 128,
    /**< Ioctl to start the UPP device @param (None)                          */

    Upp_Ioctl_STOP,
    /**< Ioctl to stop the UPP channel @param (None)                          */

    Upp_Ioctl_QUERY_STATS,
    /**< Ioctl to query the statistics from a channel @param (Upp_devStats *) */

    Upp_Ioctl_CLEAR_STATS,
    /**< Ioctl to clear the channel statistics @param (None)                  */

    Upp_Ioctl_SET_TIMEOUT,
    /**< Ioctl to modify the generic retry count used by the device
     *   @param (Uint32 *)                                                    */

    Upp_Ioctl_SUSPEND,
    /**< Function to suspend the uPP device operation @param (None)           */

    Upp_Ioctl_RESUME,
    /**< Function to resume the uPP device operation @param (None)            */

    Upp_Ioctl_CHAN_CONFIG
    /**< Ioctl command to change the configuration of the uPP channel
     * @param  (Upp_chanParams *)                                             */
} Upp_Ioctl;
/**< Upp Ioctl commands list                                                  */

/**
 * \brief   Default parameters to be used by the UPP driver
 *
 * \note    User can use this default parameters or modify them according to his
 *          requirements.
 */
extern const Upp_Params Upp_PARAMS;

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
Void Upp_init(Void);

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif /* _UPP_H_    */
/*============================================================================*/
/*                         END OF FILE                                        */
/*============================================================================*/

