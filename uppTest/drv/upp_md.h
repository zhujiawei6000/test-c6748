/********************************************************************/
/* The uPP mini driver interface header file.                       */
/* This header file defines the APIs and the data                   */
/* structures that are implemented by the driver.                   */
/*                                                                  */
/* Copyright (c) Texas Instruments Inc 2009                         */
/*                                                                  */
/* Use of this software is controlled by the terms and conditions   */
/* found in the license agreement under which this software has     */
/* been supplied.                                                   */
/********************************************************************/

/********************************************************************/
/* version 0.0.1: Joe Coombs Aug 05 2009                            */
/* Initial version                                                  */
/********************************************************************/

#ifndef UPP_MD_H_
#define UPP_MD_H_

// Standard header file
#include <xdc/std.h>
#include <ti/sysbios/BIOS.h>
// IOM interface header file
//#include <iom.h>
#include <ti/sysbios/io/IOM.h>

// DSP-BIOS header files
//#include <que.h>
#include <ti/sysbios/knl/Queue.h>
//#include <hwi.h>
#include <ti/sysbios/family/c64p/Hwi.h>
//#include <mem.h>
#include <xdc/runtime/Memory.h>
//#include <clk.h>
//#include <ecm.h>
#include <ti/sysbios/family/c64p/EventCombiner.h>
//#include <sem.h>
#include <ti/sysbios/knl/Semaphore.h>

// CSLR header files
#include "soc_OMAPL138.h"
#include "cslr_upp.h"
#include "cslr_syscfg0_OMAPL138.h"


