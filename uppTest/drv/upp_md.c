/********************************************************************/
/* This file implements the uPP peripheral interface driver.        */
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

// The uPP mini driver interface header file
#include <ti/sysbios/knl/Queue.h>
#include "upp_md.h"

// CSL register handles
CSL_UppRegs *upp_reg_hdl       = (CSL_UppRegs *)CSL_UPP_0_REGS;
CSL_SyscfgRegs *syscfg_reg_hdl = (CSL_SyscfgRegs *)CSL_SYSCFG_0_REGS;

// local macros
#define upp_isr_bit_eolq    (CSL_UPP_UPISR_EOLQ_MASK)
#define upp_isr_bit_eowq    (CSL_UPP_UPISR_EOWQ_MASK)
#define upp_isr_bit_errq    (CSL_UPP_UPISR_ERRQ_MASK)
#define upp_isr_bit_uorq    (CSL_UPP_UPISR_UORQ_MASK)
#define upp_isr_bit_dpeq    (CSL_UPP_UPISR_DPEQ_MASK)
#define upp_isr_bit_eoli    (CSL_UPP_UPISR_EOLI_MASK)
#define upp_isr_bit_eowi    (CSL_UPP_UPISR_EOWI_MASK)
#define upp_isr_bit_erri    (CSL_UPP_UPISR_ERRI_MASK)
#define upp_isr_bit_uori    (CSL_UPP_UPISR_UORI_MASK)
#define upp_isr_bit_dpei    (CSL_UPP_UPISR_DPEI_MASK)
#define upp_isr_bit_all     (upp_isr_bit_eolq | upp_isr_bit_eowq | \
                             upp_isr_bit_errq | upp_isr_bit_uorq | \
                             upp_isr_bit_dpeq | upp_isr_bit_eoli | \
                             upp_isr_bit_eowi | upp_isr_bit_erri | \
                             upp_isr_bit_uori | upp_isr_bit_dpei)

// inline functions
#define upp_int_clear(a)    upp_reg_hdl->UPIER = a
#define upp_int_status()    upp_reg_hdl->UPIER
#define upp_int_enable(a)   upp_reg_hdl->UPIES = a
#define upp_int_disable(a)  upp_reg_hdl->UPIEC = a
static inline Void upp_prog_dma_i(upp_Transfer *xfer);
static inline Void upp_prog_dma_q(upp_Transfer *xfer);
static inline Void upp_sw_reset(upp_Hdl devp);
static inline Void upp_handle_eowi(upp_Hdl devp);
static inline Void upp_handle_eowq(upp_Hdl devp);
static inline Void upp_readwrite_i(upp_ChanObj *chanp, upp_Transfer *new_transfer);
static inline Void upp_readwrite_q(upp_ChanObj *chanp, upp_Transfer *new_transfer);

// local functions
static Void upp_isr(upp_Hdl devp);
static Int upp_apply_params(upp_Hdl devp);


//  upp_isr processes uPP interrupts

static Void upp_isr (upp_Hdl devp)
{
    Uint32 intr_status = upp_int_status(), sem_trigger = 0;
    upp_Transfer *old_transfer;

    // loop to make sure we don't miss a new interrupt
    //  while servicing an old one
    while (intr_status != 0)
    {
        if (intr_status & upp_isr_bit_eoli)
        {
            upp_int_clear(upp_isr_bit_eoli);
            
            // call user function if specified
            if (devp->params->A.fxn_eol != NULL)
                (*devp->params->A.fxn_eol)(devp->A.currentTransfer, upp_isr_bit_eoli);
        }
        if (intr_status & upp_isr_bit_eowi)
        {
            upp_int_clear(upp_isr_bit_eowi);
            
            // remember old transfer for user callback
            old_transfer = devp->A.currentTransfer;

            // call inline function
            upp_handle_eowi(devp);
            
            // trigger semaphore (synchronous mode only)
            sem_trigger++;

            // call user function if specified
            if (devp->params->A.fxn_eow != NULL)
                (*devp->params->A.fxn_eow)(old_transfer, upp_isr_bit_eowi);
        }
        if (intr_status & upp_isr_bit_erri)
        {
            upp_int_clear(upp_isr_bit_erri);

            // report error condition
            devp->A.error = 1;
            
            // trigger semaphore (synchronous mode only)
            sem_trigger++;
            
            // call user function if specified
            if (devp->params->A.fxn_err != NULL)
                (*devp->params->A.fxn_err)(devp->A.currentTransfer, upp_isr_bit_erri);
        }
        if (intr_status & upp_isr_bit_uori)
        {
            upp_int_clear(upp_isr_bit_uori);
            
            // report uor condition
            devp->A.uor = 1;
            
            // trigger semaphore (synchronous mode only)
            sem_trigger++;
            
            // call user function if specified
            if (devp->params->A.fxn_uor != NULL)
                (*devp->params->A.fxn_uor)(devp->A.currentTransfer, upp_isr_bit_uori);
        }
        if (intr_status & upp_isr_bit_dpei)
        {
            upp_int_clear(upp_isr_bit_dpei);
            
            // report DMA error condition
            devp->A.dma_error = 1;
            
            // trigger semaphore (synchronous mode only)
            sem_trigger++;
            
            // call user function if specified
            if (devp->params->A.fxn_dpe != NULL)
                (*devp->params->A.fxn_dpe)(devp->A.currentTransfer, upp_isr_bit_dpei);
        }

        if (intr_status & upp_isr_bit_eolq)
        {
            upp_int_clear(upp_isr_bit_eolq);
            
            // call user function if specified
            if (devp->params->B.fxn_eol != NULL)
                (*devp->params->B.fxn_eol)(devp->B.currentTransfer, upp_isr_bit_eolq);

            // note: interleave mode NYI
        }
        if (intr_status & upp_isr_bit_eowq)
        {
            upp_int_clear(upp_isr_bit_eowq);
            
            // remember old transfer for user callback
            old_transfer = devp->B.currentTransfer;

            // call inline function
            upp_handle_eowq(devp);
            
            // trigger semaphore (synchronous mode only)
            sem_trigger++;

            // call user function if specified
            if (devp->params->A.fxn_eow != NULL)
                (*devp->params->A.fxn_eow)(old_transfer, upp_isr_bit_eowq);

            // note: interleave mode NYI
        }
        if (intr_status & upp_isr_bit_errq)
        {
            upp_int_clear(upp_isr_bit_errq);

            // report error condition
            devp->B.error = 1;
            
            // trigger semaphore (synchronous mode only)
            sem_trigger++;
            
            // call user function if specified
            if (devp->params->B.fxn_err != NULL)
                (*devp->params->B.fxn_err)(devp->B.currentTransfer, upp_isr_bit_errq);

            // note: interleave mode NYI
        }
        if (intr_status & upp_isr_bit_uorq)
        {
            upp_int_clear(upp_isr_bit_uorq);
            
            // report uor condition
            devp->B.uor = 1;
            
            // trigger semaphore (synchronous mode only)
            sem_trigger++;
            
            // call user function if specified
            if (devp->params->B.fxn_uor != NULL)
                (*devp->params->B.fxn_uor)(devp->B.currentTransfer, upp_isr_bit_uorq);

            // note: interleave mode NYI
        }
        if (intr_status & upp_isr_bit_dpeq)
        {
            upp_int_clear(upp_isr_bit_dpeq);
            
            // report DMA error condition
            devp->B.dma_error = 1;
            
            // trigger semaphore (synchronous mode only)
            sem_trigger++;
            
            // call user function if specified
            if (devp->params->B.fxn_dpe != NULL)
                (*devp->params->B.fxn_dpe)(devp->B.currentTransfer, upp_isr_bit_dpeq);

            // note: interleave mode NYI
        }

        // make sure all interrupts are handled        
        intr_status = upp_int_status();
    }
    
    // post semaphore (synchronous mode only)
    if (UPP_DEV_CONFIG.mode == upp_sync && sem_trigger > 0)
        Semaphore_post(devp->sync_helper);
    
    // finally: write 0 to EOI register
    upp_reg_hdl->UPEOI = 0;
}


