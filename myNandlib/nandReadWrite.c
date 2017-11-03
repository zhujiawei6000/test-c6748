/**
 *  \file   nandReadWrite.c
 *
 *  \brief  Sample application for NAND.
 *
 */

/*
* Copyright (C) 2010-2012 Texas Instruments Incorporated - http://www.ti.com/
*/
/*
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
#include "hw_emifa2.h"
#include "soc_C6748.h"
//#include "uartStdio.h"
#include "interrupt.h"
#include "emifa.h"
#include "nandlib.h"
#include "nand_emifa2.h"
#include "nandDma.h"
#include "lcdkC6748.h"
#include "hw_types.h"
#include "nandReadWrite.h"

/*******************************************************************************
*                       INTERNAL MACRO DEFINITIONS
*******************************************************************************/

/* #define NAND_DATAINTEGRITY_TEST_WITH_FIXED_ADDR */

/*****************************************************************************/
/*
** Macros which defines the read write size, buffer size and number of transfers
**
*/

/*****************************************************************************/
/*
** Macros which defines default block, page and num of pages for read/write.
**
*/
#define NAND_DEFAULT_START_PAGE                 (5)
#define NAND_DEFAULT_BLK                        (8)
#define NAND_DEFAULT_NMBR_OF_PAGES              (1)

/*****************************************************************************/
/*
** Macros which defines the data integrity status.
**
*/
#define NAND_DATA_INTEGRITY_PASS                (0)
#define NAND_DATA_INTEGRITY_FAIL                (1)

/*****************************************************************************/
/*
** Macros which defines the NAND timing info.
**
*/
/* Maximum extended wait time                                       */
#define NAND_MAX_EXT_WAIT                       (0xFF)

/* Setup,strobe,hold times for read/write for the dev MT29F4G08AAA  */
#define NAND_WRITE_SETUP_TIME_IN_NS             (0u)
#define NAND_WRITE_STROBE_TIME_IN_NS            (30u)
#define NAND_WRITE_HOLD_TIME_IN_NS              (30u)
#define NAND_READ_SETUP_TIME_IN_NS              (20u)
#define NAND_READ_STROBE_TIME_IN_NS             (40u)
#define NAND_READ_HOLD_TIME_IN_NS               (0u)
#define NAND_TURN_ARND_TIME_IN_NS               (0u)

/* Setup,strobe,hold times reset values                             */
#define EMIFA_WRITE_SETUP_RESETVAL              (0x0F)
#define EMIFA_WRITE_STROBE_RESETVAL             (0x3F)
#define EMIFA_WRITE_HOLD_RESETVAL               (0x07)
#define EMIFA_READ_SETUP_RESETVAL               (0x0F)
#define EMIFA_READ_STROBE_RESETVAL              (0x3F)
#define EMIFA_READ_HOLD_RESETVAL                (0x07)
#define EMIFA_TA_RESETVAL                       (0x03)

/* NAND Module clk frequency                                        */
#define NAND_MODULE_CLK                         ((100u)*(1000u)*(1000u))
#define NAND_MODULE_CLK_IN_MHZ                  (NAND_MODULE_CLK / 1000000)





/******************************************************************************
**                      INTERNAL FUNCTION PROTOTYPES
*******************************************************************************/

#define NAND_DATA_XFER_MODE                     (NAND_XFER_MODE_DMA)
#define NAND_BUSWIDTH                           (NAND_BUSWIDTH_16BIT)
//#define NAND_CHIP_SELECT                        (1)
//#define NAND_NUMOF_BLK                          (4096)
#define NAND_MANUFATURER_MICRON_ID              (0x2C)



/******************************************************************************
*                                                                             *
*                                                                             *
* \brief  Function to initalize the EMIFA NAND timing and base addr info.      *
*                                                                             *
* \param  nandTimimgInfo : Pointer to structure containing                    *
*                          NAND timing info.                                  *
*                                                                             *
* \return none.                                                               *
*                                                                             *
******************************************************************************/
static void NANDTimingInfoInit(void *TimingInfo)
{
    int moduleClkInMHz = NAND_MODULE_CLK_IN_MHZ;
    EMIFANANDTimingInfo_t *nandTimingInfo;
    
    nandTimingInfo = (EMIFANANDTimingInfo_t * )TimingInfo;
    
    /* Set the asynchronous wait timing                             */
    nandTimingInfo->writeSetup  = ( ((moduleClkInMHz * NAND_WRITE_SETUP_TIME_IN_NS)/1000u) &
                                     EMIFA_WRITE_SETUP_RESETVAL );
    nandTimingInfo->writeStrobe = ( ((moduleClkInMHz * NAND_WRITE_STROBE_TIME_IN_NS)/1000u) &
                                    EMIFA_WRITE_STROBE_RESETVAL );
    nandTimingInfo->writeHold   = ( ((moduleClkInMHz * NAND_WRITE_HOLD_TIME_IN_NS)/1000u) &
                                    EMIFA_WRITE_HOLD_RESETVAL );
    
    nandTimingInfo->readSetup   = ( ((moduleClkInMHz * NAND_READ_SETUP_TIME_IN_NS)/1000u) &
                                    EMIFA_READ_SETUP_RESETVAL );
    nandTimingInfo->readStrobe  = ( ((moduleClkInMHz * NAND_READ_STROBE_TIME_IN_NS)/1000u) &
                                    EMIFA_READ_STROBE_RESETVAL );
    nandTimingInfo->readHold    = ( ((moduleClkInMHz * NAND_READ_HOLD_TIME_IN_NS)/1000u) &
                                    EMIFA_READ_HOLD_RESETVAL );
    
    nandTimingInfo->turnAround  = ( ((moduleClkInMHz * NAND_TURN_ARND_TIME_IN_NS)/1000u) &
                                    EMIFA_TA_RESETVAL );
}


