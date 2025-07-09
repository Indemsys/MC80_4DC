#include "App.h"
#include "CAN_task.h"

#define CAN_DIAG_HEADER          "================== CAN Diagnostic Menu ==================\r\n"
#define CAN_DIAG_START_LINE      3   // Line where CAN data starts (after header)
#define CAN_DIAG_REFRESH_MS      500
#define CAN_MAX_RECENT_MESSAGES  10  // Maximum number of recent messages to display

// CAN diagnostic menu key definitions
#define CAN_KEY_SEND_TEST        '1'  // Send test message
#define CAN_KEY_CLEAR_RX         '2'  // Clear RX message list
#define CAN_KEY_RESET_COUNTERS   '3'  // Reset error counters
#define CAN_KEY_TOGGLE_AUTO_SEND '4'  // Toggle auto send mode
#define CAN_KEY_INC_CAN_ID       'A'  // Increase CAN ID
#define CAN_KEY_DEC_CAN_ID       'Z'  // Decrease CAN ID
#define CAN_KEY_INC_DLC          'S'  // Increase DLC
#define CAN_KEY_DEC_DLC          'X'  // Decrease DLC
#define CAN_KEY_INC_DATA         'D'  // Increase data byte 0
#define CAN_KEY_DEC_DATA         'C'  // Decrease data byte 0
#define CAN_KEY_EXIT             VT100_ESC

// Short macro for VT100 line clearing
#define CL                       VT100_CLR_LINE

// Macro to combine MPRINTF with line counter increment for cleaner code
#define MPRINTF_LINE(line_var, ...) \
  do                                \
  {                                 \
    MPRINTF(CL __VA_ARGS__);        \
    (line_var)++;                   \
  } while (0)

// Structure to store recent received messages
typedef struct
{
  T_can_msg message;
  uint32_t  timestamp;  // ThreadX tick when received
  bool      valid;      // Message slot is valid
} T_recent_can_msg;

// Static variables for CAN diagnostic
static uint32_t         g_can_test_id        = 0x123;                                               // Test message CAN ID
static uint8_t          g_can_test_dlc       = 8;                                                   // Test message DLC
static uint8_t          g_can_test_data[8]   = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };  // Test data
static bool             g_can_auto_send      = false;                                               // Auto send mode
static uint32_t         g_can_auto_send_last = 0;                                                   // Last auto send tick
static T_recent_can_msg g_recent_messages[CAN_MAX_RECENT_MESSAGES];
static uint8_t          g_recent_msg_index   = 0;                                                   // Next message index to write

// Diagnostic callback management
static T_can_rx_callback g_original_callback = NULL;  // Store original callback when diagnostic is active

// Function prototypes
static uint8_t _Can_diag_print_status(void);
static void    _Can_add_recent_message(const T_can_msg* msg);
static void    _Can_clear_recent_messages(void);
static void    _Can_send_test_message(void);
static void    _Can_handle_auto_send(void);
static void    _Can_diag_rx_callback(const T_can_msg* rx_msg);
static void    _Can_diag_install_callback(void);
static void    _Can_diag_restore_callback(void);

