/********************************************************************/
/* This file demonstrates the use of the uPP peripheral interface   */
/* driver in several different modes of operation.                  */
/*                                                                  */
/* Copyright (c) Texas Instruments Inc 2009                         */
/*                                                                  */
/* Use of this software is controlled by the terms and conditions   */
/* found in the license agreement under which this software has     */
/* been supplied.                                                   */
/********************************************************************/

/********************************************************************/
/* version 0.0.1: Joe Coombs Aug 05 2009                            */
/* Initial version                                                  */
/********************************************************************/

// standard header files
#include <xdc/std.h>
//#include <tsk.h>
#include <ti/sysbios/knl/Task.h>
//#include <c64.h>
//#include <gio.h>
#include <ti/sysbios/io/GIO.h>
//#include <log.h>

#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/BIOS.h>

// driver include file
#include "upp_md.h"
extern CSL_UppRegs *upp_reg_hdl;
// application include file
//#include "upp_drv_testcfg.h"

// PSC registers (lifted from GEL)
#define PSC1_BASE       0x01E27000
#define PSC1_MDCTL      (PSC1_BASE+0xA00)
#define PSC1_MDSTAT     (PSC1_BASE+0x800)
#define PSC1_PTCMD      *(unsigned int*) (PSC1_BASE + 0x120)
#define PSC1_PTSTAT     *(unsigned int*) (PSC1_BASE + 0x128)

// kick and pinmux registers (lifted from GEL)
#define SYS_BASE        0x01C14000
#define KICK0R          *(unsigned int*)(SYS_BASE + 0x038)
#define KICK1R          *(unsigned int*)(SYS_BASE + 0x03C)
#define PINMUX0         *(unsigned int*)(SYS_BASE + 0x120)
#define PINMUX1         *(unsigned int*)(SYS_BASE + 0x124)
#define PINMUX2         *(unsigned int*)(SYS_BASE + 0x128)
#define PINMUX3         *(unsigned int*)(SYS_BASE + 0x12C)
#define PINMUX4         *(unsigned int*)(SYS_BASE + 0x130)
#define PINMUX5         *(unsigned int*)(SYS_BASE + 0x134)
#define PINMUX6         *(unsigned int*)(SYS_BASE + 0x138)
#define PINMUX7         *(unsigned int*)(SYS_BASE + 0x13C)
#define PINMUX8         *(unsigned int*)(SYS_BASE + 0x140)
#define PINMUX9         *(unsigned int*)(SYS_BASE + 0x144)
#define PINMUX10        *(unsigned int*)(SYS_BASE + 0x148)
#define PINMUX11        *(unsigned int*)(SYS_BASE + 0x14C)
#define PINMUX12        *(unsigned int*)(SYS_BASE + 0x150)
#define PINMUX13        *(unsigned int*)(SYS_BASE + 0x154)
#define PINMUX14        *(unsigned int*)(SYS_BASE + 0x158)
#define PINMUX15        *(unsigned int*)(SYS_BASE + 0x15C)
#define PINMUX16        *(unsigned int*)(SYS_BASE + 0x160)
#define PINMUX17        *(unsigned int*)(SYS_BASE + 0x164)
#define PINMUX18        *(unsigned int*)(SYS_BASE + 0x168)
#define PINMUX19        *(unsigned int*)(SYS_BASE + 0x16C)

// static init struct for uPP driver (use defaults)
const upp_StaticConfig UPP_DEV_CONFIG = UPP_STATICCONFIG;

// local typedef
typedef enum LOCAL_mode {
    LOCAL_mode_AB_dlb,
    LOCAL_mode_BA_dlb,
    LOCAL_mode_A_transmit,
    LOCAL_mode_A_receive
} LOCAL_mode;

// configuration macros
#define upp_line_size        (1024)
#define upp_line_count       (2)
#define upp_line_offset      (upp_line_size)
#define upp_clock_div        (7)
#define upp_demo_mode        (LOCAL_mode_AB_dlb)
#define NUM_BUF              (1)
// global variables
volatile int upp_interrupt_count = 0;
volatile int upp_error_count = 0;
LOCAL_mode upp_mode = upp_demo_mode;
static upp_Transfer upp_xfer_a, upp_xfer_b;
static Uint8 upp_buffer_a[upp_line_size * upp_line_count];
static Uint8 upp_buffer_b[upp_line_size * upp_line_count];

// local functions
static void LOCAL_upp_demo();
static void LOCAL_upp_config(upp_UserParams *upp_setup);
static void LOCAL_upp_config_xfers(upp_Transfer *xfer_a, upp_Transfer *xfer_b);
static void LOCAL_upp_complete_cb(upp_Transfer *xfer, Uint32 upp_isr_bit);
static void LOCAL_upp_error_cb(upp_Transfer *xfer, Uint32 upp_isr_bit);
static void LOCAL_kick_unlock();
static void LOCAL_PSC1_LPSC_enable(unsigned int PD, unsigned int LPSC_num);
static void LOCAL_apply_upp_pinmux();


