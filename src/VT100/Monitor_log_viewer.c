#include "App.h"

// Macros for log viewer configuration
#define LOG_VIEWER_LINES_PER_SCREEN 32  // Number of lines displayed on screen
#define LOG_SCREEN_WIDTH            80  // Screen width in characters

// Macros for row and column positions
#define LOG_TITLE_ROW               1                                                      // Row for log title
#define LOG_INSTR_ROW               2                                                      // Row for instructions
#define LOG_INSTR_COL               2                                                      // Offset for instructions
#define LOG_SEP_LINE_ROW            3                                                      // Row for separator line
#define LOG_CONTENT_START_ROW       4                                                      // Starting row for log content
#define LOG_STATUS_ROW              (LOG_CONTENT_START_ROW + LOG_VIEWER_LINES_PER_SCREEN)  // Row for status information

// Global variable for storing current log identifier
static uint32_t current_log_id = APP_LOG_ID;

// Structure for storing log viewer state
typedef struct
{
  uint32_t total_records;      // Total number of records in log
  uint32_t view_position;      // Current view position (index of topmost displayed line)
  bool     at_end_mode;        // Mode for viewing latest records
  uint32_t last_known_record;  // Last known record at last update
} T_log_viewer_state;

static T_log_viewer_state log_state;

/*-----------------------------------------------------------------------------------------------------
  Format delta_time (in microseconds) to "DDD d HH h MM m SS s USECUS us" format

  Parameters:
    delta_time - time in microseconds
    buffer     - buffer for formatted string
    buf_size   - buffer size

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void Format_delta_time(uint64_t delta_time, char *buffer, size_t buf_size)
{
  uint32_t t32;
  uint32_t time_usec = delta_time % 1000000ull;
  t32                = (uint32_t)(delta_time / 1000000ull);
  uint32_t time_sec  = t32 % 60;
  uint32_t time_min  = (t32 / 60) % 60;
  uint32_t time_hour = (t32 / (60 * 60)) % 24;
  uint32_t time_day  = t32 / (60 * 60 * 24);

  snprintf(buffer, buf_size, "%03d d %02d h %02d m %02d s %06d us",
           time_day, time_hour, time_min, time_sec, time_usec);
}

/*-----------------------------------------------------------------------------------------------------
  Display current log state on screen

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void Display_log_screen(void)
{
  GET_MCBL;
  uint32_t         i;
  uint32_t         start_pos;
  uint32_t         records_to_show;
  uint32_t         new_records;
  T_logger_record *p_log_rec;
  uint32_t         log_capacity;
  uint32_t         overflow_cnt;
  char             time_str[40];  // Increased buffer size for new time format
  T_log_cbl       *p_log;
  const char      *log_title = (current_log_id == APP_LOG_ID) ? "APP LOG VIEWER" : "NET LOG VIEWER";

  MPRINTF(VT100_CLEAR_AND_HOME);
  // Compact header starting from the first column
  VT100_send_str_to_pos((uint8_t *)log_title, LOG_TITLE_ROW, 0);
  // Display instruction string for viewer control
  VT100_send_str_to_pos((uint8_t *)"[UP/DOWN] - Scroll, [E] - Auto-update, [D] - Switch Log, [ESC/R] - Exit", LOG_INSTR_ROW, LOG_INSTR_COL);
  VT100_send_str_to_pos(DASH_LINE, LOG_SEP_LINE_ROW, 0);                                    // Get log information using Get_log_cbl with identifier
  p_log                  = Get_log_cbl(current_log_id);
  log_capacity           = p_log->log_capacity;
  overflow_cnt           = (p_log->log_file_opened != 0) ? p_log->file_log_overfl_err : 0;  // Show only file write errors

  // Check if circular buffer has overflowed
  bool buffer_overflowed = (p_log->log_overfl_f || (p_log->head_indx == p_log->tail_indx && p_log->entries_count > 0));

  // Determine total records count considering circular buffer overflow
  if (buffer_overflowed)
  {
    // Circular buffer has overflowed, we have full capacity of records
    log_state.total_records = log_capacity;
  }
  else
  {
    // No overflow yet, use head_indx as record count
    log_state.total_records = p_log->head_indx;
  }

  // Account for file log overflow as well
  if ((p_log->log_file_opened != 0) && (p_log->file_log_overfl_err > 0))
  {
    log_state.total_records = log_capacity;
  }

  if (log_state.total_records == 0)
  {
    // No records in log
    VT100_send_str_to_pos((uint8_t *)"Log is empty. No records found.", LOG_CONTENT_START_ROW, 0);
    return;
  }
  // Determine initial position for display
  if (log_state.at_end_mode)
  {
    // In latest records viewing mode
    if (log_state.total_records <= LOG_VIEWER_LINES_PER_SCREEN)
    {
      start_pos       = 0;
      records_to_show = log_state.total_records;
    }
    else
    {
      start_pos       = log_state.total_records - LOG_VIEWER_LINES_PER_SCREEN;
      records_to_show = LOG_VIEWER_LINES_PER_SCREEN;
    }
    log_state.view_position = start_pos;
  }
  else
  {
    // In scroll mode
    start_pos = log_state.view_position;
    if (start_pos + LOG_VIEWER_LINES_PER_SCREEN <= log_state.total_records)
    {
      records_to_show = LOG_VIEWER_LINES_PER_SCREEN;
    }
    else
    {
      records_to_show = log_state.total_records - start_pos;
    }
  }

  // Display log records
  uint32_t display_row = LOG_CONTENT_START_ROW;  // Starting row for record output

  // Check if there are new records since last update
  if (!log_state.at_end_mode && log_state.last_known_record < log_state.total_records)
  {  // Show existing records
    for (i = 0; i < records_to_show; i++)
    {
      uint32_t log_idx;

      if (buffer_overflowed)
      {
        // For circular buffer overflow, calculate index properly
        // Latest records are before head_indx, oldest after head_indx
        if (start_pos + i < log_capacity)
        {
          log_idx = (p_log->head_indx - log_capacity + start_pos + i + log_capacity) % log_capacity;
        }
        else
        {
          log_idx = (start_pos + i) % log_capacity;
        }
      }
      else
      {
        log_idx = start_pos + i;
      }
      p_log_rec = &p_log->log_records[log_idx];

      // Format timestamp from delta_time
      Format_delta_time(p_log_rec->delta_time, time_str, sizeof(time_str));

      // Output log record with new time format
      MPRINTF("%s | %s\r\n", time_str, p_log_rec->msg);
      display_row++;
    }

    // Output separator line
    MPRINTF("\r\n");
    for (i = 0; i < LOG_SCREEN_WIDTH; i++)
    {
      MPRINTF("-");
    }
    MPRINTF("\r\n");
    display_row += 2;

    // Output new records
    new_records = log_state.total_records - log_state.last_known_record;
    for (i = 0; i < new_records && i < 5 && display_row < LOG_STATUS_ROW; i++)
    {  // Show no more than 5 new records
      uint32_t log_idx;

      if (buffer_overflowed)
      {
        // For circular buffer overflow, calculate index for new records
        log_idx = (p_log->head_indx - new_records + i + log_capacity) % log_capacity;
      }
      else
      {
        log_idx = log_state.last_known_record + i;
      }

      p_log_rec = &p_log->log_records[log_idx];

      // Format timestamp from delta_time
      Format_delta_time(p_log_rec->delta_time, time_str, sizeof(time_str));

      // Output log record with new time format
      MPRINTF("%s | %s\r\n", time_str, p_log_rec->msg);
      display_row++;
    }

    if (new_records > 5)
    {
      MPRINTF("\r\n... and %d more new records (press [E] to view)\r\n", new_records - 5);
    }
  }
  else
  {  // Show records in normal mode
    for (i = 0; i < records_to_show && display_row < LOG_STATUS_ROW; i++)
    {
      uint32_t log_idx;

      if (buffer_overflowed)
      {
        // For circular buffer overflow, calculate index properly
        // Latest records are before head_indx, oldest after head_indx
        if (start_pos + i < log_capacity)
        {
          log_idx = (p_log->head_indx - log_capacity + start_pos + i + log_capacity) % log_capacity;
        }
        else
        {
          log_idx = (start_pos + i) % log_capacity;
        }
      }
      else
      {
        log_idx = start_pos + i;
      }

      p_log_rec = &p_log->log_records[log_idx];

      // Format timestamp from delta_time
      Format_delta_time(p_log_rec->delta_time, time_str, sizeof(time_str));

      // Output log record with new time format
      MPRINTF("%s | %s\r\n", time_str, p_log_rec->msg);
      display_row++;
    }
  }

  // Update last known record
  log_state.last_known_record = log_state.total_records;

  // Output current view position information
  VT100_send_str_to_pos(DASH_LINE, LOG_STATUS_ROW, 0);
  if (log_state.at_end_mode)
  {
    MPRINTF("\r\nShowing latest records (%d-%d of %d)",
            start_pos + 1,
            start_pos + records_to_show,
            log_state.total_records);
    if (overflow_cnt > 0)
    {
      MPRINTF(" | File write errors: %d", overflow_cnt);
    }
    MPRINTF("\r\n");
  }
  else
  {
    MPRINTF("\r\nRecords %d-%d of %d (Scroll mode)",
            start_pos + 1,
            start_pos + records_to_show,
            log_state.total_records);

    if (overflow_cnt > 0)
    {
      MPRINTF(" | File write errors: %d", overflow_cnt);
    }
    MPRINTF("\r\n");
  }
}

/*-----------------------------------------------------------------------------------------------------
  Event log viewer function

  Parameters:
    keycode - input key code (not used)

  Return:
    void
-----------------------------------------------------------------------------------------------------*/
void Do_show_event_log(uint8_t keycode)
{
  GET_MCBL;
  uint8_t    b;
  uint32_t   update_counter     = 0;
  uint32_t   prev_records_count = 0;
  uint32_t   prev_head_indx     = 0;
  uint32_t   prev_overflow_err  = 0;
  T_log_cbl *p_log;
  // Initialize log viewer state
  log_state.at_end_mode       = true;  // Start with latest records viewing mode
  log_state.view_position     = 0;
  log_state.last_known_record = 0;
  // Determine initial record count
  p_log                       = Get_log_cbl(current_log_id);
  prev_head_indx              = p_log->head_indx;
  prev_overflow_err           = (p_log->log_file_opened != 0) ? p_log->file_log_overfl_err : 0;

  // Determine initial records count considering circular buffer overflow
  if (p_log->log_overfl_f || (p_log->head_indx == p_log->tail_indx && p_log->entries_count > 0))
  {
    // Circular buffer has overflowed, we have full capacity of records
    prev_records_count = p_log->log_capacity;
  }
  else
  {
    // No overflow yet, use head_indx as record count
    prev_records_count = p_log->head_indx;
  }

  // Account for file log overflow as well
  if ((p_log->log_file_opened != 0) && (p_log->file_log_overfl_err > 0))
  {
    prev_records_count = p_log->log_capacity;
  }
  // Display initial log state
  Display_log_screen();

  while (1)
  {
    // Check for key presses, use VT100_wait_special_key to handle ESC sequences
    if (VT100_wait_special_key(&b, 200) == RES_OK)
    {
      switch (b)
      {
        case VT100_ESC:
        case 'R':
        case 'r':
          // Exit from viewer
          return;

        case 'E':
        case 'e':
          // Switch to latest records viewing mode
          log_state.at_end_mode = true;
          Display_log_screen();
          break;

        case 'D':
        case 'd':  // Switch between logs
          current_log_id        = (current_log_id == APP_LOG_ID) ? NET_LOG_ID : APP_LOG_ID;
          log_state.at_end_mode = true;
          Display_log_screen();
          break;

        case VT100_UP_ARROW:  // Scroll up by screen
          if (log_state.view_position > 0)
          {
            if (log_state.view_position >= LOG_VIEWER_LINES_PER_SCREEN)
            {
              log_state.view_position -= LOG_VIEWER_LINES_PER_SCREEN;
            }
            else
            {
              log_state.view_position = 0;
            }
            log_state.at_end_mode = false;
            Display_log_screen();
          }
          break;

        case VT100_DOWN_ARROW:  // Scroll down by screen
          if (log_state.view_position + LOG_VIEWER_LINES_PER_SCREEN < log_state.total_records)
          {
            log_state.view_position += LOG_VIEWER_LINES_PER_SCREEN;
            if (log_state.view_position + LOG_VIEWER_LINES_PER_SCREEN > log_state.total_records)
            {
              log_state.view_position = (log_state.total_records > LOG_VIEWER_LINES_PER_SCREEN) ? (log_state.total_records - LOG_VIEWER_LINES_PER_SCREEN) : 0;
            }
            Display_log_screen();
          }
          else
          {  // If end of log is reached, enable latest records viewing mode
            log_state.at_end_mode = true;
            Display_log_screen();
          }
          break;
      }
    }

    // Periodic update to display new records
    update_counter++;
    if (update_counter >= 1)
    {  // Check every cycle (200ms timeout)
      p_log = Get_log_cbl(current_log_id);
      uint32_t current_records;
      // When overflow occurs, track changes in head_indx and file overflow counter
      bool     has_new_records       = false;
      uint32_t current_file_overflow = (p_log->log_file_opened != 0) ? p_log->file_log_overfl_err : 0;

      // Check if circular buffer has overflowed
      bool buffer_overflowed         = (p_log->log_overfl_f || (p_log->head_indx == p_log->tail_indx && p_log->entries_count > 0));

      // Determine current records count considering circular buffer overflow
      if (buffer_overflowed)
      {
        // Circular buffer has overflowed, we have full capacity of records
        current_records = p_log->log_capacity;
        // When buffer is overflowed, detect new records by head_indx change
        if (p_log->head_indx != prev_head_indx)
        {
          has_new_records = true;
        }
      }
      else
      {
        // No overflow yet, use head_indx as record count
        current_records = p_log->head_indx;
        // When buffer is not overflowed, detect new records by count change
        has_new_records = (current_records != prev_records_count);
      }

      // Account for file log overflow as well
      if (current_file_overflow > 0)
      {
        current_records = p_log->log_capacity;
        // Check for head_indx change or increase in file write overflow counter
        if ((p_log->head_indx != prev_head_indx) || (current_file_overflow != prev_overflow_err))
        {
          has_new_records = true;
        }
      }

      // Update tracking variables
      prev_head_indx    = p_log->head_indx;
      prev_overflow_err = current_file_overflow;  // If there are new records
      if (has_new_records)
      {
        prev_records_count = current_records;

        // In latest records viewing mode, update entire screen
        if (log_state.at_end_mode)
        {
          Display_log_screen();
        }
        else
        {
          // In scroll mode, update only status line
          // and set flag that new records are available
          log_state.total_records = current_records;
          VT100_send_str_to_pos(DASH_LINE, LOG_STATUS_ROW, 0);
          MPRINTF("\r\nRecords %d-%d of %d (Scroll mode) - New records available (press [E])\r\n",
                  log_state.view_position + 1,
                  log_state.view_position +
                  (log_state.view_position + LOG_VIEWER_LINES_PER_SCREEN <= log_state.total_records ? LOG_VIEWER_LINES_PER_SCREEN : log_state.total_records - log_state.view_position),
                  log_state.total_records);
        }
      }

      update_counter = 0;
    }
  }
}
