//----------------------------------------------------------------------
// File created on 2025-06-02
//----------------------------------------------------------------------
#include "App.h"

// Global variables for FreeMaster monitoring of driver enable states
uint8_t g_drv1_en_state                       = 0;  // Driver 1 enable state (0 = disabled, 1 = enabled)
uint8_t g_drv2_en_state                       = 0;  // Driver 2 enable state (0 = disabled, 1 = enabled)

// Global structure for maximum phase current tracking for all 4 motors
T_max_current_tracking g_max_current_tracking = { 0 };

// Individual maximum current variables for easy FreeMaster access
// Motor 1  max currents
float g_max_current_motor1_accel              = 0.0f;  // Motor 1 maximum current during acceleration [A]
float g_max_current_motor1_run                = 0.0f;  // Motor 1 maximum current during constant speed [A]
float g_max_current_motor1_brake              = 0.0f;  // Motor 1 maximum current during braking [A]

// Motor 2  max currents
float g_max_current_motor2_accel              = 0.0f;  // Motor 2 maximum current during acceleration [A]
float g_max_current_motor2_run                = 0.0f;  // Motor 2 maximum current during constant speed [A]
float g_max_current_motor2_brake              = 0.0f;  // Motor 2 maximum current during braking [A]

// Motor 3  max currents
float g_max_current_motor3_accel              = 0.0f;  // Motor 3 maximum current during acceleration [A]
float g_max_current_motor3_run                = 0.0f;  // Motor 3 maximum current during constant speed [A]
float g_max_current_motor3_brake              = 0.0f;  // Motor 3 maximum current during braking [A]

// Motor 4  max currents
float g_max_current_motor4_accel              = 0.0f;  // Motor 4 maximum current during acceleration [A]
float g_max_current_motor4_run                = 0.0f;  // Motor 4 maximum current during constant speed [A]
float g_max_current_motor4_brake              = 0.0f;  // Motor 4 maximum current during braking [A]

// Calibration timing constants and variables
#define CALIBRATION_INTERVAL_TICKS 10000      // 10 seconds at 1ms ticks
static uint32_t g_last_calibration_time = 0;  // Last calibration time stamp

// Calibration state tracking
typedef enum
{
  CALIBRATION_STATE_IDLE,       // No calibration in progress
  CALIBRATION_STATE_REQUESTED,  // Calibration started, waiting for completion
} T_calibration_state;

static T_calibration_state g_calibration_state = CALIBRATION_STATE_IDLE;

// Motor driver thread declarations
TX_THREAD      Motor_driver;
static uint8_t Motor_driver_stack[MOTOR_THREAD_STACK_SIZE] BSP_PLACE_IN_SECTION(".stack.Motor_driver") BSP_ALIGN_VARIABLE(BSP_STACK_ALIGNMENT);

// Global structure for PWM and output state control for all phases
T_pwm_phase_control g_pwm_phase_control;

TX_QUEUE g_motor_command_queue;

// Motor calibration event flag group for inter-thread communication
TX_EVENT_FLAGS_GROUP g_motor_calibration_events;

// State for all 4 motors - using new extended structure (global for FreeMaster access)
T_motor_extended_state g_motor_states[4];

// Global flag for Motor_driver_func readiness
static volatile uint8_t g_motor_driver_ready = 0;

static void     Motor_driver_func(ULONG thread_input);
static void     _Process_motor_commands(void);
static void     _Init_motor_command_queue(void);
static void     _Set_motor_pwm(uint8_t motor_num, uint8_t direction, uint16_t pwm_percent);
static void     _Clear_motor_phases(uint8_t motor_num);
static void     _Set_motor1_traction_pwm(uint8_t direction, uint32_t pwm_steps);
static void     _Set_motor2_pwm(uint8_t direction, uint32_t pwm_steps);
static void     _Set_motor3_pwm(uint8_t direction, uint32_t pwm_steps);
static void     _Set_motor4_pwm(uint8_t direction, uint32_t pwm_steps);
static void     _Stop_motor(uint8_t motor_num);
static uint8_t  _Get_actual_motor_direction(uint8_t motor_num, uint8_t direction);
static uint32_t _Perform_motor_current_offset_calibration(void);
static void     _Set_motor_algorithms_from_parameters(void);
static uint32_t _Clear_motor_commands_from_queue(uint8_t motor_num);
static uint8_t  _Get_paired_motor(uint8_t motor_num);
static uint8_t  _Check_direction_change_for_running_motor(uint8_t motor_num, uint8_t new_direction);
static uint8_t  _Is_motor_in_emergency_stop(uint8_t motor_num);
static void     _Set_motor_dynamic_braking(uint8_t motor_num);
static void     _Update_emergency_stop_motor_shared_phase(uint8_t motor_num, uint32_t new_shared_phase_level);
static uint8_t  _Is_motor_already_executing_command(uint8_t motor_num, uint8_t cmd_type, uint8_t direction);
static void     _Check_overcurrent_overtemperature_protection(void);
static void     _Emergency_stop_all_motors(void);
static void     _Update_driver_enable_states(void);
static void     _Update_max_current_tracking(void);
static uint8_t  _Check_all_motors_off(void);
static uint32_t _Complete_silent_motor_current_offset_calibration(void);
static void     _Handle_periodic_calibration(void);
static void     _Sync_individual_max_current_variables(void);
static void     _Reset_motor_max_currents_on_start(uint8_t motor_num);
static void     _Log_motor_max_currents_on_stop(uint8_t motor_num);
static void     _Check_and_log_stopped_motors_max_currents(void);

