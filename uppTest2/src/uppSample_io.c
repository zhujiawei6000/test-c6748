/*
 * uppSample_io.c
 *
 * This file contains the test / demo code to demonstrate the uPP driver
 * dlb mode functionality on DSP/BIOS 5.
 *
 * Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
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

/**
 * \file      uppSample_io.c
 *
 * \brief     uPP DLB mode sample application
 *
 *            This file demonstrates the usage of uPP driver in the DLB mode
 *            of operation
 *
 *            (C) Copyright 2009, Texas Instruments, Inc
 *
 * \version   0.2
 *            \author Imtiaz SMA
 *            \date   Dec 14 2009
 *
 *            1. Changes for PSP coding guidelines.
 *            2. New dev and chan param structure.
 *            3. Concept of multiple independent channels introduced.
 *
 * \version   0.1
 *            \author Joe Coombs
 *            \date   Aug 05 2009
 *
 *            Initial version
 */

/* ========================================================================== */
/*                            INCLUDE FILES                                   */
/* ========================================================================== */

#include <xdc/std.h>
#include <string.h>
#include <xdc/runtime/Memory.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/log.h>
#include <ti/sysbios/knl/task.h>
#include <ti/board/board.h>
#include <xdc/runtime/Log.h>
#include <ti/sysbios/family/c674/Power.h>
#include "upp/Upp.h"
#include <ti/board/board.h>
#include <ti/drv/uart/UART_stdio.h>

#include <ti/sysbios/io/GIO.h>

/* ========================================================================== */
/*                        IMPORTED VARIABLES                                  */
/* ========================================================================== */


/* ========================================================================== */
/*                         MACRO DEFINITIONS                                  */
/* ========================================================================== */

#define NUM_BUFS              4             /* Max of 1 outstanding requests  */
#define BUFALIGN              128           /* align buffers to 128 bytes     */
#define BUFSIZE               1024          /* 1K of data transceive          */
#define LOOP_COUNT            100           /* no of iterations               */

/* ========================================================================== */
/*                          LOCAL VARIABLES                                   */
/* ========================================================================== */

/* Handles for the TX and RX channels                                         */
static GIO_Handle uppOutHandle = NULL;
static GIO_Handle uppInHandle = NULL;

/* array to hold the pointer to the allocated buffers                         */
Ptr buf[2 * NUM_BUFS];

/* Data structures to be used for submit                                      */
Upp_transParam  transParam[2 * NUM_BUFS];

/* currently processed packet count                                           */
Uint32 pktCount = 0;

/* parameters for channel A                                                   */
Upp_ChanParams uppChanparamA =
{
    TRUE,
    Upp_ChanSel_A,
    Upp_bitWidth_8,
    Upp_dataRate_SINGLE,
    Upp_ChanMode_NORMAL,
    Upp_dataPackFmt_RJZE,
    75000000,
    0xFFFF,
    NULL,
    NULL,
    NULL,
    NULL,
    Upp_fifoThreshold_64,
    {
        TRUE,
        Upp_polarity_ACTIVE_HIGH,
        TRUE,
        Upp_polarity_ACTIVE_HIGH,
        TRUE,
        Upp_polarity_ACTIVE_HIGH,
        Upp_clkPol_RISING_EDGE_SYNC,
        Upp_PinIdleState_IDLE_VAL,
    }
};

/* parameters for channel B                                                   */
Upp_ChanParams uppChanparamB =
{
    TRUE,
    Upp_ChanSel_B,
    Upp_bitWidth_8,
    Upp_dataRate_SINGLE,
    Upp_ChanMode_NORMAL,
    Upp_dataPackFmt_RJZE,
    75000000,
    0xFFFF,
    NULL,
    NULL,
    NULL,
    NULL,
    Upp_fifoThreshold_64,
    {
        TRUE,
        Upp_polarity_ACTIVE_HIGH,
        TRUE,
        Upp_polarity_ACTIVE_HIGH,
        TRUE,
        Upp_polarity_ACTIVE_HIGH,
        Upp_clkPol_RISING_EDGE_SYNC,
        Upp_PinIdleState_IDLE_VAL,
    }
};


/* ========================================================================== */
/*                             LOCAL FUNCTIONS                                */
/* ========================================================================== */

static Void uppCreateStreams(Void);
static Void uppDriverPrime(Void);

/* ========================================================================== */
/*                           FUNCTION DEFINTIONS                              */
/* ========================================================================== */

/*
 * \brief   This function demostrates the use of Upp using an loopback
 *          communication setup.
 *
 * \param   None
 *
 * \return  None
 */
