/*
 *  ======== main.c ========
 */

#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Swi.h>
#include <xdc/runtime/Memory.h>
#include <xdc/runtime/IHeap.h>
#include "upp.h"


static Swi_Handle swi0;
extern IHeap_Handle MyHeap;

void lowPrioTaskFxn() {
    while (TRUE) {
        System_flush();
        Task_sleep(1000);
    }
}



//#define LOOP_BACK_MODE



#ifdef LOOP_BACK_MODE

void taskSendHandle(UArg a0, UArg a1)
{
    System_printf("Upp send handle thread is running...\n");

    int offset = 0;
    int i;
    int j;



    WindowBuffer *buffer = Memory_alloc(MyHeap, sizeof(WindowBuffer), 8, NULL);
    if(buffer == NULL) {
        System_printf("Error alloc for WindowBuffer\n");
        return;
    }
    else {
        System_printf("Buffer addr: 0x%x\n", buffer);
    }


    while(1)
    {

        for(i = 0; i < UPP_WINDOW_LINE_COUNT; i++) {
            for(j = 0; j < UPP_WINDOW_LINE_SIZE; j++) {

                buffer->line[i].data[j] =  (i * UPP_WINDOW_LINE_SIZE + j + offset) & 0xFF;
             }
        }
        System_printf("Send: \n");

        for(i = 0; i < 30; i++)
        {
            System_printf("%02x ", buffer->line[0].data[i]);
        }

        System_printf("\n");
        System_printf("Send offset = %d, addr =%x \n", offset++, buffer);
        System_flush();

        //sync cache and
        Cache_wbInv(buffer, sizeof(WindowBuffer) , Cache_Type_ALL, TRUE);

        //send window
        Test_StartTransWindow_A(buffer);
        //only send one line
        //Test_StartTransOneLine_A(buffer->line);



        Task_sleep(1);
    }
}

#endif

Upp_Transfer_Desc * currentWindow = NULL;



static void swiFunc(UArg a0, UArg a1)
{
    Uint32 count = Swi_getTrigger();

    System_printf("SWI triggered, count = %d, Cache invalidate addr: 0x%08x\n", count, currentWindow->windowAddr);
    //invalidate cache
    Cache_inv(currentWindow->windowAddr, UPP_WINDOW_SIZE , Cache_Type_ALL, TRUE);

    int j;

    for(j = 0; j < UPP_WINDOW_LINE_COUNT; j++) {
        System_printf("Recv[%d]: [%x], Line[%d]\n", currentWindow->windowIndex, currentWindow->windowAddr, j);
        //Check data
        int i;
        for(i = 0; i < 30;i++) {
            System_printf("%02x ", ((WindowBuffer *)currentWindow->windowAddr)->line[j].data[i]);
        }
        System_printf("\n");
    }
}


static void LOCAL_upp_complete_cb(Upp_Transfer_Desc *xfer, Uint32 upp_isr_bit)
{
    System_printf("uPP recv interrupt: 0x%x, buffer index = %d, addr = %x\n", upp_isr_bit, xfer->windowIndex, xfer->windowAddr);
    currentWindow = xfer;

    Swi_inc(swi0);
}

static void LOCAL_upp_error_cb(Upp_Transfer_Desc *xfer, Uint32 upp_isr_bit)
{
   System_printf("uPP error: transfer[0x%x], upp_isr_bit[0x%x]\n", xfer, upp_isr_bit);
}

#ifdef LOOP_BACK_MODE
static void LOCAL_upp_send_complete_cb(Upp_Transfer_Desc *xfer, Uint32 upp_isr_bit)
{
    System_printf("uPP send interrupt: 0x%x\n", upp_isr_bit);

}
#endif

/*
 *  ======== main ========
 */
Int main()
{ 

    Error_Block eb;

    System_printf("enter main()\n");
    Error_init(&eb);

    swi0 = Swi_create(swiFunc, NULL, &eb);
    if (swi0 == NULL) {
        System_abort("Swi create failed");
    }


    upp_Hdl upp = Upp_Init();
    if(upp == NULL) {
        System_printf("Init upp error\n");
        return(0);
    }
    else {
        System_printf("Init upp OK!\n");
    }
#ifdef LOOP_BACK_MODE

    //loop back mode
    upp->params->special_mode = upp_digital_loopback;
    upp->params->A.direction = upp_dir_xmit;
    upp->params->A.fxn_eow = (upp_CbFxn)LOCAL_upp_send_complete_cb;
    upp->params->A.fxn_dpe = (upp_CbFxn)LOCAL_upp_error_cb;
    upp->params->A.fxn_err = (upp_CbFxn)LOCAL_upp_error_cb;
    upp->params->A.fxn_uor = (upp_CbFxn)LOCAL_upp_error_cb;
#endif
    upp->params->B.fxn_eow = (upp_CbFxn)LOCAL_upp_complete_cb;
    upp->params->B.fxn_dpe = (upp_CbFxn)LOCAL_upp_error_cb;
    upp->params->B.fxn_err = (upp_CbFxn)LOCAL_upp_error_cb;
    upp->params->B.fxn_uor = (upp_CbFxn)LOCAL_upp_error_cb;

    if(StartRecvTransB() != 0) {
        System_printf("Start transfer B failed!\n");
        BIOS_exit(-1);
    }

    Task_Params params;

    Task_Params_init(&params);
    params.stackSize = 0x8000;

#ifdef LOOP_BACK_MODE
    Task_Handle task;
    task = Task_create(taskSendHandle, &params, &eb);
    if (task == NULL) {
        System_printf("create taskSendHandle failed!\n");
        BIOS_exit(0);
    }
#endif

    System_printf("start BIOS!\n");

    //System_flush();
    BIOS_start();    /* does not return */
    return(0);
}
