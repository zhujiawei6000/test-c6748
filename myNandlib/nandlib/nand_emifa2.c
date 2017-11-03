/**
 *  \file   nand_emifa2.c
 *
 *  \brief  This file contains the NAND EMIFA2 controller specific functions 
 *          and these are provided as platform specific code 
 *          to achieve the required functionality.
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


#include "emifa.h"
#include "nandlib.h"
#include "nand_emifa2.h"
#include "psc.h"
#include "hw_types.h"

/*******************************************************************************
*                       INTERNAL MACRO DEFINITIONS
*******************************************************************************/

/* Maximum extended wait time                                       */
#define NAND_MAX_EXT_WAIT                       (0xFF)


/******************************************************************************
**                      INTERNAL FUNCTION DEFINITIONS
*******************************************************************************/

/**
*\brief This function does the 1-bit hamming code ECC related initializes 
*       to the NAND controller.\n
*
* \param  nandInfo      : Pointer to structure containing controller and
*                         device information.\n
*
* \return 
*        NAND_STATUS_PASSED : On success.\n
*        NAND_STATUS_FAILED : On failure.\n
*
*/
static NandStatus_t EMIFAHammingCodeECCInit(NandInfo_t *nandInfo)
{
    unsigned int baseAddr = nandInfo->hNandCtrlInfo->baseAddr;
    unsigned int cs = nandInfo->hNandCtrlInfo->currChipSelect;
    
    EMIFANANDCSSet(baseAddr, cs);

    nandInfo->hNandEccInfo->eccOffSet  = NAND_ECC_1BIT_HAMMINGCODE_OOB_OFFSET + nandInfo->pageSize;
    nandInfo->hNandEccInfo->eccByteCnt = NAND_ECC_1BIT_HAMMINGCODE_BYTECNT;

    return NAND_STATUS_PASSED;
}

/**
*\brief This function reads/calculates the 1-bit hamming code ECC values.\n
*
* \param  nandInfo      : Pointer to structure containing controller and
*                         device information.\n
*
* \param  eccResReg     : ECC Result register value.\n
*
* \param  eccRead       : Pointer where read ECC data has to store.\n
*
* \return none.\n
*
*/
static void EMIFAHammingCodeECCCalculate(NandInfo_t *nandInfo, unsigned char *ptrEccData)
{
    unsigned int baseAddr = nandInfo->hNandCtrlInfo->baseAddr;
    unsigned int cs = nandInfo->hNandCtrlInfo->currChipSelect;

    unsigned int eccVal;
   
    eccVal = EMIFANANDEccValGet(baseAddr,EMIFA_NAND_1BIT_ECC,cs);
    /* Squeeze 4 bytes ECC into 3 bytes by removing RESERVED bits
       and shifting. RESERVED bits are 31 to 28 and 15 to 12. */
    eccVal = (eccVal & 0x00000fff) | ((eccVal & 0x0fff0000) >> 4);

    /* Invert so that erased block ECC is correct */
    eccVal = ~eccVal;

    *ptrEccData++ = (unsigned char)(eccVal >>  0);
    *ptrEccData++ = (unsigned char)(eccVal >>  8);
    *ptrEccData++ = (unsigned char)(eccVal >> 16);
}

