#include "App.h"

// Global variable for saving CAN callback when disabling command processing
static T_can_rx_callback g_original_can_callback = NULL;

// Global flag indicating CAN command processing state (1 = enabled, 0 = disabled)
uint8_t g_can_command_processing_enabled         = 1;

// Forward declarations
static void     _Can_message_handler_callback(const T_can_msg* rx_msg);
static void     _Handle_request_state_to_all(void);
static void     _Handle_request_sys_control(const T_can_msg* rx_msg);
static void     _Send_motor_status_packets(uint8_t motor_id);
static void     _Control_motor_from_system_command(uint8_t motor_num, uint32_t up_bit, uint32_t down_bit, uint32_t stop_bit, uint32_t hard_stop_bit, const T_sys_control* cmd);
static uint16_t _Get_movement_info(uint8_t motor_num);
static uint16_t _Get_position_sensor_value(uint8_t motor_id);
static int16_t  _Get_driver_temperature_x10(uint8_t motor_id);
static int16_t  _Get_input_voltage_x10(void);
static int16_t  _Get_input_current_x10(uint8_t motor_id);
static int16_t  _Get_input_power_x10(uint8_t motor_id);
static int16_t  _Get_motor_current_x10(uint8_t motor_id);

extern T_adc_cbl adc;  // Access to ADC data structure