//  UPP_INIT function.  Performs static initialization for uPP peripheral

Void UPP_INIT (Void)
{
    // select clock source
    switch (UPP_DEV_CONFIG.tx_clk_src)
    {
        case upp_2xTxClk:
            // set CFGCHIP3.UPP_TX_CLKSRC = 1
            CSL_FINST(syscfg_reg_hdl->CFGCHIP3, SYSCFG_CFGCHIP3_UPP_TX_CLKSRC, TXCLK);
            break;

        case upp_pll1sysclk2:
            // set CFGCHIP3.ASYNC3_CLKSRC = 1
            CSL_FINST(syscfg_reg_hdl->CFGCHIP3, SYSCFG_CFGCHIP3_ASYNC3_CLKSRC, PLL1);
            break;

        case upp_pll0sysclk2:
        case upp_defaultClk:
        default:
            // default configuration; no action required
            break;
    }

    // select driver model
    // (no action required here; see mdCreateChan, mdDeleteChan, and mdSubmitChan)
}


//  upp_mdBindDev implementation provided by uPP mini driver.
//  The function is called by BIOS after the upp_init function. 

Int upp_mdBindDev (Ptr ptr, Int devid, Ptr devParams)
{
    upp_GlobalStr  *globalp;

    // Get the pointer to the device global structure
    globalp = &UPP_GLOBALSTR;

    // Set the flag to indicate mdBindDev called 
    if (globalp->driverInit == TRUE)
        return IOM_EBADIO;
    else
        globalp->driverInit = TRUE;

    return IOM_COMPLETED;
}


//  upp_mdCreateChan implementation provided by uPP mini driver.
//  The function creates a channel instance of the uPP minidriver

Int32 upp_mdCreateChan (upp_Hdl *devpp, Ptr *ptr, 
                         String name, Int32 mode, upp_UserParams *chanParams, 
                         IOM_TiomCallback cbFxn, Ptr cbArg)
{
    upp_GlobalStr  *globalp;
    upp_Hdl devp;
    //ECM_Attrs ecmattrs = ECM_ATTRS;
    Int status = IOM_COMPLETED;

    // Get the pointer to the device global structure
    globalp = &UPP_GLOBALSTR;

    // require input/output mode (I/O channels are independent)
    if (mode != IOM_INOUT)
        return IOM_EBADMODE;
    
    //  The current driver version doesn't support creation of more than
    //  one chanel instance. If the channelInst already created, return error
    
    if (globalp->channelInst == TRUE)
        return IOM_EINUSE;
    
    // Check if channel obj provided by the application
    if (chanParams->uppObj != NULL)
    {
        // Channel handle provided by application
        *devpp = chanParams->uppObj;
        devp = *devpp;

        // Indicate that chanObj not allocated by the minidriver
        devp->drvObjAlloc = FALSE;
        devp->segID = chanParams->segID;
    }
    else
    {
        // Allocate the memory for chanObj
        devp = (upp_Hdl)(Memory_alloc(NULL, sizeof(upp_Obj), sizeof(Int32), NULL));
        if (devp == NULL)
            return IOM_EALLOC;

        *devpp =  devp;
        devp->segID = chanParams->segID;

        // Remember that chanObj is allocated by the minidriver
        devp->drvObjAlloc = TRUE;
    }
    
    // Create semaphore (synchronous mode only)
    if (UPP_DEV_CONFIG.mode == upp_sync)
    {
        //devp->sync_helper = Semaphore_create(0, &SEM_ATTRS, NULL);
        devp->sync_helper = Semaphore_create(0, NULL, NULL);
        if (devp->sync_helper == NULL)
            return IOM_EALLOC;
    }

    // Initialize the fields in the device instance handle

    // Initialize device (and channel) status
    devp->reset = TRUE;
    devp->drvAborting = FALSE;

    devp->A.idle = TRUE;
    devp->A.pend = FALSE;
    devp->A.uor = FALSE;
    devp->A.error = FALSE;
    devp->A.dma_error = FALSE;
    //QUE_new(&devp->A.pendTransferList);
    devp->A.pendTransferList = Queue_create(NULL, NULL);
    devp->A.currentTransfer = NULL;

    devp->B.idle = TRUE;
    devp->B.pend = FALSE;
    devp->B.uor = FALSE;
    devp->B.error = FALSE;
    devp->B.dma_error = FALSE;
    //QUE_new(&devp->B.pendTransferList);
    devp->B.pendTransferList = Queue_create(NULL, NULL);
    devp->B.currentTransfer = NULL;
    
    // apply uPP configuration and enable peripheral
    devp->params = &chanParams->params;
    devp->A.params = &devp->params->A;
    devp->B.params = &devp->params->B;
    if (upp_apply_params(devp) != IOM_COMPLETED)
        status = IOM_EBADARGS;

    // Indicate that the channel instance has been created
    globalp->channelInst = TRUE;
    
    //  Hook up the uPP ISR to the ECM.  The driver for now only
    //  supports one channel instance.  Thus, providing the pointer to
    //  the channel instance as the argument to the ISR.
    //  If later versions of this driver support > 1 channel instance, the
    //  channel instance pointers should be maintained in the global struct
    //ecmattrs.unmask = 1u;
    //ecmattrs.arg = (Arg)devp;
    //ECM_dispatchPlug(CSL_INTC_EVENTID_UPPINT, (ECM_Fxn)upp_isr, &ecmattrs);
    //ECM_enableEvent(CSL_INTC_EVENTID_UPPINT);

    EventCombiner_dispatchPlug(CSL_INTC_EVENTID_UPPINT, (EventCombiner_FuncPtr)upp_isr, (UArg)devp, TRUE);
    EventCombiner_enableEvent(CSL_INTC_EVENTID_UPPINT);


    return status;
}


