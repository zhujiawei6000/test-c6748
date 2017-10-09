/*
 * uppSample_main.c
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
 * \file     uppSample_main.c
 *
 * \brief    uPP sample application
 *
 *           This file demonstrates the use of uPP by using an
 *           DLB mode of operation
 *
 *           (C) Copyright 2009, Texas Instruments, Inc
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


/* ========================================================================== */
/*                           MACRO DEFINTIONS                                 */
/* ========================================================================== */


/*
 * Upp device params. To be filled in userUppInit function which
 * is called before driver creation
 */
Upp_Params     uppParams;

/* ========================================================================== */
/*                           FUNCTION DEFINITIONS                             */
/* ========================================================================== */

/**
 * \brief  Void main(Void)
 *
 *         Main function of the sample application. This function calls the
 *         function to configure the upp instance for pinmux settings.
 *
 * \param  None
 * \return None
 */
Void main(Void)
{
    Board_STATUS boardInitStatus =0;

    Board_initCfg cfg = Board_init(BOARD_INIT_PINMUX_CONFIG | BOARD_INIT_MODULE_CLOCK | BOARD_INIT_UNLOCK_MMR | BOARD_INIT_UART_STDIO);

    boardInitStatus = Board_init(cfg);
    if (boardInitStatus !=0)
    {
        UART_printf("Board_init failure\n");
        return;
    }
    UART_printf("Board_init success\n");


    UART_printf("Upp Sample Application\n");


    return;
}

/*
 * Upp init function called when creating the driver.
 */
void uppUserInit()
{
    Upp_init();

    uppParams = Upp_PARAMS;
    uppParams.devMode = Upp_DevMode_DUPLEX_1;
    uppParams.dlbMode = Upp_Loopback_ENABLE_AB;
    uppParams.emulationMode = Upp_EmuMode_SOFT_STOP;
    uppParams.pllDomain = Upp_pllDomain_0;
    uppParams.pscPwrmEnable = FALSE;
}

/* ========================================================================== */
/*                                END OF FILE                                 */
/* ========================================================================== */