/*-----------------------------------------------------------------------------------------------------
  Initialize CAN message handler by setting up the callback function

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Can_message_handler_init(void)
{
  Can_set_rx_callback(_Can_message_handler_callback);

  // Initialize parameter exchange module
  Can_param_exchange_init();

  // Set flag to indicate CAN command processing is enabled
  g_can_command_processing_enabled = 1;
}

/*-----------------------------------------------------------------------------------------------------
  CAN message reception callback handler. Processes incoming CAN messages.
  Emulates 4 motor controllers responding to central controller commands.

  Parameters:
    rx_msg - Pointer to received CAN message structure

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Can_message_handler_callback(const T_can_msg* rx_msg)
{
  if (rx_msg == NULL)
  {
    return;
  }
  // Process different message types based on CAN ID
  switch (rx_msg->can_id)
  {
    case REQUEST_STATE_TO_ALL:
      // Central controller requests status from all motor controllers
      _Handle_request_state_to_all();
      break;

    case REQUEST_SYS_CONTROL:
      // Central controller sends system control commands
      _Handle_request_sys_control(rx_msg);
      break;
    case MC80_PARAM_RD:
    case MC80_PARAM_WR:
    case MC80_PARAM_SAVE:
    case MC80_RESET:
    case MC80_CLEAR_MOTOR_ERRORS:
      // Parameter configuration protocol messages
      Can_param_process_message(rx_msg);
      break;

    default:
      // Log unknown message for debugging
      APPLOG("CAN Handler: Unknown message ID 0x%08X", rx_msg->can_id);
      break;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Handle REQUEST_STATE_TO_ALL command from central controller.
  Updates TMC6200 error status and sends 3 status packets for each of the 4 motor controllers.

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Handle_request_state_to_all(void)
{
  // Update TMC6200 driver status from monitoring data before sending status packets
  T_tmc6200_driver_status driver1_status;
  T_tmc6200_driver_status driver2_status;

  // Get current GSTAT values from TMC6200 monitoring system
  if (Motdrv_tmc6200_GetDriverStatus(1, &driver1_status) == RES_OK)
  {
    // Update detailed error flags for driver 1
    App_update_tmc6200_detailed_errors(1, driver1_status.last_gstat_value);
  }

  if (Motdrv_tmc6200_GetDriverStatus(2, &driver2_status) == RES_OK)
  {
    // Update detailed error flags for driver 2
    App_update_tmc6200_detailed_errors(2, driver2_status.last_gstat_value);
  }

  // Send status packets for all 4 motor controllers
  _Send_motor_status_packets(MOT3_ID);
  _Send_motor_status_packets(MOT4_ID);
  _Send_motor_status_packets(MOT2_ID);
  _Send_motor_status_packets(TRACTION_MOT_ID);
}

/*-----------------------------------------------------------------------------------------------------
  Handle REQUEST_SYS_CONTROL command from central controller.
  Processes motor control commands and applies them to real motors.

  Parameters:
    rx_msg - Pointer to received CAN message containing control bits

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Handle_request_sys_control(const T_can_msg* rx_msg)
{
  if (rx_msg == NULL || rx_msg->dlc < 4)
  {
    return;  // Invalid message
  }

  // Extract control bits from message data (first 4 bytes)
  uint32_t control_word = 0;
  memcpy(&control_word, rx_msg->data, 4);

  T_sys_control* cmd = (T_sys_control*)&control_word;

  // Check if emergency stop is active - block all CAN motor commands except emergency stops
  if (App_is_emergency_stop_active())
  {
    // Check if this is an emergency stop command - allow only these
    bool has_emergency_stop = (cmd->MOT3_HARD_STOP || cmd->MOT4_HARD_STOP ||
                               cmd->MOT2_HARD_STOP || cmd->MOT_HARD_STOP);

    if (!has_emergency_stop)
    {
      // Block all non-emergency CAN commands when emergency stop is active
      return;
    }
  }

  // Track command state changes for logging
  static uint32_t last_control_word = 0;
  if (control_word != last_control_word)
  {
    // Log command bits state in readable format (within 100 chars)
    APPLOG("CAN Cmd: 0x%08X L[%u%u%u%u] R[%u%u%u%u] P[%u%u%u%u] T[%u%u%u%u] C%u",
           (unsigned int)control_word,
           cmd->MOT3_UP, cmd->MOT3_DOWN, cmd->MOT3_STOP, cmd->MOT3_HARD_STOP,
           cmd->MOT4_UP, cmd->MOT4_DOWN, cmd->MOT4_STOP, cmd->MOT4_HARD_STOP,
           cmd->MOT2_UP, cmd->MOT2_DOWN, cmd->MOT2_STOP, cmd->MOT2_HARD_STOP,
           cmd->MOT_UP, cmd->MOT_DOWN, cmd->MOT_STOP, cmd->MOT_HARD_STOP, cmd->COAST_ALL);
    last_control_word = control_word;
  }

  // First check for any emergency stop commands (highest priority)
  bool has_emergency_stop = (cmd->MOT3_HARD_STOP || cmd->MOT4_HARD_STOP ||
                             cmd->MOT2_HARD_STOP || cmd->MOT_HARD_STOP);

  if (has_emergency_stop)
  {
    // Process emergency stops first, then handle other commands
    _Control_motor_from_system_command(MOTOR_3_, MOT3_UP_BIT, MOT3_DOWN_BIT, MOT3_STOP_BIT, MOT3_HARD_STOP_BIT, cmd);
    _Control_motor_from_system_command(MOTOR_4_, MOT4_UP_BIT, MOT4_DOWN_BIT, MOT4_STOP_BIT, MOT4_HARD_STOP_BIT, cmd);
    _Control_motor_from_system_command(MOTOR_2_, MOT2_UP_BIT, MOT2_DOWN_BIT, MOT2_STOP_BIT, MOT2_HARD_STOP_BIT, cmd);
    _Control_motor_from_system_command(MOTOR_1_, MOT_UP_BIT, MOT_DOWN_BIT, MOT_STOP_BIT, MOT_HARD_STOP_BIT, cmd);
    return;  // Emergency stops processed, exit
  }
  // Check for COAST_ALL command (second priority after emergency stops)
  if (cmd->COAST_ALL)
  {
    Motor_command_coast(0);  // Coast all motors (0 = all motors)
    APPLOG("CAN Command: COAST_ALL executed - all motors coasting");
    return;                  // Exit early, coast overrides individual motor commands
  }

  // Process individual motor commands (lowest priority)
  // MOT3_ID (1) -> Motor 3
  // MOT4_ID (2) -> Motor 4
  // MOT2_ID (3) -> Motor 2
  // TRACTION_MOT_ID (4) -> Motor 1 (Traction)

  _Control_motor_from_system_command(MOTOR_3_, MOT3_UP_BIT, MOT3_DOWN_BIT, MOT3_STOP_BIT, MOT3_HARD_STOP_BIT, cmd);
  _Control_motor_from_system_command(MOTOR_4_, MOT4_UP_BIT, MOT4_DOWN_BIT, MOT4_STOP_BIT, MOT4_HARD_STOP_BIT, cmd);
  _Control_motor_from_system_command(MOTOR_2_, MOT2_UP_BIT, MOT2_DOWN_BIT, MOT2_STOP_BIT, MOT2_HARD_STOP_BIT, cmd);
  _Control_motor_from_system_command(MOTOR_1_, MOT_UP_BIT, MOT_DOWN_BIT, MOT_STOP_BIT, MOT_HARD_STOP_BIT, cmd);
}

/*-----------------------------------------------------------------------------------------------------  Send all 3 status packets for a specific motor controller.
  Packet 1: Basic status (7 bytes) - MC80_ANS
  Packet 2: Sensor information (8 bytes) - MC80_SENS_INFO
  Packet 3: Power information (8 bytes) - MC80_PWR_INFO

  Parameters:
    motor_id - Motor controller ID (1-4)

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Send_motor_status_packets(uint8_t motor_id)
{
  uint32_t base_ans_id  = 0;
  uint32_t base_sens_id = 0;
  uint32_t base_pwr_id  = 0;
  uint8_t  motor_num    = 0;
  // Map motor controller IDs to CAN IDs and physical motor numbers
  switch (motor_id)
  {
    case MOT3_ID:
      base_ans_id  = MOT3_ANS;
      base_sens_id = MOT3_SENS_INFO;
      base_pwr_id  = MOT3_PWR_INFO;
      motor_num    = MOTOR_3_;
      break;
    case MOT4_ID:
      base_ans_id  = MOT4_ANS;
      base_sens_id = MOT4_SENS_INFO;
      base_pwr_id  = MOT4_PWR_INFO;
      motor_num    = MOTOR_4_;
      break;
    case MOT2_ID:
      base_ans_id  = MOT2_ANS;
      base_sens_id = MOT2_SENS_INFO;
      base_pwr_id  = MOT2_PWR_INFO;
      motor_num    = MOTOR_2_;
      break;
    case TRACTION_MOT_ID:
      base_ans_id  = TRACTION_MOT_ANS;
      base_sens_id = TRACTION_MOT_SENS_INFO;
      base_pwr_id  = TRACTION_MOT_PWR_INFO;
      motor_num    = MOTOR_1_;
      break;
    default:
      return;  // Invalid motor ID
  }
  // Packet 1: Basic status (7 bytes)
  T_can_status_packet1 status_packet = { 0 };
  status_packet.command_code         = MC80_REQ_STATUS;
  status_packet.error_flags          = App_get_error_flags();
  status_packet.movement_info        = _Get_movement_info(motor_num);

  Can_send_extended_data(base_ans_id, (uint8_t*)&status_packet, sizeof(status_packet));

  // Packet 2: Sensor information (8 bytes)
  T_can_status_packet2 sensor_packet = { 0 };
  sensor_packet.position_sensor      = _Get_position_sensor_value(motor_id);
  sensor_packet.motor_rpm            = 0;  // Always 0 as specified
  sensor_packet.driver_temp_x10      = _Get_driver_temperature_x10(motor_id);

  Can_send_extended_data(base_sens_id, (uint8_t*)&sensor_packet, sizeof(sensor_packet));

  // Packet 3: Power information (8 bytes)
  T_can_status_packet3 power_packet = { 0 };
  power_packet.input_voltage_x10    = _Get_input_voltage_x10();
  power_packet.input_current_x10    = _Get_input_current_x10(motor_id);
  power_packet.input_power_x10      = _Get_input_power_x10(motor_id);
  power_packet.motor_current_x10    = _Get_motor_current_x10(motor_id);

  Can_send_extended_data(base_pwr_id, (uint8_t*)&power_packet, sizeof(power_packet));
}

/*-----------------------------------------------------------------------------------------------------
  Control motor based on system control command bits with simple rule: ignore run commands if motor is running

  Parameters:
    motor_num     - Physical motor number (1-4)
    up_bit        - Bit mask for UP command
    down_bit      - Bit mask for DOWN command
    stop_bit      - Bit mask for SOFT STOP command
    hard_stop_bit - Bit mask for HARD STOP command
    cmd           - Pointer to system control command structure

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Control_motor_from_system_command(uint8_t motor_num, uint32_t up_bit, uint32_t down_bit, uint32_t stop_bit, uint32_t hard_stop_bit, const T_sys_control* cmd)
{
  if (cmd == NULL || motor_num < MOTOR_1_ || motor_num > MOTOR_4_)
  {
    return;
  }

  uint32_t control_word = *(uint32_t*)cmd;
  uint8_t  pwm_percent  = 50;  // Default PWM value if parameter not found

  // Get PWM value from parameters structure based on motor number
  switch (motor_num)
  {
    case MOTOR_1_:
      pwm_percent = wvar.motor_1_max_pwm_percent;
      break;
    case MOTOR_2_:
      pwm_percent = wvar.motor_2_max_pwm_percent;
      break;
    case MOTOR_3_:
      pwm_percent = wvar.motor_3_max_pwm_percent;
      break;
    case MOTOR_4_:
      pwm_percent = wvar.motor_4_max_pwm_percent;
      break;
    default:
      pwm_percent = 50;  // Fallback default value
      break;
  }

  // Ensure PWM value is within valid range
  if (pwm_percent > 100)
  {
    pwm_percent = 100;
  }
  // Check command bits with priority handling
  if (control_word & hard_stop_bit)
  {
    // Check if motor is already stopped before executing emergency stop
    T_motor_extended_state* motor_state = Motor_get_extended_state(motor_num);
    if (motor_state != NULL &&
        motor_state->enabled == 0 &&
        motor_state->direction == MOTOR_DIRECTION_STOP &&
        (motor_state->soft_start_state == MOTOR_STATE_IDLE || motor_state->soft_start_state == MOTOR_STATE_COASTING))
    {
      // Motor is already stopped - ignore duplicate emergency stop command
      return;
    }

    // Highest priority: Emergency stop (execute only if motor is not already stopped)
    uint32_t result = Motor_emergency_stop_direct(motor_num);
    if (result == 0)
    {
      APPLOG("CAN Handler: Motor %u (%s) emergency stop (direct)", motor_num, Get_motor_name(motor_num));
    }
    else
    {
      APPLOG("CAN Handler: Motor %u (%s) emergency stop failed (error: %u)", motor_num, Get_motor_name(motor_num), (unsigned int)result);
    }
  }
  else if (control_word & stop_bit)
  {
    // Check if motor is already stopped or stopping before executing soft stop
    T_motor_extended_state* motor_state = Motor_get_extended_state(motor_num);
    if (motor_state != NULL &&
        (motor_state->soft_start_state == MOTOR_STATE_SOFT_RAMPING_DOWN ||
         (motor_state->enabled == 0 && motor_state->direction == MOTOR_DIRECTION_STOP)))
    {
      // Motor is already stopping or stopped - ignore duplicate soft stop command
      return;
    }

    // High priority: Soft stop (execute only if motor is not already stopping/stopped)
    uint32_t result = Motor_command_soft_stop(motor_num);
    if (result == TX_SUCCESS)
    {
      APPLOG("CAN Handler: Motor %u (%s) soft stop", motor_num, Get_motor_name(motor_num));
    }
    else
    {
      APPLOG("CAN Handler: Motor %u (%s) soft stop failed (queue error: %u)", motor_num, Get_motor_name(motor_num), (unsigned int)result);
    }
  }
  else if (control_word & down_bit)
  {
    // Check if motor is currently running (in any direction)
    if (Motor_soft_start_is_active(motor_num))
    {
      // Motor is running - ignore run command
      return;
    }

    // Motor is stopped - execute reverse command
    uint32_t result = Motor_command_soft_start(motor_num, pwm_percent, MOTOR_DIRECTION_REVERSE);
    if (result == TX_SUCCESS)
    {
      APPLOG("CAN Handler: Motor %u (%s) reverse direction, PWM=%u%%", motor_num, Get_motor_name(motor_num), pwm_percent);
    }
    else
    {
      APPLOG("CAN Handler: Motor %u (%s) reverse command failed (queue error: %u)", motor_num, Get_motor_name(motor_num), (unsigned int)result);
    }
  }
  else if (control_word & up_bit)
  {
    // Check if motor is currently running (in any direction)
    if (Motor_soft_start_is_active(motor_num))
    {
      // Motor is running - ignore run command
      return;
    }

    // Motor is stopped - execute forward command
    uint32_t result = Motor_command_soft_start(motor_num, pwm_percent, MOTOR_DIRECTION_FORWARD);
    if (result == TX_SUCCESS)
    {
      APPLOG("CAN Handler: Motor %u (%s) forward direction, PWM=%u%%", motor_num, Get_motor_name(motor_num), pwm_percent);
    }
    else
    {
      APPLOG("CAN Handler: Motor %u (%s) forward command failed (queue error: %u)", motor_num, Get_motor_name(motor_num), (unsigned int)result);
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Get movement information for status packet.
  Encodes motor state, trajectory stage, and PWM value into 16-bit word.

  Parameters:
    motor_num - Physical motor number (1-4)

  Return:
    Movement info encoded as: bit 7=motor active, bits 0-6=soft_start_state, bits 8-15=PWM value
-----------------------------------------------------------------------------------------------------*/
static uint16_t _Get_movement_info(uint8_t motor_num)
{
  // Get extended motor state for complete information
  T_motor_extended_state* motor_state = Motor_get_extended_state(motor_num);
  if (motor_state == NULL)
  {
    return 0;  // Motor not found or error - return all zeros
  }

  uint16_t movement_info   = 0;

  // Bits 0-6: Soft start state (ensure it's within valid range)
  uint8_t soft_start_state = motor_state->soft_start_state & 0x7F;
  movement_info |= soft_start_state;
  // Bit 7: Motor active state (1 = active, 0 = inactive)
  // Motor is active if it's not in IDLE state or if it's enabled with PWM > 0
  if (motor_state->soft_start_state != MOTOR_STATE_IDLE ||
      (motor_state->enabled && motor_state->pwm_level > 0))
  {
    movement_info |= 0x80;
  }
  // Bits 8-15: PWM value (0-100)
  // Use pwm_level which is already synchronized with current PWM state
  uint16_t pwm_byte = (motor_state->pwm_level > 100) ? 100 : motor_state->pwm_level;
  movement_info |= (pwm_byte << 8);

  return movement_info;
}