//  upp_mdDeleteChan function provided by uPP mini driver.
//  The functions closes channel instance created by the upp_mdCreateChan.
//  All the allocated resources are freed

Int32 upp_mdDeleteChan (upp_Hdl devp)
{
    upp_GlobalStr  *globalp;
    
    // Get the pointer to the device global structure
    globalp = &UPP_GLOBALSTR;

    //  If transfers are active or pending, driver handle can't
    //  be closed.
    if (!Queue_empty(devp->A.pendTransferList) ||
        !Queue_empty(devp->B.pendTransferList) ||
        !devp->A.idle || !devp->B.idle)
        return IOM_EFREE;
        
    // Delete semaphore (synchronous mode only)
    if (UPP_DEV_CONFIG.mode == upp_sync)
        Semaphore_delete(&devp->sync_helper);

    // Clear channelInst flag in global str to allow future "create" call
    globalp->channelInst = FALSE;

    // turn off interrupt generation
    //ECM_disableEvent(CSL_INTC_EVENTID_UPPINT);
    EventCombiner_disableEvent(CSL_INTC_EVENTID_UPPINT);

    //  If the channel object is allocated by the driver, free it here.
    //  Otherwise, let the application take care of it.
    if (devp->drvObjAlloc == TRUE)
        Memory_free(NULL, devp, sizeof(upp_Obj));



    return IOM_COMPLETED; 
}


//  upp_mdSubmitChan function provided by uPP mini driver.
//  The mdSubmitChan function is called by the application to do
//  data exchange with the driver.

Int32 upp_mdSubmitChan (upp_Hdl devp, IOM_Packet *packet)
{
    upp_Transfer *new_transfer;
    upp_chanHdl chanp;
    Int32 oldCSR;
    
    // Handle abort or flush commands
    if ((packet->cmd == IOM_FLUSH) || (packet->cmd == IOM_ABORT))
    {
        devp->drvAborting = TRUE;
        // Disable event
        //ECM_disableEvent(CSL_INTC_EVENTID_UPPINT);
        EventCombiner_disableEvent(CSL_INTC_EVENTID_UPPINT);

        // Disable uPP peripheral
        CSL_FINST(upp_reg_hdl->UPPCR, UPP_UPPCR_EN, DISABLE);

        //  The abort operation has been initiated. The application needs
        //  to call upp_mdControlChan with the upp_ioctl_dequeue IOCTL.
        //  The application can then go and free all the allocated resource
        
        return IOM_COMPLETED;
    }
    
    // don't accept a new transfer if we're aborting
    if (devp->drvAborting)
        return IOM_EABORT;

    // catch invalid IOM commands
    if (packet->cmd != IOM_READ && packet->cmd != IOM_WRITE)
        return IOM_EBADIO;

    // Get the frame buffer provided by the application
    new_transfer = (upp_Transfer *)packet->addr;
    chanp = (new_transfer->channel == upp_A) ? &devp->A : &devp->B;

    // check that channel direction matches IOM command
    if ((packet->cmd == IOM_READ) &&
        ((new_transfer->channel == upp_A && devp->params->A.direction == upp_dir_xmit) ||
         (new_transfer->channel == upp_B && devp->params->B.direction == upp_dir_xmit)))
        return IOM_EBADIO;
    if ((packet->cmd == IOM_WRITE) &&
        ((new_transfer->channel == upp_A && devp->params->A.direction == upp_dir_rcv) ||
         (new_transfer->channel == upp_B && devp->params->B.direction == upp_dir_rcv)))
        return IOM_EBADIO;
    
    // check that line count and size are non-zero
    if (new_transfer->lineCount == 0 || new_transfer->lineSize == 0)
        return IOM_EBADIO;
        
    // reset semaphore (synchronous mode only)
    if (UPP_DEV_CONFIG.mode == upp_sync && Semaphore_getCount(devp->sync_helper) > 0)
        Semaphore_reset(devp->sync_helper, 0);

    // disable interrupts to maintain context
    oldCSR = Hwi_disable();

    // perform I/O
    if (new_transfer->channel == upp_A)
    {
        // call inline function
        upp_readwrite_i(&devp->A, new_transfer);
    }
    else
    {
        // call inline function
        upp_readwrite_q(&devp->B, new_transfer);

        // note: interleave mode NYI
    }

    // re-enable interrupts
    Hwi_restore(oldCSR);

    // return status
    if (UPP_DEV_CONFIG.mode == upp_sync)
    {
        // wait until transfer completes or error occurs
        //Semaphore_pend(devp->sync_helper, SYS_FOREVER);
        Semaphore_pend(devp->sync_helper, BIOS_WAIT_FOREVER);

        if (chanp->error || chanp->dma_error || chanp->uor)
            return IOM_EBADIO;
        else
            return IOM_COMPLETED;
    }
    else
    {
        // return immediately in async mode
        return IOM_COMPLETED;
    }
}


