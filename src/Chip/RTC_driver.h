#ifndef RTC_DRIVER_H
#define RTC_DRIVER_H

/* Reset value for RTC time struct initialization */
#define RESET_VALUE          (0)

#define SET_FLAG             (0x01)
#define RESET_FLAG           (0x00)

#define ASCII_ZERO           (48)
#define NULL_CHAR            ('\0')

#define BYTES_RECEIVED_ZERO  (0U)

#define MON_ADJUST_VALUE     (1)
#define YEAR_ADJUST_VALUE    (1900)

#define PLACE_VALUE_TEN      (10)
#define PLACE_VALUE_HUNDRED  (100)
#define PLACE_VALUE_THOUSAND (1000)

fsp_err_t
          RTC_init(void);
fsp_err_t RTC_get_system_DateTime(rtc_time_t* rt_time_p);
void      RTC_date_readability_update(rtc_time_t* time);
fsp_err_t RTC_set_system_DateTime(rtc_time_t* time);
void      RTC_deinit(void);

/* RTC callback function declaration */
void rtc_callback(rtc_callback_args_t* p_args);

#endif
