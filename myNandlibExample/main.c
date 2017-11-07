/*
 *  ======== main.c ========
 */

#include <xdc/std.h>

#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

#include <ti/sysbios/BIOS.h>

#include <ti/sysbios/knl/Task.h>
#include <ti/drv/uart/UART_stdio.h>
#include <ti/board/board.h>
#include "nandReadWrite.h"
#include "interrupt.h"
#include <ti/sysbios/family/c64p/EventCombiner.h>
#include "nandDma.h"

extern void EDMA3INTCConfigure(void)
{

    EventCombiner_dispatchPlug(SYS_INT_EDMA3_0_CC0_INT1, (EventCombiner_FuncPtr)Edma3CompletionIsr, NULL, TRUE);
    EventCombiner_enableEvent(SYS_INT_EDMA3_0_CC0_INT1);

    EventCombiner_dispatchPlug(SYS_INT_EDMA3_0_CC0_ERRINT, (EventCombiner_FuncPtr)Edma3CCErrorIsr, NULL, TRUE);
    EventCombiner_enableEvent(SYS_INT_EDMA3_0_CC0_ERRINT);
}
/*****************************************************************************/
/*
** Macros which defines the data integrity status.
**
*/
#define NAND_DATA_INTEGRITY_PASS                (0)
#define NAND_DATA_INTEGRITY_FAIL                (1)

//#define NAND_DATA_XFER_MODE                     (NAND_XFER_MODE_DMA)
//#define NAND_BUSWIDTH                           (NAND_BUSWIDTH_16BIT)
#define NAND_CHIP_SELECT                        (1)
//#define NAND_PAGE_SIZE_IN_BYTES                 (NAND_PAGESIZE_2048BYTES)
//#define NAND_BLOCK_SIZE_IN_BYTES                (NAND_BLOCKSIZE_128KB)
//#define NAND_NUMOF_BLK                          (4096)
//#define NAND_MANUFATURER_MICRON_ID              (0x2C)
//#define NAND_DEVICE_ID                          (0xCC)


#define NAND_DATA_BUFF_SIZE                     (NAND_PAGE_SIZE_IN_BYTES)
#define NAND_ECC_BUFF_SIZE                      ((NAND_PAGE_SIZE_IN_BYTES/NAND_BYTES_PER_TRNFS) \
                                                 * NAND_MAX_ECC_BYTES_PER_TRNFS)


unsigned char eccData[NAND_ECC_BUFF_SIZE];
#pragma DATA_ALIGN(txData, 4);
volatile unsigned char txData[NAND_DATA_BUFF_SIZE];
#pragma DATA_ALIGN(rxData, 4);
volatile unsigned char rxData[NAND_DATA_BUFF_SIZE];
/******************************************************************************
**                       GLOBAL FUNCTION DEFINITIONS
*******************************************************************************/
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

int main() {
    BIOS_start();
}
/******************************************************************************
*                                                                             *
* \brief  Main Function.\n                                                    *
*                                                                             *
******************************************************************************/
int StackTest(void)
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
//    EMIFAClkConfig();
//    NANDPinMuxSetup();

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
