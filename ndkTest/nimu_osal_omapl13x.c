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
 *   @file  platform_osal.c
 *
 *   @brief
 *      This file implements the OSAL layers for the various LLD's and libraries that the application
 *      use.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/* NIMU*/
#include "ti/transport/ndk/nimu/src/v6/nimu_eth.h"

/* CSL CHIP, SEM Functional layer includes */
//#ifdef _TMS320C6X
#include <ti/csl/csl_chip.h>
#include <ti/csl/csl_semAux.h>

#include <ti/csl/csl_chipAux.h>
//#endif
/* CSL Cache module includes */
#include <ti/csl/csl_cacheAux.h>

/* XDC/BIOS includes */
#include <xdc/std.h>
#include <xdc/runtime/IHeap.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Memory.h>
#include <xdc/runtime/Error.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/heaps/HeapBuf.h>
#include <ti/sysbios/heaps/HeapMem.h>
#include <ti/osal/CacheP.h>

/**********************************************************************
 *********************** Platform Library OSAL Functions **************
 **********************************************************************/
/**
 * ============================================================================
 *  @n@b Osal_platformMalloc
 *
 *  @b  brief
 *  @n  This routine implements the memory handler for Platform Library.
 *
 *  @param[in]  num_bytes
 *      Number of bytes to be allocated.
 *
 *  @return
 *      Allocated block address
 * =============================================================================
 */
uint8_t *Osal_platformMalloc (uint32_t num_bytes, uint32_t alignment)
{
	Error_Block	    errorBlock;

	/* Allocate memory.  */
	Error_init(&errorBlock);

	return Memory_alloc(NULL, num_bytes, alignment, &errorBlock);
}

/**
 * ============================================================================
 *  @n@b Osal_platformFree
 *
 *  @b  brief
 *  @n  This routine implements the memory free handler for Platform Library.
 *		Frees up memory allocated using
 *      @a Osal_platformMalloc ()
 *
 *  @param[in]  dataPtr
 *      Pointer to the memory block to be cleaned up.
 *
 *  @param[in]  num_bytes
 *      Size of the memory block to be cleaned up.
 *
 *  @return
 *      Not Applicable
 * =============================================================================
 */
void Osal_platformFree (uint8_t *dataPtr, uint32_t num_bytes)
{

    /* Free up the memory */
    if (dataPtr)
    {
        Memory_free(NULL, dataPtr, num_bytes);
    }
}

/**
 * ============================================================================
 *  @n@b Osal_platformSpiCsEnter
 *
 *  @b  brief
 *  @n  This API ensures multi-core and multi-threaded
 *      synchronization for the SPI bus.
 *
 *      This is a BLOCKING API.
 *.
 *
 *  @param[in]
 *  @n  None
 *
 *  @return
 *  @n  Nothing
 * =============================================================================
 */
void Osal_platformSpiCsEnter(void)
{
    return;
}

/**
 * ============================================================================
 *  @n@b Osal_platformSpiCsExit
 *
 *  @b  brief
 *  @n  This API needs to be called to exit a previously
 *      acquired critical section lock using @a Osal_platformSpiCsEnter ()
 *      API. It resets the multi-core and multi-threaded lock,
 *      enabling another process/core to grab the SPI bus.
 *
 *
 *  @return     None
 * =============================================================================
 */
void Osal_platformSpiCsExit (void)
{
    return;
}


/**********************************************************************
 *********************** EMAC OSAL Functions **************************
 **********************************************************************/

 /**
 *  @b Description
 *  @n
 *      The function is used to indicate that a block of memory is
 *      about to be accessed. If the memory block is cached then this
 *      indicates that the application would need to ensure that the
 *      cache is updated with the data from the actual memory.
 *
 *  @param[in]  addr
 *       Address of memory block
 *  @param[in]  size
 *       Size of memory block in bytes
 *
 *  @retval
 *      None
 */
void
Osal_beginMemAccess
(
    void*   addr,
    Uint32  size
)
{

    (void)addr;
    (void)size;

    return;
}