/*-----------------------------------------------------------------------------------------------------
  Initialize PWM phase control structure for all motors and phases

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Init_pwm_phase_control(void)
{
  for (uint8_t m = 0; m < DRIVER_COUNT; m++)
  {
    for (uint8_t p = 0; p < PHASE_COUNT; p++)
    {
      g_pwm_phase_control.pwm_level[m][p]    = 0;                     // Initial PWM level in steps (0)
      g_pwm_phase_control.output_state[m][p] = PHASE_OUTPUT_DISABLE;  // Initial output state - switches OFF
    }
  }
  // Initialize motor states
  for (uint8_t i = 0; i < 4; i++)
  {
    g_motor_states[i].direction              = MOTOR_DIRECTION_STOP;
    g_motor_states[i].pwm_level              = 0;
    g_motor_states[i].enabled                = 0;

    // Initialize soft start state
    g_motor_states[i].soft_start_state       = MOTOR_STATE_IDLE;
    g_motor_states[i].target_pwm             = 0;
    g_motor_states[i].current_pwm_x100       = 0;
    g_motor_states[i].step_counter           = 0;
    g_motor_states[i].pwm_step_size_x100     = 0;
    g_motor_states[i].target_direction       = MOTOR_DIRECTION_STOP;
    g_motor_states[i].soft_start_initialized = false;
    g_motor_states[i].conflict_detected      = false;
    g_motor_states[i].run_phase_start_time   = 0;     // Initialize RUN phase start time
    g_motor_states[i].max_current_logged     = true;  // Initially set to true (no movement to log)
  }
}

/*-----------------------------------------------------------------------------------------------------
  Initialize motor command queue.

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Init_motor_command_queue(void)
{
  static T_motor_command queue_buffer[MOTOR_COMMAND_QUEUE_SIZE];

  tx_queue_create(&g_motor_command_queue, (CHAR *)"MotorCmdQ", sizeof(T_motor_command) / sizeof(ULONG), (VOID *)queue_buffer, sizeof(queue_buffer));
}

/*-----------------------------------------------------------------------------------------------------
  Stop specific motor by putting it into coasting mode (free wheeling)

  Parameters:
    motor_num - motor number (1-4)

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Stop_motor(uint8_t motor_num)
{
  if (motor_num < 1 || motor_num > 4)
  {
    return;
  }

  // Update unified motor state
  g_motor_states[motor_num - 1].enabled                = 0;
  g_motor_states[motor_num - 1].pwm_level              = 0;
  g_motor_states[motor_num - 1].direction              = MOTOR_DIRECTION_STOP;
  g_motor_states[motor_num - 1].soft_start_state       = MOTOR_STATE_COASTING;

  // Clear all soft start parameters to prevent interference with future commands
  g_motor_states[motor_num - 1].target_pwm             = 0;
  g_motor_states[motor_num - 1].original_target_pwm    = 0;
  g_motor_states[motor_num - 1].current_pwm_x100       = 0;
  g_motor_states[motor_num - 1].step_counter           = 0;
  g_motor_states[motor_num - 1].target_direction       = MOTOR_DIRECTION_STOP;
  g_motor_states[motor_num - 1].soft_start_initialized = false;
  g_motor_states[motor_num - 1].conflict_detected      = false;
  g_motor_states[motor_num - 1].run_phase_start_time   = 0;  // Reset RUN phase start time
  // Note: max_current_logged flag is NOT reset here - it will be checked in main loop for logging

  // Set corresponding PWM phases to 0 (free wheeling)
  _Set_motor_pwm(motor_num, MOTOR_DIRECTION_STOP, 0);
}

/*-----------------------------------------------------------------------------------------------------
  Set PWM for Motor 1 (Traction) - U1-V1 phases

  Motor dependencies:
  - Motor 1 uses: U1-V1 phases
  - Motor 2 uses: V1-W1 phases (shares V1 with Motor 1)
  - Motors 1 & 2 can work together with V1 restrictions
  New logic implementation:
  - FORWARD: V1 = 0% (static), U1 = effective_pwm
  - REVERSE: V1 = 100% (static), U1 = effective_pwm
  - Motor is disabled via U1 phase, V1 serves direction control only

  Parameters:
    direction - motor direction (MOTOR_DIRECTION_FORWARD/REVERSE)
    pwm_steps - effective PWM level in steps (0-PWM_STEP_COUNT), already inverted for reverse direction

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Set_motor1_traction_pwm(uint8_t direction, uint32_t pwm_steps)
{
  uint8_t motor_2_enabled   = g_motor_states[MOTOR_2_ - 1].enabled;
  uint8_t motor_2_direction = g_motor_states[MOTOR_2_ - 1].direction;

  // Set V1 static level based on direction (0% = FORWARD, 100% = REVERSE)
  uint32_t v1_static_level  = (direction == MOTOR_DIRECTION_FORWARD) ? 0 : PWM_STEP_COUNT;
  // Check if Motor 2 is in emergency stop - if so, update its phases to match new V1 level
  if (_Is_motor_in_emergency_stop(MOTOR_2_))
  {
    // Motor 2 is in emergency stop, update its phases to new V1 level to maintain emergency stop
    _Update_emergency_stop_motor_shared_phase(MOTOR_2_, v1_static_level);
  }

  // Check if Motor 2 is running and has different V1 requirements
  if (motor_2_enabled && motor_2_direction != MOTOR_DIRECTION_STOP)
  {
    // Motor 2 is also using V1 - need to coordinate V1 level
    uint32_t motor_2_required_v1 = (motor_2_direction == MOTOR_DIRECTION_FORWARD) ? 0 : PWM_STEP_COUNT;

    if (v1_static_level != motor_2_required_v1)
    {
      // Conflicting V1 requirements - use Motor 2's V1 level (Motor 2 has priority when both enabled)
      v1_static_level = motor_2_required_v1;
    }
  }

  // Set Motor 1 phases: U1 gets effective_pwm, V1 gets static level for direction
  TX_INTERRUPT_SAVE_AREA

  TX_DISABLE
  g_pwm_phase_control.pwm_level[MOT_1][PH_U]    = pwm_steps;
  g_pwm_phase_control.output_state[MOT_1][PH_U] = PHASE_OUTPUT_ENABLE;
  g_pwm_phase_control.pwm_level[MOT_1][PH_V]    = v1_static_level;
  g_pwm_phase_control.output_state[MOT_1][PH_V] = PHASE_OUTPUT_ENABLE;
  TX_RESTORE
}

/*-----------------------------------------------------------------------------------------------------
  Set PWM for Motor 2 ( Motor 2) - V1-W1 phases

  Motor dependencies:
  - Motor 2 uses: V1-W1 phases
  - Motor 1 uses: U1-V1 phases (shares V1 with Motor 2)
  - Motors 1 & 2 can work together with V1 restrictions
  New logic implementation:
  - FORWARD: V1 = 0% (static), W1 = effective_pwm
  - REVERSE: V1 = 100% (static), W1 = effective_pwm
  - Motor is disabled via W1 phase, V1 serves direction control only

  Parameters:
    direction - motor direction (MOTOR_DIRECTION_FORWARD/REVERSE)
    pwm_steps - effective PWM level in steps (0-PWM_STEP_COUNT), already inverted for reverse direction

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Set_motor2_pwm(uint8_t direction, uint32_t pwm_steps)
{
  uint8_t motor_1_enabled   = g_motor_states[MOTOR_1_ - 1].enabled;
  uint8_t motor_1_direction = g_motor_states[MOTOR_1_ - 1].direction;

  // Set V1 static level based on direction (0% = FORWARD, 100% = REVERSE)
  uint32_t v1_static_level  = (direction == MOTOR_DIRECTION_FORWARD) ? 0 : PWM_STEP_COUNT;
  // Check if Motor 1 is in emergency stop - if so, update its phases to match new V1 level
  if (_Is_motor_in_emergency_stop(MOTOR_1_))
  {
    // Motor 1 is in emergency stop, update its phases to new V1 level to maintain emergency stop
    _Update_emergency_stop_motor_shared_phase(MOTOR_1_, v1_static_level);
  }

  // Check if Motor 1 is running and has different V1 requirements
  if (motor_1_enabled && motor_1_direction != MOTOR_DIRECTION_STOP)
  {
    // Motor 1 is also using V1 - need to coordinate V1 level
    uint32_t motor_1_required_v1 = (motor_1_direction == MOTOR_DIRECTION_FORWARD) ? 0 : PWM_STEP_COUNT;

    if (v1_static_level != motor_1_required_v1)
    {
      // Conflicting V1 requirements - use Motor 1's V1 level (Motor 1 has priority when both enabled)
      v1_static_level = motor_1_required_v1;
    }
  }

  // Set Motor 2 phases: W1 gets effective_pwm, V1 gets static level for direction
  TX_INTERRUPT_SAVE_AREA

  TX_DISABLE
  g_pwm_phase_control.pwm_level[MOT_1][PH_V]    = v1_static_level;
  g_pwm_phase_control.output_state[MOT_1][PH_V] = PHASE_OUTPUT_ENABLE;
  g_pwm_phase_control.pwm_level[MOT_1][PH_W]    = pwm_steps;
  g_pwm_phase_control.output_state[MOT_1][PH_W] = PHASE_OUTPUT_ENABLE;
  TX_RESTORE
}

/*-----------------------------------------------------------------------------------------------------
  Set PWM for Motor 3 ( Motor 3) - U2-V2 phases

  Motor dependencies:
  - Motor 3 uses: U2-V2 phases
  - Motor 4 uses: V2-W2 phases (shares V2 with Motor 3)
  - Motors 3 & 4 can work together with V2 restrictions
  New logic implementation:
  - FORWARD: V2 = 0% (static), U2 = effective_pwm
  - REVERSE: V2 = 100% (static), U2 = effective_pwm
  - Motor is disabled via U2 phase, V2 serves direction control only

  Parameters:
    direction - motor direction (MOTOR_DIRECTION_FORWARD/REVERSE)
    pwm_steps - effective PWM level in steps (0-PWM_STEP_COUNT), already inverted for reverse direction

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Set_motor3_pwm(uint8_t direction, uint32_t pwm_steps)
{
  uint8_t motor_4_enabled   = g_motor_states[MOTOR_4_ - 1].enabled;
  uint8_t motor_4_direction = g_motor_states[MOTOR_4_ - 1].direction;

  // Set V2 static level based on direction (0% = FORWARD, 100% = REVERSE)
  uint32_t v2_static_level  = (direction == MOTOR_DIRECTION_FORWARD) ? 0 : PWM_STEP_COUNT;
  // Check if Motor 4 is in emergency stop - if so, update its phases to match new V2 level
  if (_Is_motor_in_emergency_stop(MOTOR_4_))
  {
    // Motor 4 is in emergency stop, update its phases to new V2 level to maintain emergency stop
    _Update_emergency_stop_motor_shared_phase(MOTOR_4_, v2_static_level);
  }

  // Check if Motor 4 is running and has different V2 requirements
  if (motor_4_enabled && motor_4_direction != MOTOR_DIRECTION_STOP)
  {
    // Motor 4 is also using V2 - need to coordinate V2 level
    uint32_t motor_4_required_v2 = (motor_4_direction == MOTOR_DIRECTION_FORWARD) ? 0 : PWM_STEP_COUNT;

    if (v2_static_level != motor_4_required_v2)
    {
      // Conflicting V2 requirements - use Motor 4's V2 level (Motor 4 has priority when both enabled)
      v2_static_level = motor_4_required_v2;
    }
  }

  // Set Motor 3 phases: U2 gets effective_pwm, V2 gets static level for direction
  TX_INTERRUPT_SAVE_AREA

  TX_DISABLE
  g_pwm_phase_control.pwm_level[MOT_2][PH_U]    = pwm_steps;
  g_pwm_phase_control.output_state[MOT_2][PH_U] = PHASE_OUTPUT_ENABLE;
  g_pwm_phase_control.pwm_level[MOT_2][PH_V]    = v2_static_level;
  g_pwm_phase_control.output_state[MOT_2][PH_V] = PHASE_OUTPUT_ENABLE;
  TX_RESTORE
}

/*-----------------------------------------------------------------------------------------------------
  Set PWM for Motor 4 ( Motor 2) - V2-W2 phases

  Motor dependencies:
  - Motor 4 uses: V2-W2 phases
  - Motor 3 uses: U2-V2 phases (shares V2 with Motor 4)
  - Motors 3 & 4 can work together with V2 restrictions
  New logic implementation:
  - FORWARD: V2 = 0% (static), W2 = effective_pwm
  - REVERSE: V2 = 100% (static), W2 = effective_pwm
  - Motor is disabled via W2 phase, V2 serves direction control only

  Parameters:
    direction - motor direction (MOTOR_DIRECTION_FORWARD/REVERSE)
    pwm_steps - effective PWM level in steps (0-PWM_STEP_COUNT), already inverted for reverse direction

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Set_motor4_pwm(uint8_t direction, uint32_t pwm_steps)
{
  uint8_t motor_3_enabled   = g_motor_states[MOTOR_3_ - 1].enabled;
  uint8_t motor_3_direction = g_motor_states[MOTOR_3_ - 1].direction;

  // Set V2 static level based on direction (0% = FORWARD, 100% = REVERSE)
  uint32_t v2_static_level  = (direction == MOTOR_DIRECTION_FORWARD) ? 0 : PWM_STEP_COUNT;
  // Check if Motor 3 is in emergency stop - if so, update its phases to match new V2 level
  if (_Is_motor_in_emergency_stop(MOTOR_3_))
  {
    // Motor 3 is in emergency stop, update its phases to new V2 level to maintain emergency stop
    _Update_emergency_stop_motor_shared_phase(MOTOR_3_, v2_static_level);
  }

  // Check if Motor 3 is running and has different V2 requirements
  if (motor_3_enabled && motor_3_direction != MOTOR_DIRECTION_STOP)
  {
    // Motor 3 is also using V2 - need to coordinate V2 level
    uint32_t motor_3_required_v2 = (motor_3_direction == MOTOR_DIRECTION_FORWARD) ? 0 : PWM_STEP_COUNT;

    if (v2_static_level != motor_3_required_v2)
    {
      // Conflicting V2 requirements - use Motor 3's V2 level (Motor 3 has priority when both enabled)
      v2_static_level = motor_3_required_v2;
    }
  }

  // Set Motor 4 phases: W2 gets effective_pwm, V2 gets static level for direction
  TX_INTERRUPT_SAVE_AREA

  TX_DISABLE
  g_pwm_phase_control.pwm_level[MOT_2][PH_V]    = v2_static_level;
  g_pwm_phase_control.output_state[MOT_2][PH_V] = PHASE_OUTPUT_ENABLE;
  g_pwm_phase_control.pwm_level[MOT_2][PH_W]    = pwm_steps;
  g_pwm_phase_control.output_state[MOT_2][PH_W] = PHASE_OUTPUT_ENABLE;
  TX_RESTORE
}

/*-----------------------------------------------------------------------------------------------------
  Clear motor phases when stopping motor
  Motor phase connections and dependencies:
  - Motor 1 uses: U1-V1 phases (MD1 driver)
  - Motor 2 uses: V1-W1 phases (MD1 driver, shares V1 with Motor 1)
  - Motor 3 uses: U2-V2 phases (MD2 driver)
  - Motor 4 uses: V2-W2 phases (MD2 driver, shares V2 with Motor 3)

  Shared phase management (configurable by macros):
  - V1 shared between Motors 1 & 2 (can work together or mutually exclusive)
  - V2 shared between Motors 3 & 4 (can work together or mutually exclusive)

  Parameters:
    motor_num - motor number (1-4)

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Clear_motor_phases(uint8_t motor_num)
{  // Clear phases selectively based on motor dependencies
  if (motor_num == MOTOR_1_ || motor_num == MOTOR_2_)
  {
    // Motors 1 & 2 use MD1 phases (U1, V1, W1)
    // Special handling for shared V1 phase

    uint8_t both_motors_enabled = (g_motor_states[MOTOR_1_ - 1].enabled && g_motor_states[MOTOR_2_ - 1].enabled);
    if (both_motors_enabled && motor_num == MOTOR_1_)
    {
      // Motor 1 is being stopped and Motor 2 is also enabled
      // Only clear U1, keep V1 and W1 as they may be used by Motor 2
      TX_INTERRUPT_SAVE_AREA

      TX_DISABLE
      g_pwm_phase_control.pwm_level[MOT_1][PH_U]    = 0;
      g_pwm_phase_control.output_state[MOT_1][PH_U] = PHASE_OUTPUT_DISABLE;
      TX_RESTORE
    }
    else if (both_motors_enabled && motor_num == MOTOR_2_)
    {
      // Motor 2 is being stopped and Motor 1 is also enabled
      // Only clear W1, keep U1 and V1 as they may be used by Motor 1
      TX_INTERRUPT_SAVE_AREA

      TX_DISABLE
      g_pwm_phase_control.pwm_level[MOT_1][PH_W]    = 0;
      g_pwm_phase_control.output_state[MOT_1][PH_W] = PHASE_OUTPUT_DISABLE;
      TX_RESTORE
    }
    else
    {
      // Only one motor is enabled, clear all MD1 phases
      TX_INTERRUPT_SAVE_AREA

      TX_DISABLE
      for (uint8_t phase = 0; phase < PHASE_COUNT; phase++)
      {
        g_pwm_phase_control.pwm_level[MOT_1][phase]    = 0;
        g_pwm_phase_control.output_state[MOT_1][phase] = PHASE_OUTPUT_DISABLE;
      }
      TX_RESTORE
    }
  }
  else if (motor_num == MOTOR_3_ || motor_num == MOTOR_4_)
  {
    // Motors 3 & 4 use MD2 phases (U2, V2, W2)
    // Special handling for shared V2 phase

    uint8_t both_motors_enabled = (g_motor_states[MOTOR_3_ - 1].enabled && g_motor_states[MOTOR_4_ - 1].enabled);
    if (both_motors_enabled && motor_num == MOTOR_3_)
    {
      // Motor 3 is being stopped and Motor 4 is also enabled
      // Only clear U2, keep V2 and W2 as they may be used by Motor 4
      TX_INTERRUPT_SAVE_AREA

      TX_DISABLE
      g_pwm_phase_control.pwm_level[MOT_2][PH_U]    = 0;
      g_pwm_phase_control.output_state[MOT_2][PH_U] = PHASE_OUTPUT_DISABLE;
      TX_RESTORE
    }
    else if (both_motors_enabled && motor_num == MOTOR_4_)
    {
      // Motor 4 is being stopped and Motor 3 is also enabled
      // Only clear W2, keep V2 and U2 as they may be used by Motor 3
      TX_INTERRUPT_SAVE_AREA

      TX_DISABLE
      g_pwm_phase_control.pwm_level[MOT_2][PH_W]    = 0;
      g_pwm_phase_control.output_state[MOT_2][PH_W] = PHASE_OUTPUT_DISABLE;
      TX_RESTORE
    }
    else
    {  // Only one motor is enabled, clear all MD2 phases
      TX_INTERRUPT_SAVE_AREA

      TX_DISABLE
      for (uint8_t phase = 0; phase < PHASE_COUNT; phase++)
      {
        g_pwm_phase_control.pwm_level[MOT_2][phase]    = 0;
        g_pwm_phase_control.output_state[MOT_2][phase] = PHASE_OUTPUT_DISABLE;
      }
      TX_RESTORE
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Set PWM for specific motor with direction and conflict management.
  This function implements new two-phase motor control logic for 4 DC motors connected to 6 PWM phases:
  - Motor 1 : U1-V1 phases, uses MD1 driver
  - Motor 2 : V1-W1 phases, uses MD1 driver
  - Motor 3 : U2-V2 phases, uses MD2 driver
  - Motor 4 : V2-W2 phases, uses MD2 driver

  New motor control principle (implemented):
  - Direction is controlled by shared phase Vx static level: 0% = FORWARD, 100% = REVERSE
  - Motor PWM is applied to motor's own phase only (U1, W1, U2, W2)
  - PWM inversion for reverse rotation is handled in _Process_motor_commands()
  - effective_pwm = original_pwm for FORWARD, effective_pwm = 100-original_pwm for REVERSE

  Motor phase assignment with new logic:
  Motor 1 (U1-V1): FORWARD=V1(0%)+U1(effective_pwm), REVERSE=V1(100%)+U1(effective_pwm)
  Motor 2 (V1-W1): FORWARD=V1(0%)+W1(effective_pwm), REVERSE=V1(100%)+W1(effective_pwm)
  Motor 3 (U2-V2): FORWARD=V2(0%)+U2(effective_pwm), REVERSE=V2(100%)+U2(effective_pwm)
  Motor 4 (V2-W2): FORWARD=V2(0%)+W2(effective_pwm), REVERSE=V2(100%)+W2(effective_pwm)

  Parameters:
    motor_num   - motor number (1-4)
    direction   - motor direction (MOTOR_DIRECTION_FORWARD/REVERSE/STOP)
    pwm_percent - effective PWM level in percent (0-100), already inverted for reverse in caller

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Set_motor_pwm(uint8_t motor_num, uint8_t direction, uint16_t pwm_percent)
{
  if (motor_num < 1 || motor_num > 4 || pwm_percent > 100)
  {
    return;
  }

  // Convert percentage to PWM steps
  uint32_t pwm_steps = (pwm_percent * PWM_STEP_COUNT) / 100;

  // If motor is being stopped, clear all relevant phases
  if (direction == MOTOR_DIRECTION_STOP)
  {
    _Clear_motor_phases(motor_num);
    return;  // Motor stopped, phases cleared
  }
  // Set PWM based on motor number
  switch (motor_num)
  {
    case MOTOR_1_:
      _Set_motor1_traction_pwm(direction, pwm_steps);
      break;

    case MOTOR_2_:
      _Set_motor2_pwm(direction, pwm_steps);
      break;

    case MOTOR_3_:
      _Set_motor3_pwm(direction, pwm_steps);
      break;

    case MOTOR_4_:
      _Set_motor4_pwm(direction, pwm_steps);
      break;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Process motor commands from the command queue.
  4 DC motors are connected to 6 PWM phases on the board:

  Motor 1: Connected between U1-V1
  Motor 2: Connected between V1-W1
  Motor 3: Connected between U2-V2
  Motor 4: Connected between V2-W2

  Motor dependencies (configurable by macros):
  - Motors 1 & 2: Can work together or be mutually exclusive (MOTORS_1_2_MUTUALLY_EXCLUSIVE)
  - Motors 3 & 4: Can work together or be mutually exclusive (MOTORS_3_4_MUTUALLY_EXCLUSIVE)
  - When motors sharing a phase run together, shared phase signal is managed by the first motor

  Motor rotation logic (IMPLEMENTED):
  - Motor direction is controlled by switching the static state of shared phase Vx  - If shared phase state = 0 (PWM level = 0%), then forward rotation
  - If shared phase state = 1 (PWM level = 100%), then reverse rotation
  - Motor is disabled only by turning off its own phase - Ux or Wx
  - For reverse rotation, PWM level is inverted: effective_pwm = 100 - original_pwm
  PWM inversion implementation:
  - Direction inversion (motor_x_direction_invert flag) is applied before PWM inversion
  - For FORWARD direction: effective_pwm = original_pwm
  - For REVERSE direction: effective_pwm = 100 - original_pwm
  - This ensures proper motor rotation according to shared phase control logic and user inversion preferences

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Process_motor_commands(void)
{
  T_motor_command cmd;
  UINT            queue_status;

  // Process motor commands from queue
  queue_status = tx_queue_receive(&g_motor_command_queue, &cmd, TX_NO_WAIT);
  if (queue_status == TX_SUCCESS)
  {
    switch (cmd.cmd_type)
    {
      case MOTOR_CMD_COAST:
        if (cmd.motor_num >= 1 && cmd.motor_num <= 4)
        {
          // Check if motor is already executing the same command - ignore duplicate
          if (_Is_motor_already_executing_command(cmd.motor_num, cmd.cmd_type, MOTOR_DIRECTION_STOP))
          {
            APPLOG("Motor %u (%s) COAST command ignored - motor already coasting", (unsigned int)cmd.motor_num, Get_motor_name(cmd.motor_num));
            break;                     // Ignore duplicate command
          }

          _Stop_motor(cmd.motor_num);  // Motor driver EN signals are controlled from terminal
          APPLOG("Motor %u (%s) COAST command executed", (unsigned int)cmd.motor_num, Get_motor_name(cmd.motor_num));
        }
        else if (cmd.motor_num == 0)
        {
          // Stop all motors
          for (uint8_t i = 1; i <= 4; i++)
          {
            _Stop_motor(i);
          }  // Motor driver EN signals are controlled from terminal
          APPLOG("All motors COAST command executed");
        }
        break;
      case MOTOR_CMD_SOFT_START:
        if (cmd.motor_num >= 1 && cmd.motor_num <= 4)
        {
          // Get actual direction considering inversion flag
          uint8_t actual_direction = _Get_actual_motor_direction(cmd.motor_num, cmd.direction);

          // Check if motor is already executing the same command - ignore duplicate
          if (_Is_motor_already_executing_command(cmd.motor_num, cmd.cmd_type, actual_direction))
          {
            APPLOG("Motor %u (%s) SOFT_START command ignored - motor already doing soft start to %u%% in direction %u",
                   (unsigned int)cmd.motor_num, Get_motor_name(cmd.motor_num),
                   (unsigned int)cmd.pwm_level, (unsigned int)actual_direction);
            break;  // Ignore duplicate command
          }

          // Check if running motor is receiving direction change command - if so, initiate soft stop instead
          if (_Check_direction_change_for_running_motor(cmd.motor_num, actual_direction))
          {
            APPLOG("Motor %u (%s) SOFT_START command with direction change detected - initiating soft stop instead (current: %u, requested: %u)",
                   (unsigned int)cmd.motor_num, Get_motor_name(cmd.motor_num),
                   (unsigned int)g_motor_states[cmd.motor_num - 1].direction, (unsigned int)actual_direction);
            Motor_command_soft_stop(cmd.motor_num);  // Initiate soft stop instead of direction change
            break;
          }

          // Start soft start with specified parameters
          uint32_t result = Motor_soft_start_begin(cmd.motor_num, cmd.pwm_level, cmd.direction);

          if (result == 0)
          {
            // Reset maximum currents at the start of motor movement
            _Reset_motor_max_currents_on_start(cmd.motor_num);

            APPLOG("Motor %u (%s) soft start command executed: target PWM %u%%, direction %u",
                   (unsigned int)cmd.motor_num, Get_motor_name(cmd.motor_num), (unsigned int)cmd.pwm_level, (unsigned int)cmd.direction);
          }
          else
          {
            APPLOG("Motor %u (%s) soft start command failed with error %u", (unsigned int)cmd.motor_num, Get_motor_name(cmd.motor_num), (unsigned int)result);
          }
        }
        break;
      case MOTOR_CMD_SOFT_STOP:
        if (cmd.motor_num >= 1 && cmd.motor_num <= 4)
        {
          // Check if motor is already executing the same command - ignore duplicate
          if (_Is_motor_already_executing_command(cmd.motor_num, cmd.cmd_type, MOTOR_DIRECTION_STOP))
          {
            APPLOG("Motor %u (%s) SOFT_STOP command ignored - motor already stopping or stopped", (unsigned int)cmd.motor_num, Get_motor_name(cmd.motor_num));
            break;  // Ignore duplicate command
          }

          // Start soft stop
          uint32_t result = Motor_soft_start_stop(cmd.motor_num);

          if (result == 0)
          {
            APPLOG("Motor %u (%s) soft stop command executed", (unsigned int)cmd.motor_num, Get_motor_name(cmd.motor_num));
          }
          else
          {
            APPLOG("Motor %u (%s) soft stop command failed with error %u", (unsigned int)cmd.motor_num, Get_motor_name(cmd.motor_num), (unsigned int)result);
          }
        }
        break;
      case MOTOR_CMD_EMERGENCY_STOP:
        if (cmd.motor_num >= 1 && cmd.motor_num <= 4)
        {
          // Emergency stop without ramping (direct call for consistency)
          Motor_emergency_stop_direct(cmd.motor_num);
          APPLOG("Motor %u (%s) emergency stop command executed (direct)", (unsigned int)cmd.motor_num, Get_motor_name(cmd.motor_num));
        }
        else if (cmd.motor_num == 0)
        {
          // Emergency stop all motors (direct call for consistency)
          Motor_emergency_stop_direct(0);
          APPLOG("All motors emergency stop command executed (direct)");
        }
        break;

      default:
        APPLOG("Unknown motor command type: %u", (unsigned int)cmd.cmd_type);
        break;
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Get motor driver ready flag.

  Parameters:
    None

  Return:
    1 if ready, 0 if not ready
-----------------------------------------------------------------------------------------------------*/
uint8_t Get_motor_driver_ready(void)
{
  return g_motor_driver_ready;
}