/**
* \brief This function checks for ECC errors using 1-bit hamming code algorithm
*        and correct if any ECC errors.\n
*
* \param  nandInfo      : Pointer to structure containing controller and
*                         device information.\n
*
* \param   eccRead      : Pointer to the ECC data which is read from the spare
*                         area.\n
*
* \param   data         : Pointer to the data, where if an ecc error need to 
*                         correct.\n
*
* \return ECC correction Status.\n
*    NAND_STATUS_PASSED                        : If no ecc errors.\n
*    NAND_STATUS_READ_ECC_ERROR_CORRECTED      : If error are corrected.\n
*    NAND_STATUS_READ_ECC_UNCORRECTABLE_ERROR  : If errors are uncorrectable.\n
*
*/
static NandStatus_t EMIFAHammingCodeECCCheckAndCorrect(NandInfo_t *nandInfo,
                                                      unsigned char *eccRead,
                                                      unsigned char *data)
{
    NandStatus_t retVal;
    unsigned char eccCalc[4];
    unsigned int readEccVal;
    unsigned int calcEccVal;
    unsigned int eccDiffVal;
    unsigned int bitPos;
    unsigned int bytePos;

    retVal = NAND_STATUS_PASSED;

    EMIFAHammingCodeECCCalculate(nandInfo, &eccCalc[0]);
    readEccVal = eccRead[0] | (eccRead[1] << 8) | (eccRead[2] << 16);
    calcEccVal = eccCalc[0] | (eccCalc[1] << 8) | (eccCalc[2] << 16);
    eccDiffVal = readEccVal ^ calcEccVal;

    if(eccDiffVal)
    {
        /*
         * No error              : The ecc diff value (eccDiffVal) is 0.
         * Correctable error     : For 512-byte inputs, ecc diff value has
         *                         12 bits at 1. For 256 byte ecc diff value has
         *                         11 bits at 1.
         * ECC error             : The ecc diff value has only 1 bit at 1.
         * Non-correctable error : The ecc diff value provides all other results
         */

        /*
         * Beow condition checks for number of 1's in eccDiffValu.
         * Since Total ecc has 3bytes = 24 bits. Make 2 halfs and XOR.
         * If eccDiffVal has  12 1's, it produces the result 0xFFF.
        */
        if ((((eccDiffVal >> 12) ^ eccDiffVal) & 0xfff) == 0xfff)
        {
            /* Correctable error */
            /* Check bytePos is within NAND_BYTES_PER_TRNFS i.e 512 */
            if ((eccDiffVal >> (12 + 3)) < NAND_BYTES_PER_TRNFS)
            {
                bitPos  = 1 << ((eccDiffVal >> 12) & 7);
                bytePos = eccDiffVal >> (12 + 3);
                data[bytePos] ^= bitPos;
                retVal = NAND_STATUS_READ_ECC_ERROR_CORRECTED;
            }
            else
            {
                retVal = NAND_STATUS_READ_ECC_UNCORRECTABLE_ERROR;
            }
        }
        else if(!(eccDiffVal & (eccDiffVal - 1)))
        {
            /* Single bit ECC error in the ECC itself,nothing to fix */
            retVal = NAND_STATUS_READ_ECC_ERROR_CORRECTED;
        }
        else
        {
            retVal = NAND_STATUS_READ_ECC_UNCORRECTABLE_ERROR;
        }
    }

    return (retVal);
}

/**
*\brief This function does the Reed-Solomon ECC related initializes to the NAND 
*       controller.\n
*
* \param  nandInfo      : Pointer to structure containing controller and
*                         device information.\n
*
* \return
*        NAND_STATUS_PASSED          : On success.\n
*        NAND_STATUS_FAILED          : On failure.\n
*        NAND_STATUS_ECC_UNSUPPORTED : If unsupported ECC is used.\n
*
*/
static NandStatus_t EMIFAReedSolomonECCInit(NandInfo_t *nandInfo)
{
    unsigned int baseAddr = nandInfo->hNandCtrlInfo->baseAddr;
    unsigned int cs = nandInfo->hNandCtrlInfo->currChipSelect;

    EMIFANANDCSSet(baseAddr, cs);
    EMIFANAND4BitECCSelect(baseAddr,cs);
    
    nandInfo->hNandEccInfo->eccOffSet  = nandInfo->pageSize;
    nandInfo->hNandEccInfo->eccByteCnt = NAND_ECC_RS_4BIT_UNUSED_BYTECNT + NAND_ECC_RS_4BIT_BYTECNT;
    
    return NAND_STATUS_PASSED;
}

