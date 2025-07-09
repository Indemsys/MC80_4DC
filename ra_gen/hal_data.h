/* generated HAL header file - do not edit */
#ifndef HAL_DATA_H_
#define HAL_DATA_H_
#include <stdint.h>
#include "bsp_api.h"
#include "common_data.h"
#include "r_rtc.h"
#include "r_rtc_api.h"
#include "r_agt.h"
#include "r_timer_api.h"
#include "r_flash_hp.h"
#include "r_flash_api.h"
#include "r_dtc.h"
#include "r_transfer_api.h"
#include "r_spi_b.h"
FSP_HEADER
/* RTC Instance. */
extern const rtc_instance_t g_rtc;

/** Access the RTC instance using these structures when calling API functions directly (::p_api is not used). */
extern rtc_instance_ctrl_t g_rtc_ctrl;
extern const rtc_cfg_t     g_rtc_cfg;

#ifndef rtc_callback
void rtc_callback(rtc_callback_args_t* p_args);
#endif
/** AGT Timer Instance */
extern const timer_instance_t g_agt0;

/** Access the AGT instance using these structures when calling API functions directly (::p_api is not used). */
extern agt_instance_ctrl_t g_agt0_ctrl;
extern const timer_cfg_t   g_agt0_cfg;

#ifndef AGT0_callback
void AGT0_callback(timer_callback_args_t* p_args);
#endif
/* Flash on Flash HP Instance */
extern const flash_instance_t g_flash_cbl;

/** Access the Flash HP instance using these structures when calling API functions directly (::p_api is not used). */
extern flash_hp_instance_ctrl_t g_flash_ctrl;
extern const flash_cfg_t        g_flash_cfg;

#ifndef Flash_bgo_callback
void Flash_bgo_callback(flash_callback_args_t* p_args);
#endif

void hal_entry(void);
void g_hal_init(void);
FSP_FOOTER
#endif /* HAL_DATA_H_ */
