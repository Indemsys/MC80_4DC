/*-----------------------------------------------------------------------------------------------------
  Description: RTT utility functions for formatted output
               Provides sprintf-based formatting for SEGGER RTT

  Parameters:

  Return:
-----------------------------------------------------------------------------------------------------*/

#include "App.h"
#include "RTT_utils.h"
#include <stdio.h>
#include <stdarg.h>

// Local buffer for formatting
static char g_rtt_format_buffer[RTT_FORMAT_BUFFER_SIZE];

/*-----------------------------------------------------------------------------------------------------
  Description: Universal RTT printf function using sprintf for formatting

  Parameters: channel - RTT channel number
              format - printf-style format string
              ... - variable arguments

  Return: number of characters written, or negative on error
-----------------------------------------------------------------------------------------------------*/
int RTT_printf(unsigned int channel, const char* format, ...)
{
  va_list args;
  int len;

  va_start(args, format);
  len = vsprintf(g_rtt_format_buffer, format, args);
  va_end(args);

  if (len > 0 && len < RTT_FORMAT_BUFFER_SIZE)
  {
    return SEGGER_RTT_Write(channel, g_rtt_format_buffer, len);
  }

  return -1; // Error: buffer overflow or formatting error
}

/*-----------------------------------------------------------------------------------------------------
  Description: Universal RTT error printf function with [ERR] prefix

  Parameters: channel - RTT channel number
              format - printf-style format string
              ... - variable arguments

  Return: number of characters written, or negative on error
-----------------------------------------------------------------------------------------------------*/
int RTT_err_printf(unsigned int channel, const char* format, ...)
{
  va_list args;
  int len;

  // Add [ERR] prefix
  len = sprintf(g_rtt_format_buffer, "[ERR] ");
  if (len < 0 || len >= RTT_FORMAT_BUFFER_SIZE)
  {
    return -1;
  }

  va_start(args, format);
  len += vsprintf(g_rtt_format_buffer + len, format, args);
  va_end(args);

  if (len > 0 && len < RTT_FORMAT_BUFFER_SIZE)
  {
    return SEGGER_RTT_Write(channel, g_rtt_format_buffer, len);
  }

  return -1; // Error: buffer overflow or formatting error
}