/**
*\brief This function reads/calculates the Reed-Solomon 4-bit andc 8-bit ECC values.\n
*
* \param  nandInfo      : Pointer to structure containing controller and
*                         device information.\n
*
* \param  ptrEccData    : Pointer where read ECC data has to store.\n
*
* \return none.\n
*
*/
static void EMIFAReedSolomonECCCalculate(NandInfo_t *nandInfo, unsigned char *ptrEccData)
{
    int i = 0;
    unsigned int eccParity[8];
    unsigned int baseAddr = nandInfo->hNandCtrlInfo->baseAddr;
        
    for(i = 0; i < 8; i++)
    { 
        eccParity[i] = EMIFANANDEccValGet(baseAddr, EMIFA_NAND_4BIT_ECC, i+1);
    }
    
    /* Convert 8 10-bit values into 10 8-bit values */
    ptrEccData[NAND_ECC_RS_4BIT_UNUSED_BYTECNT + 0] = eccParity[0] & (0xFFu);
    ptrEccData[NAND_ECC_RS_4BIT_UNUSED_BYTECNT + 1] = ((eccParity[1] & (0x3Fu)) << 2) |
                    ((eccParity[0] & (0x300u)) >> 8);
    ptrEccData[NAND_ECC_RS_4BIT_UNUSED_BYTECNT + 2] = ((eccParity[2] & (0x0Fu)) << 4) |
                    ((eccParity[1] & (0x3C0u)) >> 6);
    ptrEccData[NAND_ECC_RS_4BIT_UNUSED_BYTECNT + 3] = ((eccParity[3] & (0x03u)) << 6) |
                    ((eccParity[2] & (0x3F0u)) >> 4);
    ptrEccData[NAND_ECC_RS_4BIT_UNUSED_BYTECNT + 4] = ((eccParity[3] & (0x3FCu))>> 2);
    ptrEccData[NAND_ECC_RS_4BIT_UNUSED_BYTECNT + 5] = eccParity[4] & (0xFFu);
    ptrEccData[NAND_ECC_RS_4BIT_UNUSED_BYTECNT + 6] = ((eccParity[5] & (0x3Fu)) << 2) |
                    ((eccParity[4] & (0x300u)) >> 8);
    ptrEccData[NAND_ECC_RS_4BIT_UNUSED_BYTECNT + 7] = ((eccParity[6] & (0x0Fu)) << 4) |
                    ((eccParity[5] & (0x3C0u)) >> 6);
    ptrEccData[NAND_ECC_RS_4BIT_UNUSED_BYTECNT + 8] = ((eccParity[7] & (0x03u)) << 6) |
                    ((eccParity[6] & (0x3F0u)) >> 4);
    ptrEccData[NAND_ECC_RS_4BIT_UNUSED_BYTECNT + 9] = ((eccParity[7] & (0x3FCu))>> 2);            

}

