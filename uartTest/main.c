
/**
 * @file   main_param_test.c
 *
 * @brief  This file tests the UART driver APIs with all the suported
 *          input parameters values
 */
/*
 * Copyright (c) 2014, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/knl/Task.h>
/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <xdc/runtime/Error.h>
/* TI-RTOS Header files */
#include <ti/drv/uart/UART.h>
#include <ti/drv/uart/UART_stdio.h>
#include <ti/csl/soc/am572x/src/cslr_control_core_pad_io.h>

#include <ti/board/board.h>
#include "stdio.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"

/*
 *  ======== echoFxn ========
 *  Task for this function is created statically. See the project's .cfg file.
 *
 */


char echoPrompt[]="uart driver and utils example test cases :please enetr the 50 bytes of character or press the esc \n";
char echoPrompt1[]="DATA RECEIVED IS\n ";

/* Length of the input in number of characters */
#define INPUT_LENGTH           (16U)

Void uart_test(UArg arg0, UArg arg1)
{
    char input = '\n';
    char *buffPointer;

    buffPointer = (char*)malloc(INPUT_LENGTH);
    memset(buffPointer,0,INPUT_LENGTH);

    UART_puts("uart driver and utils example test cases :please enetr the 16 bytes of character or press the esc \n",sizeof("uart driver and utils example test cases :please enetr the 16 bytes of character or press the esc or carriage return\n"));

    while(1) {

          memset(buffPointer,0,INPUT_LENGTH);
          UART_gets(buffPointer, INPUT_LENGTH);
          UART_puts(&input,1);
          UART_printf("Data received is\n");
          UART_puts(buffPointer, INPUT_LENGTH);
          UART_puts("\nuart driver and utils example test cases :please enetr the 16 bytes of character or press the esc \n",sizeof("uart driver and utils example test cases :please enetr the 16 bytes of character or press the esc or carriage return\n"));

    }

}

/*
 *  ======== main ========
 */
int main(void)
{
    /* Call board init functions */
    Task_Handle task;
    Error_Block eb;
    Board_initCfg boardCfg;
    boardCfg = BOARD_INIT_PINMUX_CONFIG |
        BOARD_INIT_UART_STDIO;
    Board_init(boardCfg);

    Error_init(&eb);

#if 1
    task = Task_create(uart_test, NULL, &eb);
        if (task == NULL) {
            System_printf("Task_create() failed!\n");
            BIOS_exit(0);
        }
#endif
    /* Start BIOS */
    BIOS_start();
    return (0);
}
