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
#include "hw_types.h"
#include "soc_C6748.h"
//#include "uartStdio.h"
#include "interrupt.h"
#include "emifa.h"
#include "nandlib.h"
#include "nand_emifa2.h"
#include "nandDma.h"
#include "lcdkC6748.h"
#include <ti/board/board.h>
#include <xdc/runtime/System.h>
#include <ti/drv/uart/UART_stdio.h>

/*******************************************************************************
*                       INTERNAL MACRO DEFINITIONS
*******************************************************************************/

/* #define NAND_DATAINTEGRITY_TEST_WITH_FIXED_ADDR */

/*****************************************************************************/
/*
** Macros which defines attached device info like num of pages per block, 
** number of blocks, device ID and manufacturer ID.
**
*/
#define NAND_DATA_XFER_MODE                     (NAND_XFER_MODE_DMA)
#define NAND_BUSWIDTH                           (NAND_BUSWIDTH_16BIT)
#define NAND_CHIP_SELECT                        (EMIFA_CHIP_SELECT_3)
#define NAND_PAGE_SIZE_IN_BYTES                 (NAND_PAGESIZE_2048BYTES)
#define NAND_BLOCK_SIZE_IN_BYTES                (NAND_BLOCKSIZE_128KB)
#define NAND_NUMOF_BLK                          (4096)
#define NAND_MANUFATURER_MICRON_ID              (0x2C)
#define NAND_DEVICE_ID                          (0xCC)

/*****************************************************************************/
/*
** Macros which defines the read write size, buffer size and number of transfers
**
*/
#define NAND_DATA_BUFF_SIZE                     (NAND_PAGE_SIZE_IN_BYTES)
#define NAND_ECC_BUFF_SIZE                      ((NAND_PAGE_SIZE_IN_BYTES/NAND_BYTES_PER_TRNFS) \
                                                 * NAND_MAX_ECC_BYTES_PER_TRNFS)

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
**                      INTERNAL VARIABLE DEFINITIONS
*******************************************************************************/

#if defined(__IAR_SYSTEMS_ICC__)

#pragma data_alignment=4
volatile unsigned char txData[NAND_DATA_BUFF_SIZE];
#pragma data_alignment=4
volatile unsigned char rxData[NAND_DATA_BUFF_SIZE];

#elif defined(__TMS470__) || defined(_TMS320C6X)

#pragma DATA_ALIGN(txData, 4);
volatile unsigned char txData[NAND_DATA_BUFF_SIZE];
#pragma DATA_ALIGN(rxData, 4);
volatile unsigned char rxData[NAND_DATA_BUFF_SIZE];

#else

volatile unsigned char  __attribute__ ((aligned (4))) txData[NAND_DATA_BUFF_SIZE];
volatile unsigned char  __attribute__ ((aligned (4))) rxData[NAND_DATA_BUFF_SIZE];

#endif

unsigned char eccData[NAND_ECC_BUFF_SIZE];


/******************************************************************************
**                      INTERNAL FUNCTION PROTOTYPES
*******************************************************************************/


/******************************************************************************
**                      INTERNAL FUNCTION DEFINITIONS
*******************************************************************************/

/******************************************************************************
*                                                                             *
* \brief  This function prints the device ID info of NAND.\n                  *
*                                                                             *
* \param  nandInfo        Pointer to structure which conatins controller and  *
*                         device information.                                 *
*                                                                             *
* \return none.                                                               *
*                                                                             *
******************************************************************************/
static void NANDDeviceIdInfoPrint(NandInfo_t *nandInfo)
{
    UART_printf(" ****************** NAND DEVICE INFO ****************** \r\n");
    UART_printf("    MANUFACTURER ID    : %u\r\n",(unsigned int)nandInfo->manId);
    UART_printf("    DEVICE ID          : %u\r\n",(unsigned int)nandInfo->devId);
    UART_printf("    PAGESIZE           : %uBytes\r\n",(unsigned int)nandInfo->pageSize);
    UART_printf("    BLK SIZE           : %uBytes\r\n",(unsigned int)nandInfo->blkSize);
    UART_printf("    PAGES PER BLOCK    : %u\r\n",(unsigned int)nandInfo->pagesPerBlk);
    UART_printf(" ****************************************************** \r\n");
}

/******************************************************************************
*                                                                             *
* \brief  This function initializes the read, write and ecc buffers.\n        *
*                                                                             *
* \param none                                                                 *
*                                                                             *
* \return none                                                                *
*                                                                             *
******************************************************************************/
static void NANDBuffersInit()
{
    unsigned int byteCnt;

    txData[0]  = 'S';
    txData[1]  = 'T';
    txData[2]  = 'A';
    txData[3]  = 'R';
    txData[4]  = 'T';
    txData[5]  = 'E';
    txData[6]  = 'R';
    txData[7]  = 'W';
    txData[8]  = 'A';
    txData[9]  = 'R';
    txData[10] = 'E';

    for(byteCnt = 11; byteCnt < NAND_PAGE_SIZE_IN_BYTES; byteCnt++)
    {
        txData[byteCnt]= ((byteCnt) & 0xFF);
    }

    for(byteCnt = 0; byteCnt < NAND_PAGE_SIZE_IN_BYTES; byteCnt++)
    {
        rxData[byteCnt]= 0x00;
    }

    for(byteCnt = 0; byteCnt < NAND_ECC_BUFF_SIZE; byteCnt++)
    {
        eccData[byteCnt] = 0;
    }
}