/**
* \brief This function checks for ECC errors using Reed-Solomon algorithm and corrects
*        if any ECC errors. \n
*
* \param  nandInfo      : Pointer to structure containing controller and
*                         device information.\n
*
* \param   eccRead      : Pointer to the ECC data which is read from the spare
*                         area.\n
*
* \param   data         : Pointer to the data, where if an ecc error need to 
*                         correct.\n
*
* \return ECC correction Status.\n
*    NAND_STATUS_PASSED                        : If no ecc errors.\n
*    NAND_STATUS_READ_ECC_ERROR_CORRECTED      : If error are corrected.\n
*    NAND_STATUS_READ_ECC_UNCORRECTABLE_ERROR  : If errors are uncorrectable.\n
*
*/
static NandStatus_t EMIFAReedSolomonECCCheckAndCorrect(NandInfo_t *nandInfo,
                                              unsigned char *eccRead,
                                              unsigned char *data)
{
    int i = 0;

    NandStatus_t retVal = NAND_STATUS_PASSED;
    unsigned int eccVal;
    
    unsigned int baseAddr = nandInfo->hNandCtrlInfo->baseAddr;
    unsigned int eccParity1[8];    

    
    /* Dummy read to clear the 4BITECCSTART bit */
    for(i = 1; i <= 8; i++)
    {
        EMIFANANDEccValGet(baseAddr,EMIFA_NAND_4BIT_ECC,i);
    }
    retVal = NAND_STATUS_PASSED;
    
    /* Convert 10 8-bit values into 8 10-bit values */
    eccParity1[0] = ((eccRead[NAND_ECC_RS_4BIT_UNUSED_BYTECNT + 1] & (0x3u)) << 8) |
                    (eccRead[NAND_ECC_RS_4BIT_UNUSED_BYTECNT + 0]);
    eccParity1[1] = ((eccRead[NAND_ECC_RS_4BIT_UNUSED_BYTECNT + 2] & (0xFu)) << 6) |
                    ((eccRead[NAND_ECC_RS_4BIT_UNUSED_BYTECNT + 1] & (0xFCu)) >> 2);
    eccParity1[2] = ((eccRead[NAND_ECC_RS_4BIT_UNUSED_BYTECNT + 3] & (0x3Fu)) << 4) |
                    ((eccRead[NAND_ECC_RS_4BIT_UNUSED_BYTECNT + 2] & (0xF0u)) >> 4);
    eccParity1[3] = (unsigned int)((eccRead[NAND_ECC_RS_4BIT_UNUSED_BYTECNT + 4] << 2) |
                    ((eccRead[NAND_ECC_RS_4BIT_UNUSED_BYTECNT + 3] & (0xC0u)) >> 6));
    eccParity1[4] = ((eccRead[NAND_ECC_RS_4BIT_UNUSED_BYTECNT + 6] & (0x3u)) << 8) |
                    (eccRead[NAND_ECC_RS_4BIT_UNUSED_BYTECNT + 5]);
    eccParity1[5] = ((eccRead[NAND_ECC_RS_4BIT_UNUSED_BYTECNT + 7] & (0xFu)) << 6) |
                    ((eccRead[NAND_ECC_RS_4BIT_UNUSED_BYTECNT + 6] & (0xFCu)) >> 2);
    eccParity1[6] = ((eccRead[NAND_ECC_RS_4BIT_UNUSED_BYTECNT + 8] & (0x3Fu)) << 4) |
                    ((eccRead[NAND_ECC_RS_4BIT_UNUSED_BYTECNT + 7] & (0xF0u)) >> 4);
    eccParity1[7] = (unsigned int)((eccRead[NAND_ECC_RS_4BIT_UNUSED_BYTECNT + 9] << 2) |
                    ((eccRead[NAND_ECC_RS_4BIT_UNUSED_BYTECNT + 8] & (0xC0u)) >> 6));
                
    /*
     * Write the parity values in the NAND Flash 4-bit ECC Load
     * register. The parity data is written in reverse order to
     * ECC Load reg.Write each parity value one at a time starting
     * from 4bit_ecc_val8 to 4bit_ecc_val1.                         */     
    for (i = 7; i >= 0; i--)
    {
        EMIFANAND4BitECCLoad(baseAddr,eccParity1[i]);
    }
   /*
    * Perform a dummy read to the EMIF Revision Code and Status 
    * register. This is required to ensure time for syndrome 
    * calculation after writing the ECC values in previous step    */
    EMIFAModuleIdRead(baseAddr);
    
   /*
    * Read the syndrome from the NAND Flash 4-Bit ECC 1-4 
    * registers. A syndrome value of 0 means no bit errors. If 
    * the syndrome is non-zero then go further otherwise return.   */
    for(i = 1; i <= 8; i++)
    {
        eccVal = EMIFANANDEccValGet(baseAddr,EMIFA_NAND_4BIT_ECC,i);
        if(eccVal != 0)
        {
            retVal = NAND_STATUS_READ_ECC_ERROR_CORRECTED;
            break;
        }
    }
    
    /* One or more errors appears to exist, try to correct */
    if ( retVal != NAND_STATUS_PASSED)
    {
        unsigned int eccState = 0;
        
        /*
         * Clear any previous address calculation by doing a dummy read
         * of an error address register.                               
         */
        EMIFANAND4BitEccErrAddrGet(baseAddr,EMIFA_4BITECC_ERRADDR_INDEX_1);
        EMIFANAND4BitEccErrAddrGet(baseAddr,EMIFA_4BITECC_ERRADDR_INDEX_2);
        EMIFANAND4BitEccErrAddrGet(baseAddr,EMIFA_4BITECC_ERRADDR_INDEX_3);
        EMIFANAND4BitEccErrAddrGet(baseAddr,EMIFA_4BITECC_ERRADDR_INDEX_4);
             
        /* Start the Error Address Calculation */
        EMIFANAND4BitECCAddrCalcStart(baseAddr);

        /* Wait for the 4-bit ECC state to be 1,2 or 3 */
        do {
            eccState = EMIFANAND4BitECCStateGet(baseAddr);
        } while (eccState >= 0x04);

        if(eccState == EMIFA_4BITECC_CORRECTION_ECCSTATE_0)
        {
            retVal = NAND_STATUS_PASSED;
        }
        else if(eccState == EMIFA_4BITECC_CORRECTION_ECCSTATE_2 || 
                eccState == EMIFA_4BITECC_CORRECTION_ECCSTATE_3)
        {
            unsigned int numErrors, errAddr, errVal;
            /* Read the number of errors */
            numErrors = EMIFANAND4BitECCNumOfErrsGet(baseAddr);
            for (i = 0; i <= numErrors; i++)
            {
               errAddr = EMIFANAND4BitEccErrAddrGet(baseAddr,(i+1));
               errVal  = EMIFANAND4BitEccErrValGet(baseAddr,(i+1));

               /* Address of the errored byte */         
               errAddr = ((NAND_BYTES_PER_TRNFS + 7) - errAddr);
               /* Correct the error */
               data[errAddr] ^= (unsigned char)(errVal);
            }
            retVal = NAND_STATUS_READ_ECC_ERROR_CORRECTED;
        }
        else
        {
            retVal = NAND_STATUS_READ_ECC_UNCORRECTABLE_ERROR;  
        }
    }

    return (retVal);
}