// The main function handles low-level initialization, then starts a
// TSK that demonstrates using the uPP driver

void main()
{
    // unlock device
    LOCAL_kick_unlock();

    // turn on uPP LPSC
    LOCAL_PSC1_LPSC_enable(0, 19);

    // enable uPP pinmux - enable all uPP pin functions
    LOCAL_apply_upp_pinmux();

    //todo  enable HWI7 (tied to ECM block 2; see TCF)
    Hwi_enableIER(1 << 7);

    Task_Handle task;
    Error_Block eb;
    Task_Params params;
    System_printf("enter main()\n");

    Error_init(&eb);
    Task_Params_init(&params);
    params.stackSize = 0x800;
    task = Task_create(LOCAL_upp_demo, &params, &eb);
    if (task == NULL) {
        System_printf("Task_create() failed!\n");
        BIOS_exit(0);
    }

    BIOS_start();    /* does not return */
}


// LOCAL_upp_demo is the primary DSP/BIOS TSK function.  It is called
// after main

static void LOCAL_upp_demo(UArg a0, UArg a1)
{
    upp_UserParams upp_setup;

    GIO_Handle upph;
    int i, status, target_int_count;



    // specify driver parameters
    LOCAL_upp_config(&upp_setup);

    // create driver handle
    //upph = GIO_create("/UPP", IOM_INOUT, &status, &upp_setup, NULL);
    GIO_Params gioParams;
    /* Create input GIO instance */
    GIO_Params_init(&gioParams);
    gioParams.chanParams = &upp_setup;
    gioParams.model = ti_sysbios_io_GIO_Model_STANDARD;
    upph = GIO_create("/UPP", GIO_INOUT, &gioParams, NULL);

    if (upph == NULL)
    {
        System_printf("GIO_create failed");
        return;
    }
    System_printf("Programming uPP transfers...\n");
    while (TRUE) {
        //    int j;
        //    for (j = 0; j < NUM_BUF; j++) {
        // program transfer(s)

        LOCAL_upp_config_xfers(&upp_xfer_a, &upp_xfer_b);

        // note: begin read first if in DLB mode
        switch (upp_mode)
        {
        case LOCAL_mode_AB_dlb:
            status  = GIO_read(upph, &upp_xfer_b, NULL);
            status |= GIO_write(upph, &upp_xfer_a, NULL);
            //Task_sleep(1000);
            break;

        case LOCAL_mode_BA_dlb:
            status  = GIO_read(upph, &upp_xfer_a, NULL);
            status |= GIO_write(upph, &upp_xfer_b, NULL);
            break;

        case LOCAL_mode_A_transmit:
            status  = GIO_write(upph, &upp_xfer_a, NULL);
            break;

        case LOCAL_mode_A_receive:
            status  = GIO_read(upph, &upp_xfer_a, NULL);
            break;

        default:
            // unrecognized mode (shouldn't happen)
            status  = -1;
            break;
        }

        // catch GIO_submit errors)
        if (status < 0)
        {
            System_printf("Error programming uPP transfers.\n");
            upp_error_count++;
        }


        //    }
        // wait until transfer(s) complete
        target_int_count = (upp_mode == LOCAL_mode_AB_dlb  ||
                upp_mode == LOCAL_mode_BA_dlb) ? 2 * NUM_BUF : 1 * NUM_BUF;
        while (upp_interrupt_count < target_int_count && upp_error_count == 0)
            asm(" nop");
        // check buffers (loopback modes only)
        if ((target_int_count == (upp_mode == LOCAL_mode_AB_dlb  ||
                upp_mode == LOCAL_mode_BA_dlb)) && (upp_error_count == 0))
            for (i = 0; i < sizeof(upp_buffer_a); i++)
                if (upp_buffer_a[i] != upp_buffer_b[i])
                {
                    //System_printf("Data mismatch in buffers. %x != %x\n", upp_buffer_a[j][i], upp_buffer_b[j][i]);
                    upp_error_count++;
                }

        // report test result
        if (upp_error_count)
            System_printf("uPP transfers completed with %u errors.\n", upp_error_count);
        else
            System_printf ("uPP transfers completed! upp_interrupt_count = %u\n", upp_interrupt_count);

        upp_error_count = 0;
        upp_interrupt_count = 0;
        Task_sleep(1000);
    }
//    System_printf("Reprogramming uPP transfers...\n");
//    for (j = 0; j < NUM_BUF; j++) {
//        // program transfer(s)
//
//        LOCAL_upp_config_xfers(&upp_xfer_a[j], &upp_xfer_b[j], j);
//
//        // note: begin read first if in DLB mode
//        switch (upp_mode)
//        {
//        case LOCAL_mode_AB_dlb:
//            status  = GIO_read(upph, &upp_xfer_b[j], NULL);
//            status |= GIO_write(upph, &upp_xfer_a[j], NULL);
//            //Task_sleep(1000);
//            break;
//
//        case LOCAL_mode_BA_dlb:
//            status  = GIO_read(upph, &upp_xfer_a[j], NULL);
//            status |= GIO_write(upph, &upp_xfer_b[j], NULL);
//            break;
//
//        case LOCAL_mode_A_transmit:
//            status  = GIO_write(upph, &upp_xfer_a[j], NULL);
//            break;
//
//        case LOCAL_mode_A_receive:
//            status  = GIO_read(upph, &upp_xfer_a[j], NULL);
//            break;
//
//        default:
//            // unrecognized mode (shouldn't happen)
//            status  = -1;
//            break;
//        }
//
//        // catch GIO_submit errors)
//        if (status < 0)
//        {
//            System_printf("Error programming uPP transfers.\n");
//            upp_error_count++;
//        }
//
//
//    }
//    // wait until transfer(s) complete
//    target_int_count = (upp_mode == LOCAL_mode_AB_dlb  ||
//            upp_mode == LOCAL_mode_BA_dlb) ? 2 * NUM_BUF : 1 * NUM_BUF;
//    while (upp_interrupt_count < target_int_count && upp_error_count == 0)
//        asm(" nop");
//    // check buffers (loopback modes only)
//    if ((target_int_count == (upp_mode == LOCAL_mode_AB_dlb  ||
//            upp_mode == LOCAL_mode_BA_dlb)) && (upp_error_count == 0))
//        for (j = 0; j < NUM_BUF; j++) {
//            for (i = 0; i < sizeof(upp_buffer_a[j]); i++)
//                if (upp_buffer_a[j][i] != upp_buffer_b[j][i])
//                {
//                    //System_printf("Data mismatch in buffers. %x != %x\n", upp_buffer_a[j][i], upp_buffer_b[j][i]);
//                    upp_error_count++;
//                }
//        }
//
//    // report test result
//    if (upp_error_count)
//        System_printf("uPP transfers completed with %u errors.\n", upp_error_count);
//    else
//        System_printf ("uPP transfers completed! upp_interrupt_count = %u\n", upp_interrupt_count);
}


