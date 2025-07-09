#include "App.h"

/*
  Motor Connection Information:
  - Motor 1 (Traction): Connected between U1-V1 phases
  - Motor 2 (Motor 2) : Connected between V1-W1 phases (shares V1 with Motor 1)
  - Motor 3 ( Motor 3): Connected between U2-V2 phases
  - Motor 4 ( Motor 2): Connected between V2-W2 phases (shares V2 with Motor 3)

  Motor Pair Behavior (configurable by macros in Motor_Driver_task.c):
  - Motors 1 & 2: Can work together or be mutually exclusive (MOTORS_1_2_MUTUALLY_EXCLUSIVE)
  - Motors 3 & 4: Can work together or be mutually exclusive (MOTORS_3_4_MUTUALLY_EXCLUSIVE)
  - When motors sharing a phase work together, shared phase is managed by the first motor
  Control Logic:
  - Keys 1-4: Start motors 1-4 in FORWARD direction
  - Keys Q/W/E/R: Start motors 1-4 in REVERSE direction
  - Keys A/S/D/F: Soft stop motors 1-4
  - Keys Z/X/C/V: Emergency stop motors 1-4
  - Key G: Coast stop all motors (free wheeling)
  - Key T: Toggle algorithm (INSTANT -> LINEAR -> S-CURVE -> INSTANT)
  - All motors use the same algorithm selected by key T
*/

// Macros for motor diagnostic
#define MIN_MOTOR_PWM                     10  // Minimum PWM when turning motor ON (10%)

#define MOTOR_DIAG_HEADER                 "================ Motor Diagnostic Menu ================\r\n"

// ======= KEY DEFINITIONS FOR MOTOR DIAGNOSTIC - GROUPED BY FUNCTION =======

// Main motor control keys (numeric)
#define MOTOR_KEY_MOTOR_1_FORWARD         '1'  // Start Motor 1 (Traction) FORWARD
#define MOTOR_KEY_MOTOR_2_FORWARD         '2'  // Start Motor 2 ( Motor 2) FORWARD
#define MOTOR_KEY_MOTOR_3_FORWARD         '3'  // Start Motor 3 ( Motor 3) FORWARD
#define MOTOR_KEY_MOTOR_4_FORWARD         '4'  // Start Motor 4 ( Motor 4) FORWARD

// Motor reverse control keys
#define MOTOR_KEY_MOTOR_1_REVERSE         'Q'  // Start Motor 1 (Traction) REVERSE
#define MOTOR_KEY_MOTOR_1_REVERSE_LOWER   'q'  // Start Motor 1 (Traction) REVERSE
#define MOTOR_KEY_MOTOR_2_REVERSE         'W'  // Start Motor 2 (Motor 2) REVERSE
#define MOTOR_KEY_MOTOR_2_REVERSE_LOWER   'w'  // Start Motor 2 (Motor 2) REVERSE
#define MOTOR_KEY_MOTOR_3_REVERSE         'E'  // Start Motor 3 ( Motor 3) REVERSE
#define MOTOR_KEY_MOTOR_3_REVERSE_LOWER   'e'  // Start Motor 3 ( Motor 3) REVERSE
#define MOTOR_KEY_MOTOR_4_REVERSE         'R'  // Start Motor 4 ( Motor 4) REVERSE
#define MOTOR_KEY_MOTOR_4_REVERSE_LOWER   'r'  // Start Motor 4 ( Motor 4) REVERSE

// Motor soft stop keys
#define MOTOR_KEY_MOTOR_1_SOFT_STOP       'A'  // Soft stop Motor 1 (Traction)
#define MOTOR_KEY_MOTOR_1_SOFT_STOP_LOWER 'a'  // Soft stop Motor 1 (Traction)
#define MOTOR_KEY_MOTOR_2_SOFT_STOP       'S'  // Soft stop Motor 2 (Motor 2)
#define MOTOR_KEY_MOTOR_2_SOFT_STOP_LOWER 's'  // Soft stop Motor 2 (Motor 2)
#define MOTOR_KEY_MOTOR_3_SOFT_STOP       'D'  // Soft stop Motor 3 ( Motor 3)
#define MOTOR_KEY_MOTOR_3_SOFT_STOP_LOWER 'd'  // Soft stop Motor 3 ( Motor 3)
#define MOTOR_KEY_MOTOR_4_SOFT_STOP       'F'  // Soft stop Motor 4 ( Motor 4)
#define MOTOR_KEY_MOTOR_4_SOFT_STOP_LOWER 'f'  // Soft stop Motor 4 ( Motor 4)

