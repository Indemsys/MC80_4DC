#include "App.h"

#define TMC6200_MONITORING_CALL_DELAY_MS 250
#define TMC6200_COMMAND_QUEUE_SIZE       8

// TMC6200 command types
#define TMC6200_CMD_RESET_FAULTS         1

// TMC6200 command structure
typedef struct
{
  uint8_t cmd_type;    // Command type
  uint8_t driver_num;  // Driver number (1-2, or 0 for both)
  uint8_t padding[2];  // Padding for alignment
} T_tmc6200_command;

// TMC6200 monitoring thread declarations
static TX_THREAD g_tmc6200_monitoring_thread;
static uint8_t   g_tmc6200_monitoring_thread_stack[TMC6200_MONITORING_THREAD_STACK_SIZE] BSP_PLACE_IN_SECTION(".stack.TMC6200_monitoring") BSP_ALIGN_VARIABLE(BSP_STACK_ALIGNMENT);

// TMC6200 command queue
static TX_QUEUE          g_tmc6200_command_queue;
static T_tmc6200_command g_tmc6200_command_queue_buffer[TMC6200_COMMAND_QUEUE_SIZE];

// TMC6200 fault reset event flags
static TX_EVENT_FLAGS_GROUP g_tmc6200_events;
#define TMC6200_EVENT_FAULT_RESET_COMPLETED 0x00000001

// Internal function declarations
static void _Tmc6200_monitoring_thread_entry(ULONG thread_input);
static void _Monitor_tmc6200_drivers(void);
static void _Update_driver_monitoring_status(uint8_t driver_num, uint32_t gstat_value, uint32_t comm_status);
static void _Analyze_gstat_errors(uint8_t driver_num, uint32_t gstat_value);
static void _Process_tmc6200_commands(void);

