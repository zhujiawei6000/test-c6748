/********************************************************************/
/* This file provides default configuration and transfer structs    */
/* that an application may use to initialize its own structs before */
/* passing them to the uPP peripheral interface driver.             */
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

// driver include file
#include "upp_md.h"


// Default uPP channel configuration

#define UPP_CHANPARAMS_  {  upp_dir_xmit,    \
                            0x7,             \
                            1,               \
                            8,               \
                            upp_format_rjze, \
                            upp_thresh_64,   \
                            TRUE,            \
                            0x0000,          \
                            TRUE,            \
                            TRUE,            \
                            TRUE,            \
                            FALSE,           \
                            FALSE,           \
                            FALSE,           \
                            FALSE,           \
                            TRUE,            \
                            TRUE,            \
                            TRUE,            \
                            TRUE,            \
                            TRUE,            \
                            NULL,            \
                            NULL,            \
                            NULL,            \
                            NULL,            \
                            NULL             \
                         }

upp_ChanParams UPP_CHANPARAMS = UPP_CHANPARAMS_;


// Default uPP device configuration

#define UPP_PARAMS_  {  1,               \
                        UPP_CHANPARAMS_, \
                        UPP_CHANPARAMS_, \
                        upp_thresh_64,   \
                        upp_thresh_64,   \
                        upp_none         \
                     }

upp_Params UPP_PARAMS = UPP_PARAMS_;


// Default uPP user configuration

upp_UserParams UPP_USERPARAMS =
{
    NULL,        // Let driver allocate struct
    UPP_PARAMS_, // Use default params
    0            // Use memory segment 0
};


// Default uPP transfer configuration

upp_Transfer UPP_TRANSFER =
{
    {NULL, NULL}, // Queue elem to allow drv to queue framePackets
    upp_A,        // uPP data channel selection
    0,            // Pointer to the start of the transfer data
    0,            // Number of data bytes per line
    1,            // Number of lines
    0,            // Offset address between data lines
};


// Default upp_StaticConfig: see macro in upp_md.h


//-------------
// END OF FILE 
//-------------