// Emergency stop keys
#define MOTOR_KEY_EMERGENCY_STOP          ' '  // SPACE - Emergency stop all motors (immediate stop without ramping)
#define MOTOR_KEY_EMERGENCY_STOP_1        'Z'  // Z - Emergency stop Motor 1 only
#define MOTOR_KEY_EMERGENCY_STOP_1_LOWER  'z'  // z - Emergency stop Motor 1 only
#define MOTOR_KEY_EMERGENCY_STOP_2        'X'  // X - Emergency stop Motor 2 only
#define MOTOR_KEY_EMERGENCY_STOP_2_LOWER  'x'  // x - Emergency stop Motor 2 only
#define MOTOR_KEY_EMERGENCY_STOP_3        'C'  // C - Emergency stop Motor 3 only
#define MOTOR_KEY_EMERGENCY_STOP_3_LOWER  'c'  // c - Emergency stop Motor 3 only
#define MOTOR_KEY_EMERGENCY_STOP_4        'V'  // V - Emergency stop Motor 4 only
#define MOTOR_KEY_EMERGENCY_STOP_4_LOWER  'v'  // v - Emergency stop Motor 4 only

// Coast stop keys
#define MOTOR_KEY_COAST_ALL               'G'  // G - Coast stop all motors (free wheeling)
#define MOTOR_KEY_COAST_ALL_LOWER         'g'  // g - Coast stop all motors (free wheeling)

// Algorithm control keys (moved to other keys)
#define MOTOR_KEY_TOGGLE_ALGORITHM        'T'  // Toggle between algorithms (INSTANT/LINEAR/S-CURVE)
#define MOTOR_KEY_TOGGLE_ALGORITHM_LOWER  't'  // Toggle between algorithms (INSTANT/LINEAR/S-CURVE)

// Driver enable control keys (moved to other keys)
#define MOTOR_KEY_TOGGLE_DRV1_EN          'N'  // Toggle DRV1_EN (Motor Driver 1 Enable)
#define MOTOR_KEY_TOGGLE_DRV1_EN_LOWER    'n'  // Toggle DRV1_EN (Motor Driver 1 Enable)
#define MOTOR_KEY_TOGGLE_DRV2_EN          'M'  // Toggle DRV2_EN (Motor Driver 2 Enable)
#define MOTOR_KEY_TOGGLE_DRV2_EN_LOWER    'm'  // Toggle DRV2_EN (Motor Driver 2 Enable)

// Calibration control keys
#define MOTOR_KEY_CALIBRATE_CURRENT       'K'  // Trigger motor current offset calibration
#define MOTOR_KEY_CALIBRATE_CURRENT_LOWER 'k'  // Trigger motor current offset calibration

// Navigation and system control keys (VT100 arrow keys defined in Monitor_utilites.h)
#define MOTOR_KEY_EXIT                    VT100_ESC  // ESC - Exit motor diagnostic menu

// ======= END OF KEY DEFINITIONS =======

// Timing and display constants
#define MOTOR_DIAG_REFRESH_MS             300
#define MOTOR_DIAG_START_LINE             3  // Line where motor data starts (after header)
// Arrays to store preset motor parameters (indices 0-3 for motors 1-4)
static uint16_t g_preset_motor_pwm[4]             = { 0 };  // Preset PWM levels for each motor

// Current algorithm for all motors (global setting)
static T_soft_start_algorithm g_current_algorithm = SOFT_START_ALG_S_CURVE;

// Static variable to store the active motor number (1-4)
static uint8_t g_active_motor_num                 = 1;
// Static variable to store the PWM for the active motor
static uint16_t g_active_motor_pwm                = 50;  // Initial PWM set to 50%

                                                         // Short macro for VT100 line clearing
#define CL VT100_CLR_LINE

// Macro to combine MPRINTF with line counter increment for cleaner code
#define MPRINTF_LINE(line_var, ...) \
  do                                \
  {                                 \
    MPRINTF(CL __VA_ARGS__);        \
    (line_var)++;                   \
  } while (0)

// Global variables for saving EN signal states when entering terminal mode
static uint8_t g_saved_drv1_en_state             = 0;
static uint8_t g_saved_drv2_en_state             = 0;

extern TX_QUEUE         g_motor_command_queue;
extern volatile uint8_t g_motor_driver_ready;

static uint8_t     _Motor_diag_print_adc(void);
static const char* _Get_motor_status_str(uint8_t motor_num);
static const char* _Get_motor_direction_str(uint8_t motor_num);
static const char* _Get_algorithm_str(T_soft_start_algorithm algorithm);
static const char* _Get_phase_state_str(uint8_t motor_driver, uint8_t phase, uint8_t motor_num);
static void        _Handle_calibration_request(void);

