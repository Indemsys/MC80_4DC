#include "App.h"

#define NUM_OF_LOGS 2

TX_EVENT_FLAGS_GROUP file_logger_flags;
T_log_cbl            app_log_cbl;
T_log_cbl            net_log_cbl;

T_log_cbl  *log_cbls[NUM_OF_LOGS]              = { &app_log_cbl, &net_log_cbl };
uint32_t    log_file_reset_events[NUM_OF_LOGS] = { EVT_RESET_APP_FILE_LOG, EVT_RESET_NET_FILE_LOG };
const char *log_file_names[NUM_OF_LOGS]        = { APP_LOG_FILE_PATH, NET_LOG_FILE_PATH };
const char *log_file_prev_names[NUM_OF_LOGS]   = { APP_LOG_PREV_FILE_PATH, NET_LOG_PREV_FILE_PATH };

#ifdef LOG_TO_ONBOARD_SDRAM
T_logger_record app_log[APP_LOG_CAPACITY] @ ".sdram";
T_logger_record net_log[NET_LOG_CAPACITY] @ ".sdram";
#else
T_logger_record app_log[APP_LOG_CAPACITY];
T_logger_record net_log[NET_LOG_CAPACITY];
#endif

#define TIME_DELAY_BEFORE_SAVE       100  // Time in ms before remaining records are saved
#define LOG_RECS_BEFORE_SAVE_TO_FILE 20   // Number of records that triggers immediate save

char        file_log_str[LOG_STR_MAX_SZ];
static char rtt_log_str[RTT_LOG_STR_SZ];
static void Log_write(T_log_cbl *log_cbl_ptr, char *str, const char *func_name, unsigned int line_num, unsigned int severity);

