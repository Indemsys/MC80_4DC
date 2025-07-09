#include "App.h"

static void     _Can_tx_task_func(ULONG thread_input);
static void     _Can_rx_task_func(ULONG thread_input);
static uint32_t _Process_can_tx_message(T_can_msg* tx_msg);

TX_THREAD    g_can_tx_task;
TX_THREAD    g_can_rx_task;
TX_QUEUE     g_can_tx_queue;
TX_QUEUE     g_can_rx_queue;
TX_SEMAPHORE g_can_tx_complete_semaphore;  // Semaphore for TX completion synchronization
bool         g_can_ready                   = false;

// Global CAN error counters structure
T_can_error_counters g_can_error_counters  = { 0 };

// CAN communication monitoring variables
uint32_t g_can_last_rx_time          = 0;     // Time of last received CAN message
bool     g_can_communication_active  = false; // Flag indicating if CAN communication is active

// Global RX callback function pointer
static T_can_rx_callback g_can_rx_callback = NULL;

static uint8_t g_can_tx_task_stack[CAN_THREAD_STACK_SIZE] BSP_PLACE_IN_SECTION(".stack.CAN_TX_task") BSP_ALIGN_VARIABLE(BSP_STACK_ALIGNMENT);
static uint8_t g_can_rx_task_stack[CAN_RX_THREAD_STACK_SIZE] BSP_PLACE_IN_SECTION(".stack.CAN_RX_task") BSP_ALIGN_VARIABLE(BSP_STACK_ALIGNMENT);
static ULONG   g_can_tx_queue_memory[CAN_TX_QUEUE_DEPTH * CAN_TX_QUEUE_MSG_SIZE];
static ULONG   g_can_rx_queue_memory[CAN_RX_QUEUE_DEPTH * CAN_RX_QUEUE_MSG_SIZE];

// Nominal and Data bit timing configuration
can_bit_timing_cfg_t g_CANFD_bit_timing_cfg = {
  .baud_rate_prescaler        = 1,  // Actual bitrate: 555556 Hz. Actual sample point: 75 %
  .time_segment_1             = 107,
  .time_segment_2             = 36,
  .synchronization_jump_width = 4
};

#if BSP_FEATURE_CANFD_FD_SUPPORT
can_bit_timing_cfg_t g_CANFD_data_timing_cfg = {
  .baud_rate_prescaler        = 1,  // Actual bitrate: 2000000 Hz. Actual sample point: 75 %
  .time_segment_1             = 29,
  .time_segment_2             = 10,
  .synchronization_jump_width = 4
};
#endif

// CAN Acceptance Filter List (AFL) - configured to accept ONLY extended (29-bit) messages
const canfd_afl_entry_t p_CANFD_afl[CANFD_CFG_AFL_CH0_RULE_NUM] = {
  // Rule 0: Accept all Extended ID (29-bit) messages (both data and remote frames)
  {
   .id.id                         = 0x00000000,            // ID value to match (0 = any)
   .id.frame_type                 = CAN_FRAME_TYPE_DATA,   // Data frames (but mask will ignore this)
   .id.id_mode                    = CAN_ID_MODE_EXTENDED,  // Extended ID mode (29-bit) - this is enforced
   .mask.mask_id                  = 0x00000000,            // ID mask (0 = don't check any ID bits)
   .mask.mask_frame_type          = 0,                     // Don't check frame type (accept both data and remote)
   .mask.mask_id_mode             = 1,                     // Check ID mode (only extended)
   .destination.minimum_dlc       = CANFD_MINIMUM_DLC_0,   // Accept any DLC
   .destination.rx_buffer         = CANFD_RX_MB_NONE,      // Not used for FIFO
   .destination.fifo_select_flags = CANFD_RX_FIFO_0,       // Route to RX FIFO 0 (using bit flag, not enum)
  }
  // Only one rule needed - remaining entries (1-15) will be automatically zero-initialized (unused)
  // Standard ID (11-bit) messages will be rejected since they don't match the extended ID mode filter
};