/*-----------------------------------------------------------------------------------------------------
  Display motor parameters from ADC data in aligned columns format.
  Shows 4 motors phase data, then 2 motors sensor data.

  Parameters:
    None

  Return:
    Next available line number for input operations
-----------------------------------------------------------------------------------------------------*/
static uint8_t _Motor_diag_print_adc(void)
{
  GET_MCBL;
  uint8_t cln               = MOTOR_DIAG_START_LINE;  // Start at defined line (after header)

  // Get PWM levels for all motors
  uint8_t  motor_enabled[4] = { 0 };
  uint16_t motor_pwm[4]     = { 0 };
  uint8_t  motor_dir[4]     = { 0 };

  for (uint8_t i = 0; i < 4; i++)
  {
    Motor_get_state(i + 1, &motor_enabled[i], &motor_pwm[i], &motor_dir[i]);
  }

  // Get motor parameters for timing information
  T_motor_parameters motor_params[4];
  for (uint8_t i = 0; i < 4; i++)
  {
    Motor_get_parameters(i + 1, &motor_params[i]);
  }

  // Position cursor to start line and clear data area line by line
  MPRINTF(VT100_CURSOR_SET, cln, 1);
  // Clear and display live data header
  MPRINTF_LINE(cln, "Note: M1&M2 share V1, M3&M4 share V2 - behavior depends on compile-time macros.\r\n");
  MPRINTF_LINE(cln, "Motor Phase Data (Active Phases Only):\r\n");
  // Заголовки выравниваем по ширине данных (все столбцы по 18 символов, включая пробелы и единицы измерения)
  MPRINTF_LINE(cln, "           |------------------|------------------|------------------|------------------|\r\n");
  MPRINTF_LINE(cln, "           |     Motor 1      |     Motor 2      |     Motor 3      |     Motor 4      |\r\n");
  MPRINTF_LINE(cln, "           |   (Traction)     |                  |                  |                  |\r\n");
  MPRINTF_LINE(cln, "           |  U1-V1 phases    |  V1-W1 phases    |  U2-V2 phases    |  V2-W2 phases    |\r\n");
  MPRINTF_LINE(cln, "           |------------------|------------------|------------------|------------------|\r\n");
  // Phase currents - only active phases for each motor (using slow filtered values for stable monitoring)
  MPRINTF_LINE(cln, "Ia         | %8.3f A U1    | %8.3f A V1    | %8.3f A U2    | %8.3f A V2    |\r\n", adc.i_u_motor1_slow, adc.i_v_motor1_slow, adc.i_u_motor2_slow, adc.i_v_motor2_slow);
  MPRINTF_LINE(cln, "Ib         | %8.3f A V1    | %8.3f A W1    | %8.3f A V2    | %8.3f A W2    |\r\n", adc.i_v_motor1_slow, adc.i_w_motor1_slow, adc.i_v_motor2_slow, adc.i_w_motor2_slow);
  // Phase logic states - only active phases for each motor
  MPRINTF_LINE(cln, "Phase A    |  U1= %8s    |  V1= %8s    |  U2= %8s    |  V2= %8s    |\r\n", _Get_phase_state_str(1, PHASE_U, 1), _Get_phase_state_str(1, PHASE_V, 2), _Get_phase_state_str(2, PHASE_U, 3), _Get_phase_state_str(2, PHASE_V, 4));
  MPRINTF_LINE(cln, "Phase B    |  V1= %8s    |  W1= %8s    |  V2= %8s    |  W2= %8s    |\r\n", _Get_phase_state_str(1, PHASE_V, 1), _Get_phase_state_str(1, PHASE_W, 2), _Get_phase_state_str(2, PHASE_V, 3), _Get_phase_state_str(2, PHASE_W, 4));
  // Выравниваем Stat/PWM/Dir по ширине с заголовками
  MPRINTF_LINE(cln, "Stat       |    %10s    |    %10s    |    %10s    |    %10s    |\r\n", _Get_motor_status_str(1), _Get_motor_status_str(2), _Get_motor_status_str(3), _Get_motor_status_str(4));
  MPRINTF_LINE(cln, "PWM        |          %3u%%    |          %3u%%    |          %3u%%    |          %3u%%    |\r\n", (unsigned int)motor_pwm[0], (unsigned int)motor_pwm[1], (unsigned int)motor_pwm[2], (unsigned int)motor_pwm[3]);
  MPRINTF_LINE(cln, "Dir        |    %10s    |    %10s    |    %10s    |    %10s    |\r\n", _Get_motor_direction_str(1), _Get_motor_direction_str(2), _Get_motor_direction_str(3), _Get_motor_direction_str(4));
  MPRINTF_LINE(cln, "           |------------------|------------------|------------------|------------------|\r\n");
  MPRINTF_LINE(cln, "Set PWM    |          %3u%%    |          %3u%%    |          %3u%%    |          %3u%%    |\r\n", (unsigned int)g_preset_motor_pwm[0], (unsigned int)g_preset_motor_pwm[1], (unsigned int)g_preset_motor_pwm[2], (unsigned int)g_preset_motor_pwm[3]);
  MPRINTF_LINE(cln, "PWM limit  |          %3u%%    |          %3u%%    |          %3u%%    |          %3u%%    |\r\n", (unsigned int)motor_params[0].max_pwm_percent, (unsigned int)motor_params[1].max_pwm_percent, (unsigned int)motor_params[2].max_pwm_percent, (unsigned int)motor_params[3].max_pwm_percent);
  MPRINTF_LINE(cln, "Accel Time |      %6u ms   |      %6u ms   |      %6u ms   |      %6u ms   |\r\n", (unsigned int)motor_params[0].accel_time_ms, (unsigned int)motor_params[1].accel_time_ms, (unsigned int)motor_params[2].accel_time_ms, (unsigned int)motor_params[3].accel_time_ms);
  MPRINTF_LINE(cln, "Decel Time |      %6u ms   |      %6u ms   |      %6u ms   |      %6u ms   |\r\n", (unsigned int)motor_params[0].decel_time_ms, (unsigned int)motor_params[1].decel_time_ms, (unsigned int)motor_params[2].decel_time_ms, (unsigned int)motor_params[3].decel_time_ms);
  MPRINTF_LINE(cln, "           |------------------|------------------|------------------|------------------|\r\n");

  MPRINTF_LINE(cln, "\r\n");  // Display 2 motor sensor data (only motor1 and motor2 have sensor channels)
  MPRINTF_LINE(cln, "Motor Sensor Data (2 Channels):\r\n");
  MPRINTF_LINE(cln, "                   |---------------|---------------|\r\n");
  MPRINTF_LINE(cln, "                   |   Driver1     |   Driver2     |\r\n");
  MPRINTF_LINE(cln, "                   |---------------|---------------|\r\n");
  MPRINTF_LINE(cln, "%18s | %11.3f A | %11.3f A |\r\n", "Power Current", adc.ipwr_motor1, adc.ipwr_motor2);
  MPRINTF_LINE(cln, "%18s | %9.3f V   | %9.3f V   |\r\n", "Speed", adc.speed_motor1, adc.speed_motor2);
  MPRINTF_LINE(cln, "%18s | %9.3f V   | %9.3f V   |\r\n", "Position", adc.pos_motor1, adc.pos_motor2);
  MPRINTF_LINE(cln, "%18s | %9.1f °C  | %9.1f °C  |\r\n", "Thermistor Temp", adc.temp_motor1, adc.temp_motor2);  // Note: °C alignment might depend on terminal's handling of '°'
  MPRINTF_LINE(cln, "%18s |  %11s  |  %11s  |\r\n", "Drv EN    State", Motor_driver_enable_get(DRIVER_1) ? "ENABLED" : "DISABLED", Motor_driver_enable_get(DRIVER_2) ? "ENABLED" : "DISABLED");
  MPRINTF_LINE(cln, "%18s |  %11s  |  %11s  |\r\n", "Drv FAULT State", MOTOR_DRV1_FAULT_STATE ? "FAULT" : "OK", MOTOR_DRV2_FAULT_STATE ? "FAULT" : "OK");
  MPRINTF_LINE(cln, "                   |---------------|---------------|\r\n");
  // Clear and display common system parameters
  MPRINTF_LINE(cln, "\r\n");
  MPRINTF_LINE(cln, "--- System Parameters ---\r\n");
  MPRINTF_LINE(cln, "%15s = %8.3f V\r\n", "24V Input", adc.v24v_supply);
  MPRINTF_LINE(cln, "%15s = %8.3f V\r\n", "5V Supply", adc.v5v_supply);
  MPRINTF_LINE(cln, "%15s = %8.3f V\r\n", "3.3V Supply", adc.v3v3_supply);
  MPRINTF_LINE(cln, "%15s = %8.1f C\r\n", "CPU Temperature", adc.cpu_temp);
  MPRINTF_LINE(cln, "\r\n");
  MPRINTF_LINE(cln, "ACTIVE MOTOR: %u, PWM: %3u%%, ALGORITHM: %s\r\n", g_active_motor_num, g_active_motor_pwm, _Get_algorithm_str(g_current_algorithm));
  // Clear and display menu
  MPRINTF_LINE(cln, "\r\n");
  MPRINTF_LINE(cln, "=== MOTOR CONTROL COMMANDS ===\r\n");
  MPRINTF_LINE(cln, " [1-4]    : Start M1-M4 FWD    [Q/W/E/R]   : Start M1-M4 REV       [SPACE]     : Emergency Stop All\r\n");
  MPRINTF_LINE(cln, " [A/S/D/F]: Soft Stop M1-M4    [Z/X/C/V]   : Emergency Stop M1-M4  [G]         : Coast Stop All\r\n");
  MPRINTF_LINE(cln, " [T]      : Toggle Algorithm   [N/M]       : Enable DRV1/DRV2      [Up/Down]   : PWM +/-10%%\r\n");
  MPRINTF_LINE(cln, " [K]      : Calibrate Current  [Left/Right]: Select Active Motor   [ESC]     : Exit\r\n");
  // Return next available line for input operations
  return cln + 1;
}

