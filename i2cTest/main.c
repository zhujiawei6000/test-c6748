/*
 *  ======== main.c ========
 */


#include <stdio.h>
#include <string.h>

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/System.h>
#include <stdio.h>
#include <ti/sysbios/knl/Task.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <xdc/runtime/Error.h>

/* CSL Header files */
#ifdef _TMS320C6X
#include <ti/csl/csl_chip.h>
#endif

/* OSAL Header files */
#include <ti/osal/osal.h>

/* TI-RTOS Header files */
#include <ti/drv/i2c/I2C.h>
#include <ti/drv/i2c/soc/I2C_soc.h>
#include <ti/drv/i2c/src/I2C_osal.h>
#include <ti/drv/i2c/test/src/I2C_log.h>

/* Board Header files */
#include <ti/board/board.h>
#define I2C_INSTANCE_NUM        1
#define I2C_OWN_SLAVE_ADDR      0x20
#define I2C_SLAVE_ADDR          0x30
/*
 *  ======== I2C init config ========
 */
static void I2C_initConfig(uint32_t instance)
{
    I2C_HwAttrs i2c_cfg;

    /* Get the default I2C init configurations */
    I2C_socGetInitCfg(instance, &i2c_cfg);

    /* interrupt enabled */
    i2c_cfg.enableIntr = true;

    i2c_cfg.ownSlaveAddr = I2C_OWN_SLAVE_ADDR;

    /* Set the I2C init configurations */
    I2C_socSetInitCfg(instance, &i2c_cfg);
}
static bool I2C_Read(I2C_Handle i2c, void* buf, int size) {
    bool            transferOK;
    I2C_Transaction   transaction;
    /* Initialize slave I2C transaction structure */
    I2C_transactionInit(&transaction);
    transaction.writeCount = 0;
    transaction.readCount  = size;
    transaction.timeout    = SemaphoreP_WAIT_FOREVER;
    transaction.expandSA = false;
    transaction.masterMode   = true;
    transaction.slaveAddress = I2C_SLAVE_ADDR;
    transaction.writeBuf     = (Ptr)NULL;
    transaction.readBuf      = (Ptr)buf;

    /* Initiate I2C transfer */
    transferOK = I2C_transfer(i2c, &transaction);
    return transferOK;

}
static bool I2C_Write(I2C_Handle i2c,void* buf, int size) {
    bool            transferOK;

    I2C_Transaction   transaction;
    /* Initialize slave I2C transaction structure */
    I2C_transactionInit(&transaction);
    transaction.writeCount = size;
    transaction.readCount  = 0;
    transaction.timeout    = 1;
    transaction.expandSA = false;
    transaction.masterMode   = true;
    transaction.slaveAddress = I2C_SLAVE_ADDR;
    transaction.writeBuf     = (Ptr)buf;
    transaction.readBuf      = (Ptr)NULL;

    /* Initiate I2C transfer */
    transferOK = I2C_transfer(i2c, &transaction);
    return transferOK;

}

static void FPGA_WriteReg(uint8_t address, uint32_t value) {
    I2C_Params        i2cParams;
    /* Initialize I2C handle */
    I2C_Params_init(&i2cParams);
    i2cParams.transferMode = I2C_MODE_BLOCKING;
    i2cParams.bitRate = I2C_100kHz;
    I2C_Handle i2c = I2C_open(I2C_INSTANCE_NUM, &i2cParams);
    if (i2c == NULL)
    {
        return;
    }
    uint8_t buf[5];
    buf[0] = address;
    memcpy(buf+1, &value, 4);
    I2C_Write(i2c, buf, sizeof(buf));
    I2C_close(i2c);
}
static uint32_t FPGA_ReadReg(uint8_t address) {
    I2C_Params        i2cParams;
     /* Initialize I2C handle */
     I2C_Params_init(&i2cParams);
     i2cParams.transferMode = I2C_MODE_BLOCKING;
     i2cParams.bitRate = I2C_100kHz;
     I2C_Handle i2c = I2C_open(I2C_INSTANCE_NUM, &i2cParams);
     if (i2c == NULL)
     {
         return 0;
     }

     uint32_t result = 0;
     I2C_Transaction   transaction;
     /* Initialize slave I2C transaction structure */
     I2C_transactionInit(&transaction);
     transaction.writeCount = sizeof(address);
     transaction.readCount  = sizeof(result);
     transaction.timeout    = 1000;
     transaction.expandSA = false;
     transaction.masterMode   = true;
     transaction.slaveAddress = I2C_SLAVE_ADDR;
     transaction.writeBuf     = (Ptr)&address;
     transaction.readBuf      = (Ptr)&result;

     /* Initiate I2C transfer */
     I2C_transfer(i2c, &transaction);

     I2C_close(i2c);
     return result;
}


Void masterTaskFxn (UArg arg0, UArg arg1)
{

    I2C_initConfig(I2C_INSTANCE_NUM);

    int i = 0;
    while (1) {
        uint8_t address = 1;
        i++;
        int value = FPGA_ReadReg(address);
        UART_printf("FPGA ReadReg [%u]=[%u],[%02x]\n", address, value, value);
    }

}

/*
 *  ======== main ========
 */
Int main()

{ 

    System_printf("enter main()\n");

    Board_STATUS boardInitStatus =0;

    Board_initCfg cfg = Board_init(BOARD_INIT_PINMUX_CONFIG | BOARD_INIT_MODULE_CLOCK | BOARD_INIT_UNLOCK_MMR | BOARD_INIT_UART_STDIO);

    boardInitStatus = Board_init(cfg);
    if (boardInitStatus !=0)
    {
        UART_printf("Board_init failure\n");
        return(-1);
    }
    UART_printf("Board_init success\n");

    BIOS_start();    /* does not return */
    return(0);
}
