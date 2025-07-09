#include "App.h"

//-----------------------------------------------------------------------------------------------------
// Description:
//   Get system clock divider, frequency, and tick frequency.
//
// Parameters:
//   p_sys_div   - pointer to store system divider
//   p_sys_freq  - pointer to store system frequency
//   p_ticks_freq- pointer to store tick frequency
//
// Return: void
//-----------------------------------------------------------------------------------------------------
void Get_system_timings(uint32_t *p_sys_div, uint32_t *p_sys_freq, uint32_t *p_ticks_freq)
{
  uint32_t ticks_freq;
  uint32_t sys_div = SysTick->LOAD + 1;  // Get system tick divider
  ticks_freq       = FRQ_ICLK_MHZ / sys_div;
  if (p_sys_div != 0) *p_sys_div = sys_div;
  if (p_sys_freq != 0) *p_sys_freq = FRQ_ICLK_MHZ;
  if (p_ticks_freq != 0) *p_ticks_freq = ticks_freq;
}

//-----------------------------------------------------------------------------------------------------
// Description:
//   Get current system tick count.
//
// Parameters:
//   v - pointer to store tick value (can be NULL)
//
// Return: uint32_t - current tick value
//-----------------------------------------------------------------------------------------------------
uint32_t Get_system_ticks(uint32_t *v)
{
  uint32_t t;
  t = tx_time_get();
  if (v != 0) *v = t;
  return t;
}

//-----------------------------------------------------------------------------------------------------
// Description:
//   Calculate time difference in seconds between two tick values.
//
// Parameters:
//   start_time_val - start tick value
//   stop_time_val  - stop tick value
//
// Return: uint32_t - difference in seconds
//-----------------------------------------------------------------------------------------------------
uint32_t Time_diff_seconds(uint32_t start_time_val, uint32_t stop_time_val)
{
  return (stop_time_val - start_time_val) / TX_TIMER_TICKS_PER_SECOND;
}

//-----------------------------------------------------------------------------------------------------
// Description:
//   Calculate time difference in microseconds between two tick values.
//
// Parameters:
//   start_time_val - start tick value
//   stop_time_val  - stop tick value
//
// Return: uint32_t - difference in microseconds
//-----------------------------------------------------------------------------------------------------
uint32_t Time_diff_microseconds(uint32_t start_time_val, uint32_t stop_time_val)
{
  return ((stop_time_val - start_time_val) * 1000000ul) / TX_TIMER_TICKS_PER_SECOND;
}

//-----------------------------------------------------------------------------------------------------
// Description:
//   Calculate time difference in milliseconds between two tick values.
//
// Parameters:
//   start_time_val - start tick value
//   stop_time_val  - stop tick value
//
// Return: uint32_t - difference in milliseconds
//-----------------------------------------------------------------------------------------------------
uint32_t Time_diff_miliseconds(uint32_t start_time_val, uint32_t stop_time_val)
{
  return ((stop_time_val - start_time_val) * 1000ul) / TX_TIMER_TICKS_PER_SECOND;
}

//-----------------------------------------------------------------------------------------------------
// Description:
//   Get hardware timestamp (tick and cycle count).
//
// Parameters:
//   pst - pointer to T_sys_timestump structure
//
// Return: void
//-----------------------------------------------------------------------------------------------------
void Get_hw_timestump(T_sys_timestump *pst)
{
  uint32_t scy;
  ULONG    scl1;
  ULONG    scl2;
  scl1 = _tx_timer_system_clock;
  scy  = SysTick->VAL;
  scl2 = _tx_timer_system_clock;
  if (scl1 != scl2)
  {
    pst->cycles = SysTick->VAL;
    pst->ticks  = scl2;
  }
  else
  {
    pst->cycles = scy;
    pst->ticks  = scl1;
  }
}

//-----------------------------------------------------------------------------------------------------
// Description:
//   Calculate difference between two hardware timestamps in microseconds (64-bit).
//
// Parameters:
//   p_begin - pointer to older timestamp
//   p_end   - pointer to newer timestamp
//
// Return: uint64_t - difference in microseconds
//-----------------------------------------------------------------------------------------------------
uint64_t Hw_timestump_diff64_us(T_sys_timestump *p_begin, T_sys_timestump *p_end)
{
  uint64_t val;
  int32_t  tmp;
  int32_t  uc_in_tick;
  uint32_t c1;
  uint32_t c2;
  uint32_t sys_div    = FRQ_CPUCLK_MHZ * 1000000ul / TX_TIMER_TICKS_PER_SECOND;
  uint32_t sys_freq   = FRQ_CPUCLK_MHZ;
  uint32_t ticks_freq = TX_TIMER_TICKS_PER_SECOND;
  c1                  = sys_div - p_begin->cycles - 1;
  c2                  = sys_div - p_end->cycles - 1;
  uc_in_tick          = 1000000ul / ticks_freq;
  val                 = (uint64_t)(p_end->ticks - p_begin->ticks) * uc_in_tick;
  if (c2 >= c1)
  {
    tmp = (c2 - c1) / (sys_freq);
    val += (uint64_t)tmp;
  }
  else
  {
    tmp = (sys_div - (c1 - c2)) / (sys_freq);
    val = val - (uint64_t)uc_in_tick + (uint64_t)tmp;
  }
  return val;
}

