/*
 * uppSample_io.c
 *
 * This file contains the test / demo code to demonstrate the uPP driver
 * duplex mode in an evm to evm communication mode on DSP/BIOS 5.
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
 * \brief     uPP evm to evm mode sample application
 *
 *            This file demonstrates the usage of uPP driver in the duplex mode
 *            0 of operation
 *
 *            (C) Copyright 2009, Texas Instruments, Inc
 *
 * \version   0.1
 *            \author Imtiaz SMA
 *            \date   Dec 14 2009
 *
 *            Initial version
 */

/* ========================================================================== */
/*                            INCLUDE FILES                                   */
/* ========================================================================== */

#include <std.h>
#include <sio.h>
#include <iom.h>
#include <log.h>
#include <tsk.h>
#include <string.h>
#include "ti/pspiom/upp/Upp.h"

/* ========================================================================== */
/*                        IMPORTED VARIABLES                                  */
/* ========================================================================== */

extern LOG_Obj trace;

/* ========================================================================== */
/*                         MACRO DEFINITIONS                                  */
/* ========================================================================== */

#define NUM_BUFS              1             /* Max of 1 outstanding requests  */
#define BUFALIGN              128           /* align buffers to 128 bytes     */
#define BUFSIZE               1024          /* 1K of data transceive          */
#define LOOP_COUNT            100           /* no of iterations               */

/* ========================================================================== */
/*                          LOCAL VARIABLES                                   */
/* ========================================================================== */

/* Handles for the TX and RX channels                                         */
static SIO_Handle uppOutHandle = NULL;
static SIO_Handle uppInHandle = NULL;

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
        Upp_polarity_ACTIVE_LOW,
        TRUE,
        Upp_polarity_ACTIVE_LOW,
        FALSE,
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
        FALSE,
        Upp_polarity_ACTIVE_LOW,
        FALSE,
        Upp_polarity_ACTIVE_LOW,
        FALSE,
        Upp_polarity_ACTIVE_HIGH,
        Upp_clkPol_RISING_EDGE_SYNC,
        Upp_PinIdleState_IDLE_VAL,
    }
};


/* ========================================================================== */
/*                             LOCAL FUNCTIONS                                */
/* ========================================================================== */

static Void uppCreateStreams(Void);
static Void uppDriverPrime(Uint32  channel);
static Void uppDeleteStreams(Void);

/* ========================================================================== */
/*                           FUNCTION DEFINTIONS                              */
/* ========================================================================== */