/**
 *  @b Description
 *  @n
 *      The function is used to indicate that the block of memory has
 *      finished being accessed. If the memory block is cached then the
 *      application would need to ensure that the contents of the cache
 *      are updated immediately to the actual memory.
 *
 *  @param[in]  addr
 *       Address of memory block
 *  @param[in]  size
 *       Size of memory block in bytes
 *
 *  @retval
 *      None
 */
void
Osal_endMemAccess
(
    void*   addr,
    Uint32  size
)
{
    (void)addr;
    (void)size;

    return;
}

/**
 *  @b Description
 *  @n
 *      The function is used to provide critical section to prevent access of shared
 *      resources from single core and multiple threads.
 *
 *  @param[in]  port_num
 *      EMAC port number which needs critical section to protect its resources.
 *
 *  @retval
 *      None
 */
void
Osal_enterSingleCoreCriticalSection
(
    Uint32      port_num
)
{
#ifdef _TMS320C6X
    if (port_num == 0)
    {
        IER &= ~NIMU_EMAC0_INT_FLAG;
    }
    else
    {
        IER &= ~NIMU_EMAC1_INT_FLAG;
    }
#endif
}

/**
 *  @b Description
 *  @n
 *      The function is called to end the critical section access of shared resources
 *      from single cores.
 *
 *  @param[in]  port_num
 *      EMAC port number which needs critical section to protect its resources.
 *
 *  @retval
 *      None
 */
void
Osal_exitSingleCoreCriticalSection
(
    Uint32      port_num
)
{
#ifdef _TMS320C6X
    if (port_num == 0)
    {
        IER |= NIMU_EMAC0_INT_FLAG;
    }
    else
    {
        IER |= NIMU_EMAC1_INT_FLAG;
    }
#endif
}

/**
 *  @b Description
 *  @n
 *      The function is used to provide critical section to prevent access of shared
 *      resources from multiple cores
 *
 *  @param[in]  port_num
 *      EMAC port number which needs critical section to protect its resources.
 *
 *  @retval
 *      None
 */
void
Osal_enterMultipleCoreCriticalSection
(
    Uint32      port_num
)
{
	(void)port_num;
    return;
}

/**
 *  @b Description
 *  @n
 *      The function is called to end the critical section access of shared resources
 *      from multiple cores.
 *
 *  @param[in]  port_num
 *      EMAC port number which needs critical section to protect its resources.
 *
 *  @retval
 *      None
 */
void
Osal_exitMultipleCoreCriticalSection
(
    Uint32      port_num
)
{
	(void)port_num;
    return;
}

/**
 *  @b Description
 *  @n
 *      The function is used to allocate a memory block of the specified size.
 *
 *  @param[in]  num_bytes
 *      Number of bytes to be allocated.
 *
 *  @retval
 *      Allocated block address
 */
void* Osal_malloc
(
    Uint32  num_bytes
)
{
    return mmAlloc(num_bytes);
}

/**
 *  @b Description
 *  @n
 *      The function is used to clean up a specific memory block
 *
 *  @param[in]  ptr
 *      Pointer to the memory block to be cleaned up.
 *  @param[in]  num_bytes
 *      Size of the memory block being cleaned up.
 *
 *  @retval
 *      Not Applicable
 */
void
Osal_free
(
    void*       ptr,
    Uint32      num_bytes
)
{
    (void)num_bytes;
    mmFree(ptr);
}

uint32_t Convert_CoreLocal2GlobalAddr (uint32_t  addr)
{
    uint32_t coreNum;

    /* Get the core number. */
    coreNum = 0;
    /* Check if the address is a valid Local L2 address to convert */
    if ( (addr >= (uint32_t) 0x800000) &&
         (addr <  (uint32_t) 0x880000) ) {
         /* Compute the global address. */
         return ((1 << 28) | (coreNum << 24) | (addr & 0x00ffffff));
    }
    else {
        return (addr);
    }
}


