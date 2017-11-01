/*
 *  ======== main.c ========
 */

#include <xdc/std.h>

#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

#include <ti/sysbios/BIOS.h>

#include <ti/sysbios/knl/Task.h>
#include <ti/board/board.h>
#include <ti/drv/uart/UART.h>
#include <ti/drv/uart/UART_stdio.h>
#include "timer.h"
#include "soc_C6748.h"
/******************************************************************************
**                      INTERNAL MACRO DEFINITIONS
*******************************************************************************/
#define TMR_PERIOD_LSB32               (0x06FFFFFF)
#define TMR_PERIOD_MSB32               (0x0)

/******************************************************************************
**                      INTERNAL FUNCTION PROTOTYPES
*******************************************************************************/
static void TimerSetUpWatchDog(void);

/******************************************************************************
**                      INTERNAL VARIABLE DEFINITIONS
*******************************************************************************/

/******************************************************************************
**                          FUNCTION DEFINITIONS
*******************************************************************************/
int main(void)
{

    /* Set up the UART2 peripheral */
//    UARTStdioInit();
    Board_STATUS boardInitStatus =0;

    Board_initCfg cfg = Board_init(BOARD_INIT_PINMUX_CONFIG | BOARD_INIT_MODULE_CLOCK | BOARD_INIT_UNLOCK_MMR | BOARD_INIT_UART_STDIO);

    boardInitStatus = Board_init(cfg);
    if (boardInitStatus !=0)
    {
        System_printf("Board_init failure\n");
        return(-1);
    }
    System_printf("Board_init success\n");


    /* Set up the Timer2 peripheral */
    TimerSetUpWatchDog();

    /* Send the message to UART console */
    UART_printf("Our Program Reset!");
    UART_printf("Please Input any key at least once in every 4 seconds to avoid a further reset.\n\r");

    /* Activate the Watch Dog Timer */
    TimerWatchdogActivate(SOC_TMR_1_REGS);

    /* Reset the WDT upon any character input */
    while(1)
    {
        /*
        ** Wait for an input through UART. If no input is given, the WDT
        ** will timeout and reset will occur
        */
        UART_getc();

        /*
        ** Reactivate the WDT upon character input. The WDT will restart
        ** at this point.
        */
        TimerWatchdogReactivate(SOC_TMR_1_REGS);
    }
}

/*
** Setup the timer for 64 bit WDT mode
*/
static void TimerSetUpWatchDog(void)
{
    /* Configuration of Timer */
    TimerConfigure(SOC_TMR_1_REGS, TMR_CFG_64BIT_WATCHDOG);

    /* Set the 64 bit timer period */
    TimerPeriodSet(SOC_TMR_1_REGS, TMR_TIMER12, TMR_PERIOD_LSB32);
    TimerPeriodSet(SOC_TMR_1_REGS, TMR_TIMER34, TMR_PERIOD_MSB32);
}

/***************************** End Of File ***********************************/