// LOCAL_upp_config fills in the individual elements of a upp_UserParams
// struct according to local configuration macros

static void LOCAL_upp_config(upp_UserParams *upp_setup)
{
    // start with default settings
    *upp_setup = UPP_USERPARAMS;

    // apply common settings
    upp_setup->params.A.clock_div = upp_clock_div;
    upp_setup->params.A.fxn_eow = (upp_CbFxn)LOCAL_upp_complete_cb;
    upp_setup->params.A.fxn_dpe = (upp_CbFxn)LOCAL_upp_error_cb;
    upp_setup->params.A.fxn_err = (upp_CbFxn)LOCAL_upp_error_cb;
    upp_setup->params.A.fxn_uor = (upp_CbFxn)LOCAL_upp_error_cb;
    upp_setup->params.A.idle_value = 0xAAAA;

    upp_setup->params.B.clock_div = upp_clock_div;
    upp_setup->params.B.fxn_eow = (upp_CbFxn)LOCAL_upp_complete_cb;
    upp_setup->params.B.fxn_dpe = (upp_CbFxn)LOCAL_upp_error_cb;
    upp_setup->params.B.fxn_err = (upp_CbFxn)LOCAL_upp_error_cb;
    upp_setup->params.B.fxn_uor = (upp_CbFxn)LOCAL_upp_error_cb;
    upp_setup->params.B.idle_value = 0xBBBB;

    switch (upp_mode)
    {
    case LOCAL_mode_AB_dlb:
        // A-to-B loopback
        upp_setup->params.A.direction = upp_dir_xmit;
        upp_setup->params.B.direction = upp_dir_rcv;
        upp_setup->params.channels = 2;
        upp_setup->params.special_mode = upp_digital_loopback;
        break;

    case LOCAL_mode_BA_dlb:
        // B-to-A loopback
        upp_setup->params.A.direction = upp_dir_rcv;
        upp_setup->params.B.direction = upp_dir_xmit;
        upp_setup->params.channels = 2;
        upp_setup->params.special_mode = upp_digital_loopback;
        break;

    case LOCAL_mode_A_transmit:
        // Single channel transmit
        upp_setup->params.A.direction = upp_dir_xmit;
        break;

    case LOCAL_mode_A_receive:
        // Single channel receive
        upp_setup->params.A.direction = upp_dir_rcv;
        break;

    default:
        // unrecognized mode (shouldn't happen)
        System_printf("Warning: unrecognized uPP mode\n");
        break;
    }
}