//  upp_mdControlChan function provided by uPP mini driver.
//  The mdControlChan function is called by the application to configure
//  and control the uPP mini driver.

Int upp_mdControlChan (upp_Hdl devp, Uint32 cmd, Ptr args)
{
    Ptr que_swap;

    switch (cmd)
    {
        case upp_ioctl_enable:
            // turn on peripheral
            CSL_FINST(upp_reg_hdl->UPPCR, UPP_UPPCR_EN, ENABLE);
            break;

        case upp_ioctl_disable:
            // turn on peripheral
            CSL_FINST(upp_reg_hdl->UPPCR, UPP_UPPCR_EN, DISABLE);
            break;

        case upp_ioctl_sw_reset:
            // reset peripheral
            upp_sw_reset(devp);
            break;

        case upp_ioctl_apply_params:
            // apply new params (in args)
            devp->params = args;
            devp->A.params = &devp->params->A;
            devp->B.params = &devp->params->B;
            if (upp_apply_params(devp) != IOM_COMPLETED)
                return IOM_EBADARGS;
            break;

        case upp_ioctl_get_status:
            // copy status bits to provided struct
            ((upp_Hdl)args)->reset       = devp->reset;
            ((upp_Hdl)args)->A.idle      = devp->A.idle;
            ((upp_Hdl)args)->A.pend      = devp->A.pend;
            ((upp_Hdl)args)->A.uor       = devp->A.uor;
            ((upp_Hdl)args)->A.error     = devp->A.error;
            ((upp_Hdl)args)->A.dma_error = devp->A.dma_error;
            ((upp_Hdl)args)->B.idle      = devp->B.idle;
            ((upp_Hdl)args)->B.pend      = devp->B.pend;
            ((upp_Hdl)args)->B.uor       = devp->B.uor;
            ((upp_Hdl)args)->B.error     = devp->B.error;
            ((upp_Hdl)args)->B.dma_error = devp->B.dma_error;
            break;
        
        case upp_ioctl_dequeue:
            // Move all items from pending QUEs (A and B) to user-specified QUE
            while ((que_swap = Queue_get(devp->A.pendTransferList)) != devp->A.pendTransferList)
                Queue_put((Queue_Handle)args, que_swap);
            while ((que_swap = Queue_get(devp->B.pendTransferList)) != devp->B.pendTransferList)
                Queue_put((Queue_Handle)args, que_swap);
            break;
    
        case upp_ioctl_clear_errors:
            // Reset error bits in internal struct
            devp->A.uor = FALSE;
            devp->A.error = FALSE;
            devp->A.dma_error = FALSE;
            devp->B.uor = FALSE;
            devp->B.error = FALSE;
            devp->B.dma_error = FALSE;
            break;
        
        default:
            return IOM_ENOTIMPL;
    }

    return IOM_COMPLETED;
}


//  upp_apply_params function provided by uPP mini driver.
//  Places peripheral in reset, then applies parameters in devp->params.
//  Releases peripheral from reset before returning.

