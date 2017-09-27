/**
 *  \file   main_spi_loopback_example.c
 *
 *  \brief  Example application main file. This application will read
 *          and write through spi interface by spi loopback enabled.
 *
 */

/*
 * Copyright (C) 2017 Texas Instruments Incorporated - http://www.ti.com/
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of Texas Instruments Incorporated nor the names of
 * its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef BARE_METAL
/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/System.h>
#include <stdio.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#endif

/* SPI Header files */
#include <ti/drv/spi/SPI.h>
#if defined(SOC_OMAPL137) || defined(SOC_OMAPL138)
#include <ti/drv/spi/src/v0/SPI_v0.h>
#endif
#include <ti/drv/spi/soc/SPI_soc.h>
#include <ti/drv/spi/test/src/SPI_log.h>
#include <ti/drv/spi/test/src/SPI_board.h>

/* Board Header files */
#include <ti/board/board.h>

#define SPI_INSTANCE 0

/**********************************************************************
 ************************** Macros ************************************
 **********************************************************************/

/**********************************************************************
 ************************** Internal functions ************************
 **********************************************************************/

/**********************************************************************
 ************************** Global Variables **************************
 **********************************************************************/

/*
 *  ======== Board_initSPI ========
 */
void Board_initSPI(void)
{
    Board_initCfg boardCfg;
#if defined(SOC_OMAPL137) || defined(SOC_OMAPL138)
    SPI_v0_HWAttrs spi_cfg;

    /* Get the default SPI init configurations */
    SPI_socGetInitCfg(TEST_SPI_PORT, &spi_cfg);

    /* Modify the default SPI configurations if necessary */

    /* Set the default SPI init configurations */
    SPI_socSetInitCfg(TEST_SPI_PORT, &spi_cfg);
#endif

#if defined(evmK2E) || defined(evmC6678)
    boardCfg = BOARD_INIT_MODULE_CLOCK |
        BOARD_INIT_UART_STDIO;
#else
    boardCfg = BOARD_INIT_PINMUX_CONFIG |
        BOARD_INIT_MODULE_CLOCK |
        BOARD_INIT_UART_STDIO;
#endif
    Board_init(boardCfg);
}


/*
 *  ======== test function ========
 */
#ifdef BARE_METAL
void main()
#else
void spi_test(UArg arg0, UArg arg1)
#endif
{
    SPI_Params      spiParams; /* SPI params structure */
    bool            testPassed = true;
    SPI_Handle      hwHandle;  /* SPI handle */
    SPI_Transaction transaction;     /* SPI transaction structure */
    uint32_t        xferEnable,txBuf,rxBuf,len,loop = 1;
    uint32_t        terminateXfer = 0;
    bool            retVal;


#ifdef BARE_METAL
    /* Call board init functions */
    Board_initSPI();
#endif

    SPI_v0_HWAttrs spi_cfg;

    /* Get the default UART init configurations */
    SPI_socGetInitCfg(SPI_INSTANCE, &spi_cfg);

    spi_cfg.enableIntr = false;

    SPI_socSetInitCfg(SPI_INSTANCE, &spi_cfg);

    /* Open the Board flash NOR device with the test SPI port
       and use the default SPI configurations */
    SPI_Params_init(&spiParams);
    spiParams.frameFormat  = SPI_POL0_PHA1;

    /* Init SPI driver */
    SPI_init();

    hwHandle = (SPI_Handle)SPI_open(TEST_SPI_PORT, &spiParams);

    SPI_control(hwHandle,SPI_V0_CMD_LOOPBACK,(void *) &loop);


    if (!hwHandle)
    {
        testPassed = false;
        goto err;
    }

    /* Enable transfer */
    xferEnable = 1;
    SPI_control(hwHandle, SPI_V0_CMD_XFER_ACTIVATE, (void *)&xferEnable);


    /* Write Command */
    txBuf = 0xA5A5A5A5; /* Test data */
    len = sizeof(txBuf);

    transaction.txBuf = (uint8_t *) &txBuf;
    transaction.rxBuf = (uint8_t *) &rxBuf;
    transaction.count = len;
    transaction.arg = (void *)&terminateXfer;

    retVal = SPI_transfer(hwHandle, &transaction);

    if (retVal == false)
    {
        testPassed = false;
        goto err;
    }

    SPI_log("\n Received Data : %x. \n",rxBuf);

    if (txBuf != rxBuf)
        testPassed = false;

    SPI_close(hwHandle);



err:
    if(true == testPassed)
    {
        SPI_log("\n All tests have passed. \n");
    }
    else
    {
        SPI_log("\n Some tests have failed. \n");
    }

    while(1);
}

/*
 *  ======== main ========
 */
#ifndef BARE_METAL
int main(void)
{
    /* Call board init functions */
    Board_initSPI();

    /* Start BIOS */
    BIOS_start();
    return (0);
}
#endif

