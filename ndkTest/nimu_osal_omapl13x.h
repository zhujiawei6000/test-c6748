/****************************************************************************
 * Copyright (c) 2017 Texas Instruments Incorporated - http://www.ti.com
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
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
 *****************************************************************************/

/*
 *   @file  nimu_osal_omapl137.h
 *
 *   @brief
 *      This file implements the OSAL layer for OMAPL13x SOC 
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>


uint8_t *Osal_platformMalloc (uint32_t num_bytes, uint32_t alignment);

void Osal_platformFree (uint8_t *dataPtr, uint32_t num_bytes);

void Osal_platformSpiCsEnter(void);

void Osal_platformSpiCsExit (void);

void
Osal_beginMemAccess
(
    void*   addr,
    Uint32  size
);

void
Osal_endMemAccess
(
    void*   addr,
    Uint32  size
);

void
Osal_enterSingleCoreCriticalSection
(
    Uint32      port_num
);

void
Osal_exitSingleCoreCriticalSection
(
    Uint32      port_num
);

void
Osal_enterMultipleCoreCriticalSection
(
    Uint32      port_num
);

void
Osal_exitMultipleCoreCriticalSection
(
    Uint32      port_num
);

void* Osal_malloc
(
    Uint32  num_bytes
);

void
Osal_free
(
    void*       ptr,
    Uint32      num_bytes
);

uint32_t Convert_CoreLocal2GlobalAddr (uint32_t  addr);



