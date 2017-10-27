/*
 * upp.c
 *
 *  Created on: 2017��10��12��
 *      Author: feng
 */

#include "upp.h"
#include <xdc/runtime/System.h>
#include <xdc/runtime/Memory.h>
#include <string.h>
#include <xdc/runtime/IHeap.h>
#include <xdc/runtime/Error.h>

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

static inline Void upp_prog_dma_i(Upp_Transfer_Desc *xfer);
static inline Void upp_prog_dma_q(Upp_Transfer_Desc *xfer);
static inline Void upp_sw_reset(void);
static inline Void upp_handle_eowi(upp_Hdl devp);
static inline Void upp_handle_eowq(upp_Hdl devp);

//Upp object
static upp_Obj UppDevice;

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

#if CHG_SEND_CLOCK
/*
 * @brief   Upp Send Clock setup
 * @clockSrc   Clock source
 */
static void Upp_Send_Clock_Init(upp_ClockSrc clockSrc)
{
    // select clock source
    switch (clockSrc)
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

}
#endif


static void upp_isr (upp_Hdl devp)
{
    Uint32 intr_status = upp_int_status();
    Upp_Transfer_Desc *old_transfer;

    // loop to make sure we don't miss a new interrupt
    //  while servicing an old one
    while (intr_status != 0)
    {
        //ISR for channle B
        if (intr_status & upp_isr_bit_eowq)
        {
            upp_int_clear(upp_isr_bit_eowq);

            // remember old transfer for user callback
            old_transfer = devp->B.currentTransfer;

            // call inline function
            upp_handle_eowq(devp);

            // call user function if specified
            if (devp->params->B.fxn_eow != NULL)
                (*devp->params->B.fxn_eow)(old_transfer, upp_isr_bit_eowq);
        }

        if (intr_status & upp_isr_bit_eolq)
        {
            upp_int_clear(upp_isr_bit_eolq);

            // call user function if specified
            if (devp->params->B.fxn_eol != NULL)
                (*devp->params->B.fxn_eol)(devp->B.currentTransfer, upp_isr_bit_eolq);
        }

        if (intr_status & upp_isr_bit_errq)
        {
            upp_int_clear(upp_isr_bit_errq);

            // report error condition
            devp->B.error = 1;

            // call user function if specified
            if (devp->params->B.fxn_err != NULL)
                (*devp->params->B.fxn_err)(devp->B.currentTransfer, upp_isr_bit_errq);
        }
        if (intr_status & upp_isr_bit_uorq)
        {
            upp_int_clear(upp_isr_bit_uorq);

            // report uor condition
            devp->B.uor = 1;

            // call user function if specified
            if (devp->params->B.fxn_uor != NULL)
                (*devp->params->B.fxn_uor)(devp->B.currentTransfer, upp_isr_bit_uorq);
        }
        if (intr_status & upp_isr_bit_dpeq)
        {
            upp_int_clear(upp_isr_bit_dpeq);
            // report DMA error condition
            devp->B.dma_error = 1;
            // call user function if specified
            if (devp->params->B.fxn_dpe != NULL)
                (*devp->params->B.fxn_dpe)(devp->B.currentTransfer, upp_isr_bit_dpeq);
        }

        //ISR for channle A
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

            // call user function if specified
            if (devp->params->A.fxn_eow != NULL)
                (*devp->params->A.fxn_eow)(old_transfer, upp_isr_bit_eowi);
        }
        if (intr_status & upp_isr_bit_erri)
        {
            upp_int_clear(upp_isr_bit_erri);

            // report error condition
            devp->A.error = 1;

            // call user function if specified
            if (devp->params->A.fxn_err != NULL)
                (*devp->params->A.fxn_err)(devp->A.currentTransfer, upp_isr_bit_erri);
        }
        if (intr_status & upp_isr_bit_uori)
        {
            upp_int_clear(upp_isr_bit_uori);

            // report uor condition
            devp->A.uor = 1;
            // call user function if specified
            if (devp->params->A.fxn_uor != NULL)
                (*devp->params->A.fxn_uor)(devp->A.currentTransfer, upp_isr_bit_uori);
        }
        if (intr_status & upp_isr_bit_dpei)
        {
            upp_int_clear(upp_isr_bit_dpei);

            // report DMA error condition
            devp->A.dma_error = 1;
            // call user function if specified
            if (devp->params->A.fxn_dpe != NULL)
                (*devp->params->A.fxn_dpe)(devp->A.currentTransfer, upp_isr_bit_dpei);
        }

        // make sure all interrupts are handled
        intr_status = upp_int_status();
    }

    // finally: write 0 to EOI register
    upp_reg_hdl->UPEOI = 0;
}