/*-----------------------------------------------------------------------------------------------------
  Initialize log control block

  Parameters:
    log_cbl_ptr - pointer to log control block
    log_capacity - capacity of log buffer
    records_arr - pointer to records array
    name - name of the log

  Return:
    RES_OK on success, RES_ERROR on failure
-----------------------------------------------------------------------------------------------------*/
static uint32_t Log_init(T_log_cbl *log_cbl_ptr, uint32_t log_capacity, T_logger_record *records_arr, const char *name)
{
  if (tx_mutex_create(&log_cbl_ptr->log_mutex, (char *)name, TX_INHERIT) != TX_SUCCESS)
  {
    return RES_ERROR;
  }
  Get_hw_timestump(&log_cbl_ptr->log_start_time);
  log_cbl_ptr->name         = name;
  log_cbl_ptr->log_records  = records_arr;
  log_cbl_ptr->log_capacity = log_capacity;
  log_cbl_ptr->head_indx    = 0;
  log_cbl_ptr->tail_indx    = 0;
  log_cbl_ptr->logger_ready = 1;
  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Initialize logger subsystem

  Parameters:
    void

  Return:
    RES_OK on success
-----------------------------------------------------------------------------------------------------*/
uint32_t Logger_init(void)
{
  wvar.enable_log = 1;
  Log_init(&app_log_cbl, APP_LOG_CAPACITY, app_log, "App log");
  Log_init(&net_log_cbl, NET_LOG_CAPACITY, net_log, "Net log");
  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Disable application log

  Parameters:
    void

  Return:
    void
-----------------------------------------------------------------------------------------------------*/
void App_log_disable(void)
{
  app_log_cbl.logger_ready = 0;
  net_log_cbl.logger_ready = 0;
}

/*-----------------------------------------------------------------------------------------------------
  Write log message to RTT channel

  Parameters:
    fmt_ptr - format string pointer
    ... - variable arguments

  Return:
    void
-----------------------------------------------------------------------------------------------------*/
void RTT_LOGs(const char *fmt_ptr, ...)
{
  unsigned int n;
  va_list      ap;
  va_start(ap, fmt_ptr);
  __disable_interrupt();
  n = vsnprintf(rtt_log_str, RTT_LOG_STR_SZ, (const char *)fmt_ptr, ap);
  SEGGER_RTT_Write(RTT_LOG_CH, rtt_log_str, n);
  __enable_interrupt();
  va_end(ap);
}

/*-----------------------------------------------------------------------------------------------------
  Write log message to application log

  Parameters:
    name - function name
    line_num - line number
    severity - message severity level
    fmt_ptr - format string pointer
    ... - variable arguments

  Return:
    void
-----------------------------------------------------------------------------------------------------*/
void LOGs(const char *name, unsigned int line_num, unsigned int severity, const char *fmt_ptr, ...)
{
  char    log_str[LOG_STR_MAX_SZ + 1];
  va_list ap;
  va_start(ap, fmt_ptr);
  vsnprintf(log_str, LOG_STR_MAX_SZ, (const char *)fmt_ptr, ap);
  Log_write(&app_log_cbl, log_str, name, line_num, severity);
  va_end(ap);
}

/*-----------------------------------------------------------------------------------------------------
  Write extended log message to application log

  Parameters:
    name - function name
    line_num - line number
    severity - message severity level
    fmt_ptr - format string pointer
    ... - variable arguments

  Return:
    void
-----------------------------------------------------------------------------------------------------*/
void ELOGs(const char *name, unsigned int line_num, unsigned int severity, const char *fmt_ptr, ...)
{
  char log_str[LOG_STR_MAX_SZ + 1];
  // if (wvar.verbose_log == 0) return;  // If flag disables call, exit
  va_list ap;
  va_start(ap, fmt_ptr);
  vsnprintf(log_str, LOG_STR_MAX_SZ, (const char *)fmt_ptr, ap);
  Log_write(&app_log_cbl, log_str, name, line_num, severity);
  va_end(ap);
}

/*-----------------------------------------------------------------------------------------------------
  Write log message to network log

  Parameters:
    name - function name
    line_num - line number
    severity - message severity level
    fmt_ptr - format string pointer
    ... - variable arguments

  Return:
    void
-----------------------------------------------------------------------------------------------------*/
void Net_LOGs(const char *name, unsigned int line_num, unsigned int severity, const char *fmt_ptr, ...)
{
  char    log_str[LOG_STR_MAX_SZ + 1];
  va_list ap;
  // if (wvar.en_net_log == 0) return;
  va_start(ap, fmt_ptr);
  vsnprintf(log_str, LOG_STR_MAX_SZ, (const char *)fmt_ptr, ap);
  Log_write(&net_log_cbl, log_str, name, line_num, severity);
  va_end(ap);
}

/*-----------------------------------------------------------------------------------------------------
  Get log control block by ID

  Parameters:
    log_id - log identifier

  Return:
    Pointer to log control block or NULL if invalid ID
-----------------------------------------------------------------------------------------------------*/
T_log_cbl *Get_log_cbl(uint32_t log_id)
{
  if (log_id >= NUM_OF_LOGS) return NULL;
  return log_cbls[log_id];
}

/*-----------------------------------------------------------------------------------------------------
  Request to reset application log file

  Parameters:
    void

  Return:
    void
-----------------------------------------------------------------------------------------------------*/
void Req_to_reset_log_file(void)
{
  Set_file_logger_event(EVT_RESET_APP_FILE_LOG);
}

/*-----------------------------------------------------------------------------------------------------
  Request to reset network log file

  Parameters:
    void

  Return:
    void
-----------------------------------------------------------------------------------------------------*/
void Req_to_reset_netlog_file(void)
{
  Set_file_logger_event(EVT_RESET_NET_FILE_LOG);
}

/*-----------------------------------------------------------------------------------------------------
  Write message to log table

  Parameters:
    log_cbl_ptr - pointer to log control block
    str - message string
    func_name - function name
    line_num - line number
    severity - message severity level

  Return:
    void
-----------------------------------------------------------------------------------------------------*/
static void Log_write(T_log_cbl *log_cbl_ptr, char *str, const char *func_name, unsigned int line_num, unsigned int severity)
{
  int             head;
  int             tail;
  T_sys_timestump ntime;
  rtc_time_t      date_time;

  if ((log_cbl_ptr->logger_ready == 1) && (wvar.enable_log != 0))
  {
    // This function can be called from interrupt handlers,
    // so mutexes and other synchronization services cannot be used here
    if (tx_mutex_get(&log_cbl_ptr->log_mutex, MS_TO_TICKS(LOGGER_WR_TIMEOUT_MS)) == TX_SUCCESS)
    {
      Get_hw_timestump(&ntime);
      RTC_get_system_DateTime(&date_time);
      date_time.tm_mon++;
      date_time.tm_year += 1900;

      head                                      = log_cbl_ptr->head_indx;
      // Calculate time in microseconds from log start
      log_cbl_ptr->log_records[head].date_time  = date_time;
      log_cbl_ptr->log_records[head].delta_time = Hw_timestump_diff64_us(&log_cbl_ptr->log_start_time, &ntime);
      strncpy(log_cbl_ptr->log_records[head].msg, str, LOG_STR_MAX_SZ - 1);
      strncpy(log_cbl_ptr->log_records[head].func_name, func_name, EVNT_LOG_FNAME_SZ - 1);
      log_cbl_ptr->log_records[head].line_num = line_num;
      log_cbl_ptr->log_records[head].severity = severity;
      // Move head pointer forward
      head++;
      if (head >= log_cbl_ptr->log_capacity) head = 0;
      log_cbl_ptr->head_indx = head;
      tail                   = log_cbl_ptr->tail_indx;
      // If head reaches tail, move tail pointer and lose unread data
      if (head == tail)
      {
        tail++;
        if (tail >= log_cbl_ptr->log_capacity) tail = 0;
        log_cbl_ptr->tail_indx    = tail;
        log_cbl_ptr->log_overfl_f = 1;
      }
      else
      {
        log_cbl_ptr->entries_count++;
      }

      // If head reaches file tail, move file tail pointer and lose unwritten data
      tail = log_cbl_ptr->file_tail_indx;
      if (head == tail)
      {
        tail++;
        if (tail >= log_cbl_ptr->log_capacity) tail = 0;
        log_cbl_ptr->file_tail_indx    = tail;
        log_cbl_ptr->file_log_overfl_f = 1;
        log_cbl_ptr->file_log_overfl_err++;
      }
      else
      {
        log_cbl_ptr->file_entries_count++;
      }
      tx_mutex_put(&log_cbl_ptr->log_mutex);
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Get formatted log string for FreeMaster

  Parameters:
    log_cbl_ptr - pointer to log control block
    str - output string buffer
    max_str_len - maximum string length

  Return:
    RES_OK on success, RES_ERROR on failure
-----------------------------------------------------------------------------------------------------*/
static uint32_t Log_get_FreeMaster_string(T_log_cbl *log_cbl_ptr, char *str, uint32_t max_str_len)
{
  uint32_t         tail;
  uint64_t         t64;
  uint32_t         t32;
  uint32_t         time_msec;
  uint32_t         time_sec;
  uint32_t         time_min;
  uint32_t         time_hour;
  uint32_t         time_day;
  T_logger_record *log_rec_ptr;

  if (log_cbl_ptr->entries_count == 0) return RES_ERROR;

  if (tx_mutex_get(&log_cbl_ptr->log_mutex, 10) != TX_SUCCESS) return RES_ERROR;

  tail        = log_cbl_ptr->tail_indx;

  log_rec_ptr = &log_cbl_ptr->log_records[tail];

  t64         = log_rec_ptr->delta_time;
  time_msec   = t64 % 1000000ull;
  t32         = (uint32_t)(t64 / 1000000ull);
  time_sec    = t32 % 60;
  time_min    = (t32 / 60) % 60;
  time_hour   = (t32 / (60 * 60)) % 24;
  time_day    = t32 / (60 * 60 * 24);

  snprintf(str, max_str_len, "%03d d %02d h %02d m %02d s %06d us |", time_day, time_hour, time_min, time_sec, time_msec);
  uint32_t len = strlen(str);

  if (log_rec_ptr->line_num != 0)
  {
    snprintf(&str[len], max_str_len - len, " %s (%s %d)\n\r", log_rec_ptr->msg, log_rec_ptr->func_name, log_rec_ptr->line_num);
  }
  else
  {
    snprintf(&str[len], max_str_len - len, " %s\n\r", log_rec_ptr->msg);
  }

  tail++;
  if (tail >= log_cbl_ptr->log_capacity) tail = 0;
  log_cbl_ptr->tail_indx = tail;
  log_cbl_ptr->entries_count--;

  tx_mutex_put(&log_cbl_ptr->log_mutex);
  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Get application log string for FreeMaster

  Parameters:
    str - output string buffer
    max_str_len - maximum string length

  Return:
    Result of log string retrieval
-----------------------------------------------------------------------------------------------------*/
uint32_t FreeMaster_get_app_log_string(char *str, uint32_t max_str_len)
{
  return Log_get_FreeMaster_string(&app_log_cbl, str, max_str_len);
}

/*-----------------------------------------------------------------------------------------------------
  Clear log buffer and reset counters

  Parameters:
    log_cbl_ptr - pointer to log control block

  Return:
    void
-----------------------------------------------------------------------------------------------------*/
void Log_clear(T_log_cbl *log_cbl_ptr)
{
  log_cbl_ptr->head_indx           = 0;
  log_cbl_ptr->tail_indx           = 0;
  log_cbl_ptr->entries_count       = 0;
  log_cbl_ptr->log_miss_err        = 0;
  log_cbl_ptr->log_overfl_err      = 0;
  log_cbl_ptr->file_log_overfl_err = 0;
}

/*-----------------------------------------------------------------------------------------------------
  Set file logger event flags

  Parameters:
    events_mask - event mask to set

  Return:
    void
-----------------------------------------------------------------------------------------------------*/
void Set_file_logger_event(uint32_t events_mask)
{
  tx_event_flags_set(&file_logger_flags, events_mask, TX_OR);
}

/*-----------------------------------------------------------------------------------------------------
  Open log file for writing

  Parameters:
    indx - log file index

  Return:
    void
-----------------------------------------------------------------------------------------------------*/
static void LogFile_Open(uint32_t indx)
{
  uint32_t res;
  log_cbls[indx]->t_prev = tx_time_get();
  // Open file for log writing
  res                    = fx_file_create(&fat_fs_media, (char *)log_file_names[indx]);

  if ((res == FX_SUCCESS) || (res == FX_ALREADY_CREATED))
  {
    // If file already existed, we can report that
    if (res == FX_ALREADY_CREATED)
    {
      FX_FILE temp_file;
      ULONG   file_size = 0;

      // Get file size by temporarily opening it
      if (fx_file_open(&fat_fs_media, &temp_file, (char *)log_file_names[indx], FX_OPEN_FOR_READ) == FX_SUCCESS)
      {
        file_size = temp_file.fx_file_current_file_size;
        fx_file_close(&temp_file);
        APPLOG("Log: file %s size: %lu bytes", log_file_names[indx], file_size);
      }
    }
    else
    {
      APPLOG("Log: file %s created successfully", log_file_names[indx]);
    }

    // Open file for writing
    res = fx_file_open(&fat_fs_media, &log_cbls[indx]->log_file, (char *)log_file_names[indx], FX_OPEN_FOR_WRITE);
    if (res == FX_SUCCESS)
    {
      log_cbls[indx]->log_file_opened = 1;
      APPLOG("Log: file %s opened successfully for writing", log_file_names[indx]);
    }
    else
    {
      APPLOG("Log: failed to open file %s for writing, error: %u", log_file_names[indx], res);
    }
  }
  else
  {
    APPLOG("Log: failed to create file %s, error: %u", log_file_names[indx], res);
  }
}

/*-----------------------------------------------------------------------------------------------------
  Reset log file (delete and recreate)

  Parameters:
    indx - log file index

  Return:
    void
-----------------------------------------------------------------------------------------------------*/
static void LogFile_Reset(uint32_t indx)
{
  uint8_t    flag        = 0;
  T_log_cbl *log_cbl_ptr = log_cbls[indx];

  if (log_cbl_ptr->log_file_opened == 0) return;

  if (fx_file_close(&log_cbl_ptr->log_file) == FX_SUCCESS)
  {
    if (fx_file_delete(&fat_fs_media, (char *)log_file_names[indx]) == FX_SUCCESS)
    {
      if (fx_file_create(&fat_fs_media, (char *)log_file_names[indx]) == FX_SUCCESS)
      {
        if (fx_file_open(&fat_fs_media, &log_cbl_ptr->log_file, (char *)log_file_names[indx], FX_OPEN_FOR_WRITE) == FX_SUCCESS)
        {
          flag = 1;
          EAPPLOG("Log file %s successfully reset.", log_file_names[indx]);
        }
      }
    }
  }
  if (flag == 0)
  {
    log_cbl_ptr->log_file_opened = 0;
    EAPPLOG("Error resetting log file %s.", log_file_names[indx]);
    return;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Save log records to file

  Parameters:
    indx - log file index

  Return:
    void
-----------------------------------------------------------------------------------------------------*/
static void LogFile_SaveRecords(uint32_t indx)
{
  int32_t tail;
  int32_t n;
  int32_t str_len;

  T_log_cbl *log_cbl_ptr = log_cbls[indx];

  // Check if file logging is enabled
  if (wvar.en_log_to_file == 0) return;

  if (log_cbl_ptr->log_file_opened == 0) return;
  n                  = log_cbl_ptr->file_entries_count;

  // Write if number of records exceeded threshold or timeout elapsed with unsaved records
  log_cbl_ptr->t_now = tx_time_get();
  if ((n > LOG_RECS_BEFORE_SAVE_TO_FILE) || ((n > 0) && ((log_cbl_ptr->t_now - log_cbl_ptr->t_prev) > ms_to_ticks(TIME_DELAY_BEFORE_SAVE))))
  {
    for (uint32_t i = 0; i < n; i++)
    {
      // Save records to file
      if (log_cbl_ptr->file_log_overfl_f != 0)
      {
        log_cbl_ptr->file_log_overfl_f = 0;
        str_len                        = snprintf(file_log_str, LOG_STR_MAX_SZ, "... Overflow ...\r\n");
        fx_file_write(&log_cbl_ptr->log_file, file_log_str, str_len);
      }
      if (log_cbl_ptr->log_miss_f != 0)
      {
        log_cbl_ptr->log_miss_f = 0;
        str_len                 = snprintf(file_log_str, LOG_STR_MAX_SZ, "... Missed records ....\r\n");
        fx_file_write(&log_cbl_ptr->log_file, file_log_str, str_len);
      }

      if (tx_mutex_get(&log_cbl_ptr->log_mutex, MS_TO_TICKS(100)) != TX_SUCCESS) return;

      str_len        = 0;
      tail           = log_cbl_ptr->file_tail_indx;

      rtc_time_t *pt = &log_cbl_ptr->log_records[tail].date_time;
      str_len += snprintf(&file_log_str[str_len], LOG_STR_MAX_SZ, "%04d.%02d.%02d %02d:%02d:%02d |", pt->tm_year, pt->tm_mon, pt->tm_mday, pt->tm_hour, pt->tm_min, pt->tm_sec);

      uint64_t t64 = log_cbl_ptr->log_records[tail].delta_time;
      uint32_t t32;
      uint32_t time_msec = t64 % 1000000ull;
      t32                = (uint32_t)(t64 / 1000000ull);
      uint32_t time_sec  = t32 % 60;
      uint32_t time_min  = (t32 / 60) % 60;
      uint32_t time_hour = (t32 / (60 * 60)) % 24;
      uint32_t time_day  = t32 / (60 * 60 * 24);

      str_len += snprintf(&file_log_str[str_len], LOG_STR_MAX_SZ - str_len, "%03d d %02d h %02d m %02d s %06d us |", time_day, time_hour, time_min, time_sec, time_msec);
      str_len += snprintf(&file_log_str[str_len], LOG_STR_MAX_SZ - str_len, "%02d | %-36s | %5d |", log_cbl_ptr->log_records[tail].severity, log_cbl_ptr->log_records[tail].func_name, log_cbl_ptr->log_records[tail].line_num);
      str_len += snprintf(&file_log_str[str_len], LOG_STR_MAX_SZ - str_len, " %s\r\n", log_cbl_ptr->log_records[tail].msg);

      log_cbl_ptr->file_tail_indx++;
      if (log_cbl_ptr->file_tail_indx >= log_cbl_ptr->log_capacity) log_cbl_ptr->file_tail_indx = 0;
      log_cbl_ptr->file_entries_count--;

      tx_mutex_put(&log_cbl_ptr->log_mutex);

      fx_file_write(&log_cbl_ptr->log_file, file_log_str, str_len);

      if (log_cbl_ptr->log_file.fx_file_current_file_size > MAX_LOG_FILE_SIZE)
      {
        log_cbl_ptr->log_file_opened = 0;
        if (fx_file_close(&log_cbl_ptr->log_file) == FX_SUCCESS)
        {
          fx_file_delete(&fat_fs_media, (char *)log_file_prev_names[indx]);
          if (fx_file_rename(&fat_fs_media, (char *)log_file_names[indx], (char *)log_file_prev_names[indx]) == FX_SUCCESS)
          {
            if (fx_file_create(&fat_fs_media, (char *)log_file_names[indx]) == FX_SUCCESS)
            {
              if (fx_file_open(&fat_fs_media, &log_cbl_ptr->log_file, (char *)log_file_names[indx], FX_OPEN_FOR_WRITE) == FX_SUCCESS)
              {
                log_cbl_ptr->log_file_opened = 1;
              }
            }
          }
        }
      }
      fx_media_flush(&fat_fs_media);  // Clear write cache
      log_cbl_ptr->t_prev = log_cbl_ptr->t_now;
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Save motion buffer to file (weak function)

  Parameters:
    buf_indx - buffer index

  Return:
    0 (default implementation)
-----------------------------------------------------------------------------------------------------*/
__weak uint32_t Save_motion_buffer(uint32_t buf_indx)
{
  return 0;
}

/*-----------------------------------------------------------------------------------------------------
  Check if issue counter has changed (weak function)

  Parameters:
    void

  Return:
    0 (default implementation)
-----------------------------------------------------------------------------------------------------*/
__weak uint8_t Is_issue_counter_changed(void)
{
  return 0;
}

/*-----------------------------------------------------------------------------------------------------
  Save issue counters (weak function)

  Parameters:
    void

  Return:
    0 (default implementation)
-----------------------------------------------------------------------------------------------------*/
__weak uint32_t Save_issue_counters(void)
{
  return 0;
}

/*-----------------------------------------------------------------------------------------------------
  Logger task main function

  Parameters:
    arg - task argument (not used)

  Return:
    void
-----------------------------------------------------------------------------------------------------*/
void Logger_Task(ULONG arg)
{
  ULONG           flags;
  T_sys_timestump ts;

  UINT status = tx_event_flags_create(&file_logger_flags, "Logger");
  if (status != TX_SUCCESS)
  {
    return;
  }
  Get_hw_timestump(&ts);

  // Open all log files
  for (uint32_t i = 0; i < NUM_OF_LOGS; i++) LogFile_Open(i);

  // Log file writing loop
  do
  {
    if (tx_event_flags_get(&file_logger_flags, 0xFFFFFFFF, TX_OR_CLEAR, &flags, MS_TO_TICKS(10)) == TX_SUCCESS)
    {
      if (flags & EVT_MOTION_BUF_READY_FIRST_HALF)
      {
        Save_motion_buffer(0);
      }
      else if (flags & EVT_MOTION_BUF_READY_SECOND_HALF)
      {
        Save_motion_buffer(1);
      }

      for (uint32_t i = 0; i < NUM_OF_LOGS; i++)
      {
        if (log_file_reset_events[i] & flags)
        {
          LogFile_Reset(i);
        }
      }
    }

    for (uint32_t i = 0; i < NUM_OF_LOGS; i++) LogFile_SaveRecords(i);

    if (Time_elapsed_msec(&ts) > 1000)
    {
      // Execute this save no more than once per second
      if (Is_issue_counter_changed()) Save_issue_counters();
      Get_hw_timestump(&ts);
    }

  } while (1);
}

/*-----------------------------------------------------------------------------------------------------
  Get tail record from application log

  Parameters:
    rec - pointer to logger record structure to fill

  Return:
    RES_OK if record retrieved, RES_ERROR if no records available
-----------------------------------------------------------------------------------------------------*/
int32_t AppLog_get_tail_record(T_logger_record *rec)
{
  if (app_log_cbl.entries_count == 0) return RES_ERROR;

  if (tx_mutex_get(&app_log_cbl.log_mutex, 10) != TX_SUCCESS) return RES_ERROR;

  // Copy record from tail position
  *rec = app_log_cbl.log_records[app_log_cbl.tail_indx];

  // Move tail forward
  app_log_cbl.tail_indx++;
  if (app_log_cbl.tail_indx >= app_log_cbl.log_capacity)
  {
    app_log_cbl.tail_indx = 0;
  }
  app_log_cbl.entries_count--;

  tx_mutex_put(&app_log_cbl.log_mutex);
  return RES_OK;
}