/*-----------------------------------------------------------------------------------------------------
  Restore EN signal states to their saved values

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Restore_en_signal_states(void)
{
  Motor_driver_enable_set(DRIVER_1, g_saved_drv1_en_state);  // Use centralized function
  Motor_driver_enable_set(DRIVER_2, g_saved_drv2_en_state);  // Use centralized function
  APPLOG("Motor Drivers: EN states restored (DRV1_EN=%u, DRV2_EN=%u)", g_saved_drv1_en_state, g_saved_drv2_en_state);
}

/*-----------------------------------------------------------------------------------------------------
  Get motor status string for display

  Parameters:
    motor_num - motor number (1-4)

  Return:
    String representing motor status (ON/OFF or ERROR)
-----------------------------------------------------------------------------------------------------*/
static const char* _Get_motor_status_str(uint8_t motor_num)
{
  T_motor_extended_state* motor_state = Motor_get_extended_state(motor_num);

  if (motor_state == NULL)
  {
    return "ERR";
  }
  // Check if motor is active (running or in soft start process)
  bool is_active = (motor_state->enabled && motor_state->pwm_level > 0) ||
                   Motor_soft_start_is_active(motor_num);

  if (is_active)
  {
    return "ON ";
  }
  else
  {
    return "OFF";
  }
}