static Int upp_apply_params(upp_Hdl devp)
{
    upp_Params *params = devp->params;
    volatile Uint32 temp_reg;

    // high-level error checking
    if (        (params->A.data_rate == 2 &&                        // minimum clock divisor is 1 for
                 params->A.clock_div == 0)                          //   double data rate

            ||  (params->B.data_rate == 2 &&                        // minimum clock divisor is 1 for
                 params->B.clock_div == 0)                          //   double data rate
    
            ||  (params->special_mode == upp_digital_loopback &&    // require one rcv, one xmt for
                (params->channels != 2 ||                           //   digital loopback mode
                 params->A.direction == params->B.direction))       //

            ||  (params->A.data_rate != params->B.data_rate &&      // datarates must match for digital
                 params->special_mode == upp_digital_loopback)      //   loopback mode

            ||  (params->special_mode == upp_sdr_xmit_interleave && // check SDR interleave requirements
                (params->channels != 1    ||                        //
                 params->A.data_rate != 1 ||                        //
                 params->A.direction != upp_dir_xmit))              // 

            ||  (params->special_mode == upp_ddr_interleave &&      // check DDR interleave requirements
                (params->channels != 1 ||                           //
                 params->A.data_rate != 2))                         //

            ||  (params->channels == 2 &&                           // only allow sync operation in
                 UPP_DEV_CONFIG.mode == upp_sync)                   //   1-ch mode

            //     (insert other error conditions here)

        )    return IOM_EBADARGS;

    // note: Data interleave mode is not yet implemented; don't let user
    //       configure device in interleave mode.  Remove this check
    //       if/when interleave mode is fully implemented.
    if (params->special_mode == upp_sdr_xmit_interleave ||
        params->special_mode == upp_ddr_interleave)
        return IOM_ENOTIMPL;

    // turn off peripheral
    CSL_FINST(upp_reg_hdl->UPPCR, UPP_UPPCR_EN, DISABLE);

    // wait until peripheral is idle
    while (CSL_FEXT(upp_reg_hdl->UPPCR, UPP_UPPCR_DB) == CSL_UPP_UPPCR_DB_ACTIVE)
        asm(" nop");

    // reset peripheral
    upp_sw_reset(devp);

    // configure registers: CTL, ICR, IVR, TCR, DLB

    // 1. CTL

        temp_reg = 0;

        // Channel B params

        switch (params->B.format)
        {
            case upp_format_rjze:   CSL_FINST(temp_reg, UPP_UPCTL_DPFB, RJZE);
                                    break;
            case upp_format_rjse:   CSL_FINST(temp_reg, UPP_UPCTL_DPFB, RJSE);
                                    break;
            case upp_format_ljzf:   CSL_FINST(temp_reg, UPP_UPCTL_DPFB, LJZF);
                                    break;
            default:                return IOM_EBADARGS;
        }

        if (params->B.width > 8) CSL_FINST(temp_reg, UPP_UPCTL_IWB, 16BIT);
        else                     CSL_FINST(temp_reg, UPP_UPCTL_IWB, 8BIT);

        switch (params->B.width)
        {
            case 16:
            case 8:     CSL_FINST(temp_reg, UPP_UPCTL_DPWB, FULL);
                        break;
            case 9:     CSL_FINST(temp_reg, UPP_UPCTL_DPWB, 9BIT);
                        break;
            case 10:    CSL_FINST(temp_reg, UPP_UPCTL_DPWB, 10BIT);
                        break;
            case 11:    CSL_FINST(temp_reg, UPP_UPCTL_DPWB, 11BIT);
                        break;
            case 12:    CSL_FINST(temp_reg, UPP_UPCTL_DPWB, 12BIT);
                        break;
            case 13:    CSL_FINST(temp_reg, UPP_UPCTL_DPWB, 13BIT);
                        break;
            case 14:    CSL_FINST(temp_reg, UPP_UPCTL_DPWB, 14BIT);
                        break;
            case 15:    CSL_FINST(temp_reg, UPP_UPCTL_DPWB, 15BIT);
                        break;
            default:    return IOM_EBADARGS;
        }

        switch (params->B.data_rate)
        {
            case 1:  CSL_FINST(temp_reg, UPP_UPCTL_DRB, SINGLE);
                     break;
            case 2:  CSL_FINST(temp_reg, UPP_UPCTL_DRB, DOUBLE);
                     break;
            default: return IOM_EBADARGS;
        }

        // Channel A params

        switch (params->A.format)
        {
            case upp_format_rjze:   CSL_FINST(temp_reg, UPP_UPCTL_DPFA, RJZE);
                                    break;
            case upp_format_rjse:   CSL_FINST(temp_reg, UPP_UPCTL_DPFA, RJSE);
                                    break;
            case upp_format_ljzf:   CSL_FINST(temp_reg, UPP_UPCTL_DPFA, LJZF);
                                    break;
            default:                return IOM_EBADARGS;
        }

        if (params->A.width > 8) CSL_FINST(temp_reg, UPP_UPCTL_IWA, 16BIT);
        else                     CSL_FINST(temp_reg, UPP_UPCTL_IWA, 8BIT);

        switch (params->A.width)
        {
            case 16:
            case 8:     CSL_FINST(temp_reg, UPP_UPCTL_DPWA, FULL);
                        break;
            case 9:     CSL_FINST(temp_reg, UPP_UPCTL_DPWA, 9BIT);
                        break;
            case 10:    CSL_FINST(temp_reg, UPP_UPCTL_DPWA, 10BIT);
                        break;
            case 11:    CSL_FINST(temp_reg, UPP_UPCTL_DPWA, 11BIT);
                        break;
            case 12:    CSL_FINST(temp_reg, UPP_UPCTL_DPWA, 12BIT);
                        break;
            case 13:    CSL_FINST(temp_reg, UPP_UPCTL_DPWA, 13BIT);
                        break;
            case 14:    CSL_FINST(temp_reg, UPP_UPCTL_DPWA, 14BIT);
                        break;
            case 15:    CSL_FINST(temp_reg, UPP_UPCTL_DPWA, 15BIT);
                        break;
            default:    return IOM_EBADARGS;
        }

        switch (params->A.data_rate)
        {
            case 1:  CSL_FINST(temp_reg, UPP_UPCTL_DRA, SINGLE);
                     break;
            case 2:  CSL_FINST(temp_reg, UPP_UPCTL_DRA, DOUBLE);
                     break;
            default: return IOM_EBADARGS;
        }

        // misc params

        if (params->special_mode == upp_sdr_xmit_interleave)
            CSL_FINST(temp_reg, UPP_UPCTL_SDRTXIL, ENABLE);
        else if (params->special_mode == upp_ddr_interleave)
            CSL_FINST(temp_reg, UPP_UPCTL_DDRDEMUX, ENABLE);

        switch (params->channels)
        {
            case 1:     CSL_FINST(temp_reg, UPP_UPCTL_CHN, ONE);
                        break;
            case 2:     CSL_FINST(temp_reg, UPP_UPCTL_CHN, TWO);
                        break;
            default:    return IOM_EBADARGS;
        }

        if (params->A.direction == upp_dir_xmit &&
            params->B.direction == upp_dir_xmit)
            CSL_FINST(temp_reg, UPP_UPCTL_MODE, TRANSMIT);
        else if (params->A.direction == upp_dir_rcv &&
                 params->B.direction == upp_dir_rcv)
            CSL_FINST(temp_reg, UPP_UPCTL_MODE, RECEIVE);
        else if (params->A.direction == upp_dir_rcv &&
                 params->B.direction == upp_dir_xmit)
            CSL_FINST(temp_reg, UPP_UPCTL_MODE, DUPLEX0);
        else if (params->A.direction == upp_dir_xmit &&
                 params->B.direction == upp_dir_rcv)
            CSL_FINST(temp_reg, UPP_UPCTL_MODE, DUPLEX1);
        else return IOM_EBADARGS;

        // apply all fields in one shot
        upp_reg_hdl->UPCTL = temp_reg;

    // 2. ICR

        temp_reg = 0;

        // Channel B params

        if (params->B.drive_idle)   CSL_FINST(temp_reg, UPP_UPICR_TRISB, DISABLE);
        else                        CSL_FINST(temp_reg, UPP_UPICR_TRISB, ENABLE);

        if (params->B.inv_clock)    CSL_FINST(temp_reg, UPP_UPICR_CLKINVB, INVERT);
        else                        CSL_FINST(temp_reg, UPP_UPICR_CLKINVB, NORMAL);

        if (params->B.clock_div > 0x0F)
            return IOM_EBADARGS;
        else
            CSL_FINS(temp_reg, UPP_UPICR_CLKDIVB, params->B.clock_div);

        if (params->B.en_wait)      CSL_FINST(temp_reg, UPP_UPICR_WAITB, ENABLE);
        else                        CSL_FINST(temp_reg, UPP_UPICR_WAITB, DISABLE);

        if (params->B.en_enable)    CSL_FINST(temp_reg, UPP_UPICR_ENAB, ENABLE);
        else                        CSL_FINST(temp_reg, UPP_UPICR_ENAB, DISABLE);

        if (params->B.en_start)     CSL_FINST(temp_reg, UPP_UPICR_STARTB, ENABLE);
        else                        CSL_FINST(temp_reg, UPP_UPICR_STARTB, DISABLE);

        if (params->B.inv_wait)     CSL_FINST(temp_reg, UPP_UPICR_WAITPOLB, INVERT);
        else                        CSL_FINST(temp_reg, UPP_UPICR_WAITPOLB, NORMAL);

        if (params->B.inv_enable)   CSL_FINST(temp_reg, UPP_UPICR_ENAPOLB, INVERT);
        else                        CSL_FINST(temp_reg, UPP_UPICR_ENAPOLB, NORMAL);

        if (params->B.inv_start)    CSL_FINST(temp_reg, UPP_UPICR_STARTPOLB, INVERT);
        else                        CSL_FINST(temp_reg, UPP_UPICR_STARTPOLB, NORMAL);

        // Channel A params

        if (params->A.drive_idle)   CSL_FINST(temp_reg, UPP_UPICR_TRISA, DISABLE);
        else                        CSL_FINST(temp_reg, UPP_UPICR_TRISA, ENABLE);

        if (params->A.inv_clock)    CSL_FINST(temp_reg, UPP_UPICR_CLKINVA, INVERT);
        else                        CSL_FINST(temp_reg, UPP_UPICR_CLKINVA, NORMAL);

        if (params->A.clock_div > 0x0F)
            return IOM_EBADARGS;
        else
            CSL_FINS(temp_reg, UPP_UPICR_CLKDIVA, params->A.clock_div);

        if (params->A.en_wait)      CSL_FINST(temp_reg, UPP_UPICR_WAITA, ENABLE);
        else                        CSL_FINST(temp_reg, UPP_UPICR_WAITA, DISABLE);

        if (params->A.en_enable)    CSL_FINST(temp_reg, UPP_UPICR_ENAA, ENABLE);
        else                        CSL_FINST(temp_reg, UPP_UPICR_ENAA, DISABLE);

        if (params->A.en_start)     CSL_FINST(temp_reg, UPP_UPICR_STARTA, ENABLE);
        else                        CSL_FINST(temp_reg, UPP_UPICR_STARTA, DISABLE);

        if (params->A.inv_wait)     CSL_FINST(temp_reg, UPP_UPICR_WAITPOLA, INVERT);
        else                        CSL_FINST(temp_reg, UPP_UPICR_WAITPOLA, NORMAL);

        if (params->A.inv_enable)   CSL_FINST(temp_reg, UPP_UPICR_ENAPOLA, INVERT);
        else                        CSL_FINST(temp_reg, UPP_UPICR_ENAPOLA, NORMAL);

        if (params->A.inv_start)    CSL_FINST(temp_reg, UPP_UPICR_STARTPOLA, INVERT);
        else                        CSL_FINST(temp_reg, UPP_UPICR_STARTPOLA, NORMAL);

        // apply all fields in one shot
        upp_reg_hdl->UPICR = temp_reg;

    // 3. IVR

        temp_reg = 0;

        CSL_FINS(temp_reg, UPP_UPIVR_VALB, params->B.idle_value);
        CSL_FINS(temp_reg, UPP_UPIVR_VALA, params->A.idle_value);

        // apply all fields in one shot
        upp_reg_hdl->UPIVR = temp_reg;

    // 4. TCR

        temp_reg = 0;

        switch (params->B.xmit_thresh)
        {
            case upp_thresh_64:  CSL_FINST(temp_reg, UPP_UPTCR_TXSIZEB, 64B); break;
            case upp_thresh_128: CSL_FINST(temp_reg, UPP_UPTCR_TXSIZEB, 128B); break;
            case upp_thresh_256: CSL_FINST(temp_reg, UPP_UPTCR_TXSIZEB, 256B); break;
            default:             return IOM_EBADARGS;
        }

        switch (params->A.xmit_thresh)
        {
            case upp_thresh_64:  CSL_FINST(temp_reg, UPP_UPTCR_TXSIZEA, 64B); break;
            case upp_thresh_128: CSL_FINST(temp_reg, UPP_UPTCR_TXSIZEA, 128B); break;
            case upp_thresh_256: CSL_FINST(temp_reg, UPP_UPTCR_TXSIZEA, 256B); break;
            default:             return IOM_EBADARGS;
        }

        switch (params->dma_q_thresh)
        {
            case upp_thresh_64:  CSL_FINST(temp_reg, UPP_UPTCR_RDSIZEQ, 64B); break;
            case upp_thresh_128: CSL_FINST(temp_reg, UPP_UPTCR_RDSIZEQ, 128B); break;
            case upp_thresh_256: CSL_FINST(temp_reg, UPP_UPTCR_RDSIZEQ, 256B); break;
            default:             return IOM_EBADARGS;
        }

        switch (params->dma_i_thresh)
        {
            case upp_thresh_64:  CSL_FINST(temp_reg, UPP_UPTCR_RDSIZEI, 64B); break;
            case upp_thresh_128: CSL_FINST(temp_reg, UPP_UPTCR_RDSIZEI, 128B); break;
            case upp_thresh_256: CSL_FINST(temp_reg, UPP_UPTCR_RDSIZEI, 256B); break;
            default:             return IOM_EBADARGS;
        }

        // apply all fields in one shot
        upp_reg_hdl->UPTCR = temp_reg;

    // 5. DLB

        temp_reg = 0;

        if (params->special_mode == upp_digital_loopback)
        {
            if (params->A.direction == upp_dir_rcv && params->B.direction == upp_dir_xmit)
            {
                CSL_FINST(temp_reg, UPP_UPDLB_BA, ENABLE);  // B out, A in
                CSL_FINST(temp_reg, UPP_UPDLB_AB, DISABLE); //
            }
            else if (params->A.direction == upp_dir_xmit && params->B.direction == upp_dir_rcv)
            {
                CSL_FINST(temp_reg, UPP_UPDLB_BA, DISABLE); // A out, B in
                CSL_FINST(temp_reg, UPP_UPDLB_AB, ENABLE);  //
            }
            else return IOM_EBADARGS;
        }
        else
        {
            CSL_FINST(temp_reg, UPP_UPDLB_BA, DISABLE);
            CSL_FINST(temp_reg, UPP_UPDLB_AB, DISABLE);
        }

        // apply all fields in one shot
        upp_reg_hdl->UPDLB = temp_reg;

    // enable interrupts
    
        temp_reg = 0;

        if (params->B.gen_eol) temp_reg |= upp_isr_bit_eolq;
        if (params->B.gen_eow) temp_reg |= upp_isr_bit_eowq;
        if (params->B.gen_err) temp_reg |= upp_isr_bit_errq;
        if (params->B.gen_uor) temp_reg |= upp_isr_bit_uorq;
        if (params->B.gen_dpe) temp_reg |= upp_isr_bit_dpeq;
        if (params->A.gen_eol) temp_reg |= upp_isr_bit_eoli;
        if (params->A.gen_eow) temp_reg |= upp_isr_bit_eowi;
        if (params->A.gen_err) temp_reg |= upp_isr_bit_erri;
        if (params->A.gen_uor) temp_reg |= upp_isr_bit_uori;
        if (params->A.gen_dpe) temp_reg |= upp_isr_bit_dpei;
        
        // note: interleave mode NYI

        // apply all fields in one shot
        upp_int_enable(temp_reg);

    // disable remaining interrupts

        // apply opposite
        temp_reg = (~temp_reg) & upp_isr_bit_all;

        // apply all fields in one shot
        upp_int_disable(temp_reg);

    // turn on peripheral
    CSL_FINST(upp_reg_hdl->UPPCR, UPP_UPPCR_EN, ENABLE);

    // Configuration accomplished
    return IOM_COMPLETED;
}