// LOCAL_upp_config_xfers initializes uPP transfers according to parameters
// defined in local macros

static void LOCAL_upp_config_xfers(upp_Transfer *xfer_a, upp_Transfer *xfer_b)
{
    int i;
    // initialize uPP buffers
    for (i = 0; i < sizeof(upp_buffer_a) / 4; i++)
    {
        // put data in transmit buffer and clear receive buffer
        if (upp_mode == LOCAL_mode_BA_dlb)
        {
            ((Uint32 *)upp_buffer_b)[i] = 0xAAAA5555;
            ((Uint32 *)upp_buffer_a)[i] = 0xFFFFFFFF;
        }
        else
        {
            ((Uint32 *)upp_buffer_a)[i] = 0xAAAA5555;
            ((Uint32 *)upp_buffer_b)[i] = 0xFFFFFFFF;
        }
    }
    // start with default settings
    *xfer_a = UPP_TRANSFER;
    *xfer_b =  UPP_TRANSFER;

    // apply common settings
    xfer_a->windowAddr = upp_buffer_a;

    xfer_a->lineCount = upp_line_count;
    xfer_a->lineSize = upp_line_size;
    xfer_a->lineOffset = upp_line_offset;
    xfer_a->channel = upp_A;

    xfer_b->windowAddr = upp_buffer_b;

    xfer_b->lineCount = upp_line_count;
    xfer_b->lineSize = upp_line_size;
    xfer_b->lineOffset = upp_line_offset;
    xfer_b->channel = upp_B;

    // note: upp_xfer_b will only be used in DLB modes
}


// LOCAL_upp_complete_cb is a callback function used to track the
// completion of uPP transfers (i.e. EOW events)

static void LOCAL_upp_complete_cb(upp_Transfer *xfer, Uint32 upp_isr_bit)
{
    System_printf("uPP interrupt: 0x%08X\n", upp_isr_bit);
    upp_interrupt_count++;
    // note: only uPP HWI is enabled, so increment should be atomic/safe
}


// LOCAL_upp_error_cb is a callback function used to track uPP error
// events (underrun/overflow, internal error, or DMA programming error)

static void LOCAL_upp_error_cb(upp_Transfer *xfer, Uint32 upp_isr_bit)
{
    System_printf("uPP error: 0x%08X\n", upp_isr_bit);
    upp_error_count++;
    // note: only uPP HWI is enabled, so increment should be atomic/safe
}


// LOCAL_kick_unlock allows register access at run time.  Based on code
// taken from GEL file

static void LOCAL_kick_unlock()
{
    // enables register access at run time
    KICK0R = 0x83e70b13;  // Kick0 register + data (unlock)
    KICK1R = 0x95a4f1e0;  // Kick1 register + data (unlock)
}


// LOCAL_PSC1_LPSC_enable powers on one LPSC.  Based on code taken from
// GEL file

static void LOCAL_PSC1_LPSC_enable(unsigned int PD, unsigned int LPSC_num)
{
    *(unsigned int*) (PSC1_MDCTL + 4 * LPSC_num) =
            (*(unsigned int*) (PSC1_MDCTL + 4 * LPSC_num) & 0xFFFFFFE0) | 0x0003;

    PSC1_PTCMD = 0x1 << PD;

    // Wait for power state transition to finish
    while( (PSC1_PTSTAT & (0x1 << PD)) != 0) asm(" nop"); 
    while( (*(unsigned int*)(PSC1_MDSTAT + 4 * LPSC_num) & 0x1F) !=0x3) asm(" nop");
}


// LOCAL_apply_upp_pinmux enables all uPP pin functions.  Based on code taken
// from GEL file

static void LOCAL_apply_upp_pinmux()
{
    // all pins (channel A, channel B, DATA, and XDATA)
    PINMUX13 = (PINMUX13 & 0x0000FFFF) | 0x44440000;
    PINMUX14 = (PINMUX14 & 0x000000FF) | 0x44444400;
    PINMUX15 = (PINMUX15 & 0x00000000) | 0x44444444;
    PINMUX16 = (PINMUX16 & 0x00000000) | 0x44444444;
    PINMUX17 = (PINMUX17 & 0x00000000) | 0x44444444;
    PINMUX18 = (PINMUX18 & 0xFF000000) | 0x00444444;
}


//-------------
// END OF FILE 
//-------------