#define CANFD_CFG_COMMONFIFO0 (((0) << R_CANFD_CFDCFCC_CFE_Pos) |     \
                               ((1) << R_CANFD_CFDCFCC_CFRXIE_Pos) |  \
                               ((1) << R_CANFD_CFDCFCC_CFTXIE_Pos) |  \
                               ((0) << R_CANFD_CFDCFCC_CFPLS_Pos) |   \
                               ((0) << R_CANFD_CFDCFCC_CFM_Pos) |     \
                               ((0) << R_CANFD_CFDCFCC_CFITSS_Pos) |  \
                               ((0) << R_CANFD_CFDCFCC_CFITR_Pos) |   \
                               ((0) << R_CANFD_CFDCFCC_CFIM_Pos) |    \
                               ((3U) << R_CANFD_CFDCFCC_CFIGCV_Pos) | \
                               ((0) << R_CANFD_CFDCFCC_CFTML_Pos) |   \
                               ((3) << R_CANFD_CFDCFCC_CFDC_Pos) |    \
                               (0 << R_CANFD_CFDCFCC_CFITT_Pos))

// Buffer RAM used: 320 bytes
canfd_global_cfg_t g_CANFD_global_cfg = {
  .global_interrupts = (R_CANFD_CFDGCTR_DEIE_Msk | R_CANFD_CFDGCTR_MEIE_Msk | R_CANFD_CFDGCTR_CMPOFIE_Msk | 0x3),
  .global_config     = ((R_CANFD_CFDGCFG_TPRI_Msk) | (0) | (BSP_CFG_CANFDCLK_SOURCE == BSP_CLOCKS_SOURCE_CLOCK_MAIN_OSC ? R_CANFD_CFDGCFG_DCS_Msk : 0U) | (0) | ((0) << R_CANFD_CFDGCFG_ITRCP_Pos)),
  .rx_mb_config      = (0 | ((0) << R_CANFD_CFDRMNB_RMPLS_Pos)),
  .global_err_ipl    = CANFD_CFG_GLOBAL_ERR_IPL,
  .rx_fifo_ipl       = CANFD_CFG_RX_FIFO_IPL,
  // RX FIFO Configuration: CANFD Lite has only 2 RX FIFOs (0 and 1)
  .rx_fifo_config    = {
   // FIFO 0: Enabled with interrupts - used for receiving extended ID messages
   ((3U) << R_CANFD_CFDRFCC_RFIGCV_Pos) | ((3) << R_CANFD_CFDRFCC_RFDC_Pos) | ((0) << R_CANFD_CFDRFCC_RFPLS_Pos) | (R_CANFD_CFDRFCC_RFIE_Msk | R_CANFD_CFDRFCC_RFIM_Msk) | (R_CANFD_CFDRFCC_RFE_Msk),
   // FIFO 1: Completely disabled - no configuration needed
   ((0)) },
  .common_fifo_config = { CANFD_CFG_COMMONFIFO0 }
};

canfd_extended_cfg_t g_CANFD_extended_cfg = {
  .p_afl            = p_CANFD_afl,
  .txmb_txi_enable  = ((1ULL << 0) | (1ULL << 1) | (1ULL << 2) | (1ULL << 3) | 0ULL),
  .error_interrupts = (R_CANFD_CFDC_CTR_EWIE_Msk | R_CANFD_CFDC_CTR_EPIE_Msk | R_CANFD_CFDC_CTR_BOEIE_Msk | R_CANFD_CFDC_CTR_BORIE_Msk | R_CANFD_CFDC_CTR_OLIE_Msk | 0U),
#if BSP_FEATURE_CANFD_FD_SUPPORT
  .p_data_timing = &g_CANFD_data_timing_cfg,
#else
  .p_data_timing = NULL,
#endif
  .delay_compensation = (1),
  .p_global_cfg       = &g_CANFD_global_cfg,
};

canfd_instance_ctrl_t g_CANFD_ctrl;