/******************************************************************************
*                                                                             *
* \brief  This function checks for the data integrity of tx and rx buffers.\n *
*                                                                             *
* \param none                                                                 *
*                                                                             *
* \return Data inegrity status i.e.                                           *
*                                                                             *
*        NAND_DATA_INTEGRITY_PASS -- On pass                                  *
*        NAND_DATA_INTEGRITY_FAIL -- On fail                                  *
*                                                                             *
******************************************************************************/
static unsigned int NANDDataIntegrityCheck()
{
    unsigned int status = NAND_DATA_INTEGRITY_PASS;
    unsigned int byteCnt;

    for(byteCnt = 0; byteCnt < NAND_PAGE_SIZE_IN_BYTES; byteCnt++)
    {
        if(rxData[byteCnt] != txData[byteCnt])
        {
            status = NAND_DATA_INTEGRITY_FAIL;
            break;
        }
    }
    return (status);
}

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
static void NANDInfoInit(NandInfo_t *nandInfo, unsigned int cs)
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


/******************************************************************************
**                       GLOBAL FUNCTION DEFINITIONS
*******************************************************************************/

/******************************************************************************
*                                                                             *
* \brief  Main Function.\n                                                    *
*                                                                             *
******************************************************************************/
int main(void)
{
    int blkNum;
    int pageNum;
    int numOfPages;
    unsigned int retVal;
    unsigned int eraseBlkFlg;

    /* NAND structure allocations for this application */
    NandInfo_t              nandInfo;
    NandCtrlInfo_t          nandCtrlInfo;
    NandEccInfo_t           nandEccInfo;
    NandDmaInfo_t           nandDmaInfo;
    EMIFANANDTimingInfo_t   nandTimingInfo;

    /* Initializing the UART instance for serial communication.  */
//    UARTStdioInit();
    Board_STATUS boardInitStatus =0;

    Board_initCfg cfg = Board_init(BOARD_INIT_PINMUX_CONFIG | BOARD_INIT_MODULE_CLOCK | BOARD_INIT_UNLOCK_MMR | BOARD_INIT_UART_STDIO);

    boardInitStatus = Board_init(cfg);
    if (boardInitStatus !=0)
    {
        while(1);
    }

    UART_printf("\r\n ************* StarterWare NAND Application ************"
             "\r\n\r\n");
    
    /* Pin mux and clock setting */
    EMIFAClkConfig();
    NANDPinMuxSetup();
             
    /* Initialize the nandInfo struct */
    nandCtrlInfo.hNandTimingInfo = (void *) &nandTimingInfo;    
    nandInfo.hNandCtrlInfo = &nandCtrlInfo;
    nandInfo.hNandEccInfo = &nandEccInfo;
    nandInfo.hNandDmaInfo = &nandDmaInfo;
    NANDInfoInit(&nandInfo, NAND_CHIP_SELECT);

    /* Open the NAND device */
    retVal = NANDOpen(&nandInfo);
    if (retVal & NAND_STATUS_FAILED)
    {
        UART_printf("\r\n*** ERROR : NAND Open Failed... ");
        while(1);    
    }
    else if (retVal & NAND_STATUS_WAITTIMEOUT)
    {
        UART_printf("\r\n*** ERROR : Device Is Not Ready...!!!\r\n");
        while(1);
    }
    else if (retVal & NAND_STATUS_NOT_FOUND)
    {
        UART_printf("\r\n*** ERROR : DEVICE MAY NOT BE ACCESSABLE OR NOT PRESENT."
                 "\r\n");
        while(1);
    }
    else if(nandInfo.devId != NAND_DEVICE_ID)
    {
        /* Check if detected ID matches supplied ID */
        UART_printf("\r\n*** ERROR : INVALID DEVICE ID.");
        while(1);
    }
    else
    {
        /* Print The Device ID info */
        NANDDeviceIdInfoPrint(&nandInfo);
    }
    
#ifdef NAND_DATAINTEGRITY_TEST_WITH_FIXED_ADDR
    /* Do read/write for predefined address */
    pageNum = NAND_DEFAULT_START_PAGE;
    blkNum = NAND_DEFAULT_BLK;
    numOfPages = NAND_DEFAULT_NMBR_OF_PAGES;   
#else
    /* Take the read/write address from the user */
    UART_printf("\r\n Please Enter The Block Number(0 - %u)\r\n", (unsigned int)(NAND_NUMOF_BLK - 1));
    //blkNum = UARTGetNum();
    UART_scanFmt("%d", &blkNum);
    UART_printf("\r\n Please Enter The Page Start Page Number(0 - %u)\r\n", (unsigned int)(nandInfo.pagesPerBlk - 1));
    //pageNum = UARTGetNum();
    UART_scanFmt("%d", &pageNum);
    UART_printf("\r\n Please Enter The Number Of Pages To Write\r\n");
    //numOfPages = UARTGetNum();
    UART_scanFmt("%d", &numOfPages);
#endif
    eraseBlkFlg = 1;
    
    if( (pageNum < 0 ) || (pageNum > (nandInfo.pagesPerBlk - 1))
        || (blkNum < 0 || blkNum  > (NAND_NUMOF_BLK - 1)) || (numOfPages <= 0) )
    {
        UART_printf("\r\n *** ERROR : Wrong Input(s) Entered...!!!\r\n");
        while(1);
    }
    else if( ( blkNum * (nandInfo.pagesPerBlk )
              + pageNum + numOfPages ) >
              ( NAND_NUMOF_BLK * nandInfo.pagesPerBlk))
    {
        UART_printf("\r\n *** ERROR : Requsted Page(s) For Read/Write Does Not"
                 " Exist...!!!\r\n");
        while(1);
    }    
   
    while( numOfPages > 0 )
    {
        if( eraseBlkFlg )
        {
            retVal = NANDBadBlockCheck(&nandInfo, blkNum);
            if(retVal == NAND_BLOCK_BAD)
            {
                UART_printf("\r\n Block Is Bad, Can't Continue ...!!! ");
                while(1);
            }
            if(retVal == NAND_BLOCK_SPARE_AREA_READ_FAILED)
            {
                UART_printf("\r\n Spare Area Read Failed While Checking ");
                UART_printf(" For Bad Block ");
                while(1);
            }

            UART_printf("\r\n Erasing The Block %u                         :", (unsigned int)blkNum);
            retVal = NANDBlockErase(&nandInfo, blkNum);
            if( retVal == NAND_STATUS_PASSED )
            {
                UART_printf(" Succeeded.");
            }
            else
            {
                UART_printf(" Failed.");
                UART_printf("\r\n Marking The Block As Bad.\r\n Read/Write Test");
                UART_printf(" Will Be Continued On Next block. ");
                NANDMarkBlockAsBad(&nandInfo, blkNum);
                blkNum++;
                pageNum = 0;
                eraseBlkFlg = 1;
                continue;
            }
            eraseBlkFlg = 0;
        }

        NANDBuffersInit();
        UART_printf("\r\n Writing To Page %u Of Block %u                :", (unsigned int)pageNum, (unsigned int)blkNum);

        retVal = NANDPageWrite(&nandInfo, blkNum, pageNum, &txData[0],
                               &eccData[0]);
        if( (retVal & NAND_STATUS_WAITTIMEOUT) )
        {
            UART_printf(" Failed.(Device Is Busy).");
            while(1);
        }
        else if( (retVal & NAND_STATUS_DEVWRPROTECT) )
        {
            UART_printf(" Failed.(Device Is Write Protected).");
            while(1);
        }
        else if( (retVal & NAND_STATUS_READWRITE_DMA_FAIL) )
        {
            UART_printf(" Failed.(EDMA Transfer Failed.).");
            while(1);
        }
        else
        {
            UART_printf(" Succeeded.");
        }

        /* As eccData, is filled by NANDPageWrite fun, reinit the same */
        NANDBuffersInit();

        UART_printf("\r\n Reading From Page %u Of Block %u              :", (unsigned int)pageNum, (unsigned int)blkNum);
        retVal= NANDPageRead(&nandInfo, blkNum, pageNum, &rxData[0],
                             &eccData[0]);

        if( (retVal & NAND_STATUS_READ_ECC_ERROR_CORRECTED) )
        {
            UART_printf(" Succeeded With ECC Errors And Corrected.");
        }
        else if( (retVal & NAND_STATUS_READ_ECC_UNCORRECTABLE_ERROR) )
        {
            UART_printf(" Failed.(Uncorrectable ECC errors) ");
            while(1);
        }
        else if( (retVal & NAND_STATUS_READWRITE_DMA_FAIL) )
        {
            UART_printf(" Failed.(EDMA Transfer Failed.)");
            while(1);
        }
        else
        {
            UART_printf(" Succeeded.");
        }

        UART_printf("\r\n NAND Data Integrity Test                    :");
        retVal = NANDDataIntegrityCheck();
        if(retVal == NAND_DATA_INTEGRITY_PASS)
        {
            UART_printf(" Passed\r\n");
        }
        else
        {
            UART_printf(" Failed....!!!\r\n");
        }

        pageNum++;
        numOfPages--;
        if( pageNum == ((nandInfo.pagesPerBlk) ) )
        {
            pageNum = 0;
            eraseBlkFlg = 1;
            blkNum++;
        }
    }

    UART_printf("\r\n ****************************************************** ");
    while(1);    
}


/******************************************************************************
**                              END OF FILE
*******************************************************************************/
