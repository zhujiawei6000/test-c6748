/*
 * upp.h
 *
 *  Created on: 2017年10月12日
 *      Author: feng
 */

#ifndef DRV_UPP_H_
#define DRV_UPP_H_


// Standard header file
#include <xdc/std.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/family/c64p/Hwi.h>
#include <ti/sysbios/family/c64p/EventCombiner.h>

// CSLR header files
#include <ti/csl/soc/omapl138/src/cslr_soc.h>
#include <ti/csl/src/ip/upp/V0/cslr_upp.h>
#include <ti/csl/cslr_syscfg.h>

//cache
#include <ti/sysbios/family/c64p/Cache.h>


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



//  The transfer data structure as defined by the uPP minidriver.
//  The structure mainatins the pointer to the data window.

typedef enum upp_Channel {
    upp_A,
    upp_B
} upp_Channel;


/*
 * UPP传输的DMA传输字
 */
typedef struct _Upp_Transfer_Desc {
    Ptr         windowAddr;     // Pointer to the start of the transfer data
    Int32       windowIndex;
    Uint16      lineSize;       // Number of data bytes per line
    Uint16      lineCount;      // Number of lines
    Uint16      lineOffset;     // Offset address between data lines
    struct _Upp_Transfer_Desc *next;  //Pointer to next transfer
}Upp_Transfer_Desc, *pUpp_Transfer_Desc;



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
    Uint8           isIdle;               // Channel is idle
    Uint8           uor;                // Channel has encountered an underrun/overflow condition
    Uint8           error;              // Channel has encountered an error condition
    Uint8           dma_error;          // Channel has encountered DMA programming error condition

    Upp_Transfer_Desc    *currentTransfer;   // transfer object in use

    upp_ChanParams  *params;            // Params used to configure this channel
} upp_ChanObj, *upp_chanHdl;

typedef struct upp_Obj {
    upp_ChanObj      A;             // Channel A status
    upp_ChanObj      B;             // Channel B status

    upp_Params      * params;       // Params used to configure the uPP peripheral
} upp_Obj, *upp_Hdl;







#define TRANSFER_LINK_DEPTH     (2)

#define UPP_WINDOW_LINE_SIZE    (7616)
#define UPP_WINDOW_LINE_COUNT   (3)


#define UPP_WINDOW_SIZE (UPP_WINDOW_LINE_SIZE*UPP_WINDOW_LINE_COUNT)

typedef struct _WindowLine
{
    unsigned char data[UPP_WINDOW_LINE_SIZE];
}WindowLine;

typedef struct _WindowBuffer
{
    WindowLine line[UPP_WINDOW_LINE_COUNT];
}WindowBuffer;


typedef struct _Trans_Window
{
    //if window is refreshed, driver set this flag to 1;
    int windowFlag;
    int overwriteFlag;
    WindowBuffer *buffer;
}Trans_Window, *pTrans_Window;


extern upp_Params UppDefaultConifgs;

extern upp_Hdl Upp_Init(void);



/*
 * @brief   Start transfer for channel A, one window
 * @buffer  window buffer
 */
extern void Test_StartTransWindow_A(WindowBuffer *buffer);


/*
 * @brief   Start transfer for channel A, once a line
 * @line    one line
 */
extern void Test_StartTransOneLine_A(WindowLine *line);



/*
 * @brief   Stop recv transfer for channel B
 *
 */
extern void StopRecvTransB(void);
/*
 * @brief   Start recv transfer for channel B
 *
 * @return  0 = success  -1 = failure
 */
extern int StartRecvTransB(void);


#ifdef __cplusplus
}
#endif // extern "C"




#endif /* DRV_UPP_H_ */