/*-----------------------------------------------------------------------------------------------------
  Get position sensor value for specific motor controller.
  Maps motor IDs to appropriate ADC position sensor values.

  Parameters:
    motor_id - Motor controller ID (1-4)

  Return:
    Position sensor ADC value (0 for motors without position sensors)
-----------------------------------------------------------------------------------------------------*/
static uint16_t _Get_position_sensor_value(uint8_t motor_id)
{
  switch (motor_id)
  {
    case MOT3_ID:                                  // Motor 3:
      return Adc_driver_get_position_sensor_value(1);  // Position sensor connected to motor 1 ADC channel
    case MOT4_ID:                                 // Motor 4:
      return Adc_driver_get_position_sensor_value(2);  // Position sensor connected to motor 2 ADC channel
    case MOT2_ID:                             // Motor 2: Motor 2  - no position sensor
    case TRACTION_MOT_ID:                              // Motor 1: - no position sensor
    default:
      return 0;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Get driver temperature value scaled by 10 for specific motor controller.
  Maps motor IDs to appropriate temperature sensor values.

  Parameters:
    motor_id - Motor controller ID (1-4)

  Return:
    Driver temperature in °C×10
-----------------------------------------------------------------------------------------------------*/
static int16_t _Get_driver_temperature_x10(uint8_t motor_id)
{
  float temp_celsius = 0.0f;
  switch (motor_id)
  {
    case MOT2_ID:  // Motor 2: Motor 2
    case TRACTION_MOT_ID:   // Motor 1: Traction
      temp_celsius = adc.temp_motor1;
      break;
    case MOT3_ID:       // Motor 3:
    case MOT4_ID:      // Motor 4:
      temp_celsius = adc.temp_motor2;
      break;
    default:
      temp_celsius = 25.0f;  // Default temperature
      break;
  }

  return (int16_t)(temp_celsius * 10.0f);
}

/*-----------------------------------------------------------------------------------------------------
  Get input voltage scaled by 10.
  Returns system 24V supply voltage for all motor controllers.

  Parameters:
    None

  Return:
    Input voltage in V×10
-----------------------------------------------------------------------------------------------------*/
static int16_t _Get_input_voltage_x10(void)
{
  return (int16_t)(adc.v24v_supply * 10.0f);
}

/*-----------------------------------------------------------------------------------------------------
  Get input current scaled by 10 for specific motor controller.
  Maps motor IDs to appropriate power supply current values.

  Parameters:
    motor_id - Motor controller ID (1-4)

  Return:
    Input current in A×10
-----------------------------------------------------------------------------------------------------*/
static int16_t _Get_input_current_x10(uint8_t motor_id)
{
  float current_amperes = 0.0f;

  switch (motor_id)
  {
    case MOT2_ID:  // Motor 2: Motor 2
    case TRACTION_MOT_ID:   // Motor 1: Traction
      current_amperes = adc.ipwr_motor1;
      break;
    case MOT3_ID:       // Motor 3:
    case MOT4_ID:      // Motor 4:
      current_amperes = adc.ipwr_motor2;
      break;
    default:
      current_amperes = 0.0f;
      break;
  }

  return (int16_t)(current_amperes * 10.0f);
}

/*-----------------------------------------------------------------------------------------------------
  Get input power scaled by 10 for specific motor controller.
  Calculates power as voltage × current for appropriate motor driver.

  Parameters:
    motor_id - Motor controller ID (1-4)

  Return:
    Input power in W×10
-----------------------------------------------------------------------------------------------------*/
static int16_t _Get_input_power_x10(uint8_t motor_id)
{
  float power_watts     = 0.0f;
  float current_amperes = 0.0f;

  switch (motor_id)
  {
    case MOT2_ID:  // Motor 2: Motor 2
    case TRACTION_MOT_ID:   // Motor 1: Traction
      current_amperes = adc.ipwr_motor1;
      break;
    case MOT3_ID:       // Motor 3:
    case MOT4_ID:      // Motor 4:
      current_amperes = adc.ipwr_motor2;
      break;
    default:
      current_amperes = 0.0f;
      break;
  }

  power_watts = adc.v24v_supply * current_amperes;
  return (int16_t)(power_watts * 10.0f);
}

/*-----------------------------------------------------------------------------------------------------
  Get motor current scaled by 10 for specific motor controller.
  Maps motor IDs to appropriate power supply current values (same as input current).

  Parameters:
    motor_id - Motor controller ID (1-4)

  Return:
    Motor current in A×10
-----------------------------------------------------------------------------------------------------*/
static int16_t _Get_motor_current_x10(uint8_t motor_id)
{
  float motor_current_amperes = 0.0f;

  // Map CAN motor IDs to physical motor numbers using existing macros
  uint8_t physical_motor_num  = 0;
  switch (motor_id)
  {
    case TRACTION_MOT_ID:   // Motor 1: Traction
      physical_motor_num = MOTOR_1_;
      break;
    case MOT2_ID:  // Motor 2: Motor 2
      physical_motor_num = MOTOR_2_;
      break;
    case MOT3_ID:       // Motor 3:
      physical_motor_num = MOTOR_3_;
      break;
    case MOT4_ID:      // Motor 4:
      physical_motor_num = MOTOR_4_;
      break;
    default:
      return 0;  // Invalid motor ID
  }

  motor_current_amperes = Adc_driver_get_dc_motor_current(physical_motor_num);
  return (int16_t)(motor_current_amperes * 10.0f);
}

/*-----------------------------------------------------------------------------------------------------
  Get motor name string for specific motor number.
  Maps physical motor numbers to human-readable names.

  Parameters:
    motor_num - Physical motor number (1-4)

  Return:
    Pointer to constant string with motor name
-----------------------------------------------------------------------------------------------------*/
const char* Get_motor_name(uint8_t motor_num)
{
  switch (motor_num)
  {
    case MOTOR_1_:
      return "Traction";
    case MOTOR_2_:
      return "Motor 2";
    case MOTOR_3_:
      return " Motor 3";
    case MOTOR_4_:
      return " Motor 2";
    default:
      return "Unknown";
  }
}

/*-----------------------------------------------------------------------------------------------------
  Disable CAN command processing by setting callback to NULL

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Disable_can_command_processing(void)
{
  // Save current callback
  g_original_can_callback = Can_get_rx_callback();

  // Disable CAN command processing by setting callback to NULL
  Can_set_rx_callback(NULL);

  // Update global flag
  g_can_command_processing_enabled = 0;

  APPLOG("CAN command processing disabled");
}

/*-----------------------------------------------------------------------------------------------------
  Restore CAN command processing by restoring original callback

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Restore_can_command_processing(void)
{
  // Restore original callback
  if (g_original_can_callback != NULL)
  {
    Can_set_rx_callback(g_original_can_callback);
    g_original_can_callback          = NULL;

    // Update global flag
    g_can_command_processing_enabled = 1;

    APPLOG("CAN command processing restored");
  }
}
