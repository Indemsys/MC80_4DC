#include "App.h"

// Function declarations
static void _Fm_motor_command_forward(uint8_t motor_num);
static void _Fm_motor_command_reverse(uint8_t motor_num);
static void _Fm_motor_command_soft_stop(uint8_t motor_num);
static void _Fm_motor_command_emergency_stop(uint8_t motor_num);
static void _Fm_motor_command_emergency_stop_all(void);
static void _Fm_motor_command_coast_all(void);
static void _Fm_motor_toggle_drv_en(uint8_t driver_num);
static void _Fm_motor_calibrate_current(void);
static void _Fm_motor_reset_max_current(void);

/*-----------------------------------------------------------------------------------------------------
  FreeMaster command handler switch-case implementation

  Parameters:
    app_command - Command code from FreeMaster

  Return:
    uint8_t - Command result (0 for success, other values for errors)
-----------------------------------------------------------------------------------------------------*/
uint8_t Freemaster_Command_Manager(uint16_t app_command)
{
  uint8_t res;
  res = 0;

  switch (app_command)
  {
  case FMCMD_CHECK_LOG_PIPE:
    APPLOG("FreeMaster: Log pipe checked");
    break;

  case FMCMD_SAVE_APP_PARAMS:
    Request_save_app_settings();
    break;

  case FMCMD_RESET_DRIVER_FAULTS:
    Tmc6200_request_fault_reset(0);  // Reset both drivers
    break;

  case FMCMD_RESET_DEVICE:
    Reset_SoC();
    break;

  // Motor control commands - Forward direction
  case FMCMD_MOTOR_1_FORWARD:
    _Fm_motor_command_forward(MOTOR_1_);
    break;
  case FMCMD_MOTOR_2_FORWARD:
    _Fm_motor_command_forward(MOTOR_2_);
    break;
  case FMCMD_MOTOR_3_FORWARD:
    _Fm_motor_command_forward(MOTOR_3_);
    break;
  case FMCMD_MOTOR_4_FORWARD:
    _Fm_motor_command_forward(MOTOR_4_);
    break;

  // Motor control commands - Reverse direction
  case FMCMD_MOTOR_1_REVERSE:
    _Fm_motor_command_reverse(MOTOR_1_);
    break;
  case FMCMD_MOTOR_2_REVERSE:
    _Fm_motor_command_reverse(MOTOR_2_);
    break;
  case FMCMD_MOTOR_3_REVERSE:
    _Fm_motor_command_reverse(MOTOR_3_);
    break;
  case FMCMD_MOTOR_4_REVERSE:
    _Fm_motor_command_reverse(MOTOR_4_);
    break;

  // Motor control commands - Soft stop
  case FMCMD_MOTOR_1_SOFT_STOP:
    _Fm_motor_command_soft_stop(MOTOR_1_);
    break;
  case FMCMD_MOTOR_2_SOFT_STOP:
    _Fm_motor_command_soft_stop(MOTOR_2_);
    break;
  case FMCMD_MOTOR_3_SOFT_STOP:
    _Fm_motor_command_soft_stop(MOTOR_3_);
    break;
  case FMCMD_MOTOR_4_SOFT_STOP:
    _Fm_motor_command_soft_stop(MOTOR_4_);
    break;

  // Motor control commands - Emergency stop
  case FMCMD_MOTOR_EMERGENCY_STOP_ALL:
    _Fm_motor_command_emergency_stop_all();
    break;
  case FMCMD_MOTOR_1_EMERGENCY_STOP:
    _Fm_motor_command_emergency_stop(MOTOR_1_);
    break;
  case FMCMD_MOTOR_2_EMERGENCY_STOP:
    _Fm_motor_command_emergency_stop(MOTOR_2_);
    break;
  case FMCMD_MOTOR_3_EMERGENCY_STOP:
    _Fm_motor_command_emergency_stop(MOTOR_3_);
    break;
  case FMCMD_MOTOR_4_EMERGENCY_STOP:
    _Fm_motor_command_emergency_stop(MOTOR_4_);
    break;

  // Motor control commands - Coast stop
  case FMCMD_MOTOR_COAST_ALL:
    _Fm_motor_command_coast_all();
    break;

  // Motor control commands - Driver settings
  case FMCMD_MOTOR_TOGGLE_DRV1_EN:
    _Fm_motor_toggle_drv_en(1);
    break;
  case FMCMD_MOTOR_TOGGLE_DRV2_EN:
    _Fm_motor_toggle_drv_en(2);
    break;
  case FMCMD_MOTOR_CALIBRATE_CURRENT:
    _Fm_motor_calibrate_current();
    break;
  case FMCMD_MOTOR_RESET_MAX_CURRENT:
    _Fm_motor_reset_max_current();
    break;

  // CAN command processing control
  case FMCMD_DISABLE_CAN_COMMANDS:
    Disable_can_command_processing();
    APPLOG("FreeMaster: CAN command processing disabled");
    break;
  case FMCMD_ENABLE_CAN_COMMANDS:
    Restore_can_command_processing();
    APPLOG("FreeMaster: CAN command processing enabled");
    break;

  default:
    res = 0;
    break;
  }

  return res;
}