/*
 * @brief   software reset
 */
static inline void upp_sw_reset(void)
{
    // turn off peripheral
    CSL_FINST(upp_reg_hdl->UPPCR, UPP_UPPCR_EN, DISABLE);

    // wait until peripheral is idle
    while (CSL_FEXT(upp_reg_hdl->UPPCR, UPP_UPPCR_DB) == CSL_UPP_UPPCR_DB_ACTIVE)
        asm(" nop");

    // toggle reset with short delay
    CSL_FINST(upp_reg_hdl->UPPCR, UPP_UPPCR_SWRST, RESET);
    int i;
    for (i = 0; i < 200; i++) asm(" nop");
    CSL_FINST(upp_reg_hdl->UPPCR, UPP_UPPCR_SWRST, RUNNING);
}

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
            //     (insert other error conditions here)

        )    return -1;

    // note: Data interleave mode is not yet implemented; don't let user
    //       configure device in interleave mode.  Remove this check
    //       if/when interleave mode is fully implemented.
    if (params->special_mode == upp_sdr_xmit_interleave ||
        params->special_mode == upp_ddr_interleave)
        return -1;

    // reset peripheral
    upp_sw_reset();

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
            default:                return -1;
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
            default:    return -1;
        }

        switch (params->B.data_rate)
        {
            case 1:  CSL_FINST(temp_reg, UPP_UPCTL_DRB, SINGLE);
                     break;
            case 2:  CSL_FINST(temp_reg, UPP_UPCTL_DRB, DOUBLE);
                     break;
            default: return -1;
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
            default:                return -1;
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
            default:    return -1;
        }

        switch (params->A.data_rate)
        {
            case 1:  CSL_FINST(temp_reg, UPP_UPCTL_DRA, SINGLE);
                     break;
            case 2:  CSL_FINST(temp_reg, UPP_UPCTL_DRA, DOUBLE);
                     break;
            default: return -1;
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
            default:    return -1;
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
        else return -1;

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
            return -1;
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
            return -1;
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
            default:             return -1;
        }

        switch (params->A.xmit_thresh)
        {
            case upp_thresh_64:  CSL_FINST(temp_reg, UPP_UPTCR_TXSIZEA, 64B); break;
            case upp_thresh_128: CSL_FINST(temp_reg, UPP_UPTCR_TXSIZEA, 128B); break;
            case upp_thresh_256: CSL_FINST(temp_reg, UPP_UPTCR_TXSIZEA, 256B); break;
            default:             return -1;
        }

        switch (params->dma_q_thresh)
        {
            case upp_thresh_64:  CSL_FINST(temp_reg, UPP_UPTCR_RDSIZEQ, 64B); break;
            case upp_thresh_128: CSL_FINST(temp_reg, UPP_UPTCR_RDSIZEQ, 128B); break;
            case upp_thresh_256: CSL_FINST(temp_reg, UPP_UPTCR_RDSIZEQ, 256B); break;
            default:             return -1;
        }

        switch (params->dma_i_thresh)
        {
            case upp_thresh_64:  CSL_FINST(temp_reg, UPP_UPTCR_RDSIZEI, 64B); break;
            case upp_thresh_128: CSL_FINST(temp_reg, UPP_UPTCR_RDSIZEI, 128B); break;
            case upp_thresh_256: CSL_FINST(temp_reg, UPP_UPTCR_RDSIZEI, 256B); break;
            default:             return -1;
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
            else return -1;
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

    // Configuration accomplished
    return 0;
}