//-----------------------------------------------------------------------------------------------------
// Description:
//   Calculate difference between two hardware timestamps in microseconds (32-bit).
//
// Parameters:
//   p_begin - pointer to older timestamp
//   p_end   - pointer to newer timestamp
//
// Return: uint32_t - difference in microseconds
//-----------------------------------------------------------------------------------------------------
uint32_t Hw_timestump_diff32_us(T_sys_timestump *p_begin, T_sys_timestump *p_end)
{
  return (uint32_t)Hw_timestump_diff64_us(p_begin, p_end);
}

//-----------------------------------------------------------------------------------------------------
// Description:
//   Convert milliseconds to system ticks.
//
// Parameters:
//   time_ms - time in milliseconds
//
// Return: uint32_t - number of ticks
//-----------------------------------------------------------------------------------------------------
uint32_t ms_to_ticks(uint32_t time_ms)
{
  return (((time_ms * TX_TIMER_TICKS_PER_SECOND) / 1000U) + 1U);
}

//-----------------------------------------------------------------------------------------------------
// Description:
//   Wait for a specified number of milliseconds.
//
// Parameters:
//   ms - time in milliseconds
//
// Return: uint32_t - result of tx_thread_sleep
//-----------------------------------------------------------------------------------------------------
uint32_t Wait_ms(uint32_t ms)
{
  return tx_thread_sleep(ms_to_ticks(ms));
}

//-----------------------------------------------------------------------------------------------------
// Description:
//   Get elapsed time in seconds since a hardware timestamp.
//
// Parameters:
//   p_time - pointer to timestamp
//
// Return: uint32_t - elapsed seconds
//-----------------------------------------------------------------------------------------------------
uint32_t Time_elapsed_sec(T_sys_timestump *p_time)
{
  uint64_t        secs;
  T_sys_timestump now;
  Get_hw_timestump(&now);
  secs = Hw_timestump_diff64_us(p_time, &now) / 1000000ull;
  return (uint32_t)secs;
}

//-----------------------------------------------------------------------------------------------------
// Description:
//   Get elapsed time in milliseconds since a hardware timestamp.
//
// Parameters:
//   p_time - pointer to timestamp
//
// Return: uint32_t - elapsed milliseconds
//-----------------------------------------------------------------------------------------------------
uint32_t Time_elapsed_msec(T_sys_timestump *p_time)
{
  uint64_t        msecs;
  T_sys_timestump now;
  Get_hw_timestump(&now);
  msecs = Hw_timestump_diff64_us(p_time, &now) / 1000ull;
  return (uint32_t)msecs;
}

//-----------------------------------------------------------------------------------------------------
// Description:
//   Get elapsed time in microseconds since a hardware timestamp.
//
// Parameters:
//   p_time - pointer to timestamp
//
// Return: uint32_t - elapsed microseconds
//-----------------------------------------------------------------------------------------------------
uint32_t Time_elapsed_usec(T_sys_timestump *p_time)
{
  uint64_t        usecs;
  T_sys_timestump now;
  Get_hw_timestump(&now);
  usecs = Hw_timestump_diff64_us(p_time, &now);
  return (uint32_t)usecs;
}

//-----------------------------------------------------------------------------------------------------
// Description:
//   Convert timestamp to seconds and microseconds.
//
// Parameters:
//   p_timestump - pointer to timestamp
//   sec         - pointer to store seconds
//   usec        - pointer to store microseconds
//
// Return: void
//-----------------------------------------------------------------------------------------------------
void Timestump_convert_to_sec_usec(T_sys_timestump *p_timestump, uint32_t *sec, uint32_t *usec)
{
  *sec  = p_timestump->ticks / TX_TIMER_TICKS_PER_SECOND;
  *usec = (p_timestump->ticks % TX_TIMER_TICKS_PER_SECOND) * (1000000ul / TX_TIMER_TICKS_PER_SECOND) + p_timestump->cycles / (FRQ_CPUCLK_MHZ);
}

//-----------------------------------------------------------------------------------------------------
// Description:
//   Calculate difference between two timestamps in microseconds.
//
// Parameters:
//   p_old_time - pointer to older timestamp
//   p_new_time - pointer to newer timestamp
//
// Return: uint32_t - difference in microseconds
//-----------------------------------------------------------------------------------------------------
uint32_t Timestump_diff_to_usec(T_sys_timestump *p_old_time, T_sys_timestump *p_new_time)
{
  uint64_t diff64;
  diff64 = Hw_timestump_diff64_us(p_old_time, p_new_time);
  return (uint32_t)(diff64);
}

