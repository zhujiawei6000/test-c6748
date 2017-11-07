/**
 *  \file   nandDma.h
 *
 *  \brief  This file contains the NAND DMA related function prototypes.
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

#ifndef _NANDDMA_H_
#define _NANDDMA_H_

#include "nandlib.h"

#ifdef __cplusplus
extern "C" {
#endif


/*******************************************************************************
*                           MACRO DEFINITIONS
*******************************************************************************/

/*******************************************************************************
*                           FUNCTION PROTOTYPE DECLARATION
*******************************************************************************/
extern void EMIFANANDEdmaInit(void *unused);
extern void EMIFANANDXferSetup(NandInfo_t *nandInfo, unsigned char *data,
                              unsigned int len, NandDmaDir_t dir);
extern unsigned int EMIFANANDXferStatusGet();

extern void Edma3CompletionIsr(void);
extern void Edma3CCErrorIsr();
//user must implement the interface using event combiner
extern void EDMA3INTCConfigure(void);


#ifdef __cplusplus
}
#endif
#endif