/******************************************************************************
*                                                                             *
* \brief  Function to initialize the device and controller info.              *
*                                                                             *
* \param  nandInfo      : Pointer to structure containing controller and      *
*                         device information.                                 *
*                                                                             *
* \param  csNum         : Chip select where device is interfaced.             *
*                                                                             *
* \return none.                                                               *
*                                                                             *
******************************************************************************/
extern void NANDInfoInit(NandInfo_t *nandInfo, unsigned int cs)
{   
    NandCtrlInfo_t *hNandCtrlInfo = nandInfo->hNandCtrlInfo;
    NandDmaInfo_t  *hNandDmaInfo  = nandInfo->hNandDmaInfo;
    NandEccInfo_t  *hNandEccInfo  = nandInfo->hNandEccInfo;

    /* Init the NAND Device Info */
    nandInfo->opMode                        = NAND_DATA_XFER_MODE;
    nandInfo->eccType                       = NAND_ECC_ALGO_RS_4BIT;
    
    nandInfo->chipSelectCnt                 = 1;
    nandInfo->dieCnt                        = 1;
    nandInfo->chipSelects[0]                = cs;
    nandInfo->busWidth                      = NAND_BUSWIDTH;
    nandInfo->pageSize                      = NAND_PAGE_SIZE_IN_BYTES;
    nandInfo->blkSize                       = NAND_BLOCK_SIZE_IN_BYTES;
    nandInfo->manId                         = NAND_MANUFATURER_MICRON_ID;
    nandInfo->devId                         = NAND_DEVICE_ID;

    nandInfo->dataRegAddr                   = (SOC_EMIFA_CS3_ADDR + 0x00);
    nandInfo->addrRegAddr                   = (SOC_EMIFA_CS3_ADDR + 0x08);
    nandInfo->cmdRegAddr                    = (SOC_EMIFA_CS3_ADDR + 0x10);


    /* Init the NAND Controller Info struct */
    hNandCtrlInfo->CtrlInit                 = EMIFANANDInit;
    hNandCtrlInfo->WaitPinStatusGet         = EMIFANANDWaitPinStatusGet;
    hNandCtrlInfo->currChipSelect           = cs;
    hNandCtrlInfo->baseAddr                 = SOC_EMIFA_0_REGS;
    hNandCtrlInfo->eccSupported             = ( NAND_ECC_ALGO_HAMMING_1BIT |
                                                NAND_ECC_ALGO_RS_4BIT );
    hNandCtrlInfo->waitPin                  = EMIFA_EMA_WAIT_PIN0;
    hNandCtrlInfo->waitPinPol               = EMIFA_EMA_WAIT_PIN_POLARITY_HIGH;
    hNandCtrlInfo->wpPinPol                 = 0;
    hNandCtrlInfo->chipSelectBaseAddr[0]    = SOC_EMIFA_CS3_ADDR;
    hNandCtrlInfo->chipSelectRegionSize[0]  = EMIFA_CHIP_SELECT_3_SIZE;
    NANDTimingInfoInit(hNandCtrlInfo->hNandTimingInfo);
                                                
                                                
    /* Init the NAND Ecc Info */
    hNandEccInfo->baseAddr                  = 0;    
    hNandEccInfo->ECCInit                   = EMIFANANDECCInit;
    hNandEccInfo->ECCEnable                 = EMIFANANDECCEnable;
    hNandEccInfo->ECCDisable                = EMIFANANDECCDisable;
    hNandEccInfo->ECCWriteSet               = EMIFANANDECCWriteSet;
    hNandEccInfo->ECCReadSet                = EMIFANANDECCReadSet;
    hNandEccInfo->ECCCalculate              = EMIFANANDECCCalculate;
    hNandEccInfo->ECCCheckAndCorrect        = EMIFANANDECCCheckAndCorrect;
    
    /* Init the NAND DMA info */
    hNandDmaInfo->DMAXfer                   = EMIFANANDDMAXfer;    
    hNandDmaInfo->DMAInit                   = EMIFANANDEdmaInit;
    hNandDmaInfo->DMAXferSetup              = EMIFANANDXferSetup;
    hNandDmaInfo->DMAXferStatusGet          = EMIFANANDXferStatusGet;
}