static inline void upp_prog_dma_i(Upp_Transfer_Desc *xfer)
{
    upp_reg_hdl->UPID0 = (Uint32)xfer->windowAddr;
    upp_reg_hdl->UPID1 = ((Uint32)xfer->lineCount << 16) | (Uint32)xfer->lineSize;
    upp_reg_hdl->UPID2 = (Uint32)xfer->lineOffset;
}

static inline void upp_prog_dma_q(Upp_Transfer_Desc *xfer)
{
    upp_reg_hdl->UPQD0 = (Uint32)xfer->windowAddr;
    upp_reg_hdl->UPQD1 = ((Uint32)xfer->lineCount << 16) | (Uint32)xfer->lineSize;
    upp_reg_hdl->UPQD2 = (Uint32)xfer->lineOffset;
}

static inline void upp_handle_eowi(upp_Hdl devp)
{
    if(devp->A.currentTransfer == NULL) return;

    Upp_Transfer_Desc * new_transfer = devp->A.currentTransfer->next;

    if(new_transfer != NULL) {
        // make sure that DMA is ready for new descriptor set
        while (CSL_FEXT(upp_reg_hdl->UPIS2, UPP_UPIS2_PEND) == CSL_UPP_UPIS2_PEND_TRUE)
            asm(" nop");

        // program DMA descriptors
        upp_prog_dma_i(new_transfer);
        devp->A.currentTransfer = new_transfer;
    }
}

static inline void upp_handle_eowq(upp_Hdl devp)
{
    if(devp->B.currentTransfer == NULL) return;

    Upp_Transfer_Desc * new_transfer = devp->B.currentTransfer->next;

    if(new_transfer != NULL)
    {    // make sure that DMA is ready for new descriptor set
        while (CSL_FEXT(upp_reg_hdl->UPQS2, UPP_UPQS2_PEND) == CSL_UPP_UPQS2_PEND_TRUE)
            asm(" nop");

        // program DMA descriptors
        upp_prog_dma_q(new_transfer);
        devp->B.currentTransfer = new_transfer;
    }
}




//static buffer for channel B
static Upp_Transfer_Desc TransferLink[TRANSFER_LINK_DEPTH];

extern IHeap_Handle MyHeap;

static int InitRecvChanLinkB(void)
{
    int i;
    for(i = 0; i < TRANSFER_LINK_DEPTH; i++)
    {
        //System_printf("TransferLink[%d] address: %x\n", i, &TransferLink[i]);
        TransferLink[i].lineSize = UPP_WINDOW_LINE_SIZE;
        TransferLink[i].lineOffset = UPP_WINDOW_LINE_SIZE;
        TransferLink[i].lineCount = UPP_WINDOW_LINE_COUNT;
        Error_Block eb;

        //malloc for windows buffer
        WindowBuffer * buffer = (WindowBuffer *)Memory_alloc(MyHeap, sizeof(WindowBuffer), 8, &eb);
        if(buffer == NULL) {
            System_printf("alloc buffer for windowBuffer[%d] error\n", i);
            return -1;
        }
        else {
            System_printf("alloc buffer for windowBuffer[%d]: 0x%08x\n", i, buffer);
            //init buffer
            memset(buffer, 0 ,sizeof(WindowBuffer));

            TransferLink[i].windowAddr = buffer;
            TransferLink[i].windowIndex = i;
            TransferLink[i].next = (i + 1 < TRANSFER_LINK_DEPTH) ? &TransferLink[i + 1] : &TransferLink[0];

            Cache_wbInv(buffer, sizeof(WindowBuffer) , Cache_Type_ALL, TRUE);
        }
    }

    return 0;
}






/*
 * @brief   Start recv transfer for channel B
 *
 * @return  0 = success  -1 = failure
 */