/*-----------------------------------------------------------------------------------------------------
  Send motor COAST command to the command queue

  Parameters:
    motor_num - motor number (1-4 for specific motor, 0=all motors)

  Return:
    TX_SUCCESS if command was sent successfully, otherwise error code
-----------------------------------------------------------------------------------------------------*/
uint32_t Motor_command_coast(uint8_t motor_num)
{
  T_motor_command cmd = {
    .cmd_type  = MOTOR_CMD_COAST,
    .motor_num = motor_num,
    .pwm_level = 0,                     // Not used for COAST command
    .direction = MOTOR_DIRECTION_STOP,  // Not used for COAST command
    .ramp_time = 0                      // Not used for COAST command
  };

  return tx_queue_send(&g_motor_command_queue, &cmd, TX_NO_WAIT);
}

/*-----------------------------------------------------------------------------------------------------
  Get motor state information for diagnostic display

  Parameters:
    motor_num - motor number (1-4)
    enabled   - pointer to store motor enabled state
    pwm_level - pointer to store motor PWM level (0-100%)
    direction - pointer to store motor direction

  Return:
    TX_SUCCESS if motor_num is valid, otherwise error code
-----------------------------------------------------------------------------------------------------*/
uint32_t Motor_get_state(uint8_t motor_num, uint8_t *enabled, uint16_t *pwm_level, uint8_t *direction)
{
  if (motor_num < 1 || motor_num > 4 || enabled == 0 || pwm_level == 0 || direction == 0)
  {
    return TX_PTR_ERROR;
  }

  *enabled   = g_motor_states[motor_num - 1].enabled;
  *pwm_level = g_motor_states[motor_num - 1].pwm_level;
  *direction = g_motor_states[motor_num - 1].direction;

  return TX_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Perform motor current offset calibration for all motor phases.
  This function calibrates ADC current offset measurements when motor drivers are disabled.

  Parameters:
    None

  Return:
    CALIBRATION_SUCCESS          - Calibration completed successfully
    CALIBRATION_ERROR_DRIVERS_ON - Motor drivers are still enabled
    CALIBRATION_ERROR_TIMEOUT    - Calibration timeout occurred
-----------------------------------------------------------------------------------------------------*/
static uint32_t _Perform_motor_current_offset_calibration(void)
{
  uint32_t result;

  APPLOG("Motor Current Offset Calibration: Starting calibration process");

  // Start ADC calibration process (driver state check is performed inside)
  result = Adc_driver_start_calibration(CALIBRATION_SAMPLES_COUNT);
  if (result != TX_SUCCESS)
  {
    APPLOG("Motor Current Offset Calibration: ERROR - Failed to start calibration (motor drivers may be enabled)");
    return CALIBRATION_ERROR_DRIVERS_ON;
  }

  APPLOG("Motor Current Offset Calibration: Started with %d samples per motor", CALIBRATION_SAMPLES_COUNT);

  // Wait for calibration completion with timeout
  result = Adc_driver_wait_averaging_done(CALIBRATION_TIMEOUT_MS);
  if (result != TX_SUCCESS)
  {
    APPLOG("Motor Current Offset Calibration: ERROR - Timeout after %d ms", CALIBRATION_TIMEOUT_MS);
    return CALIBRATION_ERROR_TIMEOUT;
  }

  // Calculate averaged offset values for all motor phases
  // All phases for both motors use the same number of samples
  if (adc.samples_per_motor_phase > 0)
  {
    TX_INTERRUPT_SAVE_AREA
    TX_DISABLE
    adc.smpl_i_u_offs_m1 = (uint16_t)(adc.smpl_i_u_m1_acc / adc.samples_per_motor_phase);
    adc.smpl_i_v_offs_m1 = (uint16_t)(adc.smpl_i_v_m1_acc / adc.samples_per_motor_phase);
    adc.smpl_i_w_offs_m1 = (uint16_t)(adc.smpl_i_w_m1_acc / adc.samples_per_motor_phase);

    adc.smpl_i_u_offs_m2 = (uint16_t)(adc.smpl_i_u_m2_acc / adc.samples_per_motor_phase);
    adc.smpl_i_v_offs_m2 = (uint16_t)(adc.smpl_i_v_m2_acc / adc.samples_per_motor_phase);
    adc.smpl_i_w_offs_m2 = (uint16_t)(adc.smpl_i_w_m2_acc / adc.samples_per_motor_phase);
    TX_RESTORE

    APPLOG("Motor Current Offset Calibration: Completed successfully");
    APPLOG("Driver 1  Offsets: U=%d, V=%d, W=%d", adc.smpl_i_u_offs_m1, adc.smpl_i_v_offs_m1, adc.smpl_i_w_offs_m1);
    APPLOG("Driver 2  Offsets: U=%d, V=%d, W=%d", adc.smpl_i_u_offs_m2, adc.smpl_i_v_offs_m2, adc.smpl_i_w_offs_m2);
    APPLOG("Calibration completed with %d samples per phase", adc.samples_per_motor_phase);
  }
  else
  {
    APPLOG("Motor Current Offset Calibration: ERROR - No samples collected");
  }

  return CALIBRATION_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Create Motor Driver thread.

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Motor_thread_create(void)
{
  tx_thread_create(
  &Motor_driver,
  (CHAR *)"Motor Driver",
  Motor_driver_func,
  (ULONG)NULL,
  &Motor_driver_stack,
  MOTOR_THREAD_STACK_SIZE,
  THREAD_PRIORITY_MOTOR,
  THREAD_PREEMPT_MOTOR,
  THREAD_TIME_SLICE_MOTOR,
  TX_AUTO_START);
}

/*-----------------------------------------------------------------------------------------------------
  Get motor parameters from wvar structure

  Parameters:
    motor_num - motor number (1-4)
    params    - pointer to structure for saving motor parameters

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Get_motor_parameters(uint8_t motor_num, T_motor_parameters *params)
{
  if (motor_num < 1 || motor_num > 4 || params == NULL)
  {
    return;
  }

  switch (motor_num)
  {
    case 1:
      params->max_pwm_percent  = wvar.motor_1_max_pwm_percent;
      params->direction_invert = wvar.motor_1_direction_invert;
      params->accel_time_ms    = wvar.motor_1_accel_time_ms;
      params->decel_time_ms    = wvar.motor_1_decel_time_ms;
      params->algorithm        = wvar.motor_1_algorithm;
      break;

    case 2:
      params->max_pwm_percent  = wvar.motor_2_max_pwm_percent;
      params->direction_invert = wvar.motor_2_direction_invert;
      params->accel_time_ms    = wvar.motor_2_accel_time_ms;
      params->decel_time_ms    = wvar.motor_2_decel_time_ms;
      params->algorithm        = wvar.motor_2_algorithm;
      break;

    case 3:
      params->max_pwm_percent  = wvar.motor_3_max_pwm_percent;
      params->direction_invert = wvar.motor_3_direction_invert;
      params->accel_time_ms    = wvar.motor_3_accel_time_ms;
      params->decel_time_ms    = wvar.motor_3_decel_time_ms;
      params->algorithm        = wvar.motor_3_algorithm;
      break;

    case 4:
      params->max_pwm_percent  = wvar.motor_4_max_pwm_percent;
      params->direction_invert = wvar.motor_4_direction_invert;
      params->accel_time_ms    = wvar.motor_4_accel_time_ms;
      params->decel_time_ms    = wvar.motor_4_decel_time_ms;
      params->algorithm        = wvar.motor_4_algorithm;
      break;
  }
}

/*-----------------------------------------------------------------------------------------------------
  PWM setter callback for motor soft start system

  Parameters:
    motor_num - motor number (1-4)
    direction - motor direction (MOTOR_DIRECTION_FORWARD/REVERSE/STOP)
    pwm_percent - PWM level in percent (0-100)

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Motor_soft_start_pwm_setter(uint8_t motor_num, uint8_t direction, uint16_t pwm_percent)
{
  // Apply PWM inversion for reverse direction (same logic as in motor commands)
  uint16_t effective_pwm = pwm_percent;
  if (direction == MOTOR_DIRECTION_REVERSE)
  {
    effective_pwm = 100 - pwm_percent;  // Invert PWM for reverse rotation
  }

  // Use existing motor PWM setting function with effective PWM
  _Set_motor_pwm(motor_num, direction, effective_pwm);
}

/*-----------------------------------------------------------------------------------------------------
  Send soft start motor command to the command queue

  Parameters:
    motor_num - motor number (1-4)
    target_pwm - target PWM level (0-100 percent)
    direction - motor direction (MOTOR_DIRECTION_FORWARD, MOTOR_DIRECTION_REVERSE)

  Return:
    TX_SUCCESS if command was sent successfully, otherwise error code
-----------------------------------------------------------------------------------------------------*/
uint32_t Motor_command_soft_start(uint8_t motor_num, uint16_t target_pwm, uint8_t direction)
{
  T_motor_command cmd = {
    .cmd_type  = MOTOR_CMD_SOFT_START,
    .motor_num = motor_num,
    .pwm_level = target_pwm,
    .direction = direction,
    .ramp_time = 0  // Not used (times come from motor parameters)
  };

  return tx_queue_send(&g_motor_command_queue, &cmd, TX_NO_WAIT);
}

/*-----------------------------------------------------------------------------------------------------
  Send soft stop motor command to the command queue

  Parameters:
    motor_num - motor number (1-4)

  Return:
    TX_SUCCESS if command was sent successfully, otherwise error code
-----------------------------------------------------------------------------------------------------*/
uint32_t Motor_command_soft_stop(uint8_t motor_num)
{
  T_motor_command cmd = {
    .cmd_type  = MOTOR_CMD_SOFT_STOP,
    .motor_num = motor_num,
    .pwm_level = 0,                     // Not used for SOFT_STOP command
    .direction = MOTOR_DIRECTION_STOP,  // Not used for SOFT_STOP command
    .ramp_time = 0                      // Not used for SOFT_STOP command
  };

  return tx_queue_send(&g_motor_command_queue, &cmd, TX_NO_WAIT);
}

/*-----------------------------------------------------------------------------------------------------
  Send emergency stop motor command to the command queue

  Parameters:
    motor_num - motor number (1-4)

  Return:
    TX_SUCCESS if command was sent successfully, otherwise error code
-----------------------------------------------------------------------------------------------------*/
uint32_t Motor_command_emergency_stop(uint8_t motor_num)
{
  T_motor_command cmd = {
    .cmd_type  = MOTOR_CMD_EMERGENCY_STOP,
    .motor_num = motor_num,
    .pwm_level = 0,                     // Not used for EMERGENCY_STOP command
    .direction = MOTOR_DIRECTION_STOP,  // Not used for EMERGENCY_STOP command
    .ramp_time = 0                      // Not used for EMERGENCY_STOP command
  };
  return tx_queue_send(&g_motor_command_queue, &cmd, TX_NO_WAIT);
}

/*-----------------------------------------------------------------------------------------------------
  Set motor to dynamic braking state for emergency stop.
  During emergency stop, both motor phases must be connected to the same potential
  (either both to ground or both to 24V) to short circuit the motor winding.

  The potential choice depends on the paired motor state to avoid conflicts on shared phases.

  Parameters:
    motor_num - motor number (1-4)

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Set_motor_dynamic_braking(uint8_t motor_num)
{
  if (motor_num < 1 || motor_num > 4)
  {
    return;
  }

  uint8_t paired_motor        = _Get_paired_motor(motor_num);
  bool    shared_phase_to_24v = false;  // Default: short circuit to ground (0V)
  // Check if paired motor is active and determine shared phase state
  if (paired_motor > 0)
  {
    T_motor_extended_state *paired_state = Motor_get_extended_state(paired_motor);

    if (paired_state != NULL && paired_state->enabled &&
        paired_state->direction != MOTOR_DIRECTION_STOP)
    {
      // Paired motor is active - shared phase state depends on its direction
      if ((motor_num == MOTOR_1_ || motor_num == MOTOR_2_))
      {
        // Motors 1&2 share V1: 0V=FORWARD, 24V=REVERSE
        shared_phase_to_24v = (paired_state->direction == MOTOR_DIRECTION_REVERSE);
      }
      else if ((motor_num == MOTOR_3_ || motor_num == MOTOR_4_))
      {
        // Motors 3&4 share V2: 0V=FORWARD, 24V=REVERSE
        shared_phase_to_24v = (paired_state->direction == MOTOR_DIRECTION_REVERSE);
      }
    }
    else
    {
      // Paired motor is not active (stopped or in emergency stop)
      // Default to 0V shared phase to prevent unexpected motor rotation
      shared_phase_to_24v = false;
    }
  }
  // Set both motor phases to the same potential for dynamic braking
  uint32_t short_circuit_level = shared_phase_to_24v ? PWM_STEP_COUNT : 0;

  TX_INTERRUPT_SAVE_AREA

  TX_DISABLE
  switch (motor_num)
  {
    case MOTOR_1_:
      // Motor 1: U1-V1 phases -> both to same potential
      g_pwm_phase_control.pwm_level[MOT_1][PH_U]    = short_circuit_level;
      g_pwm_phase_control.output_state[MOT_1][PH_U] = PHASE_OUTPUT_ENABLE;
      g_pwm_phase_control.pwm_level[MOT_1][PH_V]    = short_circuit_level;
      g_pwm_phase_control.output_state[MOT_1][PH_V] = PHASE_OUTPUT_ENABLE;
      break;

    case MOTOR_2_:
      // Motor 2: V1-W1 phases -> both to same potential
      g_pwm_phase_control.pwm_level[MOT_1][PH_V]    = short_circuit_level;
      g_pwm_phase_control.output_state[MOT_1][PH_V] = PHASE_OUTPUT_ENABLE;
      g_pwm_phase_control.pwm_level[MOT_1][PH_W]    = short_circuit_level;
      g_pwm_phase_control.output_state[MOT_1][PH_W] = PHASE_OUTPUT_ENABLE;
      break;

    case MOTOR_3_:
      // Motor 3: U2-V2 phases -> both to same potential
      g_pwm_phase_control.pwm_level[MOT_2][PH_U]    = short_circuit_level;
      g_pwm_phase_control.output_state[MOT_2][PH_U] = PHASE_OUTPUT_ENABLE;
      g_pwm_phase_control.pwm_level[MOT_2][PH_V]    = short_circuit_level;
      g_pwm_phase_control.output_state[MOT_2][PH_V] = PHASE_OUTPUT_ENABLE;
      break;

    case MOTOR_4_:
      // Motor 4: V2-W2 phases -> both to same potential
      g_pwm_phase_control.pwm_level[MOT_2][PH_V]    = short_circuit_level;
      g_pwm_phase_control.output_state[MOT_2][PH_V] = PHASE_OUTPUT_ENABLE;
      g_pwm_phase_control.pwm_level[MOT_2][PH_W]    = short_circuit_level;
      g_pwm_phase_control.output_state[MOT_2][PH_W] = PHASE_OUTPUT_ENABLE;
      break;

    default:
      break;
  }
  TX_RESTORE
  APPLOG("Motor %u (%s) dynamic braking: both phases set to %s for emergency stop", (unsigned int)motor_num, Get_motor_name(motor_num),
         shared_phase_to_24v ? "24V" : "0V");
}

/*-----------------------------------------------------------------------------------------------------
  Update emergency stop motor's phases to match new shared phase level.
  When a paired motor changes direction and requires a different shared phase potential,
  this function updates both phases of the emergency stopped motor to the new shared
  phase level, maintaining its emergency stop state (both phases at same potential).

  Parameters:
    motor_num - motor number (1-4) that is in emergency stop
    new_shared_phase_level - new shared phase PWM level (0 or PWM_STEP_COUNT)

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Update_emergency_stop_motor_shared_phase(uint8_t motor_num, uint32_t new_shared_phase_level)
{
  if (motor_num < 1 || motor_num > 4)
  {
    return;
  }

  // Update both motor phases to the new shared phase level for emergency stop
  TX_INTERRUPT_SAVE_AREA

  TX_DISABLE
  switch (motor_num)
  {
    case MOTOR_1_:
      // Motor 1: U1-V1 phases -> both to new shared level (V1 is shared with Motor 2)
      g_pwm_phase_control.pwm_level[MOT_1][PH_U]    = new_shared_phase_level;
      g_pwm_phase_control.output_state[MOT_1][PH_U] = PHASE_OUTPUT_ENABLE;
      g_pwm_phase_control.pwm_level[MOT_1][PH_V]    = new_shared_phase_level;
      g_pwm_phase_control.output_state[MOT_1][PH_V] = PHASE_OUTPUT_ENABLE;
      break;

    case MOTOR_2_:
      // Motor 2: V1-W1 phases -> both to new shared level (V1 is shared with Motor 1)
      g_pwm_phase_control.pwm_level[MOT_1][PH_V]    = new_shared_phase_level;
      g_pwm_phase_control.output_state[MOT_1][PH_V] = PHASE_OUTPUT_ENABLE;
      g_pwm_phase_control.pwm_level[MOT_1][PH_W]    = new_shared_phase_level;
      g_pwm_phase_control.output_state[MOT_1][PH_W] = PHASE_OUTPUT_ENABLE;
      break;

    case MOTOR_3_:
      // Motor 3: U2-V2 phases -> both to new shared level (V2 is shared with Motor 4)
      g_pwm_phase_control.pwm_level[MOT_2][PH_U]    = new_shared_phase_level;
      g_pwm_phase_control.output_state[MOT_2][PH_U] = PHASE_OUTPUT_ENABLE;
      g_pwm_phase_control.pwm_level[MOT_2][PH_V]    = new_shared_phase_level;
      g_pwm_phase_control.output_state[MOT_2][PH_V] = PHASE_OUTPUT_ENABLE;
      break;

    case MOTOR_4_:
      // Motor 4: V2-W2 phases -> both to new shared level (V2 is shared with Motor 3)
      g_pwm_phase_control.pwm_level[MOT_2][PH_V]    = new_shared_phase_level;
      g_pwm_phase_control.output_state[MOT_2][PH_V] = PHASE_OUTPUT_ENABLE;
      g_pwm_phase_control.pwm_level[MOT_2][PH_W]    = new_shared_phase_level;
      g_pwm_phase_control.output_state[MOT_2][PH_W] = PHASE_OUTPUT_ENABLE;
      break;

    default:
      break;
  }
  TX_RESTORE
}

/*-----------------------------------------------------------------------------------------------------
  Emergency stop motor directly (bypasses command queue for immediate stop)

  Parameters:
    motor_num - motor number (1-4) or 0 for all motors

  Return:
    0 - success, non-zero - error
-----------------------------------------------------------------------------------------------------*/
uint32_t Motor_emergency_stop_direct(uint8_t motor_num)
{
  if (motor_num > 4)
  {
    return 1;  // Invalid motor number
  }

  if (motor_num == 0)
  {
    // Emergency stop all motors

    for (uint8_t i = 1; i <= 4; i++)
    {
      Motor_emergency_stop_direct(i);
    }
    APPLOG("All motors emergency stop executed (direct)");
    return 0;
  }

  // Get paired motor information
  uint8_t paired_motor        = _Get_paired_motor(motor_num);
  bool    paired_motor_active = false;

  if (paired_motor > 0)
  {
    paired_motor_active = (g_motor_states[paired_motor - 1].enabled &&
                           g_motor_states[paired_motor - 1].direction != MOTOR_DIRECTION_STOP) ||
                          (g_motor_states[paired_motor - 1].soft_start_state != MOTOR_STATE_IDLE);
  }  // Emergency dynamic braking motor for fast stop instead of just stopping PWM
  _Set_motor_dynamic_braking(motor_num);

  // Update unified motor state
  g_motor_states[motor_num - 1].enabled                = 0;
  g_motor_states[motor_num - 1].pwm_level              = 0;
  g_motor_states[motor_num - 1].direction              = MOTOR_DIRECTION_STOP;

  // Update soft start state directly (Motor_soft_start_emergency_stop removed - all logic now in Motor_emergency_stop_direct)
  g_motor_states[motor_num - 1].soft_start_state       = MOTOR_STATE_IDLE;
  g_motor_states[motor_num - 1].target_pwm             = 0;
  g_motor_states[motor_num - 1].original_target_pwm    = 0;
  g_motor_states[motor_num - 1].current_pwm_x100       = 0;
  g_motor_states[motor_num - 1].step_counter           = 0;
  g_motor_states[motor_num - 1].target_direction       = MOTOR_DIRECTION_STOP;
  g_motor_states[motor_num - 1].soft_start_initialized = false;
  g_motor_states[motor_num - 1].conflict_detected      = false;
  g_motor_states[motor_num - 1].run_phase_start_time   = 0;  // Reset RUN phase start time
  // Note: max_current_logged flag is NOT reset here - it will be checked in main loop for logging
  // Update paired motor emergency stop state if it exists and is stopped
  // This ensures that when shared phase changes due to emergency stop,
  // the paired stopped motor maintains correct emergency stop configuration
  if (paired_motor > 0 && _Is_motor_in_emergency_stop(paired_motor))
  {
    _Set_motor_dynamic_braking(paired_motor);
    APPLOG("Motor %u (%s) emergency stop: updated paired motor %u (%s) emergency stop configuration",
           (unsigned int)motor_num, Get_motor_name(motor_num),
           (unsigned int)paired_motor, Get_motor_name(paired_motor));
  }

  // Clear pending commands for this motor from queue
  uint32_t cleared_commands = _Clear_motor_commands_from_queue(motor_num);

  if (paired_motor_active)
  {
    APPLOG("Motor %u (%s) emergency stop executed (direct) - paired motor %u (%s) still active, commands cleared: %u",
           (unsigned int)motor_num, Get_motor_name(motor_num),
           (unsigned int)paired_motor, Get_motor_name(paired_motor),
           (unsigned int)cleared_commands);
  }
  else
  {
    APPLOG("Motor %u (%s) emergency stop executed (direct) - no paired motor conflict, commands cleared: %u",
           (unsigned int)motor_num, Get_motor_name(motor_num),
           (unsigned int)cleared_commands);
  }

  return 0;
}

/*-----------------------------------------------------------------------------------------------------
  Clear all commands for specific motor from command queue

  Parameters:
    motor_num - motor number (1-4)

  Return:
    Number of commands cleared from queue
-----------------------------------------------------------------------------------------------------*/
static uint32_t _Clear_motor_commands_from_queue(uint8_t motor_num)
{
  T_motor_command cmd;
  uint32_t        cleared_count   = 0;
  uint32_t        preserved_count = 0;
  T_motor_command preserved_commands[MOTOR_COMMAND_QUEUE_SIZE];

  // Extract all commands and preserve only those for other motors
  while (tx_queue_receive(&g_motor_command_queue, &cmd, TX_NO_WAIT) == TX_SUCCESS)
  {
    if (cmd.motor_num != motor_num && cmd.motor_num != 0)
    {
      // Preserve commands for other motors
      if (preserved_count < MOTOR_COMMAND_QUEUE_SIZE)
      {
        preserved_commands[preserved_count] = cmd;
        preserved_count++;
      }
    }
    else
    {
      // Count commands that will be cleared
      cleared_count++;
    }
  }

  // Put back preserved commands
  for (uint32_t i = 0; i < preserved_count; i++)
  {
    tx_queue_send(&g_motor_command_queue, &preserved_commands[i], TX_NO_WAIT);
  }

  return cleared_count;
}

/*-----------------------------------------------------------------------------------------------------
  Get paired motor number for shared phase conflict detection

  Parameters:
    motor_num - motor number (1-4)

  Return:
    Paired motor number (1-4) or 0 if no paired motor
-----------------------------------------------------------------------------------------------------*/
static uint8_t _Get_paired_motor(uint8_t motor_num)
{
  // Motor pairs that share phases:
  // Motor 1 (Traction) and Motor 2 ( Motor 2) share V1
  // Motor 3 ( Motor 3) and Motor 4 ( Motor 2) share V2
  switch (motor_num)
  {
    case 1:
      return 2;  // Motor 1 paired with Motor 2
    case 2:
      return 1;  // Motor 2 paired with Motor 1
    case 3:
      return 4;  // Motor 3 paired with Motor 4
    case 4:
      return 3;  // Motor 4 paired with Motor 3
    default:
      return 0;  // No paired motor
  }
}

/*-----------------------------------------------------------------------------------------------------
  Check if a running motor is receiving a direction change command

  Parameters:
    motor_num - motor number to check (1-4)
    new_direction - new direction being requested

  Return:
    1 if motor is running and direction change is detected, 0 otherwise
-----------------------------------------------------------------------------------------------------*/
static uint8_t _Check_direction_change_for_running_motor(uint8_t motor_num, uint8_t new_direction)
{
  // Check if motor number is valid
  if (motor_num < 1 || motor_num > 4)
  {
    return 0;
  }

  // Check if motor is currently enabled and running
  if (!g_motor_states[motor_num - 1].enabled)
  {
    return 0;  // Motor is not running, no direction change issue
  }

  // Get current motor direction
  uint8_t current_direction = g_motor_states[motor_num - 1].direction;

  // Check if direction is actually changing
  if (current_direction == MOTOR_DIRECTION_STOP)
  {
    return 0;  // Motor is stopped, no direction change issue
  }

  // Check if new direction is different from current direction
  if (new_direction != current_direction && new_direction != MOTOR_DIRECTION_STOP)
  {
    return 1;  // Direction change detected for running motor
  }
  return 0;    // No direction change or stopping command
}

/*-----------------------------------------------------------------------------------------------------
  Check if motor is in emergency stop state

  Parameters:
    motor_num - motor number to check (1-4)

  Return:
    1 if motor is in emergency stop state, 0 otherwise
-----------------------------------------------------------------------------------------------------*/
static uint8_t _Is_motor_in_emergency_stop(uint8_t motor_num)
{
  // Check if motor number is valid
  if (motor_num < 1 || motor_num > 4)
  {
    return 0;
  }

  T_motor_extended_state *motor_state = Motor_get_extended_state(motor_num);

  if (motor_state == NULL)
  {
    return 0;
  }
  // Motor is in emergency stop if it's disabled, stopped, and idle
  if (!motor_state->enabled &&
      motor_state->direction == MOTOR_DIRECTION_STOP &&
      motor_state->soft_start_state == MOTOR_STATE_IDLE)
  {
    return 1;  // Motor is in emergency stop state
  }

  return 0;    // Motor is not in emergency stop state
}

/*-----------------------------------------------------------------------------------------------------
  Get actual motor direction considering direction inversion flag

  Parameters:
    motor_num - motor number (1-4)
    direction - requested direction

  Return:
    Actual direction after applying inversion
-----------------------------------------------------------------------------------------------------*/
static uint8_t _Get_actual_motor_direction(uint8_t motor_num, uint8_t direction)
{
  if (motor_num < 1 || motor_num > 4)
  {
    return direction;
  }
  T_motor_parameters motor_params;
  Get_motor_parameters(motor_num, &motor_params);

  if (motor_params.direction_invert)
  {
    if (direction == MOTOR_DIRECTION_FORWARD)
    {
      return MOTOR_DIRECTION_REVERSE;
    }
    else if (direction == MOTOR_DIRECTION_REVERSE)
    {
      return MOTOR_DIRECTION_FORWARD;
    }
  }

  return direction;
}

/*-----------------------------------------------------------------------------------------------------
  Set motor algorithms from configuration parameters

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Set_motor_algorithms_from_parameters(void)
{
  // Update algorithms for all motors based on their parameter configuration
  for (uint8_t motor_num = 1; motor_num <= 4; motor_num++)
  {
    T_motor_parameters motor_params;
    Get_motor_parameters(motor_num, &motor_params);

    // Set algorithm for each motor based on its configuration
    uint32_t result = Motor_soft_start_set_algorithm(motor_num, (T_soft_start_algorithm)motor_params.algorithm);
    if (result != 0)
    {
      APPLOG("Motor %u (%s): Failed to set algorithm %u (error: %u)",
             (unsigned int)motor_num, Get_motor_name(motor_num),
             (unsigned int)motor_params.algorithm, (unsigned int)result);
    }
    else
    {
      APPLOG("Motor %u (%s): Algorithm set to %u from configuration",
             (unsigned int)motor_num, Get_motor_name(motor_num),
             (unsigned int)motor_params.algorithm);
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Get motor parameters for external access

  Parameters:
    motor_num - motor number (1-4)
    params - pointer to parameters structure to fill

  Return:
    0 - success, non-zero - error
-----------------------------------------------------------------------------------------------------*/
uint32_t Motor_get_parameters(uint8_t motor_num, T_motor_parameters *params)
{
  if (motor_num < 1 || motor_num > 4 || params == NULL)
  {
    return 1;  // Invalid parameters
  }

  Get_motor_parameters(motor_num, params);
  return 0;
}

/*-----------------------------------------------------------------------------------------------------
  Get pointer to motor's extended state structure

  Parameters:
    motor_num - motor number (1-4)

  Return:
    Pointer to motor's extended state or NULL if invalid motor number
-----------------------------------------------------------------------------------------------------*/
T_motor_extended_state *Motor_get_extended_state(uint8_t motor_num)
{
  if (motor_num < 1 || motor_num > 4)
  {
    return NULL;
  }

  return &g_motor_states[motor_num - 1];
}

/*-----------------------------------------------------------------------------------------------------
  Check if motor is already executing the same command (to avoid redundant processing)

  Parameters:
    motor_num - motor number (1-4)
    cmd_type - command type to check
    direction - requested direction
    pwm_level - requested PWM level

  Return:
    1 if motor is already executing the same command, 0 otherwise
-----------------------------------------------------------------------------------------------------*/
static uint8_t _Is_motor_already_executing_command(uint8_t motor_num, uint8_t cmd_type, uint8_t direction)
{
  if (motor_num < 1 || motor_num > 4)
  {
    return 0;
  }

  T_motor_extended_state *motor_state = Motor_get_extended_state(motor_num);
  if (motor_state == NULL)
  {
    return 0;
  }

  switch (cmd_type)
  {
    case MOTOR_CMD_COAST:
      // Check if motor is already coasting
      if (!motor_state->enabled && motor_state->direction == MOTOR_DIRECTION_STOP && motor_state->soft_start_state == MOTOR_STATE_COASTING)
      {
        return 1;  // Motor is already coasting
      }
      break;
    case MOTOR_CMD_SOFT_START:
      // Check if motor is already running soft start with same direction OR already running with same direction
      if ((motor_state->soft_start_state == MOTOR_STATE_SOFT_RAMPING_UP && motor_state->target_direction == direction) ||
          (motor_state->soft_start_state == MOTOR_STATE_RUNNING && motor_state->direction == direction))
      {
        return 1;  // Motor is already executing soft start or running with same direction
      }
      break;

    case MOTOR_CMD_SOFT_STOP:
      // Check if motor is already doing soft stop or is stopped
      if (motor_state->soft_start_state == MOTOR_STATE_SOFT_RAMPING_DOWN || (!motor_state->enabled && motor_state->direction == MOTOR_DIRECTION_STOP))
      {
        return 1;  // Motor is already stopping or stopped
      }
      break;

    case MOTOR_CMD_EMERGENCY_STOP:
      // Emergency stop should always be executed for safety
      return 0;

    default:
      return 0;
  }

  return 0;
}

/*-----------------------------------------------------------------------------------------------------
  Request motor current offset calibration from external thread (diagnostic terminal)
  NOTE: Manual calibration requests are no longer supported.
  Calibration now runs automatically every 10 seconds when all motors are off.

  Parameters:
    None

  Return:
    TX_NOT_AVAILABLE - Manual calibration not supported
-----------------------------------------------------------------------------------------------------*/
uint32_t Motor_request_calibration(void)
{
  // Manual calibration requests are no longer supported - calibration runs automatically
  return TX_NOT_AVAILABLE;
}

/*-----------------------------------------------------------------------------------------------------
  Set driver enable state using centralized control.
  Updates both hardware pins and global state variables.

  Parameters:
    driver_num: Driver number (1 or 2)
    enable_state: Desired enable state (0=disable, 1=enable)

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Motor_driver_enable_set(uint8_t driver_num, uint8_t enable_state)
{
  if (driver_num == DRIVER_1)
  {
    MOTOR_DRV1_EN   = enable_state;
    g_drv1_en_state = MOTOR_DRV1_EN_STATE;  // Update global variable
  }
  else if (driver_num == DRIVER_2)
  {
    MOTOR_DRV2_EN   = enable_state;
    g_drv2_en_state = MOTOR_DRV2_EN_STATE;  // Update global variable
  }
}

/*-----------------------------------------------------------------------------------------------------
  Toggle driver enable state using centralized control.
  Updates both hardware pins and global state variables.

  Parameters:
    driver_num: Driver number (1 or 2)

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Motor_driver_enable_toggle(uint8_t driver_num)
{
  if (driver_num == DRIVER_1)
  {
    MOTOR_DRV1_EN   = !MOTOR_DRV1_EN_STATE;
    g_drv1_en_state = MOTOR_DRV1_EN_STATE;  // Update global variable
  }
  else if (driver_num == DRIVER_2)
  {
    MOTOR_DRV2_EN   = !MOTOR_DRV2_EN_STATE;
    g_drv2_en_state = MOTOR_DRV2_EN_STATE;  // Update global variable
  }
}

/*-----------------------------------------------------------------------------------------------------
  Get current driver enable state.

  Parameters:
    driver_num: Driver number (1 or 2)

  Return:
    Driver enable state (0=disabled, 1=enabled), 0xFF for invalid driver number
-----------------------------------------------------------------------------------------------------*/
uint8_t Motor_driver_enable_get(uint8_t driver_num)
{
  if (driver_num == DRIVER_1)
  {
    return MOTOR_DRV1_EN_STATE;
  }
  else if (driver_num == DRIVER_2)
  {
    return MOTOR_DRV2_EN_STATE;
  }

  return 0xFF;  // Invalid driver number
}

/*-----------------------------------------------------------------------------------------------------
  Update driver enable global state variables from hardware pins.
  Helper function to keep state variables synchronized with hardware.

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Update_driver_enable_states(void)
{
  g_drv1_en_state = MOTOR_DRV1_EN_STATE;
  g_drv2_en_state = MOTOR_DRV2_EN_STATE;
}

/*-----------------------------------------------------------------------------------------------------
  Update maximum current tracking for all motors based on their operational state
  Uses Adc_driver_get_dc_motor_current() to get accurate phase current measurements:
  - Motor 1: U1 phase current (exclusive to Motor 1)
  - Motor 2: W1 phase current (exclusive to Motor 2)
  - Motor 3: U2 phase current (exclusive to Motor 3)
  - Motor 4: W2 phase current (exclusive to Motor 4)
  This provides much more accurate current tracking compared to driver power supply currents.

  For RUN phase: tracking starts only after 1 second delay to allow current stabilization
  after acceleration phase.

  Parameters: None

  Return: None
-----------------------------------------------------------------------------------------------------*/
static void _Update_max_current_tracking(void)
{
  float   current_value;
  uint8_t operation_phase;

  // Check each motor and determine its operational phase
  for (uint8_t motor_num = 1; motor_num <= 4; motor_num++)
  {
    T_motor_extended_state *state = Motor_get_extended_state(motor_num);

    if (state == NULL || !state->enabled)
    {
      continue;  // Skip disabled motors
    }

    // Determine operational phase based on soft start state
    if (state->soft_start_state == MOTOR_STATE_SOFT_RAMPING_UP)
    {
      operation_phase = PHASE_ACCEL;  // Acceleration phase
    }
    else if (state->soft_start_state == MOTOR_STATE_RUNNING)
    {
      operation_phase            = PHASE_RUN;  // Constant speed phase

      // Check if motor has been in RUN phase for at least 1 second
      // This allows the current to stabilize after acceleration phase
      uint32_t current_time      = tx_time_get();
      uint32_t time_in_run_phase = current_time - state->run_phase_start_time;

      if (time_in_run_phase < MAX_CURRENT_RUN_PHASE_DELAY_TICKS)
      {
        continue;  // Skip RUN phase current tracking for first second
      }
    }
    else if (state->soft_start_state == MOTOR_STATE_SOFT_RAMPING_DOWN)
    {
      operation_phase = PHASE_BRAKE;  // Braking phase
    }
    else
    {
      continue;  // Skip motors not in active operation phases
    }

    // Get current value using dedicated motor current measurement function
    // This function returns the absolute value of the motor's exclusive phase current
    current_value = Adc_driver_get_dc_motor_current(motor_num);

    // Skip if current value is invalid (function returns 0.0f for invalid motor numbers)
    if (current_value <= 0.0f)
    {
      continue;  // Skip invalid readings
    }

    Motor_max_current_update(motor_num, operation_phase, current_value);
  }
}

/*-----------------------------------------------------------------------------------------------------
  Synchronize individual max current variables from the structure

  Parameters: None

  Return: None
-----------------------------------------------------------------------------------------------------*/
static void _Sync_individual_max_current_variables(void)
{
  // Motor 1 (Traction) - index 0
  g_max_current_motor1_accel = g_max_current_tracking.motor[0][PHASE_ACCEL];
  g_max_current_motor1_run   = g_max_current_tracking.motor[0][PHASE_RUN];
  g_max_current_motor1_brake = g_max_current_tracking.motor[0][PHASE_BRAKE];

  // Motor 2 ( Motor 2) - index 1
  g_max_current_motor2_accel = g_max_current_tracking.motor[1][PHASE_ACCEL];
  g_max_current_motor2_run   = g_max_current_tracking.motor[1][PHASE_RUN];
  g_max_current_motor2_brake = g_max_current_tracking.motor[1][PHASE_BRAKE];

  // Motor 3 ( Motor 3) - index 2
  g_max_current_motor3_accel = g_max_current_tracking.motor[2][PHASE_ACCEL];
  g_max_current_motor3_run   = g_max_current_tracking.motor[2][PHASE_RUN];
  g_max_current_motor3_brake = g_max_current_tracking.motor[2][PHASE_BRAKE];

  // Motor 4 ( Motor 2) - index 3
  g_max_current_motor4_accel = g_max_current_tracking.motor[3][PHASE_ACCEL];
  g_max_current_motor4_run   = g_max_current_tracking.motor[3][PHASE_RUN];
  g_max_current_motor4_brake = g_max_current_tracking.motor[3][PHASE_BRAKE];
}

/*-----------------------------------------------------------------------------------------------------
  Reset maximum current value for specific motor and phase

  Parameters:
    motor_num - Motor number (1-4), or 0xFF for all motors
    phase - Operation phase (PHASE_ACCEL, PHASE_RUN, PHASE_BRAKE), or 0xFF for all phases

  Return: None
-----------------------------------------------------------------------------------------------------*/
void Motor_max_current_reset(uint8_t motor_num, uint8_t phase)
{
  if (motor_num == 0xFF)
  {
    // Reset all motors
    for (uint8_t m = 0; m < 4; m++)
    {
      if (phase == 0xFF)
      {
        // Reset all phases for all motors
        for (uint8_t p = 0; p < MAX_CURRENT_PHASES; p++)
        {
          g_max_current_tracking.motor[m][p] = 0.0f;
        }
      }
      else if (phase < MAX_CURRENT_PHASES)
      {
        // Reset specific phase for all motors
        g_max_current_tracking.motor[m][phase] = 0.0f;
      }
    }
  }
  else if ((motor_num >= 1) && (motor_num <= 4))
  {
    // Reset specific motor (convert to 0-based index)
    uint8_t motor_index = motor_num - 1;

    if (phase == 0xFF)
    {
      // Reset all phases for specific motor
      for (uint8_t p = 0; p < MAX_CURRENT_PHASES; p++)
      {
        g_max_current_tracking.motor[motor_index][p] = 0.0f;
      }
    }
    else if (phase < MAX_CURRENT_PHASES)
    {
      // Reset specific phase for specific motor
      g_max_current_tracking.motor[motor_index][phase] = 0.0f;
    }
  }

  // Synchronize individual variables with structure
  _Sync_individual_max_current_variables();
}

/*-----------------------------------------------------------------------------------------------------
  Update maximum current value if new value is higher

  Parameters:
    motor_num - Motor number (1-4)
    phase - Operation phase (PHASE_ACCEL, PHASE_RUN, PHASE_BRAKE)
    current_value - New current value to compare [A]

  Return: None
-----------------------------------------------------------------------------------------------------*/
void Motor_max_current_update(uint8_t motor_num, uint8_t phase, float current_value)
{
  if ((motor_num >= 1) && (motor_num <= 4) && (phase < MAX_CURRENT_PHASES))
  {
    uint8_t motor_index = motor_num - 1;  // Convert to 0-based index

    if (current_value > g_max_current_tracking.motor[motor_index][phase])
    {
      g_max_current_tracking.motor[motor_index][phase] = current_value;

      // Synchronize individual variables with structure
      _Sync_individual_max_current_variables();
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Reset all maximum current values to zero

  Parameters: None

  Return: None
-----------------------------------------------------------------------------------------------------*/
void Motor_max_current_reset_all(void)
{
  Motor_max_current_reset(0xFF, 0xFF);  // Reset all motors and all phases
}

/*-----------------------------------------------------------------------------------------------------
  Get maximum current value for specific motor and phase

  Parameters:
    motor_num - Motor number (1-4)
    phase - Operation phase (PHASE_ACCEL, PHASE_RUN, PHASE_BRAKE)

  Return: Maximum current value [A], or 0.0f if invalid parameters
-----------------------------------------------------------------------------------------------------*/
float Motor_max_current_get(uint8_t motor_num, uint8_t phase)
{
  if ((motor_num >= 1) && (motor_num <= 4) && (phase < MAX_CURRENT_PHASES))
  {
    uint8_t motor_index = motor_num - 1;  // Convert to 0-based index
    return g_max_current_tracking.motor[motor_index][phase];
  }

  return 0.0f;  // Return 0 for invalid parameters
}

/*-----------------------------------------------------------------------------------------------------
  Set RUN phase start time for max current tracking delay.
  Called when motor transitions to MOTOR_STATE_RUNNING to start tracking time.

  Parameters:
    motor_num - Motor number (1-4)

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Motor_set_run_phase_start_time(uint8_t motor_num)
{
  if (motor_num >= 1 && motor_num <= 4)
  {
    g_motor_states[motor_num - 1].run_phase_start_time = tx_time_get();
  }
}

/*-----------------------------------------------------------------------------------------------------
  Check all motors for stop condition and log maximum currents if needed

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Check_and_log_stopped_motors_max_currents(void)
{
  for (uint8_t motor_num = 1; motor_num <= 4; motor_num++)
  {
    T_motor_extended_state *state = Motor_get_extended_state(motor_num);
    if (state != NULL)
    {
      // Check if motor is stopped and max currents haven't been logged yet
      if (!state->enabled &&
          state->direction == MOTOR_DIRECTION_STOP &&
          (state->soft_start_state == MOTOR_STATE_IDLE || state->soft_start_state == MOTOR_STATE_COASTING) &&
          !state->max_current_logged)
      {
        // Motor is stopped and max currents need to be logged
        _Log_motor_max_currents_on_stop(motor_num);
      }
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Reset motor maximum currents at the start of movement and set logging flag

  Parameters:
    motor_num - motor number (1-4)

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Reset_motor_max_currents_on_start(uint8_t motor_num)
{
  if (motor_num >= 1 && motor_num <= 4)
  {
    // Reset all max current values for this motor
    Motor_max_current_reset(motor_num, 0xFF);  // 0xFF = all phases

    // Set flag that max currents have not been logged yet
    g_motor_states[motor_num - 1].max_current_logged = false;

    APPLOG("Motor %u (%s): Maximum currents reset at movement start",
           (unsigned int)motor_num, Get_motor_name(motor_num));
  }
}

/*-----------------------------------------------------------------------------------------------------
  Log motor maximum currents after motor stop

  Parameters:
    motor_num - motor number (1-4)

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Log_motor_max_currents_on_stop(uint8_t motor_num)
{
  if (motor_num >= 1 && motor_num <= 4)
  {
    uint8_t motor_index = motor_num - 1;

    // Get maximum current values for all phases
    float max_accel     = Motor_max_current_get(motor_num, PHASE_ACCEL);
    float max_run       = Motor_max_current_get(motor_num, PHASE_RUN);
    float max_brake     = Motor_max_current_get(motor_num, PHASE_BRAKE);

    // Log the maximum current values
    APPLOG("Motor %u (%s) maximum currents: Accel=%.2fA, Run=%.2fA, Brake=%.2fA",
           (unsigned int)motor_num, Get_motor_name(motor_num),
           max_accel, max_run, max_brake);

    // Set flag that max currents have been logged
    g_motor_states[motor_index].max_current_logged = true;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Check if all motors are currently off (not enabled and not moving)

  Parameters:
    None

  Return:
    1 if all motors are off, 0 if any motor is active
-----------------------------------------------------------------------------------------------------*/
static uint8_t _Check_all_motors_off(void)
{
  for (uint8_t motor_num = 1; motor_num <= 4; motor_num++)
  {
    T_motor_extended_state *state = &g_motor_states[motor_num - 1];

    if (state->enabled ||
        state->direction != MOTOR_DIRECTION_STOP ||
        (state->soft_start_state != MOTOR_STATE_IDLE && state->soft_start_state != MOTOR_STATE_COASTING))
    {
      return 0;  // Motor is active
    }
  }
  return 1;      // All motors are off
}

/*-----------------------------------------------------------------------------------------------------
  Check and complete motor current offset calibration if ready (non-blocking version)

  Parameters:
    None

  Return:
    CALIBRATION_SUCCESS if completed successfully
    CALIBRATION_ERROR_TIMEOUT if not ready yet
    CALIBRATION_ERROR_DRIVERS_ON if failed
-----------------------------------------------------------------------------------------------------*/
static uint32_t _Complete_silent_motor_current_offset_calibration(void)
{
  // Check if calibration is completed without waiting
  uint32_t result = Adc_driver_check_averaging_done();
  if (result != TX_SUCCESS)
  {
    return CALIBRATION_ERROR_TIMEOUT;  // Not ready yet
  }

  // Calculate averaged offset values for all motor phases
  if (adc.samples_per_motor_phase > 0)
  {
    TX_INTERRUPT_SAVE_AREA
    TX_DISABLE
    adc.smpl_i_u_offs_m1 = (uint16_t)(adc.smpl_i_u_m1_acc / adc.samples_per_motor_phase);
    adc.smpl_i_v_offs_m1 = (uint16_t)(adc.smpl_i_v_m1_acc / adc.samples_per_motor_phase);
    adc.smpl_i_w_offs_m1 = (uint16_t)(adc.smpl_i_w_m1_acc / adc.samples_per_motor_phase);

    adc.smpl_i_u_offs_m2 = (uint16_t)(adc.smpl_i_u_m2_acc / adc.samples_per_motor_phase);
    adc.smpl_i_v_offs_m2 = (uint16_t)(adc.smpl_i_v_m2_acc / adc.samples_per_motor_phase);
    adc.smpl_i_w_offs_m2 = (uint16_t)(adc.smpl_i_w_m2_acc / adc.samples_per_motor_phase);
    TX_RESTORE
  }

  return CALIBRATION_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Handle periodic calibration logic based on system time (non-blocking state machine)

  State machine logic:
  - IDLE: Check if motors are off, then check if 10 seconds elapsed since they stopped, then start calibration
  - REQUESTED: Check for motor activity (cancel if needed) or calibration completion

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Handle_periodic_calibration(void)
{
  uint32_t current_time = tx_time_get();

  switch (g_calibration_state)
  {
    case CALIBRATION_STATE_IDLE:
      // First check if all motors are off
      if (_Check_all_motors_off())
      {
        // Motors are off, now check if calibration interval has elapsed since they stopped
        if ((current_time - g_last_calibration_time) >= CALIBRATION_INTERVAL_TICKS)
        {
          // Start ADC calibration process (always succeeds when motors are off)
          Adc_driver_start_calibration(CALIBRATION_SAMPLES_COUNT);
          g_calibration_state = CALIBRATION_STATE_REQUESTED;
        }
      }
      else
      {
        // Motors are active, reset timer to wait for motors to stop
        g_last_calibration_time = current_time;
      }
      break;

    case CALIBRATION_STATE_REQUESTED:
      // Check if any motor started during calibration
      if (!_Check_all_motors_off())
      {
        // Cancel calibration if any motor starts
        Adc_driver_cancel_calibration();
        g_calibration_state     = CALIBRATION_STATE_IDLE;
        g_last_calibration_time = current_time;
      }
      else
      {
        // Check if calibration is completed
        uint32_t result = _Complete_silent_motor_current_offset_calibration();
        if (result == CALIBRATION_SUCCESS)
        {
          // Calibration completed successfully
          g_calibration_state     = CALIBRATION_STATE_IDLE;
          g_last_calibration_time = current_time;
        }
        else if (result != CALIBRATION_ERROR_TIMEOUT)
        {
          // Calibration failed for some other reason
          g_calibration_state     = CALIBRATION_STATE_IDLE;
          g_last_calibration_time = current_time;
        }
        // If result == CALIBRATION_ERROR_TIMEOUT, calibration is still in progress, just continue
      }
      break;

    default:
      g_calibration_state = CALIBRATION_STATE_IDLE;
      break;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Emergency stop all motors helper function.
  Stops all 4 motors immediately by setting all PWM outputs to zero with active outputs to handle critical situations like overcurrent or overtemperature.
  Sets emergency stop flag to block all motor control commands.

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Emergency_stop_all_motors(void)
{
  TX_INTERRUPT_SAVE_AREA

  TX_DISABLE
  // Set all PWM levels to zero and enable all outputs immediately
  for (uint8_t driver = 0; driver < DRIVER_COUNT; driver++)
  {
    for (uint8_t phase = 0; phase < PHASE_COUNT; phase++)
    {
      g_pwm_phase_control.pwm_level[driver][phase]    = 0;
      g_pwm_phase_control.output_state[driver][phase] = PHASE_OUTPUT_ENABLE;
    }
  }

  // Update all motor states to emergency stop condition
  for (uint8_t motor_num = 1; motor_num <= 4; motor_num++)
  {
    g_motor_states[motor_num - 1].enabled                = 0;
    g_motor_states[motor_num - 1].pwm_level              = 0;
    g_motor_states[motor_num - 1].direction              = MOTOR_DIRECTION_STOP;
    g_motor_states[motor_num - 1].soft_start_state       = MOTOR_STATE_IDLE;
    g_motor_states[motor_num - 1].target_pwm             = 0;
    g_motor_states[motor_num - 1].original_target_pwm    = 0;
    g_motor_states[motor_num - 1].current_pwm_x100       = 0;
    g_motor_states[motor_num - 1].step_counter           = 0;
    g_motor_states[motor_num - 1].target_direction       = MOTOR_DIRECTION_STOP;
    g_motor_states[motor_num - 1].soft_start_initialized = false;
    g_motor_states[motor_num - 1].conflict_detected      = false;
    g_motor_states[motor_num - 1].run_phase_start_time   = 0;  // Reset RUN phase start time
    // Note: max_current_logged flag is NOT reset here - it will be checked in main loop for logging
  }
  TX_RESTORE

  // Set emergency stop flag to block all motor control commands
  App_set_emergency_stop_flag();
}

/*-----------------------------------------------------------------------------------------------------
  Check overcurrent and overtemperature protection for all motors and system components.
  This function monitors current and temperature readings and triggers emergency stop if thresholds are exceeded.

  Overcurrent protection for motors is only active during linear movement (MOTOR_STATE_RUNNING)
  and after the same delay used in _Update_max_current_tracking to allow current stabilization.

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Check_overcurrent_overtemperature_protection(void)
{
  // Check overcurrent for each motor individually based on their operational state
  for (uint8_t motor_num = 1; motor_num <= 4; motor_num++)
  {
    T_motor_extended_state *state = Motor_get_extended_state(motor_num);

    // Skip motors that are not enabled or not in RUN state
    if (state == NULL || !state->enabled || state->soft_start_state != MOTOR_STATE_RUNNING)
    {
      continue;  // Skip motors not in linear movement phase
    }

    // Check if motor has been in RUN phase for sufficient time to allow current stabilization
    // This uses the same delay logic as in _Update_max_current_tracking
    uint32_t current_time      = tx_time_get();
    uint32_t time_in_run_phase = current_time - state->run_phase_start_time;

    if (time_in_run_phase < MAX_CURRENT_RUN_PHASE_DELAY_TICKS)
    {
      continue;  // Skip overcurrent check during stabilization period
    }

    // Get current value and threshold for this motor
    float motor_current = Adc_driver_get_dc_motor_current(motor_num);
    float max_current_threshold;

    switch (motor_num)
    {
      case MOTOR_1_:
        max_current_threshold = wvar.motor_1_max_current_a;
        break;
      case MOTOR_2_:
        max_current_threshold = wvar.motor_2_max_current_a;
        break;
      case MOTOR_3_:
        max_current_threshold = wvar.motor_3_max_current_a;
        break;
      case MOTOR_4_:
        max_current_threshold = wvar.motor_4_max_current_a;
        break;
      default:
        continue;  // Skip invalid motor numbers
    }

    // Check overcurrent condition for this motor
    if (motor_current > max_current_threshold)
    {
      _Emergency_stop_all_motors();
      App_set_motor_overcurrent_flag(motor_num, motor_current, max_current_threshold);
      break;  // Exit loop after first overcurrent detection (all motors will be stopped)
    }
  }

  // Check motor 1 driver overtemperature
  if (adc.temp_motor1 > MOTOR_TEMP_EMERGENCY_THRESHOLD)
  {
    _Emergency_stop_all_motors();
    App_set_driver_overtemperature_flag(DRIVER_1, adc.temp_motor1, MOTOR_TEMP_EMERGENCY_THRESHOLD);
  }

  // Check motor 2 driver overtemperature
  if (adc.temp_motor2 > MOTOR_TEMP_EMERGENCY_THRESHOLD)
  {
    _Emergency_stop_all_motors();
    App_set_driver_overtemperature_flag(DRIVER_2, adc.temp_motor2, MOTOR_TEMP_EMERGENCY_THRESHOLD);
  }

  // За температурой процессора не следим. Так как она слишком грубо измеряется
  // Check CPU overtemperature
  // if (adc.cpu_temp > CPU_TEMP_EMERGENCY_THRESHOLD)
  // {
  //   _Emergency_stop_all_motors();
  //   // CPU overtemperature protection triggered
  //   App_set_cpu_overtemperature_flag(adc.cpu_temp, CPU_TEMP_EMERGENCY_THRESHOLD);
  // }

  // Check TMC6200 driver 1 fault flag
  if (g_system_error_flags.tmc6200_driver1_fault)
  {
    _Emergency_stop_all_motors();
  }

  // Check TMC6200 driver 2 fault flag
  if (g_system_error_flags.tmc6200_driver2_fault)
  {
    _Emergency_stop_all_motors();
  }
}

/*-----------------------------------------------------------------------------------------------------
  Motor Driver entry function.

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void Motor_driver_func(ULONG thread_input)
{
  uint32_t init_status_1;
  uint32_t init_status_2;
  uint8_t  driver_errors   = 0;
  g_motor_driver_ready     = 0;  // Reset readiness flag

  // Initialize calibration timer
  g_last_calibration_time  = tx_time_get();

  // Create motor calibration event flag group
  UINT event_create_status = tx_event_flags_create(&g_motor_calibration_events, "Motor Calibration Events");
  if (event_create_status != TX_SUCCESS)
  {
    APPLOG("Motor Driver: Failed to create calibration event flag group (status: %u)", (unsigned int)event_create_status);
    return;  // Cannot proceed without event flag group
  }

  // Reset TMC6200 driver enable signals
  Motor_driver_enable_set(DRIVER_1, 0);  // Disable motor driver 1
  Motor_driver_enable_set(DRIVER_2, 0);  // Disable motor driver 2
  APPLOG("Motor Drivers: Enable signals reset (disabled)");

  // Initialize global state variables for FreeMaster monitoring
  _Update_driver_enable_states();

  // Initialize TMC6200 motor driver chips - check both drivers before exiting
  init_status_1 = Motdrv_tmc6200_Initialize(DRIVER_1);  // Initialize motor driver 1
  if (init_status_1 != RES_OK)
  {
    APPLOG("Motor Driver 1: Initialization failed - %s", Motdrv_tmc6200_GetErrorString(g_tmc6200_driver1_error_code));
    App_set_tmc6200_driver_fault_flag(DRIVER_1);  // Set driver 1 fault flag
    driver_errors++;
  }
  else
  {
    APPLOG("Motor Driver 1: Initialization successful");
  }

  init_status_2 = Motdrv_tmc6200_Initialize(DRIVER_2);  // Initialize motor driver 2
  if (init_status_2 != RES_OK)
  {
    APPLOG("Motor Driver 2: Initialization failed - %s", Motdrv_tmc6200_GetErrorString(g_tmc6200_driver2_error_code));
    App_set_tmc6200_driver_fault_flag(DRIVER_2);  // Set driver 2 fault flag
    driver_errors++;
  }
  else
  {
    APPLOG("Motor Driver 2: Initialization successful");
  }

  // Check if any driver initialization failed
  if (driver_errors > 0)
  {
    APPLOG("Motor Drivers: %u out of 2 drivers failed initialization", (unsigned int)driver_errors);

    // Update monitoring structure with initialization error codes
    Motdrv_tmc6200_UpdateInitErrorCodes();

    // Activate LED alarm indication for motor driver errors
    Led_blink_set_patterns_GR(&LED_PATTERN_OFF, &LED_PATTERN_ERROR);
    APPLOG("Motor Drivers: LED alarm indication activated (Red LED error pattern)");

    g_motor_driver_ready = 0;
    return;  // Не входить в главный цикл
  }

  // Update monitoring structure with successful initialization error codes
  Motdrv_tmc6200_UpdateInitErrorCodes();

  // Initialize PWM phase control structure for all motors and phases
  _Init_pwm_phase_control();
  Init_PWM_triangle_buffered(wvar.pwm_frequency);
  Adc_driver_init(Pwm_update_all_phases_callback);
  PWM_start();

  // Initialize motor command queue
  _Init_motor_command_queue();
  // Initialize motor soft start system
  uint32_t soft_start_init_result = Motor_soft_start_init();
  if (soft_start_init_result != 0)
  {
    APPLOG("Motor Soft Start: Initialization failed with error %u", (unsigned int)soft_start_init_result);
    g_motor_driver_ready = 0;
    return;  // Do not enter main loop
  }
  else
  {
    APPLOG("Motor Soft Start: Initialization successful");
  }
  // Set motor algorithms based on configuration parameters
  _Set_motor_algorithms_from_parameters();

  g_motor_driver_ready = 1;  // Ready for operation

  // Enable motor drivers for normal operation
  Motor_driver_enable_set(DRIVER_1, 1);  // Enable motor driver 1
  Motor_driver_enable_set(DRIVER_2, 1);  // Enable motor driver 2
  APPLOG("Motor Drivers: Enable signals activated for normal operation");

  // Initialize TMC6200 monitoring system before creating thread
  Motdrv_tmc6200_InitMonitoring();

  // Create TMC6200 monitoring thread before entering main loop
  Tmc6200_monitoring_thread_create();

  // Perform motor current offset calibration before starting PWM
  _Perform_motor_current_offset_calibration();

  // Main motor driver loop
  while (1)
  {
    _Process_motor_commands();
    Adc_driver_process_samples();
    Motor_soft_start_process();                       // Process soft start/stop for all motors
    _Check_overcurrent_overtemperature_protection();  // Check overcurrent and overtemperature protection

    // Check each motor for stop condition and log max currents if needed
    _Check_and_log_stopped_motors_max_currents();

    // Update global state variables for FreeMaster monitoring
    _Update_driver_enable_states();
    _Update_max_current_tracking();

    // Handle periodic calibration
    _Handle_periodic_calibration();

    tx_thread_sleep(1);
  }
}
