/*
 * uppSample_main.c
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
 * \file     uppSample_main.c
 *
 * \brief    uPP sample application for evm to evm communication
 *
 *           This file demonstrates the use of uPP by using an
 *           duplex mode 0 of operation
 *
 *           (C) Copyright 2009, Texas Instruments, Inc
 *
 * \version  0.1
 *           \author Imtiaz SMA
 *           \date   Dec 14 2009
 *
 *           Initial version
 */

/* ========================================================================== */
/*                            INCLUDE FILES                                   */
/* ========================================================================== */

#include <log.h>
#include "ti/pspiom/upp/Upp.h"
#include "ti/pspiom/platforms/evm6748/Upp_evmInit.h"

/* ========================================================================== */
/*                           MACRO DEFINTIONS                                 */
/* ========================================================================== */

extern LOG_Obj trace;

/*
 * Upp device params. To be filled in userMcbspInit function which
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
 *         function to configure the upp instance
 *
 * \param  None
 * \return None
 */
Void main(Void)
{
    LOG_printf(&trace,"Upp Sample Application\n");

    /* enable the pinmux for the upp device                                   */
    configureUpp();

    return;
}

/*
 * Upp init function called when creating the driver.
 */
void uppUserInit()
{
    Upp_init();

    uppParams = Upp_PARAMS;
    uppParams.devMode = Upp_DevMode_DUPLEX_0;
    uppParams.dlbMode = Upp_Loopback_DISABLE;
    uppParams.emulationMode = Upp_EmuMode_RTEMU;
    uppParams.pllDomain = Upp_pllDomain_0;
    uppParams.pscPwrmEnable = FALSE;
}

/* ========================================================================== */
/*                                END OF FILE                                 */
/* ========================================================================== */