const can_cfg_t g_CANFD_cfg = {
  .channel      = 0,
  .p_bit_timing = &g_CANFD_bit_timing_cfg,
  .p_callback   = CANFD_callback,
  .p_extend     = &g_CANFD_extended_cfg,
  .p_context    = NULL,
  .ipl          = (6),
  .rx_irq       = VECTOR_NUMBER_CAN0_COMFRX,
  .tx_irq       = VECTOR_NUMBER_CAN0_TX,
  .error_irq    = VECTOR_NUMBER_CAN0_CHERR,
};

// Instance structure to use this module
const can_instance_t g_CANFD = {
  .p_ctrl = &g_CANFD_ctrl,
  .p_cfg  = &g_CANFD_cfg,
  .p_api  = &g_canfd_on_canfd
};

/*-----------------------------------------------------------------------------------------------------
  Create CAN TX task

  Parameters:

  Return:
-----------------------------------------------------------------------------------------------------*/
void Can_thread_create(void)
{
  UINT err;

  err = tx_thread_create(
  &g_can_tx_task,
  (CHAR*)"CAN_TX",
  _Can_tx_task_func,
  (ULONG)NULL,
  &g_can_tx_task_stack,
  CAN_THREAD_STACK_SIZE,
  THREAD_PRIORITY_CAN,
  THREAD_PREEMPT_CAN,
  THREAD_TIME_SLICE_CAN,
  TX_AUTO_START);
  if (TX_SUCCESS != err)
  {
    APPLOG("CAN Task: Failed to create CAN TX task, error: %d", err);
  }
}

/*-----------------------------------------------------------------------------------------------------
  Create CAN RX task

  Parameters:

  Return:
-----------------------------------------------------------------------------------------------------*/
void Can_rx_thread_create(void)
{
  UINT err;

  err = tx_thread_create(
  &g_can_rx_task,
  (CHAR*)"CAN_RX",
  _Can_rx_task_func,
  (ULONG)NULL,
  &g_can_rx_task_stack,
  CAN_RX_THREAD_STACK_SIZE,
  THREAD_PRIORITY_CAN_RX,
  THREAD_PREEMPT_CAN_RX,
  THREAD_TIME_SLICE_CAN_RX,
  TX_AUTO_START);
  if (TX_SUCCESS != err)
  {
    APPLOG("CAN Task: Failed to create CAN RX task, error: %d", err);
  }
}