/******************************************************************************
**                       GLOBAL FUNCTION DEFINITIONS
*******************************************************************************/

/**
* \brief  Function to get the wait pin status.\n
*
* \param  nandInfo      : Pointer to structure containing controller and 
*                       : device information.\n
*
* \return Wait pin status.\n
*
*         0 : If the status is active low.\n
*         1 : If the status is active high.\n
*
*/
unsigned int EMIFANANDWaitPinStatusGet(NandInfo_t *nandInfo)
{
    unsigned int pinStatus;

    pinStatus = EMIFAWaitPinStatusGet(nandInfo->hNandCtrlInfo->baseAddr, nandInfo->hNandCtrlInfo->waitPin);

    return(pinStatus);
}

/**
* \brief  Function to initialize the NAND controller.\n
*
* \param  nandCtrlInfo  : Pointer to structure containing controller info.\n
*
* \return 
*
*        NAND_STATUS_PASSED          : On success.\n
*        NAND_STATUS_FAILED          : On failure.\n
*
*/
NandStatus_t EMIFANANDInit(NandInfo_t *nandInfo)
{
    unsigned int wtTimeConf = 0;    
    unsigned int baseAddr = nandInfo->hNandCtrlInfo->baseAddr;
    unsigned int cs = nandInfo->hNandCtrlInfo->currChipSelect;
    EMIFANANDTimingInfo_t *nandTimingInfo;
    
    nandTimingInfo = (EMIFANANDTimingInfo_t *) nandInfo->hNandCtrlInfo->hNandTimingInfo;
                     
    /* Enable the NAND for selected chip-select */
    EMIFANANDCSSet(baseAddr, cs);

    /* Enable the Extended wait cycles */
    /* Set the Max extended wait cycles */
    EMIFAMaxExtWaitCycleSet(baseAddr, NAND_MAX_EXT_WAIT);
    /*  Select pin0 to control external wait states. */
    EMIFACSWaitPinSelect(baseAddr, cs, nandInfo->hNandCtrlInfo->waitPin);
    /* set the polarity if EMA_WAIT[0] pin. */
    EMIFAWaitPinPolaritySet(baseAddr, nandInfo->hNandCtrlInfo->waitPin,
                            nandInfo->hNandCtrlInfo->waitPinPol);

    /* Disable the interrupts */
    EMIFAMskedIntDisable(baseAddr,EMIFA_ASYNC_TIMOUT_INT);
    EMIFAMskedIntDisable(baseAddr,EMIFA_LINE_TRAP_INT);
    EMIFAMskedIntDisable(baseAddr,EMIFA_WAIT_RISE_INT);

    /* Configure the EMIFA Async timings */    
    wtTimeConf = EMIFA_ASYNC_WAITTIME_CONFIG( nandTimingInfo->writeSetup,
                                              nandTimingInfo->writeStrobe, 
                                              nandTimingInfo->writeHold, 
                                              nandTimingInfo->readSetup, 
                                              nandTimingInfo->readStrobe, 
                                              nandTimingInfo->readHold,
                                              nandTimingInfo->turnAround);
    EMIFAWaitTimingConfig(baseAddr, cs, wtTimeConf);    

    /* Set the bus width */
    if ( nandInfo->busWidth == NAND_BUSWIDTH_16BIT)
    {
      EMIFAAsyncDevDataBusWidthSelect(baseAddr,cs, EMIFA_DATA_BUSWITTH_16BIT);    
    }
    else
    {
      EMIFAAsyncDevDataBusWidthSelect(baseAddr,cs, EMIFA_DATA_BUSWITTH_8BIT);    
    }
    
    return(NAND_STATUS_PASSED);
}

