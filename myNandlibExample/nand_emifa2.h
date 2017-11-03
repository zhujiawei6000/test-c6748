/**
 *  \file   nand_emifa2.h
 *
 *  \brief  This file contains the NAND EMIF2.x controller specific struct 
 *          definition, macros and function prototypes.
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


#ifndef __NAND_EMIFA2_H_
#define __NAND_EMIFA2_H_

#include "nandlib.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
*                           STRUCTURES DECLARION
*******************************************************************************/

/*****************************************************************************/
/*
** Contains the timing params and base address info for the EMIFA NAND access.
*/
typedef struct _EMIFA_NAND_TIMING_INFO_
{
    unsigned int writeSetup;
    unsigned int writeStrobe;
    unsigned int writeHold;
    
    unsigned int readSetup;
    unsigned int readStrobe;
    unsigned int readHold;

    unsigned int turnAround;
}
EMIFANANDTimingInfo_t;

/*******************************************************************************
*                           FUNCTION PROTOTYPE DECLARATION
*******************************************************************************/

/* Generic EMIFA/NAND APIs */
extern NandStatus_t     EMIFANANDInit(NandInfo_t *nandInfo);
extern unsigned int     EMIFANANDWaitPinStatusGet(NandInfo_t *nandInfo);
extern NandStatus_t     EMIFANANDDMAXfer( NandInfo_t *nandInfo,
                                          unsigned char *data,
                                          unsigned int len, 
                                          NandDmaDir_t dir);


/* EMIFA/NAND ECC related APIs */
extern NandStatus_t     EMIFANANDECCInit(NandInfo_t *nandInfo);
extern void             EMIFANANDECCEnable(NandInfo_t *nandInfo);
extern void             EMIFANANDECCDisable(NandInfo_t *nandInfo);
extern void             EMIFANANDECCReadSet(NandInfo_t *nandInfo);
extern void             EMIFANANDECCWriteSet(NandInfo_t *nandInfo);
extern void             EMIFANANDECCCalculate(NandInfo_t *nandInfo, unsigned char *ptrEccData);
extern NandStatus_t     EMIFANANDECCCheckAndCorrect(NandInfo_t *nandInfo,
                                               unsigned char *eccRead,
                                               unsigned char *data);


#ifdef __cplusplus
}
#endif
#endif /* __NAND_EMIFA2_H_ */