/*
 * \brief   This function demostrates the use of Upp using an EVM to EVM
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
    uppDriverPrime(IOM_OUTPUT);

    /* reclaim each packet and resubmit for "LOOP_COUNT" number of iteration  */
    for (count = 0; count < LOOP_COUNT; count++)
    {
        /* Reclaim empty buffer from the output stream to be reused           */
        nmadus0 = SIO_reclaim(uppOutHandle, (Ptr *)&xmt, NULL);

        if (nmadus0 < 0)
        {
            LOG_printf(&trace,
                "\r\nError reclaiming empty buffer from the output stream");
        }

        TSK_sleep(100);

        /* Issue full buffer to the output stream                             */
        if (SIO_issue(uppOutHandle, xmt, BUFSIZE, NULL) != SYS_OK)
        {
            LOG_printf(&trace,"\r\nFailed to issue empty buffer to stream\n");
        }

        pktCount++;
    }

    /* reclaim all the empty transmit buffers                                 */
    for (count = 0; count < NUM_BUFS; count++)
    {
        nmadus0 = SIO_reclaim(uppOutHandle, (Ptr *)&xmt, NULL);

        if (nmadus0 < 0)
        {
            LOG_printf(&trace,
                "\r\nError reclaiming empty buffer from the output stream");
        }
    }

    /* prime the driver                                                       */
    uppDriverPrime(IOM_INPUT);

    /* reclaim each packet and resubmit for "LOOP_COUNT" number of iteration  */
    for (count = 0; count < LOOP_COUNT; count++)
    {
        nmadus0 = SIO_reclaim(uppInHandle, (Ptr *)&rcv, NULL);

        /* Reclaim full buffer from the input stream                          */
        if (nmadus0 < 0)
        {
            LOG_printf(&trace,
                "\r\nError reclaiming full buffer from the input stream");
        }
        else
        {
            /* compare the received data                                      */
            for (i = 0; i < BUFSIZE; i++)
            {
                tempPtr = (Uint8 *)rcv->windowAddr;

                if (tempPtr[i] != (i % 0x100))
                {
                    LOG_printf(&trace,"Iteration %d",count);
                    LOG_printf(&trace,"Data compare failed %x %x",
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
        if (SIO_issue(uppInHandle, rcv, BUFSIZE, NULL) != SYS_OK)
        {
            LOG_printf(&trace,"\r\nFailed to issue empty buffer to stream\n");
        }
        pktCount++;
    }

    /* reclaim all the empty receive buffers                                  */
    for (count = 0; count < NUM_BUFS; count++)
    {
        nmadus0 = SIO_reclaim(uppInHandle, (Ptr *)&xmt, NULL);

        if (nmadus0 < 0)
        {
            LOG_printf(&trace,
                "\r\nError reclaiming empty buffer from the input stream");
        }
    }

    /* delete the streams and deallocate the buffers                          */
    uppDeleteStreams();
}

/**
 * \brief   Function to submit requests to the driver.Depending on the macro
 *          "NUM_BUFS" either the driver can be buffered with multiple requests
 *          or only one buffer by using the NUM_BUFS as 1.
 *
 * \param   channel [IN] channel to be primed.
 *
 * \return  None
 */
static Void uppDriverPrime(Uint32  channel)
{
    Uint32     count      = 0;

    if (IOM_INPUT == channel)
    {
        /* prime the channel for the reception of data                        */
        for (count = NUM_BUFS; count < (2 * NUM_BUFS); count++)
        {
            transParam[count].windowAddr = buf[count];
            transParam[count].bytesPerLine = BUFSIZE;
            transParam[count].lineCount = 1;
            transParam[count].lineOffset = 0;

            if (IOM_COMPLETED
                != SIO_issue(uppInHandle, (Ptr)&transParam[count], BUFSIZE, NULL))
            {
                LOG_printf(&trace,"Issue to Input stream failed.");
                SYS_abort("Issue to input stream failed\n");
            }
        }
    }
    else
    {
        for (count = 0; count < NUM_BUFS; count++)
        {
            transParam[count].windowAddr = buf[count];
            transParam[count].bytesPerLine = BUFSIZE;
            transParam[count].lineCount = 1;
            transParam[count].lineOffset = 0;

            if (IOM_COMPLETED
                != SIO_issue(uppOutHandle, (Ptr)&transParam[count], BUFSIZE, NULL))
            {
                LOG_printf(&trace,"Issue to output stream failed.");
                SYS_abort("Issue to output stream failed\n");
            }
        }
    }
    LOG_printf(&trace,"Requests submitted to the uPP driver.");
}

/**
 * \brief    Function to create the required streams for the transmission and
 *           reception of Upp data.
 *
 * \params   None
 *
 * \return   None
 */
Void uppCreateStreams(Void)
{
    SIO_Attrs      sioAttrs  = SIO_ATTRS;
    Uint32         count     = 0;
    Uint32         tempCount = 0;

    sioAttrs.nbufs = NUM_BUFS;
    sioAttrs.align = BUFALIGN;
    sioAttrs.model = SIO_ISSUERECLAIM;

    /* Create a TX and RX channel for the duplex 0 mode (B = TX, A = RX)       */
    uppOutHandle = SIO_create("/dioUppOUT", SIO_OUTPUT, BUFSIZE, &sioAttrs);
    uppInHandle  = SIO_create("/dioUppIN", SIO_INPUT, BUFSIZE, &sioAttrs);

    if ((NULL == uppOutHandle) || (NULL == uppInHandle))
    {
        LOG_printf(&trace,"Streams Creation Failed");
        SYS_abort("Streams Creation Failed\n");
    }

    /* create the buffers required for the TX and RX operation                 */
    for (count = 0; count < (2 * NUM_BUFS ); count++)
    {
        buf[count] = (Ptr)MEM_calloc(0, BUFSIZE, BUFALIGN);

        if (NULL == buf[count])
        {
            LOG_printf(&trace,"MEM_calloc failed.");
            SYS_abort("MEM_calloc failed.\n");
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

/**
 * \brief    Function to free the allocated resources and delete the channels
 *
 *           This function frees the memory buffers allocated and also deletes
 *           the uPP channels.
 *
 * \params   NONE
 *
 * \return   NONE
 */
static Void uppDeleteStreams(Void)
{
    Int32   status = IOM_COMPLETED;
    Uint32  count  = 0x00;


    /* delete the channels for the RX and the Tx                              */
    status = SIO_delete(uppOutHandle);

    if (IOM_COMPLETED != status)
    {
        LOG_printf(&trace,"Output Stream Deletion Failed");
        SYS_abort("Output Stream Deletion Failed\n");
    }

    status = SIO_delete(uppInHandle);

    if (IOM_COMPLETED != status)
    {
        LOG_printf(&trace,"Input Stream Deletion Failed");
        SYS_abort("Input Stream Deletion Failed\n");
    }


    /* free the buffers allocated for the RX and TX channels                  */
    for (count = 0; count < (NUM_BUFS * 2); count ++)
    {
        if (TRUE != MEM_free(0,buf[count],BUFSIZE))
        {
            LOG_printf(&trace,"Freeing Memory failed\n");
            SYS_abort("Freeing Memory failed\n");
        }
    }
}

/*
 * \brief  entry point function for the Upp demo task.
 *
 *         -> creates the required channels for the demonstration of the UPP
 *            working in EVM to EVM communication mode
 *         -> compares the transmit and receive data and prints an appropriate
 *            message
 *
 * \params  None
 *
 * \return  None.
 */
Void uppDemoTask(Void)
{
    uppStartDemo();

    LOG_printf(&trace,"Sample Application completed...");
}

/*============================================================================*/
/*                              END OF FILE                                   */
/*============================================================================*/

