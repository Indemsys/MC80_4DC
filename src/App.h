#ifndef APP_H_
#define APP_H_

#include <ctype.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <time.h>
#include <arm_itm.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>

#include "bsp_api.h"
#include "tx_api.h"
#include "fx_api.h"
#include "gx_api.h"
#include "nx_crypto.h"

#include "ux_api.h"
#include "ux_device_class_cdc_acm.h"
#include "ux_device_class_storage.h"
#include "ux_device_class_hid.h"
#include "r_usb_basic.h"
#include "r_usb_basic_api.h"
#include "r_usb_basic_cfg.h"

#include "tx_timer.h"
#include "r_dtc.h"
#include "r_rtc.h"
#include "r_dmac.h"
#include "r_spi_b.h"
#include "r_spi_api.h"
#include "r_rtc_api.h"
#include "r_timer_api.h"
#include "r_flash_api.h"

#include "r_canfd.h"
#include "r_can_api.h"

#include "SEGGER_RTT.h"
#include "rm_block_media_api.h"
#include "rm_filex_block_media_api.h"
#include "hal_data.h"

#define SEGGER_INDEX            (0)
#define LVL_ERR                 (1u)  /* error conditions   */
#define APP_PRINT(fn_, ...)     (SEGGER_RTT_printf(SEGGER_INDEX, (fn_), ##__VA_ARGS__))
#define APP_ERR_PRINT(fn_, ...) ({if(LVL_ERR)SEGGER_RTT_printf (SEGGER_INDEX, "[ERR] In Function: %s(), %s",__FUNCTION__,(fn_),##__VA_ARGS__); })
#define APP_ERR_TRAP(err)       ({if(err) {SEGGER_RTT_printf(SEGGER_INDEX, "\r\nReturned Error Code: 0x%x  \r\n", (err)); __asm("BKPT #0\n");} }) /* trap upon the error  */
#define APP_READ(read_data)     (SEGGER_RTT_Read(SEGGER_INDEX, (read_data), sizeof(read_data)))
#define APP_CHECK_DATA          (SEGGER_RTT_HasKey())

#include "MC80.h"

#endif  // APP_H
