/* generated HAL source file - do not edit */
#include "hal_data.h"
rtc_instance_ctrl_t              g_rtc_ctrl;
const rtc_error_adjustment_cfg_t g_rtc_err_cfg = {
  .adjustment_mode   = RTC_ERROR_ADJUSTMENT_MODE_AUTOMATIC,
  .adjustment_period = RTC_ERROR_ADJUSTMENT_PERIOD_1_MINUTE,
  .adjustment_type   = RTC_ERROR_ADJUSTMENT_ADD_PRESCALER,
  .adjustment_value  = 0,
};
const rtc_cfg_t g_rtc_cfg = {
  .clock_source       = RTC_CLOCK_SOURCE_SUBCLK,
  .freq_compare_value = 255,
  .p_err_cfg          = &g_rtc_err_cfg,
  .p_callback         = rtc_callback,
  .p_context          = NULL,
  .p_extend           = NULL,
  .alarm_ipl          = (12),
  .periodic_ipl       = (12),
  .carry_ipl          = (12),
#if defined(VECTOR_NUMBER_RTC_ALARM)
  .alarm_irq = VECTOR_NUMBER_RTC_ALARM,
#else
  .alarm_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_RTC_PERIOD)
  .periodic_irq = VECTOR_NUMBER_RTC_PERIOD,
#else
  .periodic_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_RTC_CARRY)
  .carry_irq = VECTOR_NUMBER_RTC_CARRY,
#else
  .carry_irq = FSP_INVALID_VECTOR,
#endif
};
/* Instance structure to use this module. */
const rtc_instance_t g_rtc = {
  .p_ctrl = &g_rtc_ctrl,
  .p_cfg  = &g_rtc_cfg,
  .p_api  = &g_rtc_on_rtc
};

agt_instance_ctrl_t      g_agt0_ctrl;
const agt_extended_cfg_t g_agt0_extend = {
  .count_source            = AGT_CLOCK_PCLKB,
  .agto                    = AGT_PIN_CFG_DISABLED,
  .agtoab_settings_b.agtoa = AGT_PIN_CFG_DISABLED,
  .agtoab_settings_b.agtob = AGT_PIN_CFG_DISABLED,
  .measurement_mode        = AGT_MEASURE_DISABLED,
  .agtio_filter            = AGT_AGTIO_FILTER_NONE,
  .enable_pin              = AGT_ENABLE_PIN_NOT_USED,
  .trigger_edge            = AGT_TRIGGER_EDGE_RISING,
  .counter_bit_width       = AGT_COUNTER_BIT_WIDTH_16,
};

const timer_cfg_t g_agt0_cfg = {
  .mode              = TIMER_MODE_PERIODIC, // Actual period: 0.0001 seconds. Actual duty: 50%.
  .period_counts     = (uint32_t)0x1770,
  .duty_cycle_counts = 0xbb8,
  .source_div        = (timer_source_div_t)0,
  .channel           = 0,
  .p_callback        = AGT0_callback,
/** If NULL then do not add & */
#if defined(NULL)
  .p_context = NULL,
#else
  .p_context = &NULL,
#endif
  .p_extend      = &g_agt0_extend,
  .cycle_end_ipl = (5),
#if defined(VECTOR_NUMBER_AGT0_INT)
  .cycle_end_irq = VECTOR_NUMBER_AGT0_INT,
#else
  .cycle_end_irq = FSP_INVALID_VECTOR,
#endif
};
/* Instance structure to use this module. */
const timer_instance_t g_agt0 = {
  .p_ctrl = &g_agt0_ctrl,
  .p_cfg  = &g_agt0_cfg,
  .p_api  = &g_timer_on_agt
};