/*-----------------------------------------------------------------------------------------------------
  Get motor direction string for display

  Parameters:
    motor_num - motor number (1-4)

  Return:
    String representing motor direction (FWD/REV/STP/ERR)
-----------------------------------------------------------------------------------------------------*/
static const char* _Get_motor_direction_str(uint8_t motor_num)
{
  T_motor_extended_state* motor_state = Motor_get_extended_state(motor_num);

  if (motor_state == NULL)
  {
    return "ERR";
  }

  if (motor_state->direction == MOTOR_DIRECTION_FORWARD)
  {
    return "FWD";
  }
  else if (motor_state->direction == MOTOR_DIRECTION_REVERSE)
  {
    return "REV";
  }
  else
  {
    return "STOP";
  }
}

/*-----------------------------------------------------------------------------------------------------
  Get phase state string for display

  Parameters:
    motor_driver - motor driver number (1 or 2)
    phase - phase identifier (PHASE_U, PHASE_V, PHASE_W)
    motor_num - motor number (1-4) to check if this phase is active for this motor

  Return:
    String representing phase state (PWM/24V/0V/Z)
-----------------------------------------------------------------------------------------------------*/
static const char* _Get_phase_state_str(uint8_t motor_driver, uint8_t phase, uint8_t motor_num)
{
  // Convert motor_driver and phase to array indices
  uint8_t motor_index = motor_driver - 1;  // Convert 1-based to 0-based index
  uint8_t phase_index = phase - 1;         // Convert 1-based to 0-based index

  // Check array bounds
  if (motor_index >= DRIVER_COUNT || phase_index >= PHASE_COUNT)
  {
    return "Z";  // Invalid indices
  }

  // Always check real hardware phase state first, regardless of motor logical state
  // This ensures correct display even after emergency stop when phases may be shorted for braking
  if (g_phase_state_0_percent[motor_index][phase_index])
  {
    return "0V";  // 0% duty state: lower switch ON, upper switch OFF
  }
  else if (g_phase_state_100_percent[motor_index][phase_index])
  {
    return "24V";  // 100% duty state: upper switch ON, lower switch OFF
  }
  else if (g_phase_state_pwm_mode[motor_index][phase_index])
  {
    return "PWM";  // Normal PWM operation
  }
  else if (g_phase_state_Z_stage[motor_index][phase_index])
  {
    return "Z";  // Z-state: both switches OFF (high impedance)
  }
  else
  {
    return "Z";  // Unknown state or both switches OFF
  }
}

/*-----------------------------------------------------------------------------------------------------
  Get algorithm string for display

  Parameters:
    algorithm - algorithm type

  Return:
    String representing algorithm (INSTANT/LINEAR/S-CURVE)
-----------------------------------------------------------------------------------------------------*/
static const char* _Get_algorithm_str(T_soft_start_algorithm algorithm)
{
  switch (algorithm)
  {
    case SOFT_START_ALG_INSTANT:
      return "INSTANT";
    case SOFT_START_ALG_LINEAR:
      return "LINEAR";
    case SOFT_START_ALG_S_CURVE:
      return "S-CURVE";
    default:
      return "UNKNOWN";
  }
}

