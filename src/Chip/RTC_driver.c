#include "App.h"

static rtc_time_t g_set_time = {
 .tm_hour  = RESET_VALUE,
 .tm_isdst = RESET_VALUE,
 .tm_mday  = RESET_VALUE,
 .tm_min   = RESET_VALUE,
 .tm_mon   = RESET_VALUE,
 .tm_sec   = RESET_VALUE,
 .tm_wday  = RESET_VALUE,
 .tm_yday  = RESET_VALUE,
 .tm_year  = RESET_VALUE,
};

volatile uint32_t g_alarm_irq_flag;
volatile uint32_t g_periodic_irq_flag;
volatile uint32_t g_invalid_sequence;

// Flag to track RTC initialization state
static bool g_rtc_initialized = false;

static bool _Is_rtc_time_valid(rtc_time_t *time);
static void _Reset_rtc_time_values(rtc_time_t *time);

/*-----------------------------------------------------------------------------------------------------
  RTC initialization. Opens RTC, checks and sets clock source, sets default time if needed, logs all actions.

  Parameters:
    None

  Return:
    fsp_err_t - result of initialization
-----------------------------------------------------------------------------------------------------*/
fsp_err_t RTC_init(void)
{
  fsp_err_t  err = FSP_SUCCESS;
  rtc_info_t rtc_info;

  // Parse compilation date and time to g_set_time
  Parse_compile_date_time(&g_set_time);

  err = R_RTC_Open(&g_rtc_ctrl, &g_rtc_cfg);
  if (FSP_SUCCESS != err)
  {
    APPLOG("RTC: module open failed. Restart the Application");
    return err;
  }

  err = R_RTC_InfoGet(&g_rtc_ctrl, &rtc_info);
  if (FSP_SUCCESS != err)
  {
    APPLOG("RTC: failed to get info");
    return err;
  }

  if (rtc_info.clock_source != RTC_CLOCK_SOURCE_SUBCLK)
  {
    APPLOG("RTC: clock source is not Sub-Clock. Changing to Sub-Clock.");
    err = R_RTC_ClockSourceSet(&g_rtc_ctrl);
    if (FSP_SUCCESS != err)
    {
      APPLOG("RTC: failed to set clock source to Sub-Clock.");
      return err;
    }
    err = R_RTC_InfoGet(&g_rtc_ctrl, &rtc_info);
    if (FSP_SUCCESS != err)
    {
      APPLOG("RTC: failed to get info after changing clock source.");
      return err;
    }
    if (rtc_info.clock_source != RTC_CLOCK_SOURCE_SUBCLK)
    {
      APPLOG("RTC: clock source is still not Sub-Clock after change.");
      return FSP_ERR_INTERNAL;
    }
    APPLOG("RTC: clock source changed to Sub-Clock.");
  }
  if (rtc_info.status == RTC_STATUS_STOPPED)
  {
    APPLOG("RTC: not running. Starting.");
    APPLOG("RTC: setting default time: %04d-%02d-%02d %02d:%02d:%02d", g_set_time.tm_year + 1900, g_set_time.tm_mon, g_set_time.tm_mday, g_set_time.tm_hour, g_set_time.tm_min, g_set_time.tm_sec);
    err = R_RTC_CalendarTimeSet(&g_rtc_ctrl, &g_set_time);
    if (FSP_SUCCESS != err)
    {
      APPLOG("RTC: failed to set default time and start.");
      return err;
    }
    APPLOG("RTC: started.");
  }
  else if (rtc_info.status == RTC_STATUS_RUNNING)
  {
    rtc_time_t running_time;
    fsp_err_t  running_time_err = R_RTC_CalendarTimeGet(&g_rtc_ctrl, &running_time);
    if (FSP_SUCCESS == running_time_err)
    {
      // Validate the read time before using it
      if (_Is_rtc_time_valid(&running_time))
      {
        RTC_date_readability_update(&running_time);
        APPLOG("RTC: already running. Current time: %04d-%02d-%02d %02d:%02d:%02d", running_time.tm_year, running_time.tm_mon, running_time.tm_mday, running_time.tm_hour, running_time.tm_min, running_time.tm_sec);
      }
      else
      {
        APPLOG("RTC: invalid time detected. Resetting to compilation time.");
        APPLOG("RTC: setting compilation time: %04d-%02d-%02d %02d:%02d:%02d", g_set_time.tm_year + 1900, g_set_time.tm_mon, g_set_time.tm_mday, g_set_time.tm_hour, g_set_time.tm_min, g_set_time.tm_sec);
        err = R_RTC_CalendarTimeSet(&g_rtc_ctrl, &g_set_time);
        if (FSP_SUCCESS != err)
        {
          APPLOG("RTC: failed to set compilation time after invalid time detection.");
          return err;
        }
        APPLOG("RTC: time reset to compilation time.");
      }
    }
    else
    {
      APPLOG("RTC: already running. Failed to read current time.");
    }
  }
  else
  {
    APPLOG("RTC: unknown status: %u", (unsigned)rtc_info.status);
  }

  // Set initialization flag after successful RTC initialization
  g_rtc_initialized = true;

  return err;
}