/*-----------------------------------------------------------------------------------------------------
  Display CAN status, error counters, recent messages and controls

  Parameters:
    None

  Return:
    Next available line number for input operations
-----------------------------------------------------------------------------------------------------*/
static uint8_t _Can_diag_print_status(void)
{
  GET_MCBL;
  uint8_t cln                          = CAN_DIAG_START_LINE;

  T_can_error_counters* counters       = Can_get_error_counters();
  bool                  can_ready      = Can_is_ready();
  uint32_t              rx_queue_count = Can_get_rx_queue_count();

  // Position cursor to start line
  MPRINTF(VT100_CURSOR_SET, cln, 1);

  // CAN System Status
  MPRINTF_LINE(cln, "=== CAN System Status ===\r\n");
  MPRINTF_LINE(cln, "CAN Ready:        %s\r\n", can_ready ? "YES" : "NO");
  MPRINTF_LINE(cln, "RX Queue Count:   %u messages\r\n", (unsigned int)rx_queue_count);
  MPRINTF_LINE(cln, "Auto Send:        %s\r\n", g_can_auto_send ? "ENABLED" : "DISABLED");
  MPRINTF_LINE(cln, "\r\n");

  // Error Counters
  MPRINTF_LINE(cln, "=== Error Counters ===\r\n");
  MPRINTF_LINE(cln, "TX Messages:      %u\r\n", (unsigned int)counters->total_tx_messages);
  MPRINTF_LINE(cln, "RX Messages:      %u\r\n", (unsigned int)counters->total_rx_messages);
  MPRINTF_LINE(cln, "TX Queue Full:    %u\r\n", (unsigned int)counters->tx_queue_full_errors);
  MPRINTF_LINE(cln, "RX Queue Full:    %u\r\n", (unsigned int)counters->rx_queue_full_errors);
  MPRINTF_LINE(cln, "TX HW Errors:     %u\r\n", (unsigned int)counters->tx_hardware_errors);
  MPRINTF_LINE(cln, "TX Sem Timeout:   %u\r\n", (unsigned int)counters->tx_semaphore_timeout_errors);
  MPRINTF_LINE(cln, "Bus Off Errors:   %u\r\n", (unsigned int)counters->bus_off_errors);
  MPRINTF_LINE(cln, "Warning Count:    %u\r\n", (unsigned int)counters->error_warning_count);
  MPRINTF_LINE(cln, "Passive Count:    %u\r\n", (unsigned int)counters->error_passive_count);
  MPRINTF_LINE(cln, "\r\n");

  // Test Message Configuration
  MPRINTF_LINE(cln, "=== Test Message Config ===\r\n");
  MPRINTF_LINE(cln, "CAN ID:           0x%08X\r\n", (unsigned int)g_can_test_id);
  MPRINTF_LINE(cln, "DLC:              %u\r\n", g_can_test_dlc);
  MPRINTF_LINE(cln, "Data:             ");
  for (uint8_t i = 0; i < g_can_test_dlc && i < 8; i++)
  {
    MPRINTF("%02X ", g_can_test_data[i]);
  }
  MPRINTF("\r\n");
  cln++;
  MPRINTF_LINE(cln, "\r\n");  // Recent Received Messages
  MPRINTF_LINE(cln, "=== Recent RX Messages ===\r\n");
  MPRINTF_LINE(cln, "Buffer Index: %u | Valid Messages: ", g_recent_msg_index);
  uint8_t valid_count = 0;
  for (uint8_t k = 0; k < CAN_MAX_RECENT_MESSAGES; k++)
  {
    if (g_recent_messages[k].valid) valid_count++;
  }
  MPRINTF("%u\r\n", valid_count);
  cln++;
  MPRINTF_LINE(cln, "Time(ticks) | CAN ID     | DLC | Data\r\n");
  MPRINTF_LINE(cln, "------------|------------|-----|------------------\r\n");
  // Display recent messages (newest first)
  uint8_t displayed = 0;

  for (int8_t i = 0; i < CAN_MAX_RECENT_MESSAGES && displayed < 8; i++)
  {
    // Calculate index going backwards from current position
    int8_t idx = (g_recent_msg_index - 1 - i + CAN_MAX_RECENT_MESSAGES) % CAN_MAX_RECENT_MESSAGES;

    if (g_recent_messages[idx].valid)
    {
      T_recent_can_msg* recent = &g_recent_messages[idx];
      MPRINTF_LINE(cln, "%10u  | 0x%08X | %3u | ", (unsigned int)recent->timestamp, (unsigned int)recent->message.can_id, recent->message.dlc);
      for (uint8_t j = 0; j < recent->message.dlc && j < 8; j++)
      {
        MPRINTF("%02X ", recent->message.data[j]);
      }
      MPRINTF("\r\n");
      displayed++;
    }
  }

  if (displayed == 0)
  {
    MPRINTF_LINE(cln, "  (no messages received yet)\r\n");
  }

  MPRINTF_LINE(cln, "\r\n");

  // Control Help
  MPRINTF_LINE(cln, "=== Controls ===\r\n");
  MPRINTF_LINE(cln, "1 - Send Test Message    | 2 - Clear RX Messages\r\n");
  MPRINTF_LINE(cln, "3 - Reset Error Counters | 4 - Toggle Auto Send\r\n");
  MPRINTF_LINE(cln, "A/Z - CAN ID +/-         | S/X - DLC +/-\r\n");
  MPRINTF_LINE(cln, "D/C - Data[0] +/-        | ESC - Exit\r\n");

  return cln + 1;
}