/**
*\brief This function does the ECC related initializes to the NAND controller 
*       depending on the ecc type.\n
*
* \param  nandInfo      : Pointer to structure containing controller and
*                         device information.\n
*
* \return 
*        NAND_STATUS_PASSED : On success.\n
*        NAND_STATUS_FAILED : On failure.\n
*
*/
NandStatus_t EMIFANANDECCInit(NandInfo_t *nandInfo)
{
    NandStatus_t retVal;
    retVal = NAND_STATUS_PASSED;

    if(nandInfo->eccType == NAND_ECC_ALGO_HAMMING_1BIT)
    {
        retVal = EMIFAHammingCodeECCInit(nandInfo);        
    }
    else if(nandInfo->eccType == NAND_ECC_ALGO_RS_4BIT)
    {
        retVal = EMIFAReedSolomonECCInit(nandInfo);        
    }    

    return(retVal);
}

/**
*\brief This function enable the ECC.\n
*
* \param  nandCtrlInfo  : Pointer to structure containing controller info.\n
*
* \return none.\n
*
*/
void EMIFANANDECCEnable(NandInfo_t *nandInfo)
{
    unsigned int baseAddr = nandInfo->hNandCtrlInfo->baseAddr;
    unsigned int cs = nandInfo->hNandCtrlInfo->currChipSelect;

    if(nandInfo->eccType == NAND_ECC_ALGO_HAMMING_1BIT)
    {
        EMIFANANDECCStart(baseAddr, EMIFA_NAND_1BIT_ECC, cs);
    }
    else if(nandInfo->eccType == NAND_ECC_ALGO_RS_4BIT)
    {
        EMIFANANDECCStart(baseAddr, EMIFA_NAND_4BIT_ECC, cs);
    }
}

/**
*\brief This function disable the ECC.\n
*
* \param  nandCtrlInfo  : Pointer to structure containing controller info.\n
*
* \return none.\n
*
*/
void EMIFANANDECCDisable(NandInfo_t *nandInfo)
{
    /* Nothing to do here */
}

/**
*\brief  This Function does the ECC setting for write.\n
*
* \param  nandInfo      : Pointer to structure containing controller and
*                         device information.\n
*
* \return none.\n
*
*/
void EMIFANANDECCWriteSet(NandInfo_t *nandInfo)
{
    /* Nothing to do here */
}