//-----------------------------------------------------------------------------------------------------
// Description:
//   Calculate difference between two timestamps in milliseconds.
//
// Parameters:
//   p_old_time - pointer to older timestamp
//   p_new_time - pointer to newer timestamp
//
// Return: uint32_t - difference in milliseconds
//-----------------------------------------------------------------------------------------------------
uint32_t Timestump_diff_to_msec(T_sys_timestump *p_old_time, T_sys_timestump *p_new_time)
{
  uint64_t diff64;
  diff64 = Hw_timestump_diff64_us(p_old_time, p_new_time);
  return (uint32_t)(diff64 / 1000ull);
}

//-----------------------------------------------------------------------------------------------------
// Description:
//   Calculate difference between two timestamps in seconds.
//
// Parameters:
//   p_old_time - pointer to older timestamp
//   p_new_time - pointer to newer timestamp
//
// Return: uint32_t - difference in seconds
//-----------------------------------------------------------------------------------------------------
uint32_t Timestump_diff_to_sec(T_sys_timestump *p_old_time, T_sys_timestump *p_new_time)
{
  uint64_t diff64;
  diff64 = Hw_timestump_diff64_us(p_old_time, p_new_time);
  return (uint32_t)(diff64 / 1000000ull);
}

//-----------------------------------------------------------------------------------------------------
// Description:
//   Convert month name string to month number (0-11).
//
// Parameters:
//   month - pointer to 3-letter month string
//
// Return: int - month number (0-11), or -1 if invalid
//-----------------------------------------------------------------------------------------------------
int month_to_number(const char *month)
{
  const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  for (int i = 0; i < 12; i++)
  {
    if (strcmp(month, months[i]) == 0)
    {
      return i;
    }
  }
  return -1;
}

//-----------------------------------------------------------------------------------------------------
// Description:
//   Parse compile date and time into rtc_time_t structure.
//
// Parameters:
//   time - pointer to rtc_time_t structure
//
// Return: void
//-----------------------------------------------------------------------------------------------------
void Parse_compile_date_time(rtc_time_t *time)
{
  char month_str[4];
  int  day, year, hour, min, sec;
  sscanf(Get_build_date(), "%3s %d %d", month_str, &day, &year);  // Parse date
  time->tm_mon  = month_to_number(month_str);                     // Set month
  time->tm_mday = day;                                            // Set day
  time->tm_year = year - 1900;                                    // Set year
  sscanf(Get_build_time(), "%d:%d:%d", &hour, &min, &sec);        // Parse time
  time->tm_hour = hour;                                           // Set hour
  time->tm_min  = min;                                            // Set minute
  time->tm_sec  = sec;                                            // Set second
  time->tm_wday = 0;                                              // Set weekday (not used)
}

//-----------------------------------------------------------------------------------------------------
// Description:
//   Retrieve system uptime, calculate days/hours/minutes/seconds, and format the string.
//   Omits zero fields for days, hours, or minutes.
//
// Parameters:
//   buffer   - pointer to the buffer for the formatted string
//   buf_size - buffer size
//
// Return: void
//-----------------------------------------------------------------------------------------------------
void Get_and_format_uptime_string(GX_CHAR *buffer, uint32_t buf_size)
{
  uint32_t current_time;                   // System time in ticks
  uint32_t elapsed_seconds;                // Uptime in seconds
  uint32_t days, hours, minutes, seconds;  // Time breakdown
  Get_system_ticks(&current_time);         // Get current system time (uptime)
  elapsed_seconds = current_time / TX_TIMER_TICKS_PER_SECOND;
  days            = elapsed_seconds / (24 * 60 * 60);
  elapsed_seconds %= (24 * 60 * 60);
  hours = elapsed_seconds / (60 * 60);
  elapsed_seconds %= (60 * 60);
  minutes = elapsed_seconds / 60;
  seconds = elapsed_seconds % 60;
  if (days > 0)
  {
    snprintf(buffer, buf_size, "%lu d %lu h %lu m %lu s", days, hours, minutes, seconds);  // Days, hours, minutes, seconds
  }
  else if (hours > 0)
  {
    snprintf(buffer, buf_size, "%lu h %lu m %lu s", hours, minutes, seconds);  // Hours, minutes, seconds
  }
  else if (minutes > 0)
  {
    snprintf(buffer, buf_size, "%lu m %lu s", minutes, seconds);  // Minutes, seconds
  }
  else
  {
    snprintf(buffer, buf_size, "%lu s", seconds);  // Seconds only
  }
}