/*-----------------------------------------------------------------------------------------------------
  Validates RTC time structure for reasonable values

  Parameters:
    time - pointer to RTC time structure to validate

  Return:
    bool - true if time is valid, false otherwise
-----------------------------------------------------------------------------------------------------*/
static bool _Is_rtc_time_valid(rtc_time_t *time)
{
  if (time == NULL)
  {
    return false;
  }

  // Check seconds (0-59)
  if (time->tm_sec > 59)
  {
    return false;
  }

  // Check minutes (0-59)
  if (time->tm_min > 59)
  {
    return false;
  }

  // Check hours (0-23)
  if (time->tm_hour > 23)
  {
    return false;
  }

  // Check month (0-11 in tm structure)
  if (time->tm_mon > 11)
  {
    return false;
  }

  // Check day of month (1-31, basic check)
  if (time->tm_mday < 1 || time->tm_mday > 31)
  {
    return false;
  }

  // Check year (reasonable range, years since 1900)
  if (time->tm_year < 100 || time->tm_year > 200)  // 2000-2100
  {
    return false;
  }

  // Check day of week (0-6)
  if (time->tm_wday > 6)
  {
    return false;
  }

  // Check day of year (0-365)
  if (time->tm_yday > 365)
  {
    return false;
  }

  return true;
}

/*-----------------------------------------------------------------------------------------------------
  Gets current time from RTC

  Parameters:
    rt_time_p - pointer to structure where time will be written

  Return:
    fsp_err_t - operation result
-----------------------------------------------------------------------------------------------------*/
fsp_err_t RTC_get_system_DateTime(rtc_time_t *rt_time_p)
{
  // Check if RTC has been initialized
  if (!g_rtc_initialized)
  {
    // Reset time and return error if RTC not initialized
    _Reset_rtc_time_values(rt_time_p);
    return FSP_ERR_NOT_INITIALIZED;
  }

  fsp_err_t err = FSP_SUCCESS;
  err           = R_RTC_CalendarTimeGet(&g_rtc_ctrl, rt_time_p);
  if (FSP_SUCCESS != err)
  {
    // Reset time in case of failure
    _Reset_rtc_time_values(rt_time_p);
    return err;
  }

  // Validate the read time before using it
  if (!_Is_rtc_time_valid(rt_time_p))
  {
    // Reset time in case of invalid data
    _Reset_rtc_time_values(rt_time_p);
    return FSP_ERR_INVALID_DATA;
  }

  RTC_date_readability_update(rt_time_p);
  return err;
}

/*-----------------------------------------------------------------------------------------------------
  Modifies the date to readable format for the user

  Parameters:
    time - date to be modified

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void RTC_date_readability_update(rtc_time_t *time)
{
  time->tm_mon += MON_ADJUST_VALUE;
  time->tm_year += YEAR_ADJUST_VALUE;
}

/*-----------------------------------------------------------------------------------------------------
  Sets date and time

  Parameters:
    time - pointer to structure with date and time

  Return:
    fsp_err_t - operation result
-----------------------------------------------------------------------------------------------------*/
fsp_err_t RTC_set_system_DateTime(rtc_time_t *time)
{
  fsp_err_t err = FSP_SUCCESS;
  // Check for valid input
  if (time == NULL)
  {
    APPLOG("RTC: set time failed - NULL pointer");
    return FSP_ERR_INVALID_POINTER;
  }

  // Validate the time using the same validation function
  if (!_Is_rtc_time_valid(time))
  {
    APPLOG("RTC: set time failed - invalid time values");
    return FSP_ERR_INVALID_ARGUMENT;
  }

  err = R_RTC_CalendarTimeSet(&g_rtc_ctrl, time);
  if (FSP_SUCCESS != err)
  {
    return err;
  }

  return err;
}
/*-----------------------------------------------------------------------------------------------------
  RTC callback function

  Parameters:
    p_args - callback arguments

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void rtc_callback(rtc_callback_args_t *p_args)
{
  if (RTC_EVENT_ALARM_IRQ == p_args->event)
  {
    g_alarm_irq_flag = SET_FLAG;
  }
  else
  {
    g_periodic_irq_flag = SET_FLAG;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Closes opened RTC module before the project ends up in an Error Trap

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void RTC_deinit(void)
{
  fsp_err_t err     = FSP_SUCCESS;

  // Reset initialization flag
  g_rtc_initialized = false;

  err               = R_RTC_Close(&g_rtc_ctrl);
  if (FSP_SUCCESS != err)
  {
    APP_ERR_PRINT("** RTC module Close failed **  \r\n");
  }
}

/*-----------------------------------------------------------------------------------------------------
  Resets RTC time structure to default values

  Parameters:
    time - pointer to RTC time structure to reset

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Reset_rtc_time_values(rtc_time_t *time)
{
  time->tm_hour  = RESET_VALUE;
  time->tm_isdst = RESET_VALUE;
  time->tm_mday  = RESET_VALUE;
  time->tm_min   = RESET_VALUE;
  time->tm_mon   = RESET_VALUE;
  time->tm_sec   = RESET_VALUE;
  time->tm_wday  = RESET_VALUE;
  time->tm_yday  = RESET_VALUE;
  time->tm_year  = RESET_VALUE;
}