/*-----------------------------------------------------------------------------------------------------
  Request to save application settings

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Request_save_app_settings(void)
{
  // Request saving of application parameters to DataFlash
  Request_save_settings(APPLICATION_PARAMS, MEDIA_TYPE_DATAFLASH, NULL);
}


/*-----------------------------------------------------------------------------------------------------
  Reset System on Chip (system reset)

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Reset_SoC(void)
{
  // Call the system reset function
  Reset_system();
}

/*-----------------------------------------------------------------------------------------------------
  Check if motor commands are allowed to execute

  Parameters:
    None

  Return:
    true - motor commands allowed, false - motor commands blocked
-----------------------------------------------------------------------------------------------------*/
static bool _Is_motor_command_allowed(void)
{
  if (g_can_command_processing_enabled)
  {
    APPLOG("FreeMaster: Motor commands blocked - CAN command processing is enabled");
    return false;
  }

  if (!Get_motor_driver_ready())
  {
    APPLOG("FreeMaster: ERROR: Motor driver not ready");
    return false;
  }

  return true;
}

/*-----------------------------------------------------------------------------------------------------
  Common function to start motor in specified direction

  Parameters:
    motor_num - motor number (1-4)
    direction - motor direction (MOTOR_DIRECTION_FORWARD/REVERSE)
    direction_str - string for logging ("FORWARD" or "REVERSE")

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Fm_motor_start_common(uint8_t motor_num, uint8_t direction, const char* direction_str)
{
  if (!_Is_motor_command_allowed())
  {
    return;
  }

  // Get motor parameters to determine PWM value
  T_motor_parameters motor_params;
  uint32_t result = Motor_get_parameters(motor_num, &motor_params);
  if (result != RES_OK)
  {
    APPLOG("FreeMaster: ERROR: Failed to get motor %d parameters", motor_num);
    return;
  }

  // Set algorithm from motor parameters
  Motor_soft_start_set_algorithm(motor_num, (T_soft_start_algorithm)motor_params.algorithm);

  // Start motor with configured PWM level
  result = Motor_command_soft_start(motor_num, motor_params.max_pwm_percent, direction);
  if (result == RES_OK)
  {
    APPLOG("FreeMaster: Motor %d started %s", motor_num, direction_str);
  }
  else
  {
    APPLOG("FreeMaster: Failed to start Motor %d %s", motor_num, direction_str);
  }
}

/*-----------------------------------------------------------------------------------------------------
  Start motor in forward direction using parameters from configuration

  Parameters:
    motor_num - motor number (1-4)

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Fm_motor_command_forward(uint8_t motor_num)
{
  _Fm_motor_start_common(motor_num, MOTOR_DIRECTION_FORWARD, "FORWARD");
}

/*-----------------------------------------------------------------------------------------------------
  Start motor in reverse direction using parameters from configuration

  Parameters:
    motor_num - motor number (1-4)

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Fm_motor_command_reverse(uint8_t motor_num)
{
  _Fm_motor_start_common(motor_num, MOTOR_DIRECTION_REVERSE, "REVERSE");
}

/*-----------------------------------------------------------------------------------------------------
  Soft stop motor

  Parameters:
    motor_num - motor number (1-4)

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Fm_motor_command_soft_stop(uint8_t motor_num)
{
  if (!_Is_motor_command_allowed())
  {
    return;
  }

  uint32_t result = Motor_command_soft_stop(motor_num);
  if (result == RES_OK)
  {
    APPLOG("FreeMaster: Motor %d soft stop", motor_num);
  }
  else
  {
    APPLOG("FreeMaster: Failed to soft stop Motor %d", motor_num);
  }
}

/*-----------------------------------------------------------------------------------------------------
  Emergency stop specific motor

  Parameters:
    motor_num - motor number (1-4)

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Fm_motor_command_emergency_stop(uint8_t motor_num)
{
  // Emergency stop commands are always allowed for safety
  uint32_t result = Motor_emergency_stop_direct(motor_num);
  if (result == RES_OK)
  {
    APPLOG("FreeMaster: Motor %d emergency stop", motor_num);
  }
  else
  {
    APPLOG("FreeMaster: Failed to emergency stop Motor %d", motor_num);
  }
}

/*-----------------------------------------------------------------------------------------------------
  Emergency stop all motors

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Fm_motor_command_emergency_stop_all(void)
{
  // Emergency stop commands are always allowed for safety
  uint32_t result = Motor_emergency_stop_direct(0);  // 0 = all motors
  if (result == RES_OK)
  {
    APPLOG("FreeMaster: All motors emergency stop");
  }
  else
  {
    APPLOG("FreeMaster: Failed to emergency stop all motors");
  }
}

/*-----------------------------------------------------------------------------------------------------
  Coast stop all motors

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Fm_motor_command_coast_all(void)
{
  if (!_Is_motor_command_allowed())
  {
    return;
  }

  uint32_t result = Motor_command_coast(0);  // 0 = all motors
  if (result == RES_OK)
  {
    APPLOG("FreeMaster: All motors coast stop");
  }
  else
  {
    APPLOG("FreeMaster: Failed to coast stop all motors");
  }
}

/*-----------------------------------------------------------------------------------------------------
  Toggle motor driver enable signal

  Parameters:
    driver_num - driver number (1 or 2)

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Fm_motor_toggle_drv_en(uint8_t driver_num)
{
  if (driver_num == 1 || driver_num == 2)
  {
    Motor_driver_enable_toggle(driver_num);  // Use centralized function
    APPLOG("FreeMaster: Motor driver %d enable toggled", driver_num);
  }
  else
  {
    APPLOG("FreeMaster: Invalid driver number %d", driver_num);
  }
}

/*-----------------------------------------------------------------------------------------------------
  Trigger motor current offset calibration (non-blocking)

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Fm_motor_calibrate_current(void)
{
  APPLOG("FreeMaster: Starting motor current offset calibration...");

  // Request calibration from motor driver thread (non-blocking)
  uint32_t request_result = Motor_request_calibration();
  if (request_result != RES_OK)
  {
    APPLOG("FreeMaster: ERROR: Failed to request motor calibration (result: %lu)", request_result);
    return;
  }

  APPLOG("FreeMaster: Calibration request sent successfully (non-blocking)");
  APPLOG("FreeMaster: Check motor driver thread logs for calibration completion status");
}

/*-----------------------------------------------------------------------------------------------------
  Reset maximum current tracking for all motors and phases

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Fm_motor_reset_max_current(void)
{
  // Reset all max current values to zero
  Motor_max_current_reset_all();

  APPLOG("FreeMaster: Maximum current tracking reset completed");
}