//  upp_handle_eowi function provided by uPP mini driver.
//  Updates the status of the driver handle and automatically
//  programs a new transfer if one is waiting in the queue

static inline Void upp_handle_eowi(upp_Hdl devp)
{
    upp_Transfer *new_transfer;

    if (devp->A.nextTransfer != NULL)
    {
        // move pending transfer to active slot
        devp->A.currentTransfer = devp->A.nextTransfer;
        devp->A.nextTransfer = NULL;

        // update status bits: active, but not pending (yet)
        devp->A.idle = 0;
        devp->A.pend = 0;
    }
    else
    {
        // update status bits: inactive, not pending
        devp->A.idle = 1;
        devp->A.pend = 0;
    }
    
    // make sure that DMA is ready for new descriptor set
    while (CSL_FEXT(upp_reg_hdl->UPIS2, UPP_UPIS2_PEND) == CSL_UPP_UPIS2_PEND_TRUE)
        asm(" nop");

    // retrieve new transfer from QUE
    new_transfer = Queue_get(devp->A.pendTransferList);

    // note: Queue_get returns the QUE handle if empty
    if ((Queue_Handle)new_transfer != devp->A.pendTransferList)
    {
        // program DMA descriptors
        upp_prog_dma_i(new_transfer);

        // update status bits and tracking slots
        if (devp->A.idle)
        {
            devp->A.currentTransfer = new_transfer;
            //devp->A.nextTransfer = NULL; // redundant
            devp->A.idle = 0;
            //devp->A.pend = 0; // redundant
        }
        else
        {
            devp->A.nextTransfer = new_transfer;
            //devp->A.idle = 0; // redundant
            devp->A.pend = 1;
        }
    }
}


