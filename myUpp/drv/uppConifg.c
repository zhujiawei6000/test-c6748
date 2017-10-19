/*
 * uppConifg.c
 *
 *  Created on: 2017Äê10ÔÂ13ÈÕ
 *      Author: feng
 */
#include <xdc/std.h>
#include "upp.h"


upp_Params UppDefaultConifgs = {

    .channels = 2,

    .A = {
        .direction = upp_dir_rcv,
        .clock_div = 7,
        .data_rate = 1,
        .width = 16,
        .format = upp_format_rjze,
        .xmit_thresh = upp_thresh_64,
        .drive_idle = TRUE,
        .idle_value = 0,
        // signal enable
        .en_start = TRUE,
        .en_enable = TRUE,
        .en_wait = TRUE,
        // signal polarity
        .inv_start = FALSE,
        .inv_enable = FALSE,
        .inv_wait = FALSE,
        .inv_clock = FALSE,
        // interrupt generation (async mode only)
        .gen_eow = TRUE,
        .gen_eol = FALSE,
        .gen_uor = TRUE,
        .gen_err = TRUE,
        .gen_dpe = TRUE,
        // interrupt dispatch (async mode only)
        .fxn_eow = NULL,
        .fxn_eol = NULL,
        .fxn_uor = NULL,
        .fxn_err = NULL,
        .fxn_dpe = NULL,
    },

    .B = {
        .direction = upp_dir_rcv,
        .clock_div = 7,
        .data_rate = 1,
        .width = 16,
        .format = upp_format_rjze,
        .xmit_thresh = upp_thresh_64,
        .drive_idle = TRUE,
        .idle_value = 0,
        // signal enable
        .en_start = TRUE,
        .en_enable = TRUE,
        .en_wait = TRUE,
        // signal polarity
        .inv_start = FALSE,
        .inv_enable = FALSE,
        .inv_wait = FALSE,
        .inv_clock = FALSE,
        // interrupt generation (async mode only)
        .gen_eow = TRUE,
        .gen_eol = FALSE,
        .gen_uor = TRUE,
        .gen_err = TRUE,
        .gen_dpe = TRUE,
        // interrupt dispatch (async mode only)
        .fxn_eow = NULL,
        .fxn_eol = NULL,
        .fxn_uor = NULL,
        .fxn_err = NULL,
        .fxn_dpe = NULL,
    },

    .dma_i_thresh = upp_thresh_64,
    .dma_q_thresh = upp_thresh_64,

    .special_mode = upp_none,
};