/*-----------------------------------------------------------------------------------------------------
  Add received message to recent messages buffer

  Parameters:
    msg - pointer to received CAN message

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Can_add_recent_message(const T_can_msg* msg)
{
  if (msg == NULL)
  {
    return;
  }

  // Copy message to circular buffer
  g_recent_messages[g_recent_msg_index].message   = *msg;
  g_recent_messages[g_recent_msg_index].timestamp = tx_time_get();
  g_recent_messages[g_recent_msg_index].valid     = true;

  // Move to next position
  g_recent_msg_index                              = (g_recent_msg_index + 1) % CAN_MAX_RECENT_MESSAGES;
}

/*-----------------------------------------------------------------------------------------------------
  Clear all recent messages

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Can_clear_recent_messages(void)
{
  memset(g_recent_messages, 0, sizeof(g_recent_messages));
  g_recent_msg_index = 0;
}

/*-----------------------------------------------------------------------------------------------------
  Send test CAN message

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Can_send_test_message(void)
{
  uint32_t result = Can_send_extended_data(g_can_test_id, g_can_test_data, g_can_test_dlc);
  // Test message send result is not critical for diagnostics
  (void)result;
}

/*-----------------------------------------------------------------------------------------------------
  Handle auto send functionality

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Can_handle_auto_send(void)
{
  if (!g_can_auto_send)
  {
    return;
  }

  uint32_t current_time = tx_time_get();

  // Send message every 1 second (1000 ticks)
  if ((current_time - g_can_auto_send_last) >= 1000)
  {
    _Can_send_test_message();
    g_can_auto_send_last = current_time;

    // Increment test data to make messages unique
    g_can_test_data[0]++;
    if (g_can_test_data[0] == 0)
    {
      g_can_test_data[1]++;
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Diagnostic RX callback function - called when CAN message is received

  Parameters:
    rx_msg - pointer to received CAN message

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Can_diag_rx_callback(const T_can_msg* rx_msg)
{
  if (rx_msg == NULL)
  {
    return;
  }

  // Log received message in diagnostic context
  APPLOG("CAN RX: ID=0x%08X, DLC=%d, Type=%s, Mode=%s",
         rx_msg->can_id,
         rx_msg->dlc,
         (rx_msg->frame_type == CAN_FRAME_TYPE_DATA) ? "DATA" : "REMOTE",
         (rx_msg->id_mode == CAN_ID_MODE_EXTENDED) ? "EXT" : "STD");

  // Add message to diagnostic buffer
  _Can_add_recent_message(rx_msg);

  // Call original callback if it exists (to maintain normal operation)
  if (g_original_callback != NULL)
  {
    g_original_callback(rx_msg);
  }
}

/*-----------------------------------------------------------------------------------------------------
  Install diagnostic callback to capture RX messages

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Can_diag_install_callback(void)
{
  // Save current callback
  g_original_callback = Can_get_rx_callback();

  // Install our diagnostic callback
  Can_set_rx_callback(_Can_diag_rx_callback);
}

/*-----------------------------------------------------------------------------------------------------
  Restore original callback when exiting diagnostic mode

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Can_diag_restore_callback(void)
{
  // Restore original callback
  Can_set_rx_callback(g_original_callback);
  g_original_callback = NULL;
}

/*-----------------------------------------------------------------------------------------------------
  CAN Diagnostic menu entry point

  Parameters:
    keycode - Not used

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Diagnostic_CAN(uint8_t keycode)
{
  GET_MCBL;

  // Check if CAN is ready
  if (!Can_is_ready())
  {
    MPRINTF("\r\n[ERROR] CAN system is not ready or initialization failed.\r\n");
    MPRINTF("\r\nReturning to previous menu in 2 seconds...\r\n");
    tx_thread_sleep(ms_to_ticks(2000));
    return;
  }  // Initialize diagnostic state
  g_can_auto_send      = false;
  g_can_auto_send_last = tx_time_get();
  // Don't clear recent messages to show previously received packets
  // _Can_clear_recent_messages();

  // Display header once at menu entry
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF(CAN_DIAG_HEADER);

  uint8_t key = 0;
  (void)_Can_diag_print_status();  // Initial display

  // Install diagnostic callback
  _Can_diag_install_callback();
  while (1)
  {
    // Handle auto send
    _Can_handle_auto_send();

    if (VT100_wait_special_key(&key, ms_to_ticks(CAN_DIAG_REFRESH_MS)) == RES_OK)
    {
      switch (key)
      {
        case CAN_KEY_SEND_TEST:
          _Can_send_test_message();
          break;

        case CAN_KEY_CLEAR_RX:
          _Can_clear_recent_messages();
          break;
        case CAN_KEY_RESET_COUNTERS:
          Can_reset_error_counters();
          break;
        case CAN_KEY_TOGGLE_AUTO_SEND:
          g_can_auto_send      = !g_can_auto_send;
          g_can_auto_send_last = tx_time_get();
          break;
        case 'a':  // Accept lowercase
        case CAN_KEY_INC_CAN_ID:
          if (g_can_test_id < 0x1FFFFFFF)
          {
            g_can_test_id++;
          }
          break;
        case 'z':  // Accept lowercase
        case CAN_KEY_DEC_CAN_ID:
          if (g_can_test_id > 0)
          {
            g_can_test_id--;
          }
          break;
        case 's':  // Accept lowercase
        case CAN_KEY_INC_DLC:
          if (g_can_test_dlc < 8)
          {
            g_can_test_dlc++;
          }
          break;
        case 'x':  // Accept lowercase
        case CAN_KEY_DEC_DLC:
          if (g_can_test_dlc > 0)
          {
            g_can_test_dlc--;
          }
          break;
        case 'd':  // Accept lowercase
        case CAN_KEY_INC_DATA:
          g_can_test_data[0]++;
          break;
        case 'c':  // Accept lowercase
        case CAN_KEY_DEC_DATA:
          g_can_test_data[0]--;
          break;
        case CAN_KEY_EXIT:
          g_can_auto_send = false;  // Stop auto send
          // Restore original callback before exiting
          _Can_diag_restore_callback();
          return;

        default:
          break;
      }

      (void)_Can_diag_print_status();  // Refresh display after action
    }
    else
    {
      (void)_Can_diag_print_status();  // Refresh display on timeout
    }
  }
}