int StartRecvTransB(void)
{
    if(UppDevice.B.isIdle == TRUE) {

        UppDevice.B.isIdle = FALSE;
        UppDevice.B.uor = FALSE;
        UppDevice.B.error = FALSE;
        UppDevice.B.dma_error = FALSE;
        //record current transfer
        UppDevice.B.currentTransfer = &TransferLink[0];

        if (upp_apply_params(&UppDevice) != 0) {
            return -1;
        }

        // turn on peripheral
        CSL_FINST(upp_reg_hdl->UPPCR, UPP_UPPCR_EN, ENABLE);
        //start transfer
        upp_prog_dma_q(&TransferLink[0]);
        return 0;
    }
    else return -1;
}

/*
 * @brief   Stop recv transfer for channel B
 *
 */
void StopRecvTransB(void)
{
    if(UppDevice.B.isIdle == FALSE) {
        //reset upp state machine
        upp_sw_reset();
        UppDevice.B.isIdle = TRUE;
    }
}


/*
 * @brief   Start transfer for channel A
 * @buffer  window buffer
 */
void Test_StartTransWindow_A(WindowBuffer *buffer)
{
    Upp_Transfer_Desc trans;

    trans.next = NULL;
    trans.lineCount = UPP_WINDOW_LINE_COUNT;
    trans.lineSize = UPP_WINDOW_LINE_SIZE;
    trans.lineOffset = UPP_WINDOW_LINE_SIZE;
    trans.windowAddr = buffer;
    trans.windowIndex = 0;

    // make sure that DMA is ready for new descriptor set
    while (CSL_FEXT(upp_reg_hdl->UPIS2, UPP_UPIS2_PEND) == CSL_UPP_UPIS2_PEND_TRUE)
        asm(" nop");

    upp_prog_dma_i(&trans);
}


/*
 * @brief   Start transfer for channel A
 * @line    one line
 */
void Test_StartTransOneLine_A(WindowLine *line)
{
    Upp_Transfer_Desc trans;

    trans.next = NULL;
    trans.lineCount = 1;
    trans.lineSize = UPP_WINDOW_LINE_SIZE;
    trans.lineOffset = UPP_WINDOW_LINE_SIZE;
    trans.windowAddr = line;
    trans.windowIndex = 0;

    // make sure that DMA is ready for new descriptor set
    while (CSL_FEXT(upp_reg_hdl->UPIS2, UPP_UPIS2_PEND) == CSL_UPP_UPIS2_PEND_TRUE)
        asm(" nop");

    upp_prog_dma_i(&trans);
}






/*
 * @brief   Upp init
 * @return  NULL = error or device struct
 */
upp_Hdl Upp_Init(void)
{
    LOCAL_apply_upp_pinmux();
    // turn on uPP LPSC
    LOCAL_PSC1_LPSC_enable(0, 19);
#if CHG_SEND_CLOCK
    Upp_Send_Clock_Init(upp_defaultClk);
#endif

    UppDevice.A.isIdle = TRUE;
    UppDevice.A.uor = FALSE;
    UppDevice.A.error = FALSE;
    UppDevice.A.dma_error = FALSE;
    UppDevice.A.currentTransfer = NULL;

    UppDevice.B.isIdle = TRUE;
    UppDevice.B.uor = FALSE;
    UppDevice.B.error = FALSE;
    UppDevice.B.dma_error = FALSE;
    UppDevice.B.currentTransfer = NULL;

    // apply uPP configuration and enable peripheral
    UppDevice.params = &UppDefaultConifgs;
    UppDevice.A.params = &UppDevice.params->A;
    UppDevice.B.params = &UppDevice.params->B;

    EventCombiner_dispatchPlug(CSL_INTC_EVENTID_UPPINT, (EventCombiner_FuncPtr)upp_isr, (UArg)&UppDevice, TRUE);
    EventCombiner_enableEvent(CSL_INTC_EVENTID_UPPINT);

    if(InitRecvChanLinkB()) {
        return NULL;
    }
    else {
        return &UppDevice;
    }
}
