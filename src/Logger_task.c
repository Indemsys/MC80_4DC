#include "App.h"

TX_THREAD      Logger_thread;
static uint8_t Logger_thread_stack[LOGGER_THREAD_STACK_SIZE] BSP_PLACE_IN_SECTION(".stack.Logger_thread") BSP_ALIGN_VARIABLE(BSP_STACK_ALIGNMENT);

static void _Logger_thread_func(ULONG thread_input);

/*-----------------------------------------------------------------------------------------------------
  Creates and initializes Logger thread

  Parameters:

  Return:
-----------------------------------------------------------------------------------------------------*/
void Logger_thread_create(void)
{  // Check if file logging is enabled
  if (wvar.en_log_to_file == 0)
  {
    APPLOG("Logger thread not created - file logging disabled");
    Logger_delete_log_files();  // Delete log files when logging is disabled
    return;  // Do not create logger thread if file logging is disabled
  }

  tx_thread_create(
  &Logger_thread,
  (CHAR *)"Logger",
  _Logger_thread_func,
  (ULONG)NULL,
  &Logger_thread_stack,
  LOGGER_THREAD_STACK_SIZE,
  THREAD_PRIORITY_LOGGER,
  THREAD_PREEMPT_LOGGER,
  THREAD_TIME_SLICE_LOGGER,
  TX_AUTO_START);
  APPLOG("Logger thread created successfully - file logging enabled");
}

/*-----------------------------------------------------------------------------------------------------
  Delete all log files when file logging is disabled

  Parameters:

  Return:
-----------------------------------------------------------------------------------------------------*/
void Logger_delete_log_files(void)
{
  UINT res;

  // Delete application log file
  res = fx_file_delete(&fat_fs_media, APP_LOG_FILE_PATH);
  if (res == FX_SUCCESS)
  {
    APPLOG("Application log file deleted successfully");
  }
  else if (res != FX_NOT_FOUND)
  {
    APPLOG("Failed to delete application log file, error: %u", res);
  }

  // Delete application previous log file
  res = fx_file_delete(&fat_fs_media, APP_LOG_PREV_FILE_PATH);
  if (res == FX_SUCCESS)
  {
    APPLOG("Previous application log file deleted successfully");
  }
  else if (res != FX_NOT_FOUND)
  {
    APPLOG("Failed to delete previous application log file, error: %u", res);
  }

  // Delete network log file
  res = fx_file_delete(&fat_fs_media, NET_LOG_FILE_PATH);
  if (res == FX_SUCCESS)
  {
    APPLOG("Network log file deleted successfully");
  }
  else if (res != FX_NOT_FOUND)
  {
    APPLOG("Failed to delete network log file, error: %u", res);
  }

  // Delete network previous log file
  res = fx_file_delete(&fat_fs_media, NET_LOG_PREV_FILE_PATH);
  if (res == FX_SUCCESS)
  {
    APPLOG("Previous network log file deleted successfully");
  }
  else if (res != FX_NOT_FOUND)
  {
    APPLOG("Failed to delete previous network log file, error: %u", res);
  }
}

/*-----------------------------------------------------------------------------------------------------
  Logger thread function

  Parameters:
    thread_input - Thread input parameter

  Return:
-----------------------------------------------------------------------------------------------------*/
static void _Logger_thread_func(ULONG thread_input)
{
  Logger_Task(0);
}