/**
*\brief  This Function does the ECC setting for read.\n
*
* \param  nandInfo      : Pointer to structure containing controller and
*                         device information.\n
*
* \return none.\n
*
*/
void EMIFANANDECCReadSet(NandInfo_t *nandInfo)
{
    /* Nothing to do here */
}

/**
*\brief This function read the ecc data.\n
*
* \param  nandInfo      : Pointer to structure containing controller and
*                         device information.\n
*
* \param   eccRead      : Pointer where read ECC data has to store.\n
*
*
* \return none.\n
*
*/
void EMIFANANDECCCalculate(NandInfo_t *nandInfo, unsigned char *ptrEccData)
{
    if(nandInfo->eccType == NAND_ECC_ALGO_HAMMING_1BIT)
    {
        EMIFAHammingCodeECCCalculate(nandInfo, ptrEccData);
    }
    else if(nandInfo->eccType == NAND_ECC_ALGO_RS_4BIT)
    {
        EMIFAReedSolomonECCCalculate(nandInfo, ptrEccData);
    }
}

/**
*\brief This function checks and corrects ECC errors.\n
*
* \param  nandInfo      : Pointer to structure containing controller and
*                         device information.\n
*
* \param   eccRead      : Pointer to the ECC data which is read from the spare 
*                         area.\n
*
* \param   data         : Pointer to the data, where if an ecc error need to 
*                         correct.\n
*
* \return ECC correction Status.\n
*    NAND_STATUS_PASSED                        : If no ecc errors.\n
*    NAND_STATUS_READ_ECC_ERROR_CORRECTED      : If error are corrected.\n
*    NAND_STATUS_READ_ECC_UNCORRECTABLE_ERROR  : If errors are uncorrectable.\n
*
*/
NandStatus_t EMIFANANDECCCheckAndCorrect(NandInfo_t *nandInfo,
                                        unsigned char *eccRead,
                                        unsigned char *data)
{
    NandStatus_t retVal = NAND_STATUS_PASSED;

    if(nandInfo->eccType == NAND_ECC_ALGO_HAMMING_1BIT)
    {
        retVal = EMIFAHammingCodeECCCheckAndCorrect(nandInfo, eccRead, data);
    }
    else if(nandInfo->eccType == NAND_ECC_ALGO_RS_4BIT)
    {
        retVal = EMIFAReedSolomonECCCheckAndCorrect(nandInfo, eccRead, data);
    }
    
    return(retVal);
}

/**
*\brief This function transfer the data to NAND through the DMA depeding on
*       the direction.\n
*
* \param  nandInfo      : Pointer to structure containing controller and
*                         device information.\n
*
* \param   data         : Pointer to the data.\n 
*
* \param   len          : Transfer length.\n 
*
* \param   dir          : Direction of transfer.\n
*                         This can take one of the following values :\n
*                         NAND_EDMA_READ  -- Read from NAND.\n
*                         NAND_EDMA_WRITE -- Write to NAND.\n
*
* \return EDMA transfer status.\n
*
*         NAND_STATUS_PASSED             : if transfer is successful.\n
*         NAND_STATUS_READWRITE_DMA_FAIL : if transfer is failed.\n
*/

NandStatus_t EMIFANANDDMAXfer(NandInfo_t *nandInfo, unsigned char *data,
                             unsigned int len, NandDmaDir_t dir)
{
    unsigned int result = 0;
    NandStatus_t retVal = NAND_STATUS_PASSED;
    unsigned int timeOut = 0xFFFFF;;

    /* Start Transfer */
    (nandInfo->hNandDmaInfo->DMAXferSetup)(nandInfo, (unsigned char *)data, len, dir);

    /* Wait till EDMA transfer is complete. */
    while((result == 0) && (--timeOut))
    {
        result = (nandInfo->hNandDmaInfo->DMAXferStatusGet)(nandInfo);
    }
    
    if(timeOut == 0)
    {
        retVal = NAND_STATUS_READWRITE_DMA_FAIL;
    }
    
    return (retVal);
}


/******************************************************************************
**                              END OF FILE
*******************************************************************************/