/*-----------------------------------------------------------------------------------------------------
  Monitor TMC6200 drivers for errors and status changes.
  This function polls both TMC6200 drivers according to the configured interval
  and updates their monitoring status.

  Parameters:
    none

  Return:
    none
-----------------------------------------------------------------------------------------------------*/
static void _Monitor_tmc6200_drivers(void)
{
  uint32_t current_time = tx_time_get();

  // Check if monitoring is active and if it's time to poll
  if (!g_tmc6200_monitoring.monitoring_active ||
      (current_time - g_tmc6200_monitoring.last_monitor_time) < ms_to_ticks(g_tmc6200_monitoring.poll_interval_ms))
  {
    return;
  }

  g_tmc6200_monitoring.last_monitor_time = current_time;

  // Poll both drivers
  for (uint8_t driver_num = 1; driver_num <= 2; driver_num++)
  {
    uint32_t gstat_value = 0;
    uint32_t status      = Motdrv_tmc6200_ReadRegister(driver_num, TMC6200_REG_GSTAT, &gstat_value);

    // Update monitoring status
    _Update_driver_monitoring_status(driver_num, gstat_value, status);

    g_tmc6200_monitoring.driver[driver_num - 1].last_poll_time = current_time;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Update driver monitoring status based on GSTAT register and communication status.

  Parameters:
    driver_num   - driver number (1 or 2)
    gstat_value  - value read from GSTAT register
    comm_status  - communication status (RES_OK or error code)

  Return:
    none
-----------------------------------------------------------------------------------------------------*/
static void _Update_driver_monitoring_status(uint8_t driver_num, uint32_t gstat_value, uint32_t comm_status)
{
  if (driver_num < 1 || driver_num > 2)
  {
    return;
  }

  T_tmc6200_driver_status *driver = &g_tmc6200_monitoring.driver[driver_num - 1];

  // Update communication status
  if (comm_status != RES_OK)
  {
    driver->communication_failures++;
    driver->driver_operational = false;
    // Set TMC6200 driver fault flag when communication error is detected
    App_set_tmc6200_driver_fault_flag(driver_num);
  }

  // Update GSTAT register value and analyze errors
  driver->last_gstat_value = gstat_value;
  _Analyze_gstat_errors(driver_num, gstat_value);
}

/*-----------------------------------------------------------------------------------------------------
  Analyze GSTAT register errors and update driver status flags.

  Parameters:
    driver_num  - driver number (1 or 2)
    gstat_value - value from GSTAT register

  Return:
    none
-----------------------------------------------------------------------------------------------------*/
static void _Analyze_gstat_errors(uint8_t driver_num, uint32_t gstat_value)
{
  if (driver_num < 1 || driver_num > 2)
  {
    return;
  }

  T_tmc6200_driver_status *driver = &g_tmc6200_monitoring.driver[driver_num - 1];

  // Check if there are any errors in GSTAT register
  if (gstat_value != 0)
  {
    // Update error count and store GSTAT value in error history
    driver->error_count++;
    driver->last_gstat_error                           = (uint8_t)(gstat_value & 0xFF);

    // Store error in circular buffer
    driver->error_history[driver->error_history_index] = gstat_value;
    driver->error_history_index                        = (driver->error_history_index + 1) % TMC6200_ERROR_HISTORY_SIZE;

    // Set driver as not operational if serious errors detected
    if (gstat_value & (1 << 6))  // overtemperature error
    {
      driver->driver_operational = false;
    }
    // Set TMC6200 driver fault flag when any hardware error is detected
    App_set_tmc6200_driver_fault_flag(driver_num);
  }
  else
  {
    // No errors in GSTAT, clear last error
    driver->last_gstat_error = 0;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Initialize TMC6200 driver monitoring system.
  This function initializes the monitoring structure and sets default values.

  Parameters:
    none

  Return:
    none
-----------------------------------------------------------------------------------------------------*/
void Motdrv_tmc6200_InitMonitoring(void)
{
  // Initialize monitoring structure
  for (uint8_t i = 0; i < 2; i++)
  {
    g_tmc6200_monitoring.driver[i].driver_num             = i + 1;
    g_tmc6200_monitoring.driver[i].init_error_code        = TMC6200_ERROR_NONE;
    g_tmc6200_monitoring.driver[i].last_gstat_error       = 0;
    g_tmc6200_monitoring.driver[i].last_gstat_value       = 0;
    g_tmc6200_monitoring.driver[i].error_count            = 0;
    g_tmc6200_monitoring.driver[i].communication_failures = 0;
    g_tmc6200_monitoring.driver[i].last_poll_time         = 0;
    g_tmc6200_monitoring.driver[i].driver_operational     = false;
    g_tmc6200_monitoring.driver[i].error_history_index    = 0;

    // Clear error history
    for (uint8_t j = 0; j < TMC6200_ERROR_HISTORY_SIZE; j++)
    {
      g_tmc6200_monitoring.driver[i].error_history[j] = 0;
    }
  }

  g_tmc6200_monitoring.poll_interval_ms  = TMC6200_MONITORING_CALL_DELAY_MS;
  g_tmc6200_monitoring.last_monitor_time = 0;
  g_tmc6200_monitoring.monitoring_active = true;
}

/*-----------------------------------------------------------------------------------------------------
  Update TMC6200 monitoring structure with initialization error codes after driver initialization.
  This function should be called after both drivers have been initialized.

  Parameters:
    none

  Return:
    none
-----------------------------------------------------------------------------------------------------*/
void Motdrv_tmc6200_UpdateInitErrorCodes(void)
{
  // Store initialization error codes after driver initialization
  g_tmc6200_monitoring.driver[0].init_error_code    = g_tmc6200_driver1_error_code;
  g_tmc6200_monitoring.driver[1].init_error_code    = g_tmc6200_driver2_error_code;

  // Set operational status based on initialization results
  g_tmc6200_monitoring.driver[0].driver_operational = (g_tmc6200_driver1_error_code == TMC6200_ERROR_NONE);
  g_tmc6200_monitoring.driver[1].driver_operational = (g_tmc6200_driver2_error_code == TMC6200_ERROR_NONE);

  APPLOG("TMC6200 monitoring: Initialization error codes updated - Driver1: %s, Driver2: %s",
         Motdrv_tmc6200_GetErrorString(g_tmc6200_driver1_error_code),
         Motdrv_tmc6200_GetErrorString(g_tmc6200_driver2_error_code));
}

/*-----------------------------------------------------------------------------------------------------
  Process TMC6200 commands from the command queue

  Parameters:
    none

  Return:
    none
-----------------------------------------------------------------------------------------------------*/
static void _Process_tmc6200_commands(void)
{
  T_tmc6200_command command;
  UINT              status;

  // Check if there are any commands in the queue
  status = tx_queue_receive(&g_tmc6200_command_queue, &command, TX_NO_WAIT);
  if (status != TX_SUCCESS)
  {
    return;  // No commands available
  }

  // Process the command based on type
  switch (command.cmd_type)
  {
    case TMC6200_CMD_RESET_FAULTS:
      if (command.driver_num == 0)  // Reset both drivers
      {
        // Reset driver 1
        uint32_t result1 = Motdrv_tmc6200_Initialize(1);
        if (result1 == RES_OK)
        {
          APPLOG("TMC6200 Driver 1: Faults reset successfully");
          // Re-enable driver after successful reset
          Motor_driver_enable_set(1, 1);
        }
        else
        {
          APPLOG("TMC6200 Driver 1: Reset failed - %s", Motdrv_tmc6200_GetErrorString(g_tmc6200_driver1_error_code));
        }

        // Reset driver 2
        uint32_t result2 = Motdrv_tmc6200_Initialize(2);
        if (result2 == RES_OK)
        {
          APPLOG("TMC6200 Driver 2: Faults reset successfully");
          // Re-enable driver after successful reset
          Motor_driver_enable_set(2, 1);
        }
        else
        {
          APPLOG("TMC6200 Driver 2: Reset failed - %s", Motdrv_tmc6200_GetErrorString(g_tmc6200_driver2_error_code));
        }
      }
      else if (command.driver_num >= 1 && command.driver_num <= 2)  // Reset specific driver
      {
        uint32_t result = Motdrv_tmc6200_Initialize(command.driver_num);
        if (result == RES_OK)
        {
          APPLOG("TMC6200 Driver %d: Faults reset successfully", command.driver_num);
          // Re-enable driver after successful reset
          Motor_driver_enable_set(command.driver_num, 1);
        }
        else
        {
          uint32_t error_code = (command.driver_num == 1) ? g_tmc6200_driver1_error_code : g_tmc6200_driver2_error_code;
          APPLOG("TMC6200 Driver %d: Reset failed - %s", command.driver_num, Motdrv_tmc6200_GetErrorString(error_code));
        }
      }

      // Set event regardless of success/failure
      tx_event_flags_set(&g_tmc6200_events, TMC6200_EVENT_FAULT_RESET_COMPLETED, TX_OR);
      break;

    default:
      APPLOG("TMC6200 monitoring: Unknown command type %d", command.cmd_type);
      break;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Request TMC6200 fault reset via command queue.
  This function posts a command to the monitoring thread to safely reset TMC6200 faults.

  Parameters:
    driver_num - driver number to reset (1 or 2), or 0 to reset both drivers

  Return:
    uint32_t - RES_OK if command posted successfully, error code otherwise
-----------------------------------------------------------------------------------------------------*/
uint32_t Tmc6200_request_fault_reset(uint8_t driver_num)
{
  T_tmc6200_command command;
  UINT              status;

  // Validate driver number
  if (driver_num > 2)
  {
    APPLOG("TMC6200: Invalid driver number %d for fault reset", driver_num);
    return RES_ERROR;
  }

  // Prepare command
  command.cmd_type   = TMC6200_CMD_RESET_FAULTS;
  command.driver_num = driver_num;
  command.padding[0] = 0;
  command.padding[1] = 0;

  // Clear event before sending command
  tx_event_flags_set(&g_tmc6200_events, ~TMC6200_EVENT_FAULT_RESET_COMPLETED, TX_AND);

  // Post command to queue
  status             = tx_queue_send(&g_tmc6200_command_queue, &command, TX_NO_WAIT);
  if (status != TX_SUCCESS)
  {
    APPLOG("TMC6200: Failed to post fault reset command to queue. Error=%d", status);
    return RES_ERROR;
  }

  APPLOG("TMC6200: Fault reset command posted for driver %s",
         (driver_num == 0) ? "both" : ((driver_num == 1) ? "1" : "2"));
  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Creates TMC6200 monitoring thread and initializes command queue

  Parameters:
    none

  Return:
    none
-----------------------------------------------------------------------------------------------------*/
void Tmc6200_monitoring_thread_create(void)
{
  UINT status;

  // Create event flags group first
  status = tx_event_flags_create(&g_tmc6200_events, "TMC6200 Events");
  if (status != TX_SUCCESS)
  {
    APPLOG("TMC6200 monitoring: Failed to create event flags group. Error=%d", status);
    return;
  }

  // Create command queue
  status = tx_queue_create(&g_tmc6200_command_queue,
                           "TMC6200 Command Queue",
                           sizeof(T_tmc6200_command) / sizeof(ULONG),
                           g_tmc6200_command_queue_buffer,
                           sizeof(g_tmc6200_command_queue_buffer));

  if (status != TX_SUCCESS)
  {
    APPLOG("TMC6200 monitoring: Failed to create command queue. Error=%d", status);
    return;
  }

  // Create monitoring thread
  status = tx_thread_create(&g_tmc6200_monitoring_thread,
                            "TMC6200 Monitoring",
                            _Tmc6200_monitoring_thread_entry,
                            0,
                            &g_tmc6200_monitoring_thread_stack,
                            TMC6200_MONITORING_THREAD_STACK_SIZE,
                            THREAD_PRIORITY_TMC6200_MONITORING,
                            THREAD_PREEMPT_TMC6200_MONITORING,
                            THREAD_TIME_SLICE_TMC6200_MONITORING,
                            TX_AUTO_START);

  if (status != TX_SUCCESS)
  {
    APPLOG("TMC6200 monitoring thread: Failed to create thread. Error=%d", status);
  }
}

/*-----------------------------------------------------------------------------------------------------
  TMC6200 monitoring thread entry function

  Parameters:
    thread_input - thread input parameter (not used)

  Return:
    none
-----------------------------------------------------------------------------------------------------*/
static void _Tmc6200_monitoring_thread_entry(ULONG thread_input)
{
  uint32_t delay_ticks;

  // Convert milliseconds to ticks
  delay_ticks = (TMC6200_MONITORING_CALL_DELAY_MS * TX_TIMER_TICKS_PER_SECOND) / 1000;
  if (delay_ticks == 0)
  {
    delay_ticks = 1;  // Minimum delay of 1 tick
  }

  APPLOG("TMC6200 monitoring thread: Started, monitoring every %d ms", TMC6200_MONITORING_CALL_DELAY_MS);

  // Main monitoring loop
  while (1)
  {
    // Process any pending commands first
    _Process_tmc6200_commands();

    // Call the monitoring function
    _Monitor_tmc6200_drivers();

    // Wait before next monitoring cycle
    tx_thread_sleep(delay_ticks);
  }
}

/*-----------------------------------------------------------------------------------------------------
  Wait for TMC6200 fault reset command completion with timeout.
  This function waits for the reset command to be completed by the monitoring thread.

  Parameters:
    timeout_ms - timeout in milliseconds

  Return:
    true if reset command completed within timeout, false if timeout occurred
-----------------------------------------------------------------------------------------------------*/
bool Tmc6200_wait_fault_reset_completion(uint32_t timeout_ms)
{
  ULONG actual_flags;
  UINT  status;

  status = tx_event_flags_get(&g_tmc6200_events, TMC6200_EVENT_FAULT_RESET_COMPLETED,
                              TX_OR_CLEAR, &actual_flags, ms_to_ticks(timeout_ms));

  return (status == TX_SUCCESS);
}