/*-----------------------------------------------------------------------------------------------------
  Toggle between hard and soft mode

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Toggle_algorithm(void)
{
  // Cycle through algorithms: INSTANT -> LINEAR -> S_CURVE -> INSTANT
  if (g_current_algorithm == SOFT_START_ALG_INSTANT)
  {
    g_current_algorithm = SOFT_START_ALG_LINEAR;
  }
  else if (g_current_algorithm == SOFT_START_ALG_LINEAR)
  {
    g_current_algorithm = SOFT_START_ALG_S_CURVE;
  }
  else
  {
    g_current_algorithm = SOFT_START_ALG_INSTANT;
  }

  APPLOG("Motor algorithm switched to: %s", _Get_algorithm_str(g_current_algorithm));
}

/*-----------------------------------------------------------------------------------------------------
  Motor Diagnostic menu entry point.

  Parameters:
    keycode   Not used.

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Diagnostic_Motor(uint8_t keycode)
{
  GET_MCBL;
  if (!Get_motor_driver_ready())
  {
    MPRINTF("\r\n[ERROR] Motor driver task is not running or initialization failed.\r\n");
    MPRINTF("\r\nReturning to previous menu in 2 seconds...\r\n");
    tx_thread_sleep(ms_to_ticks(2000));
    return;
  }
  // Save current EN signal states before entering diagnostic mode
  g_saved_drv1_en_state = Motor_driver_enable_get(DRIVER_1);  // Use centralized function
  g_saved_drv2_en_state = Motor_driver_enable_get(DRIVER_2);  // Use centralized function
  APPLOG("Motor Drivers: EN states saved (DRV1_EN=%u, DRV2_EN=%u)", g_saved_drv1_en_state, g_saved_drv2_en_state);

  // Disable CAN command processing during motor diagnostic
  Disable_can_command_processing();
  // Turn off all motors and reset PWM when entering diagnostic mode
  Motor_command_coast(0);   // 0 = all motors
  Motor_driver_enable_set(DRIVER_1, 0);  // Disable motor driver 1 using centralized function
  Motor_driver_enable_set(DRIVER_2, 0);  // Disable motor driver 2 using centralized function
  g_active_motor_num = 1;   // Set active motor to 1
  g_active_motor_pwm = 50;  // Set initial PWM to 50% for testing  // Initialize preset motor parameters
  for (uint8_t i = 0; i < 4; i++)
  {
    if (i == (g_active_motor_num - 1))
    {
      g_preset_motor_pwm[i] = g_active_motor_pwm;  // Set active motor preset to current PWM
    }
    else
    {
      g_preset_motor_pwm[i] = 0;  // Set others to 0%
    }

    // Set initial algorithm for each motor
    Motor_soft_start_set_algorithm(i + 1, g_current_algorithm);
  }

  APPLOG("All motors turned off automatically when entering diagnostic mode");

  // Display header once at menu entry
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF(MOTOR_DIAG_HEADER);
  uint8_t b = 0;
  (void)_Motor_diag_print_adc();  // Ignore return value for initial display

  while (1)
  {
    if (VT100_wait_special_key(&b, ms_to_ticks(MOTOR_DIAG_REFRESH_MS)) == RES_OK)
    {
      switch (b)
      {
        // Forward direction start commands
        case MOTOR_KEY_MOTOR_1_FORWARD:
          Motor_soft_start_set_algorithm(MOTOR_1_, g_current_algorithm);
          uint16_t motor1_pwm;
          if (g_active_motor_num == 1)
          {
            motor1_pwm = g_active_motor_pwm;
          }
          else
          {
            motor1_pwm = g_preset_motor_pwm[0];
          }
          Motor_command_soft_start(MOTOR_1_, motor1_pwm, MOTOR_DIRECTION_FORWARD);
          APPLOG("Motor 1 started FORWARD by user command '1'");
          break;
        case MOTOR_KEY_MOTOR_2_FORWARD:
          Motor_soft_start_set_algorithm(MOTOR_2_, g_current_algorithm);
          uint16_t motor2_pwm;
          if (g_active_motor_num == 2)
          {
            motor2_pwm = g_active_motor_pwm;
          }
          else
          {
            motor2_pwm = g_preset_motor_pwm[1];
          }
          Motor_command_soft_start(MOTOR_2_, motor2_pwm, MOTOR_DIRECTION_FORWARD);
          APPLOG("Motor 2 started FORWARD by user command '2'");
          break;
        case MOTOR_KEY_MOTOR_3_FORWARD:
          Motor_soft_start_set_algorithm(MOTOR_3_, g_current_algorithm);
          uint16_t motor3_pwm;
          if (g_active_motor_num == 3)
          {
            motor3_pwm = g_active_motor_pwm;
          }
          else
          {
            motor3_pwm = g_preset_motor_pwm[2];
          }
          Motor_command_soft_start(MOTOR_3_, motor3_pwm, MOTOR_DIRECTION_FORWARD);
          APPLOG("Motor 3 started FORWARD by user command '3'");
          break;
        case MOTOR_KEY_MOTOR_4_FORWARD:
          Motor_soft_start_set_algorithm(MOTOR_4_, g_current_algorithm);
          uint16_t motor4_pwm;
          if (g_active_motor_num == 4)
          {
            motor4_pwm = g_active_motor_pwm;
          }
          else
          {
            motor4_pwm = g_preset_motor_pwm[3];
          }
          Motor_command_soft_start(MOTOR_4_, motor4_pwm, MOTOR_DIRECTION_FORWARD);
          APPLOG("Motor 4 started FORWARD by user command '4'");
          break;

        // Reverse direction start commands
        case MOTOR_KEY_MOTOR_1_REVERSE_LOWER:  // Accept lowercase for convenience
        case MOTOR_KEY_MOTOR_1_REVERSE:
          Motor_soft_start_set_algorithm(MOTOR_1_, g_current_algorithm);
          uint16_t motor1_rev_pwm;
          if (g_active_motor_num == 1)
          {
            motor1_rev_pwm = g_active_motor_pwm;
          }
          else
          {
            motor1_rev_pwm = g_preset_motor_pwm[0];
          }
          Motor_command_soft_start(MOTOR_1_, motor1_rev_pwm, MOTOR_DIRECTION_REVERSE);
          APPLOG("Motor 1 started REVERSE by user command 'Q'");
          break;
        case MOTOR_KEY_MOTOR_2_REVERSE_LOWER:  // Accept lowercase for convenience
        case MOTOR_KEY_MOTOR_2_REVERSE:
          Motor_soft_start_set_algorithm(MOTOR_2_, g_current_algorithm);
          uint16_t motor2_rev_pwm;
          if (g_active_motor_num == 2)
          {
            motor2_rev_pwm = g_active_motor_pwm;
          }
          else
          {
            motor2_rev_pwm = g_preset_motor_pwm[1];
          }
          Motor_command_soft_start(MOTOR_2_, motor2_rev_pwm, MOTOR_DIRECTION_REVERSE);
          APPLOG("Motor 2 started REVERSE by user command 'W'");
          break;
        case MOTOR_KEY_MOTOR_3_REVERSE_LOWER:  // Accept lowercase for convenience
        case MOTOR_KEY_MOTOR_3_REVERSE:
          Motor_soft_start_set_algorithm(MOTOR_3_, g_current_algorithm);
          uint16_t motor3_rev_pwm;
          if (g_active_motor_num == 3)
          {
            motor3_rev_pwm = g_active_motor_pwm;
          }
          else
          {
            motor3_rev_pwm = g_preset_motor_pwm[2];
          }
          Motor_command_soft_start(MOTOR_3_, motor3_rev_pwm, MOTOR_DIRECTION_REVERSE);
          APPLOG("Motor 3 started REVERSE by user command 'E'");
          break;
        case MOTOR_KEY_MOTOR_4_REVERSE_LOWER:  // Accept lowercase for convenience
        case MOTOR_KEY_MOTOR_4_REVERSE:
          Motor_soft_start_set_algorithm(MOTOR_4_, g_current_algorithm);
          uint16_t motor4_rev_pwm;
          if (g_active_motor_num == 4)
          {
            motor4_rev_pwm = g_active_motor_pwm;
          }
          else
          {
            motor4_rev_pwm = g_preset_motor_pwm[3];
          }
          Motor_command_soft_start(MOTOR_4_, motor4_rev_pwm, MOTOR_DIRECTION_REVERSE);
          APPLOG("Motor 4 started REVERSE by user command 'R'");
          break;

        // Soft stop commands
        case MOTOR_KEY_MOTOR_1_SOFT_STOP_LOWER:  // Accept lowercase for convenience
        case MOTOR_KEY_MOTOR_1_SOFT_STOP:
          Motor_command_soft_stop(MOTOR_1_);
          APPLOG("Motor 1 soft stop by user command 'A'");
          break;
        case MOTOR_KEY_MOTOR_2_SOFT_STOP_LOWER:  // Accept lowercase for convenience
        case MOTOR_KEY_MOTOR_2_SOFT_STOP:
          Motor_command_soft_stop(MOTOR_2_);
          APPLOG("Motor 2 soft stop by user command 'S'");
          break;
        case MOTOR_KEY_MOTOR_3_SOFT_STOP_LOWER:  // Accept lowercase for convenience
        case MOTOR_KEY_MOTOR_3_SOFT_STOP:
          Motor_command_soft_stop(MOTOR_3_);
          APPLOG("Motor 3 soft stop by user command 'D'");
          break;
        case MOTOR_KEY_MOTOR_4_SOFT_STOP_LOWER:  // Accept lowercase for convenience
        case MOTOR_KEY_MOTOR_4_SOFT_STOP:
          Motor_command_soft_stop(MOTOR_4_);
          APPLOG("Motor 4 soft stop by user command 'F'");
          break;
        case MOTOR_KEY_EMERGENCY_STOP:
          Motor_emergency_stop_direct(0);                      // Direct emergency stop all motors (0 = all motors)
          APPLOG("EMERGENCY STOP executed by user command 'SPACE' - all motors stopped immediately (direct)");
          break;
        case MOTOR_KEY_EMERGENCY_STOP_1_LOWER:                 // Accept lowercase for convenience
        case MOTOR_KEY_EMERGENCY_STOP_1:
          Motor_emergency_stop_direct(MOTOR_1_);       // Emergency stop Motor 1 only
          APPLOG("EMERGENCY STOP Motor 1 executed by user command 'Z'");
          break;
        case MOTOR_KEY_EMERGENCY_STOP_2_LOWER:                 // Accept lowercase for convenience
        case MOTOR_KEY_EMERGENCY_STOP_2:
          Motor_emergency_stop_direct(MOTOR_2_);  // Emergency stop Motor 2 only
          APPLOG("EMERGENCY STOP Motor 2 executed by user command 'X'");
          break;
        case MOTOR_KEY_EMERGENCY_STOP_3_LOWER:                 // Accept lowercase for convenience
        case MOTOR_KEY_EMERGENCY_STOP_3:
          Motor_emergency_stop_direct(MOTOR_3_);   // Emergency stop Motor 3 only
          APPLOG("EMERGENCY STOP Motor 3 executed by user command 'C'");
          break;
        case MOTOR_KEY_EMERGENCY_STOP_4_LOWER:                 // Accept lowercase for convenience
        case MOTOR_KEY_EMERGENCY_STOP_4:
          Motor_emergency_stop_direct(MOTOR_4_);  // Emergency stop Motor 4 only
          APPLOG("EMERGENCY STOP Motor 4 executed by user command 'V'");
          break;
        case MOTOR_KEY_COAST_ALL_LOWER:                        // Accept lowercase for convenience
        case MOTOR_KEY_COAST_ALL:
          Motor_command_coast(0);                              // Coast all motors (0 = all motors)
          APPLOG("COAST ALL motors executed by user command 'G'");
          break;
        case MOTOR_KEY_TOGGLE_ALGORITHM_LOWER:                 // Accept lowercase for convenience
        case MOTOR_KEY_TOGGLE_ALGORITHM:
          _Toggle_algorithm();
          break;
        case MOTOR_KEY_TOGGLE_DRV1_EN_LOWER:     // Accept lowercase for convenience
        case MOTOR_KEY_TOGGLE_DRV1_EN:
          Motor_driver_enable_toggle(DRIVER_1);  // Use centralized function
          APPLOG("MOTOR_DRV1_EN toggled by user command 'N'");
          break;
        case MOTOR_KEY_TOGGLE_DRV2_EN_LOWER:     // Accept lowercase for convenience
        case MOTOR_KEY_TOGGLE_DRV2_EN:
          Motor_driver_enable_toggle(DRIVER_2);  // Use centralized function
          APPLOG("MOTOR_DRV2_EN toggled by user command 'M'");
          break;
        case MOTOR_KEY_CALIBRATE_CURRENT_LOWER:                 // Accept lowercase for convenience
        case MOTOR_KEY_CALIBRATE_CURRENT:
          _Handle_calibration_request();  // Trigger motor current offset calibration via event
          break;
        case VT100_UP_ARROW:
          if (g_active_motor_pwm <= 90)
          {
            g_active_motor_pwm += 10;
          }
          else
          {
            g_active_motor_pwm = 100;
          }  // Update preset PWM for active motor
          g_preset_motor_pwm[g_active_motor_num - 1] = g_active_motor_pwm;
          // Note: Real-time PWM update is no longer supported due to removed commands
          APPLOG("Active motor %d PWM increased to %d%%", g_active_motor_num, g_active_motor_pwm);
          break;
        case VT100_DOWN_ARROW:
          if (g_active_motor_pwm >= 10)
          {
            g_active_motor_pwm -= 10;
          }
          else
          {
            g_active_motor_pwm = 0;
          }  // Update preset PWM for active motor
          g_preset_motor_pwm[g_active_motor_num - 1] = g_active_motor_pwm;
          // Note: Real-time PWM update is no longer supported due to removed commands
          APPLOG("Active motor %d PWM decreased to %d%%", g_active_motor_num, g_active_motor_pwm);
          break;
        case VT100_RIGHT_ARROW:
          g_active_motor_num++;
          if (g_active_motor_num > 4)
          {
            g_active_motor_num = 1;
          }
          // Update active motor PWM from preset when switching motors
          g_active_motor_pwm = g_preset_motor_pwm[g_active_motor_num - 1];
          APPLOG("Active motor changed to %d, PWM set to %d%%", g_active_motor_num, g_active_motor_pwm);
          break;
        case VT100_LEFT_ARROW:
          g_active_motor_num--;
          if (g_active_motor_num < 1)
          {
            g_active_motor_num = 4;
          }
          // Update active motor PWM from preset when switching motors
          g_active_motor_pwm = g_preset_motor_pwm[g_active_motor_num - 1];
          APPLOG("Active motor changed to %d, PWM set to %d%%", g_active_motor_num, g_active_motor_pwm);
          break;
        case MOTOR_KEY_EXIT:
          // Turn off all motors before exiting
          Motor_command_coast(0);

          // Restore saved EN signal states
          _Restore_en_signal_states();

          // Restore CAN command processing
          Restore_can_command_processing();

          APPLOG("Exiting motor diagnostic, all motors OFF, EN states restored, CAN command processing restored.");
          return;
        default:
          break;
      }
      (void)_Motor_diag_print_adc();  // Refresh display after any action
    }
    else
    {
      (void)_Motor_diag_print_adc();  // Refresh display on timeout
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Handle motor current offset calibration request from diagnostic terminal.
  Uses inter-thread communication via event flags to request calibration
  from the Motor_driver_func thread and wait for completion/error.

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Handle_calibration_request(void)
{
  APPLOG("Starting motor current offset calibration...");

  // Request calibration from motor driver thread
  uint32_t request_result = Motor_request_calibration();
  if (request_result != RES_OK)
  {
    APPLOG("ERROR: Failed to request motor calibration (result: %lu)", request_result);
    return;
  }
}