//  upp_handle_eowq function provided by uPP mini driver.
//  Updates the status of the driver handle and automatically
//  programs a new transfer if one is waiting in the queue

static inline Void upp_handle_eowq(upp_Hdl devp)
{
    upp_Transfer *new_transfer;

    if (devp->B.nextTransfer != NULL)
    {
        // move pending transfer to active slot
        devp->B.currentTransfer = devp->B.nextTransfer;
        devp->B.nextTransfer = NULL;

        // update status bits: active, but not pending (yet)
        devp->B.idle = 0;
        devp->B.pend = 0;
    }
    else
    {
        // update status bits: inactive, not pending
        devp->B.idle = 1;
        devp->B.pend = 0;
    }
    
    // make sure that DMA is ready for new descriptor set
    while (CSL_FEXT(upp_reg_hdl->UPQS2, UPP_UPQS2_PEND) == CSL_UPP_UPQS2_PEND_TRUE)
        asm(" nop");

    // retrieve new transfer from QUE
    new_transfer = Queue_get(devp->B.pendTransferList);

    // note: Queue_get returns the QUE handle if empty
    if ((Queue_Handle)new_transfer != devp->B.pendTransferList)
    {
        // program DMA descriptors
        upp_prog_dma_q(new_transfer);

        // update status bits and tracking slots
        if (devp->B.idle)
        {
            devp->B.currentTransfer = new_transfer;
            //devp->B.nextTransfer = NULL; // redundant
            devp->B.idle = 0;
            //devp->B.pend = 0; // redundant
        }
        else
        {
            devp->B.nextTransfer = new_transfer;
            //devp->B.idle = 0; // redundant
            devp->B.pend = 1;
        }
    }
}


//  upp_readwrite_i function provided by uPP mini driver.
//  Executes a new transfer by programming DMA descriptors
//  or adding an element to the queue