Void uppStartDemo(Void)
{
    Upp_transParam    *xmt       = NULL;
    Upp_transParam    *rcv       = NULL;
    Uint8             *tempPtr   = 0x00;
    Uint32             count     = 0;
    Int32              nmadus0   = 0;
    Uint32             i         = 0x00;
    Bool               fail      = FALSE;

    /* create the streams required for the transactions                       */
    uppCreateStreams();

    /* prime the driver                                                       */
    uppDriverPrime();

    /* reclaim each packet and resubmit for "LOOP_COUNT" number of iteration  */
    for (count = 0; count < LOOP_COUNT; count++)
    {
        nmadus0 = GIO_reclaim(uppInHandle, (Ptr *)&rcv, NULL);

        /* Reclaim full buffer from the input stream                          */
        if (nmadus0 < 0)
        {
            UART_printf(
                "\r\nError reclaiming full buffer from the input stream");
        }

        /* Reclaim empty buffer from the output stream to be reused           */
        nmadus0 = SIO_reclaim(uppOutHandle, (Ptr *)&xmt, NULL);

        if (nmadus0 < 0)
        {
            UART_printf(
                "\r\nError reclaiming empty buffer from the output stream");
        }
        else
        {
            /* compare the received data                                      */
            for (i = 0; i < BUFSIZE; i++)
            {
                tempPtr = (Uint8 *)rcv->windowAddr;

                if (tempPtr[i] != (i % 0x100))
                {
                    UART_printf("Iteration %d",count);
                    UART_printf("Data compare failed %x %x",
                        (i % 0x100),tempPtr[i]);
                    fail = TRUE;
                    break;
                }
            }

            if (TRUE == fail)
            {
                break;
            }
        }

        /* Issue an empty buffer to the input stream                          */
        if (GIO_issue(uppInHandle, rcv, BUFSIZE, NULL) != SYS_OK)
        {
            UART_printf("\r\nFailed to issue empty buffer to stream\n");
        }

        /* Issue full buffer to the output stream                             */
        if (GIO_issue(uppOutHandle, xmt, BUFSIZE, NULL) != SYS_OK)
        {
            UART_printf("\r\nFailed to issue empty buffer to stream\n");
        }
        pktCount++;
    }

    if (FALSE == fail)
    {
        UART_printf("Data compare passed.");
    }
}

/**
 * \brief   Function to submit requests to the driver.Depending on the macro
 *          "NUM_BUFS" either the driver can be buffered with multiple requests
 *          or only one buffer by using the NUM_BUFS as 1.
 *
 * \param   None
 *
 * \return  None
 */
static Void uppDriverPrime(Void)
{
    Uint32            count      = 0;

    for (count = NUM_BUFS; count < (2 * NUM_BUFS); count++)
    {
        transParam[count].windowAddr = buf[count];
        transParam[count].bytesPerLine = BUFSIZE;
        transParam[count].lineCount = 1;
        transParam[count].lineOffset = 0;

        if (IOM_COMPLETED
            != GIO_issue(uppInHandle, (Ptr)&transParam[count], BUFSIZE, NULL))
        {
            UART_printf("Issue to Input stream failed.");
            System_abort("Issue to input stream failed\n");
        }
    }

    for (count = 0; count < NUM_BUFS; count++)
    {
        transParam[count].windowAddr = buf[count];
        transParam[count].bytesPerLine = BUFSIZE;
        transParam[count].lineCount = 1;
        transParam[count].lineOffset = 0;

        if (IOM_COMPLETED
            != GIO_issue(uppOutHandle, (Ptr)&transParam[count], BUFSIZE, NULL))
        {
            UART_printf("Issue to output stream failed.");
            System_abort("Issue to output stream failed\n");
        }
    }
    UART_printf("Requests submitted to the uPP driver.");
}

/**
 * \brief    Function to create the required streams for the transmission and
 *           reception of Upp data.
 *
 * \params   NONE
 *
 * \return   NONE
 */
Void uppCreateStreams(Void)
{
    SIO_Attrs      sioAttrs  = SIO_ATTRS;
    Uint32         count     = 0;
    Uint32         tempCount = 0;

    sioAttrs.nbufs = NUM_BUFS;
    sioAttrs.align = BUFALIGN;
    sioAttrs.model = SIO_ISSUERECLAIM;

    /* Create a TX and RX channel for the DLB mode                            */
    uppOutHandle = SIO_create("/dioUppOUT", SIO_OUTPUT, BUFSIZE, &sioAttrs);
    uppInHandle  = SIO_create("/dioUppIN", SIO_INPUT, BUFSIZE, &sioAttrs);

    if ((NULL == uppOutHandle) || (NULL == uppInHandle))
    {
        UART_printf("Stream Creation Failed");
        System_abort("Stream Creation Failed\n");
    }

    /* create the buffers required for the TX  operation                      */
    for (count = 0; count < (NUM_BUFS ); count++)
    {
        buf[count] = (Ptr)Memory_calloc(0, BUFSIZE, BUFALIGN);

        if (NULL == buf[count])
        {
            UART_printf("Memory_calloc failed.");
            System_abort("Memory_calloc failed.\n");
        }
    }

    /* create the buffers required for the RX  operation                      */
    for(count = NUM_BUFS; count < (NUM_BUFS * 2); count ++)
    {
        buf[count] = (Ptr)Memory_calloc(0, BUFSIZE, BUFALIGN);

        if (NULL == buf[count])
        {
            UART_printf("Memory_calloc failed.");
            System_abort("Memory_calloc failed.\n");
        }
    }

    /* Fill the buffers with known data and transmit the same and check if the*
     * same pattern is received on the RX channel                             */
    for (count = 0; count < (NUM_BUFS); count++)
    {
        for (tempCount = 0; tempCount < BUFSIZE; tempCount++)
        {
            ((Uint8 *)buf[count])[tempCount] = (tempCount % 0x100);
        }
    }
}

/*
 * \brief entry point function for the Upp demo task.
 *
 *       -> creates the required channels for the demonstration of the UPP
 *          working loopback mode
 *       -> compares the transmit and receive data and prints an appropriate
 *          message
 *
 * \params  None
 *
 * \return  None.
 */
Void uppDemoTask(Void)
{
    uppStartDemo();

    UART_printf("Sample Application completed...");
}

/*============================================================================*/
/*                              END OF FILE                                   */
/*============================================================================*/