#ifdef __cplusplus
extern "C" {
#endif


// uPP static configuration struct.  Must be provided by user
// in application code and can't be changed at run time.

typedef enum upp_ClockSrc {
    upp_pll0sysclk2,
    upp_pll1sysclk2,
    upp_2xTxClk,
    upp_defaultClk
} upp_ClockSrc;

typedef enum upp_DrvModel {
    upp_sync,
    upp_async
} upp_DrvModel;

typedef struct upp_StaticConfig {
    upp_DrvModel mode;          // sync or async operation
    upp_ClockSrc tx_clk_src;    // transmit clock source (both channels)
} upp_StaticConfig;


//  The transfer data structure as defined by the uPP minidriver.
//  The structure mainatins the pointer to the data window.

typedef enum upp_Channel {
    upp_A,
    upp_B
} upp_Channel;

typedef struct upp_Transfer {
//    QUE_Elem    link;           // Queue elem to allow drv to queue framePackets
    Queue_Elem    link;           // Queue elem to allow drv to queue framePackets
    upp_Channel channel;        // uPP data channel selection
    Ptr         windowAddr;     // Pointer to the start of the transfer data
    Uint16      lineSize;       // Number of data bytes per line
    Uint16      lineCount;      // Number of lines
    Uint16      lineOffset;     // Offset address between data lines
} upp_Transfer;


//  Global structure implemented by the uPP mini driver. The global
//  structure is provided to all the channel instances. It maintains
//  information required by the driver globally.

typedef struct upp_GlobalStr {
    Uint32       driverInit;        // Flag to check if mdBindDev already called
    Uint32       channelInst;       // Flag to check if channelInst created
} upp_GlobalStr;


//  uPP mini driver parameter enums and data structures. The structures are used
//  to program the device (uPP).

typedef enum upp_Direction {
    upp_dir_xmit,
    upp_dir_rcv
} upp_Direction;

typedef enum upp_DataFormat {
    upp_format_ljzf,
    upp_format_rjse,
    upp_format_rjze
} upp_DataFormat;

typedef enum upp_Threshold {
    upp_thresh_64 = 64,
    upp_thresh_128 = 128,
    upp_thresh_256 = 256
} upp_Threshold;

typedef void (* upp_CbFxn)(Ptr xferp, Uint32 val);

typedef enum upp_SpecialMode {
    upp_none,                    // no restrictions
    upp_digital_loopback,        // 2-channel only; requires one receive and one transmit channel
    upp_sdr_xmit_interleave,     // 1-channel only; channel A must transmit and have data_rate = 1
    upp_ddr_interleave           // 1-channel only; channel A must have data_rate = 2
} upp_SpecialMode;

typedef struct upp_ChanParams {
    upp_Direction   direction;   // direction of uPP channel
    Uint8           clock_div;   // clock divider (transmit mode only, 1-16)
    Uint8           data_rate;   // data rate (1-2)
    Uint8           width;       // data width (in bits, 8-16)
    upp_DataFormat  format;      // data format (8 < width < 16 only)
    upp_Threshold   xmit_thresh; // transmit threshold (transmit only)
    Uint8           drive_idle;  // drive value from IVR while idle (else hi-z)
    Uint16          idle_value;  // value for IVR
    // signal enable
    Uint8           en_start;    // enable start signal
    Uint8           en_enable;   // enable enable signal
    Uint8           en_wait;     // enable wait signal
    // signal polarity
    Uint8           inv_start;   // invert start signal
    Uint8           inv_enable;  // invert enable signal
    Uint8           inv_wait;    // invert wait signal
    Uint8           inv_clock;   // invert clock signal
    // interrupt generation (async mode only)
    Uint8           gen_eow;     // generate end of window (EOW) events
    Uint8           gen_eol;     // generate end of line (EOL) events
    Uint8           gen_uor;     // generate underflow/overflow (UOR) events
    Uint8           gen_err;     // generate internal error (ERR) events
    Uint8           gen_dpe;     // generate DMA programming error (DPE) events
    // interrupt dispatch (async mode only)
    upp_CbFxn       fxn_eow;     // function pointer for EOW user-callback (optional; called if != NULL)
    upp_CbFxn       fxn_eol;     // function pointer for EOL user-callback (optional; called if != NULL)
    upp_CbFxn       fxn_uor;     // function pointer for UOR user-callback (optional; called if != NULL)
    upp_CbFxn       fxn_err;     // function pointer for ERR user-callback (optional; called if != NULL)
    upp_CbFxn       fxn_dpe;     // function pointer for DPE user-callback (optional; called if != NULL)
} upp_ChanParams;

typedef struct upp_Params {
    Uint8           channels;       // number of active uPP channels
    upp_ChanParams  A;              // channel-specific configuration for channel A
    upp_ChanParams  B;              // channel-specific configuration for channel B
    upp_Threshold   dma_i_thresh;   // Read threshold for DMA channel I
    upp_Threshold   dma_q_thresh;   // Read threshold for DMA channel Q
    upp_SpecialMode special_mode;   // enable special modes (restrictions apply; see enum above)
} upp_Params;


//  uPP mini driver channel instance data structure. This data structure
//  holds information required by any channel instance. Pointer to this
//  data structure is the handle of the channel instance

typedef struct upp_ChanObj {
    Uint8           idle;               // Channel is idle
    Uint8           pend;               // Channel has additional transactions pending
    Uint8           uor;                // Channel has encountered an underrun/overflow condition
    Uint8           error;              // Channel has encountered an error condition
    Uint8           dma_error;          // Channel has encountered DMA programming error condition
    
    //QUE_Obj         pendTransferList;   // Queue object to hold the pending transfer objects
    Queue_Handle    pendTransferList;   // Queue object to hold the pending transfer objects
    upp_Transfer    *currentTransfer;   // transfer object in use
    upp_Transfer    *nextTransfer;      // transfer object pre-programmed into internal DMA

    upp_ChanParams  *params;            // Params used to configure this channel
} upp_ChanObj, *upp_chanHdl;

typedef struct upp_Obj {
    upp_ChanObj      A;             // Channel A status
    upp_ChanObj      B;             // Channel B status
    Uint8            reset;         // uPP peripheral in reset
    
    Uint8            drvObjAlloc;   // Track whether driver allocated this struct
    Uint32           segID;         // Memory segment for driver object allocation

    Uint8            drvAborting;   // Track whether driver is aborting
    //SEM_Handle       sync_helper;   // Semaphore for pending in synchronous mode
    Semaphore_Handle       sync_helper;   // Semaphore for pending in synchronous mode
    upp_Params       *params;       // Params used to configure the uPP peripheral
} upp_Obj, *upp_Hdl;


//  The channel parameters provided by the application.
//  The data structure includes the pointer to the uPP channel object.
//  The application may allocate the memory and provide to the driver.
//  If the chanObj provided to driver is NULL, the driver will allocate
//  the chanObj from the specified segID.
//  The data structure also includes the pointer to the upp_Params. These
//  parameters are used to configure the uPP device.

typedef struct upp_UserParams {
    upp_Obj     *uppObj;    // Pointer to object data structure
    upp_Params   params;    // Parameters used to configure uPP
    Int32        segID;     // Memory segment to allocate chanObj from
} upp_UserParams;


//  The upp_mdCtrlChan function supports the below commands.
//  The application can use these commands to configure and
//  control the driver. The driver also supports the below 
//  std IOM commands:

typedef enum upp_ioctl {
    upp_ioctl_enable = IOM_CNTL_USER + 1,  // Enable uPP peripheral via PCR
    upp_ioctl_disable,                     // Disable uPP peripheral via PCR
    upp_ioctl_sw_reset,                    // Toggle uPP peripheral software reset via PCR
    upp_ioctl_apply_params,                // Apply new param struct to driver
    upp_ioctl_get_status,                  // Get status of uPP driver
    upp_ioctl_dequeue,                     // Move all items from pending QUEs to user-specified QUE
    upp_ioctl_clear_errors                 // Reset error bits in internal struct
} upp_ioctl;


// global objects 

// function table (upp_md.c)
extern IOM_Fxns UPP_FXN_TABLE;

// global struct (upp_md.c)
extern upp_GlobalStr UPP_GLOBALSTR;

// default param and transfer structs (upp_md_defaults.c)
extern upp_ChanParams UPP_CHANPARAMS;
extern upp_Params UPP_PARAMS;
extern upp_UserParams UPP_USERPARAMS;
extern upp_Transfer UPP_TRANSFER;
#define UPP_STATICCONFIG { upp_async,      \
                           upp_pll0sysclk2 \
                         }

// static configuration file (application code)
extern const upp_StaticConfig UPP_DEV_CONFIG;

#ifdef __cplusplus
}
#endif // extern "C"

#endif // UPP_MD_H_