static inline Void upp_readwrite_i(upp_ChanObj *chanp, upp_Transfer *new_transfer)
{
    if (chanp->idle)
    {
        // program DMA
        upp_prog_dma_i(new_transfer);

        // update status
        chanp->currentTransfer = new_transfer;
        chanp->idle = 0;
        //devp->A.pend = 0; // redundant (?)
    }
    else if (chanp->pend)
    {
        // enqueue transfer (ISR will program DMA later)
        Queue_put(chanp->pendTransferList, (Queue_Elem *)new_transfer);
    }
    else
    {
        // program DMA
        upp_prog_dma_i(new_transfer);

        // update status
        chanp->nextTransfer = new_transfer;
        //chanp->idle = 0;  // redundant
        chanp->pend = 1;
    }

    // note: does not handle impossible case of (idle && pend)
    //       differently from (idle && !pend)
}


//  upp_readwrite_q function provided by uPP mini driver.
//  Executes a new transfer by programming DMA descriptors
//  or adding an element to the queue

static inline Void upp_readwrite_q(upp_ChanObj *chanp, upp_Transfer *new_transfer)
{
    if (chanp->idle)
    {
        // program DMA
        upp_prog_dma_q(new_transfer);

        // update status
        chanp->currentTransfer = new_transfer;
        chanp->idle = 0;
        //devp->A.pend = 0; // redundant (?)
    }
    else if (chanp->pend)
    {
        // enqueue transfer (ISR will program DMA later)
        Queue_put(chanp->pendTransferList, (Queue_Elem *)new_transfer);
    }
    else
    {
        // program DMA
        upp_prog_dma_q(new_transfer);

        // update status
        chanp->nextTransfer = new_transfer;
        //chanp->idle = 0;  // redundant
        chanp->pend = 1;
    }

    // note: does not handle impossible case of (idle && pend)
    //       differently from (idle && !pend)
}


//  inline upp_prog_dma_i function provided by uPP mini driver.
//  Programs DMA descriptor set

static inline Void upp_prog_dma_i(upp_Transfer *xfer)
{
    upp_reg_hdl->UPID0 = (Uint32)xfer->windowAddr;
    upp_reg_hdl->UPID1 = ((Uint32)xfer->lineCount << 16) | (Uint32)xfer->lineSize;
    upp_reg_hdl->UPID2 = (Uint32)xfer->lineOffset;
}


//  inline upp_prog_dma_q function provided by uPP mini driver.
//  Programs DMA descriptor set

static inline Void upp_prog_dma_q(upp_Transfer *xfer)
{
    upp_reg_hdl->UPQD0 = (Uint32)xfer->windowAddr;
    upp_reg_hdl->UPQD1 = ((Uint32)xfer->lineCount << 16) | (Uint32)xfer->lineSize;
    upp_reg_hdl->UPQD2 = (Uint32)xfer->lineOffset;
}


//  inline upp_sw_reset function provided by uPP mini driver.
//  Toggles uPP device in and out of software reset and updates
//  status tracking information accordingly

static inline Void upp_sw_reset(upp_Hdl devp)
{
    upp_Transfer *que_transfer;
    Int32 i, oldCSR;

    // toggle reset with short delay
    CSL_FINST(upp_reg_hdl->UPPCR, UPP_UPPCR_SWRST, RESET);
    for (i = 0; i < 200; i++) asm(" nop");
    CSL_FINST(upp_reg_hdl->UPPCR, UPP_UPPCR_SWRST, RUNNING);

    // update status bits and transfer pointers
    devp->A.idle = 1;
    devp->A.pend = 0;
    devp->A.uor = 0;
    devp->A.error = 0;
    devp->A.dma_error = 0;
    devp->A.currentTransfer = NULL;
    devp->A.nextTransfer = NULL;
    devp->B.idle = 1;
    devp->B.pend = 0;
    devp->B.uor = 0;
    devp->B.error = 0;
    devp->B.dma_error = 0;
    devp->B.currentTransfer = NULL;
    devp->B.nextTransfer = NULL;

    // disable interrupts to maintain context
    oldCSR = Hwi_disable();

    // move waiting transfers from QUE to active and pending slots
    
        // Channel A
        if ((Queue_Handle)(que_transfer = Queue_get(devp->A.pendTransferList))
            != devp->A.pendTransferList)
        {
            // program DMA descriptors
            upp_prog_dma_q(que_transfer);
            devp->A.currentTransfer = que_transfer;
            devp->A.idle = 0;
        }
        if ((Queue_Handle)(que_transfer = Queue_get(devp->A.pendTransferList))
            != devp->A.pendTransferList)
        {
            // move transfer to pending slot
            devp->A.nextTransfer = que_transfer;
            devp->A.pend = 1;
        }
        
        // Channel B
        if ((Queue_Handle)(que_transfer = Queue_get(devp->B.pendTransferList))
            != devp->B.pendTransferList)
        {
            // program DMA descriptors
            upp_prog_dma_q(que_transfer);
            devp->B.currentTransfer = que_transfer;
            devp->B.idle = 0;
        }
        if ((Queue_Handle)(que_transfer = Queue_get(devp->B.pendTransferList))
            != devp->B.pendTransferList)
        {
            // move transfer to pending slot
            devp->B.nextTransfer = que_transfer;
            devp->B.pend = 1;
        }

    // note: Queue_get returns the QUE handle if empty

    // re-enable interrupts
    Hwi_restore(oldCSR);
}


// Global struct

upp_GlobalStr UPP_GLOBALSTR =
{
    FALSE,   // was mdBindDev already called?
    FALSE    // was channelInst already created?
};


//  IOM function table for the uPP mini driver implementation.

IOM_Fxns UPP_FXN_TABLE = {
    (IOM_TmdBindDev)upp_mdBindDev,
    IOM_UNBINDDEVNOTIMPL,  // upp_mdUnBindDev not implemented
    (IOM_TmdControlChan)upp_mdControlChan,
    (IOM_TmdCreateChan)upp_mdCreateChan,
    (IOM_TmdDeleteChan)upp_mdDeleteChan,
    (IOM_TmdSubmitChan)upp_mdSubmitChan,
};


//-------------
// END OF FILE 
//-------------