/*-----------------------------------------------------------------------------------------------------
  CAN callback function called by FSP CAN driver on various CAN events.
  This function is invoked from interrupt context when CAN hardware events occur.

  Events that trigger this callback:
  - CAN_EVENT_RX_COMPLETE (0x0400): Message received successfully
  - CAN_EVENT_TX_COMPLETE (0x0800): Message transmitted successfully
  - CAN_EVENT_TX_FIFO_EMPTY (0x2000): Transmit FIFO became empty
  - CAN_EVENT_ERR_WARNING (0x0002): Error warning threshold exceeded
  - CAN_EVENT_ERR_PASSIVE (0x0004): Error passive state entered
  - CAN_EVENT_ERR_BUS_OFF (0x0008): Bus off condition detected
  - CAN_EVENT_BUS_RECOVERY (0x0010): Recovery from bus off state
  - CAN_EVENT_ERR_CHANNEL (0x0100): Channel error occurred
  - CAN_EVENT_ERR_GLOBAL (0x1000): Global CAN error occurred
  - CAN_EVENT_MAILBOX_MESSAGE_LOST (0x0020): Message lost in mailbox
  - CAN_EVENT_FIFO_MESSAGE_LOST (0x4000): Message lost due to FIFO overrun
  - CAN_EVENT_ERR_BUS_LOCK (0x0080): Bus lock detected (32 consecutive dominant bits)
  - CAN_EVENT_TX_ABORTED (0x0200): Transmission was aborted

  Parameters:
  p_args - Pointer to can_callback_args_t structure containing:
           - channel: CAN channel number (0 or 1)
           - event: Event code indicating the type of CAN event
           - error: Error code with additional error information
           - mailbox/buffer: Mailbox or buffer number that triggered the event
           - p_context: User-defined context pointer
           - frame: Received CAN frame data (valid for RX_COMPLETE events)

  Return:
  void
-----------------------------------------------------------------------------------------------------*/
void CANFD_callback(can_callback_args_t* p_args)
{
  T_can_msg rx_msg;
  UINT      queue_status;

  if (p_args == NULL)
  {
    g_can_error_counters.callback_null_pointer++;
    return;
  }
  // Handle received message event
  if (p_args->event == CAN_EVENT_RX_COMPLETE)
  {
    g_can_error_counters.total_rx_messages++;

    // Update last received message time for communication monitoring
    g_can_last_rx_time = tx_time_get();

    // Prepare RX message structure
    rx_msg.can_id     = p_args->frame.id;
    rx_msg.dlc        = p_args->frame.data_length_code;
    rx_msg.frame_type = (uint8_t)p_args->frame.type;
    rx_msg.id_mode    = (uint8_t)p_args->frame.id_mode;
    rx_msg.reserved   = 0;

    // Copy received data using optimized memory function
    memset(rx_msg.data, 0, 8);
    if (rx_msg.dlc > 0)
    {
      memcpy(rx_msg.data, p_args->frame.data, rx_msg.dlc > 8 ? 8 : rx_msg.dlc);
    }

    // Send message to RX queue (non-blocking from interrupt context)
    queue_status = tx_queue_send(&g_can_rx_queue, &rx_msg, TX_NO_WAIT);

    if (queue_status == TX_QUEUE_FULL)
    {
      g_can_error_counters.rx_queue_full_errors++;
    }
    else if (queue_status != TX_SUCCESS)
    {
      g_can_error_counters.rx_queue_receive_errors++;
    }
  }
  // Handle TX completion event
  else if (p_args->event == CAN_EVENT_TX_COMPLETE)
  {
    // Signal that mailbox is available for next transmission
    tx_semaphore_put(&g_can_tx_complete_semaphore);
  }
  // Handle error events and update counters
  else if (p_args->event & CAN_EVENT_ERR_BUS_OFF)
  {
    g_can_error_counters.bus_off_errors++;
  }
  else if (p_args->event & CAN_EVENT_ERR_WARNING)
  {
    g_can_error_counters.error_warning_count++;
  }
  else if (p_args->event & CAN_EVENT_ERR_PASSIVE)
  {
    g_can_error_counters.error_passive_count++;
  }
  else if (p_args->event & CAN_EVENT_ERR_BUS_LOCK)
  {
    g_can_error_counters.bus_lock_errors++;
  }
  else if (p_args->event & CAN_EVENT_TX_ABORTED)
  {
    g_can_error_counters.tx_aborted_count++;
  }
  else if (p_args->event & (CAN_EVENT_MAILBOX_MESSAGE_LOST | CAN_EVENT_FIFO_MESSAGE_LOST))
  {
    g_can_error_counters.message_lost_errors++;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Process CAN transmission message

  Parameters:
    tx_msg - pointer to CAN message structure

  Return:
    0 - success
    error code - failure
-----------------------------------------------------------------------------------------------------*/
static uint32_t _Process_can_tx_message(T_can_msg* tx_msg)
{
  fsp_err_t   err;
  can_frame_t can_frame;
  UINT        semaphore_status;

  if (tx_msg == NULL)
  {
    g_can_error_counters.tx_invalid_param_errors++;
    return 1;  // Invalid parameter
  }
  // Wait for mailbox to be available (with timeout)
  semaphore_status = tx_semaphore_get(&g_can_tx_complete_semaphore, 100);  // 100 ticks timeout
  if (semaphore_status != TX_SUCCESS)
  {
    g_can_error_counters.tx_semaphore_timeout_errors++;
    APPLOG("CAN Task: Timeout waiting for TX mailbox, status: %d", semaphore_status);
    return semaphore_status;
  }

  // Prepare CAN frame structure
  can_frame.id               = tx_msg->can_id;
  can_frame.id_mode          = (can_id_mode_t)tx_msg->id_mode;
  can_frame.type             = (can_frame_type_t)tx_msg->frame_type;
  can_frame.data_length_code = tx_msg->dlc;
  can_frame.options          = 0;  // No special options
  // Clear all data bytes first
  memset(can_frame.data, 0, 8);

  // Copy data from message to CAN frame (only copy valid data)
  if (tx_msg->dlc > 0)
  {
    memcpy(can_frame.data, tx_msg->data, tx_msg->dlc);
  }
  // Try to write the message to CAN
  err = R_CANFD_Write(&g_CANFD_ctrl, 0, &can_frame);  // Using mailbox 0

  if (err != FSP_SUCCESS)
  {
    // Return semaphore if write failed
    tx_semaphore_put(&g_can_tx_complete_semaphore);
    g_can_error_counters.tx_hardware_errors++;
    APPLOG("CAN Task: Failed to write CAN message, error: 0x%x", err);
    return err;
  }

  // Note: semaphore will be released in TX_COMPLETE callback
  g_can_error_counters.total_tx_messages++;
  return 0;
}

/*-----------------------------------------------------------------------------------------------------
  CAN TX task function

  Parameters:
    thread_input - thread input parameter (not used)

  Return:
-----------------------------------------------------------------------------------------------------*/
static void _Can_tx_task_func(ULONG thread_input)
{
  fsp_err_t err;
  UINT      queue_status;
  UINT      status;
  T_can_msg tx_msg;
  // Log CAN initialization start
  APPLOG("CAN Task: Starting CAN interface initialization");

  // Enable CAN transceiver via IO extender
  status = Write_to_IO_extender(CAN_EN, 1);
  if (status != TX_SUCCESS)
  {
    APPLOG("CAN Task: Failed to enable CAN_EN signal, error: %d", status);
    return;  // Exit task on error
  }
  APPLOG("CAN Task: CAN_EN signal enabled successfully");

  // Initialize CAN TX queue
  status = tx_queue_create(&g_can_tx_queue,
                           (CHAR*)"CAN_TX_QUEUE",
                           CAN_TX_QUEUE_MSG_SIZE,
                           g_can_tx_queue_memory,
                           sizeof(g_can_tx_queue_memory));
  if (status != TX_SUCCESS)
  {
    APPLOG("CAN Task: Failed to create CAN TX queue, error: %d", status);
    return;  // Exit task on error
  }

  // Initialize CAN RX queue
  status = tx_queue_create(&g_can_rx_queue,
                           (CHAR*)"CAN_RX_QUEUE",
                           CAN_RX_QUEUE_MSG_SIZE,
                           g_can_rx_queue_memory,
                           sizeof(g_can_rx_queue_memory));
  if (status != TX_SUCCESS)
  {
    APPLOG("CAN Task: Failed to create CAN RX queue, error: %d", status);
    return;  // Exit task on error
  }

  // Initialize CAN TX completion semaphore
  status = tx_semaphore_create(&g_can_tx_complete_semaphore,
                               (CHAR*)"CAN_TX_COMPLETE_SEM",
                               1);  // Start with 1 - mailbox is initially available
  if (status != TX_SUCCESS)
  {
    APPLOG("CAN Task: Failed to create CAN TX completion semaphore, error: %d", status);
    return;  // Exit task on error
  }

  // Open CAN interface
  err = R_CANFD_Open(&g_CANFD_ctrl, &g_CANFD_cfg);
  if (FSP_SUCCESS != err)
  {
    // Log CAN open error with error code
    APPLOG("CAN Task: Failed to open CAN interface, error code: 0x%x", err);
    return;  // Exit task on error
  }
  // Log successful CAN initialization
  APPLOG("CAN Task: CAN interface opened successfully");

  // Create CAN RX task after successful CAN initialization
  Can_rx_thread_create();

  // Set global ready flag
  g_can_ready = true;

  // Main loop - process messages from queue
  while (1)
  {
    // Wait for message in queue with timeout
    queue_status = tx_queue_receive(&g_can_tx_queue, &tx_msg, 100);  // 100 tick timeout

    if (queue_status == TX_SUCCESS)
    {
      // Process the received message
      _Process_can_tx_message(&tx_msg);
    }
    else if (queue_status != TX_QUEUE_EMPTY)
    {
      // Log error if it's not just an empty queue
      APPLOG("CAN Task: Queue receive error: %d", queue_status);
    }
    // If queue is empty, loop continues with next timeout
  }
}

/*-----------------------------------------------------------------------------------------------------
  CAN RX task function

  Parameters:
    thread_input - thread input parameter (not used)

  Return:
-----------------------------------------------------------------------------------------------------*/
static void _Can_rx_task_func(ULONG thread_input)
{
  UINT      queue_status;
  T_can_msg rx_msg;
  uint32_t  message_count = 0;

  APPLOG("CAN Task: CAN RX task started");
  while (1)
  {
    // Wait for message in RX queue with timeout
    queue_status = tx_queue_receive(&g_can_rx_queue, &rx_msg, 50);  // 50 tick timeout

    if (queue_status == TX_SUCCESS)
    {
      // Call user callback if registered
      if (g_can_rx_callback != NULL)
      {
        g_can_rx_callback(&rx_msg);
      }      message_count++;

      // Log message count every 10 messages (for monitoring)
      // if ((message_count % 10) == 0)
      // {
      //   APPLOG("CAN RX: Processed %d messages", message_count);
      // }
    }
    else if (queue_status == TX_QUEUE_EMPTY)
    {
      // Normal timeout - no messages to process
      // Could perform periodic housekeeping here
    }
    else
    {
      // Log error if it's not just an empty queue
      APPLOG("CAN RX: Queue receive error: %d", queue_status);
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Send CAN extended data frame to transmission queue

  Parameters:
    can_id - CAN identifier (29-bit extended ID)
    data - pointer to data bytes (up to 8 bytes for CAN, can be NULL)
    dlc - data length code (0-8)

  Return:
    CAN_SEND_SUCCESS - success
    CAN_SEND_INVALID_PARAM - invalid parameter
    CAN_SEND_QUEUE_FULL - queue full
    CAN_SEND_QUEUE_ERROR - queue send error
    CAN_SEND_NOT_READY - CAN not ready
-----------------------------------------------------------------------------------------------------*/
uint32_t Can_send_extended_data(uint32_t can_id, uint8_t* data, uint8_t dlc)
{
  UINT      queue_status;
  T_can_msg tx_msg;

  // Check if CAN is ready
  if (!g_can_ready)
  {
    g_can_error_counters.tx_not_ready_errors++;
    return CAN_SEND_NOT_READY;
  }

  // Validate parameters
  if (dlc > 8)
  {
    g_can_error_counters.tx_invalid_param_errors++;
    return CAN_SEND_INVALID_PARAM;
  }

  // For extended CAN, ID must be 29 bits max
  if (can_id > 0x1FFFFFFF)
  {
    g_can_error_counters.tx_invalid_param_errors++;
    return CAN_SEND_INVALID_PARAM;
  }

  // Prepare message structure
  tx_msg.can_id     = can_id;
  tx_msg.dlc        = dlc;
  tx_msg.frame_type = CAN_FRAME_TYPE_DATA;
  tx_msg.id_mode    = CAN_ID_MODE_EXTENDED;
  tx_msg.reserved   = 0;

  // Initialize data array to zero
  memset(tx_msg.data, 0, 8);

  // Copy data to message structure if provided
  if (data != NULL && dlc > 0)
  {
    memcpy(tx_msg.data, data, dlc);
  }

  // Send message to queue (non-blocking)
  queue_status = tx_queue_send(&g_can_tx_queue, &tx_msg, TX_NO_WAIT);

  if (queue_status == TX_SUCCESS)
  {
    return CAN_SEND_SUCCESS;
  }
  else if (queue_status == TX_QUEUE_FULL)
  {
    g_can_error_counters.tx_queue_full_errors++;
    APPLOG("CAN Task: TX queue full, message dropped");
    return CAN_SEND_QUEUE_FULL;
  }
  else
  {
    g_can_error_counters.tx_queue_send_errors++;
    APPLOG("CAN Task: Queue send error: %d", queue_status);
    return CAN_SEND_QUEUE_ERROR;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Check if CAN system is ready for message transmission

  Parameters:

  Return:
    true - CAN system is ready
    false - CAN system is not ready
-----------------------------------------------------------------------------------------------------*/
bool Can_is_ready(void)
{
  return g_can_ready;
}

/*-----------------------------------------------------------------------------------------------------
  Reset all CAN error counters to zero

  Parameters:

  Return:
-----------------------------------------------------------------------------------------------------*/
void Can_reset_error_counters(void)
{
  memset(&g_can_error_counters, 0, sizeof(T_can_error_counters));
}

/*-----------------------------------------------------------------------------------------------------
  Get pointer to CAN error counters structure

  Parameters:

  Return:
    Pointer to CAN error counters structure
-----------------------------------------------------------------------------------------------------*/
T_can_error_counters* Can_get_error_counters(void)
{
  return &g_can_error_counters;
}

/*-----------------------------------------------------------------------------------------------------
  Get number of messages currently in RX queue

  Parameters:

  Return:
    Number of messages in RX queue
-----------------------------------------------------------------------------------------------------*/
uint32_t Can_get_rx_queue_count(void)
{
  CHAR*      queue_name;
  ULONG      messages_waiting;
  ULONG      available_space;
  TX_THREAD* first_suspended;
  ULONG      suspended_count;
  TX_QUEUE*  next_queue;

  // Get RX queue information
  UINT status = tx_queue_info_get(&g_can_rx_queue, &queue_name, &messages_waiting,
                                  &available_space, &first_suspended, &suspended_count, &next_queue);

  if (status == TX_SUCCESS)
  {
    return (uint32_t)messages_waiting;
  }

  return 0;  // Return 0 if error
}

/*-----------------------------------------------------------------------------------------------------
  Set user callback function for received CAN messages

  Parameters:
    callback - pointer to callback function (NULL to disable)

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Can_set_rx_callback(T_can_rx_callback callback)
{
  g_can_rx_callback = callback;
}

/*-----------------------------------------------------------------------------------------------------
  Get current user callback function for received CAN messages

  Parameters:
    None

  Return:
    Current callback function pointer (NULL if not set)
-----------------------------------------------------------------------------------------------------*/
T_can_rx_callback Can_get_rx_callback(void)
{
  return g_can_rx_callback;
}

/*-----------------------------------------------------------------------------------------------------
  Check CAN communication timeout and set error flag if no messages received for 0.5 seconds

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Can_check_communication_timeout(void)
{
  uint32_t current_time = tx_time_get();
  uint32_t timeout_ticks = ms_to_ticks(CAN_COMMUNICATION_TIMEOUT_MS);

  // Check if we have received any CAN messages at all
  if (g_can_last_rx_time == 0)
  {
    // No messages received yet - no error flag set
    return;
  }

  // Check if timeout has elapsed since last received message
  if ((current_time - g_can_last_rx_time) >= timeout_ticks)
  {
    // Timeout detected - set CAN bus error flag if not already set
    if (g_can_communication_active)
    {
      App_set_can_bus_error_flag();
      g_can_communication_active = false;
      APPLOG("CAN Communication: Timeout detected - no messages received for %u ms", CAN_COMMUNICATION_TIMEOUT_MS);
    }
  }
  else
  {
    // Messages received within timeout - clear error flag if set
    if (!g_can_communication_active)
    {
      g_can_communication_active = true;
      App_clear_can_bus_error_flag();
      APPLOG("CAN Communication: Resumed after timeout");
    }
  }
}
