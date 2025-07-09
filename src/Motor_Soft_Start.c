//----------------------------------------------------------------------
// File created on 2025-01-03
//----------------------------------------------------------------------
#include "App.h"

// Algorithm function pointer types
typedef void (*Alg_init_func)(uint8_t motor_index, T_motor_extended_state *motor);
typedef bool (*Alg_process_func)(uint8_t motor_index, T_motor_extended_state *motor);

// Algorithm implementation structure
typedef struct
{
  Alg_init_func    init_ramp_up;       // Initialize ramp up
  Alg_init_func    init_ramp_down;     // Initialize ramp down
  Alg_process_func process_ramp_up;    // Process ramp up step
  Alg_process_func process_ramp_down;  // Process ramp down step
} T_algorithm_impl;

// Global variables
static T_soft_start_algorithm g_motor_algorithms[MOTOR_SOFT_START_MAX_MOTORS] = {
  SOFT_START_ALG_LINEAR,  // Motor 1 - Traction
  SOFT_START_ALG_LINEAR,  // Motor 2 -  Motor 2
  SOFT_START_ALG_LINEAR,  // Motor 3 -  Motor 3
  SOFT_START_ALG_LINEAR   // Motor 4 -  Motor 2
};

// Algorithm implementations
static T_algorithm_impl g_algorithms[SOFT_START_ALG_COUNT];

// Static function declarations
static void    _Init_instant_algorithm(void);
static void    _Init_linear_algorithm(void);
static void    _Init_s_curve_algorithm(void);
static uint8_t _Check_direction_conflict(uint8_t motor_num, uint8_t new_direction);
static uint8_t _Get_actual_direction(uint8_t motor_num, uint8_t direction);
static uint8_t _Get_conflicting_motor(uint8_t motor_num);

// Linear algorithm implementation
static void _Linear_init_ramp_up(uint8_t motor_index, T_motor_extended_state *motor);
static void _Linear_init_ramp_down(uint8_t motor_index, T_motor_extended_state *motor);
static bool _Linear_process_ramp_up(uint8_t motor_index, T_motor_extended_state *motor);
static bool _Linear_process_ramp_down(uint8_t motor_index, T_motor_extended_state *motor);

// Instant algorithm implementation
static void _Instant_init_ramp_up(uint8_t motor_index, T_motor_extended_state *motor);
static void _Instant_init_ramp_down(uint8_t motor_index, T_motor_extended_state *motor);
static bool _Instant_process_ramp_up(uint8_t motor_index, T_motor_extended_state *motor);
static bool _Instant_process_ramp_down(uint8_t motor_index, T_motor_extended_state *motor);

// S-curve algorithm implementation
static void _S_curve_init_ramp_up(uint8_t motor_index, T_motor_extended_state *motor);
static void _S_curve_init_ramp_down(uint8_t motor_index, T_motor_extended_state *motor);
static bool _S_curve_process_ramp_up(uint8_t motor_index, T_motor_extended_state *motor);
static bool _S_curve_process_ramp_down(uint8_t motor_index, T_motor_extended_state *motor);

// S-curve mathematical functions (3-2 polynomial variant)
static float _S_curve_accel(float t, float T);
static float _S_curve_decel(float t, float T);

/*-----------------------------------------------------------------------------------------------------
  Initialize soft start system

  Parameters:
    None

  Return:
    0 - success, non-zero - error
-----------------------------------------------------------------------------------------------------*/
uint32_t Motor_soft_start_init(void)
{
  // Initialize algorithm implementations
  _Init_instant_algorithm();
  _Init_linear_algorithm();
  _Init_s_curve_algorithm();

  return 0;
}

/*-----------------------------------------------------------------------------------------------------
  Set soft start algorithm for specific motor

  Parameters:
    motor_num - motor number (1-4), or 0 for all motors
    algorithm - algorithm type to use

  Return:
    0 - success, non-zero - error
-----------------------------------------------------------------------------------------------------*/
uint32_t Motor_soft_start_set_algorithm(uint8_t motor_num, T_soft_start_algorithm algorithm)
{
  if (algorithm >= SOFT_START_ALG_COUNT)
  {
    return 1;  // Invalid algorithm
  }

  if (motor_num == 0)
  {
    // Set algorithm for all motors
    for (uint8_t i = 0; i < MOTOR_SOFT_START_MAX_MOTORS; i++)
    {
      g_motor_algorithms[i] = algorithm;
    }
    APPLOG("Soft start algorithm set to %u for all motors", (unsigned int)algorithm);
  }
  else if (motor_num >= 1 && motor_num <= MOTOR_SOFT_START_MAX_MOTORS)
  {
    // Set algorithm for specific motor
    g_motor_algorithms[motor_num - 1] = algorithm;
    APPLOG("Motor %u (%s) soft start algorithm set to %u", (unsigned int)motor_num, Get_motor_name(motor_num), (unsigned int)algorithm);
  }
  else
  {
    return 2;  // Invalid motor number
  }

  return 0;
}

/*-----------------------------------------------------------------------------------------------------
  Start motor with soft start

  Parameters:
    motor_num - motor number (1-4)
    target_pwm - target PWM level (0-100%)
    direction - motor direction (MOTOR_DIRECTION_FORWARD/REVERSE)

  Return:
    0 - motor started successfully
    1 - motor start rejected due to conflict
    2 - invalid parameters
-----------------------------------------------------------------------------------------------------*/
uint32_t Motor_soft_start_begin(uint8_t motor_num, uint16_t target_pwm, uint8_t direction)
{
  if (motor_num < 1 || motor_num > MOTOR_SOFT_START_MAX_MOTORS || target_pwm > 100)
  {
    return 2;  // Invalid parameters
  }

  // Check direction conflict
  if (_Check_direction_conflict(motor_num, direction) != 0)
  {
    APPLOG("Motor %u (%s) start rejected: direction conflict", (unsigned int)motor_num, Get_motor_name(motor_num));
    return 1;  // Conflict detected
  }
  uint8_t                 motor_index = motor_num - 1;
  T_motor_extended_state *motor       = Motor_get_extended_state(motor_num);

  if (motor == NULL)
  {
    return 2;  // Invalid motor number
  }
  // Get motor parameters from wvar
  T_motor_parameters motor_params;
  Get_motor_parameters(motor_num, &motor_params);

  // Apply direction inversion if configured
  uint8_t actual_direction = _Get_actual_direction(motor_num, direction);

  // Limit target PWM to maximum allowed
  if (target_pwm > motor_params.max_pwm_percent)
  {
    target_pwm = motor_params.max_pwm_percent;
    APPLOG("Motor %u (%s) PWM limited to %u%% (max allowed)", (unsigned int)motor_num, Get_motor_name(motor_num), (unsigned int)target_pwm);
  }  // Initialize motor for ramp up
  motor->soft_start_state       = MOTOR_STATE_SOFT_RAMPING_UP;
  motor->target_pwm             = target_pwm;
  motor->original_target_pwm    = target_pwm;  // Save original target for proportional deceleration
  motor->current_pwm_x100       = 0;           // Start from 0
  motor->target_direction       = actual_direction;
  motor->step_counter           = 0;
  motor->soft_start_initialized = true;
  // Update main motor state for proper display
  motor->enabled                = 1;  // Motor is now enabled
  motor->direction              = actual_direction;
  motor->pwm_level              = 0;  // Start from 0, will be updated in process function

  // Initialize algorithm-specific parameters
  T_algorithm_impl *alg         = &g_algorithms[g_motor_algorithms[motor_index]];
  if (alg->init_ramp_up != NULL)
  {
    alg->init_ramp_up(motor_index, motor);
  }
  APPLOG("Motor %u (%s) soft start initiated (target: %u%%, accel time: %ums, direction: %u, algorithm: %u)",
         (unsigned int)motor_num, Get_motor_name(motor_num), (unsigned int)target_pwm, (unsigned int)motor_params.accel_time_ms,
         (unsigned int)actual_direction, (unsigned int)g_motor_algorithms[motor_index]);

  return 0;
}

/*-----------------------------------------------------------------------------------------------------
  Stop motor with soft stop

  Parameters:
    motor_num - motor number (1-4)

  Return:
    0 - success, non-zero - error
-----------------------------------------------------------------------------------------------------*/
uint32_t Motor_soft_start_stop(uint8_t motor_num)
{
  if (motor_num < 1 || motor_num > MOTOR_SOFT_START_MAX_MOTORS)
  {
    return 1;  // Invalid parameters
  }
  uint8_t                 motor_index = motor_num - 1;
  T_motor_extended_state *motor       = Motor_get_extended_state(motor_num);

  if (motor == NULL)
  {
    return 1;  // Invalid motor number
  }

  // Only initiate soft stop if motor is not already stopping or idle
  if (motor->soft_start_state == MOTOR_STATE_RUNNING || motor->soft_start_state == MOTOR_STATE_SOFT_RAMPING_UP)
  {
    // Get motor parameters for deceleration
    T_motor_parameters motor_params;
    Get_motor_parameters(motor_num, &motor_params);
    motor->soft_start_state = MOTOR_STATE_SOFT_RAMPING_DOWN;
    motor->target_pwm       = 0;
    motor->step_counter     = 0;

    // Initialize algorithm-specific parameters for ramp down
    T_algorithm_impl *alg   = &g_algorithms[g_motor_algorithms[motor_index]];
    if (alg->init_ramp_down != NULL)
    {
      alg->init_ramp_down(motor_index, motor);
    }
    APPLOG("Motor %u (%s) soft stop initiated (current PWM: %u%%, decel time: %ums)",
           (unsigned int)motor_num, Get_motor_name(motor_num), (unsigned int)(motor->current_pwm_x100 / 100), (unsigned int)motor_params.decel_time_ms);
  }
  else if (motor->soft_start_state == MOTOR_STATE_IDLE)
  {
    // Motor is already stopped, update main state for consistency
    motor->enabled   = 0;
    motor->pwm_level = 0;
    motor->direction = MOTOR_DIRECTION_STOP;
  }
  return 0;
}

/*-----------------------------------------------------------------------------------------------------
  Process soft start/stop for all motors (call every millisecond)

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Motor_soft_start_process(void)
{
  for (uint8_t i = 0; i < MOTOR_SOFT_START_MAX_MOTORS; i++)
  {
    T_motor_extended_state *motor = Motor_get_extended_state(i + 1);

    if (motor == NULL || motor->soft_start_state == MOTOR_STATE_IDLE || !motor->soft_start_initialized)
    {
      continue;
    }

    // Get algorithm for this specific motor
    T_algorithm_impl *alg           = &g_algorithms[g_motor_algorithms[i]];
    bool              state_changed = false;

    if (motor->soft_start_state == MOTOR_STATE_SOFT_RAMPING_UP)
    {
      if (alg->process_ramp_up != NULL)
      {
        state_changed = alg->process_ramp_up(i, motor);
      }
    }
    else if (motor->soft_start_state == MOTOR_STATE_SOFT_RAMPING_DOWN)
    {
      if (alg->process_ramp_down != NULL)
      {
        state_changed = alg->process_ramp_down(i, motor);
      }
    }  // Apply current PWM to motor and update unified motor state
    if (motor->soft_start_state != MOTOR_STATE_IDLE)
    {
      uint16_t current_pwm = motor->current_pwm_x100 / 100;
      Motor_soft_start_pwm_setter(i + 1, motor->target_direction, current_pwm);
      // Synchronize PWM level with main motor state for display
      motor->pwm_level = current_pwm;
      motor->direction = motor->target_direction;
    }
    else
    {
      Motor_soft_start_pwm_setter(i + 1, MOTOR_DIRECTION_STOP, 0);
      // Synchronize stopped state with main motor state for display
      motor->pwm_level = 0;
      motor->direction = MOTOR_DIRECTION_STOP;
    }

    // Log state changes
    if (state_changed)
    {
      if (motor->soft_start_state == MOTOR_STATE_RUNNING)
      {
        APPLOG("Motor %u (%s) soft start completed (PWM: %u%%)", (unsigned int)(i + 1), Get_motor_name(i + 1), (unsigned int)(motor->current_pwm_x100 / 100));
      }
      else if (motor->soft_start_state == MOTOR_STATE_IDLE)
      {
        APPLOG("Motor %u (%s) soft stop completed", (unsigned int)(i + 1), Get_motor_name(i + 1));
      }
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Get soft start status for a motor

  Parameters:
    motor_num - motor number (1-4)
    status - pointer to status structure to fill

  Return:
    0 - success, non-zero - error
-----------------------------------------------------------------------------------------------------*/
uint32_t Motor_soft_start_get_status(uint8_t motor_num, T_soft_start_status *status)
{
  if (motor_num < 1 || motor_num > MOTOR_SOFT_START_MAX_MOTORS || status == NULL)
  {
    return 1;  // Invalid parameters
  }

  T_motor_extended_state *motor = Motor_get_extended_state(motor_num);

  if (motor == NULL)
  {
    return 1;  // Invalid motor number
  }

  status->state             = motor->soft_start_state;
  status->target_pwm        = motor->target_pwm;
  status->current_pwm       = motor->current_pwm_x100 / 100;
  status->target_direction  = motor->target_direction;
  status->conflict_detected = (_Check_direction_conflict(motor_num, motor->target_direction) != 0);

  // Calculate progress percentage
  if (motor->target_pwm > 0)
  {
    status->progress_percent = (uint8_t)((status->current_pwm * 100) / motor->target_pwm);
    if (status->progress_percent > 100)
    {
      status->progress_percent = 100;
    }
  }
  else
  {
    status->progress_percent = 0;
  }

  return 0;
}

/*-----------------------------------------------------------------------------------------------------
  Check if motor is active (in any state except IDLE)

  Parameters:
    motor_num - motor number (1-4)

  Return:
    true - motor is active, false - motor is idle
-----------------------------------------------------------------------------------------------------*/
bool Motor_soft_start_is_active(uint8_t motor_num)
{
  if (motor_num < 1 || motor_num > MOTOR_SOFT_START_MAX_MOTORS)
  {
    return false;
  }

  T_motor_extended_state *motor = Motor_get_extended_state(motor_num);
  if (motor == NULL)
  {
    return false;
  }

  return (motor->soft_start_state != MOTOR_STATE_IDLE && motor->soft_start_initialized);
}

/*-----------------------------------------------------------------------------------------------------
  Get current algorithm type for specific motor

  Parameters:
    motor_num - motor number (1-4)

  Return:
    Current algorithm type for the motor, or SOFT_START_ALG_LINEAR if invalid motor number
-----------------------------------------------------------------------------------------------------*/
T_soft_start_algorithm Motor_soft_start_get_algorithm(uint8_t motor_num)
{
  if (motor_num >= 1 && motor_num <= MOTOR_SOFT_START_MAX_MOTORS)
  {
    return g_motor_algorithms[motor_num - 1];
  }

  return SOFT_START_ALG_LINEAR;  // Default fallback
}

/*-----------------------------------------------------------------------------------------------------
  Get algorithms for all motors

  Parameters:
    algorithms - array to store algorithms (must be at least MOTOR_SOFT_START_MAX_MOTORS elements)

  Return:
    0 - success, non-zero - error
-----------------------------------------------------------------------------------------------------*/
uint32_t Motor_soft_start_get_all_algorithms(T_soft_start_algorithm *algorithms)
{
  if (algorithms == NULL)
  {
    return 1;  // Invalid parameter
  }

  for (uint8_t i = 0; i < MOTOR_SOFT_START_MAX_MOTORS; i++)
  {
    algorithms[i] = g_motor_algorithms[i];
  }
  return 0;
}

/*-----------------------------------------------------------------------------------------------------
  Initialize linear algorithm implementation

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Init_linear_algorithm(void)
{
  g_algorithms[SOFT_START_ALG_LINEAR].init_ramp_up      = _Linear_init_ramp_up;
  g_algorithms[SOFT_START_ALG_LINEAR].init_ramp_down    = _Linear_init_ramp_down;
  g_algorithms[SOFT_START_ALG_LINEAR].process_ramp_up   = _Linear_process_ramp_up;
  g_algorithms[SOFT_START_ALG_LINEAR].process_ramp_down = _Linear_process_ramp_down;
}

/*-----------------------------------------------------------------------------------------------------
  Initialize instant algorithm implementation

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Init_instant_algorithm(void)
{
  g_algorithms[SOFT_START_ALG_INSTANT].init_ramp_up      = _Instant_init_ramp_up;
  g_algorithms[SOFT_START_ALG_INSTANT].init_ramp_down    = _Instant_init_ramp_down;
  g_algorithms[SOFT_START_ALG_INSTANT].process_ramp_up   = _Instant_process_ramp_up;
  g_algorithms[SOFT_START_ALG_INSTANT].process_ramp_down = _Instant_process_ramp_down;
}

/*-----------------------------------------------------------------------------------------------------
  Initialize S-curve algorithm implementation

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Init_s_curve_algorithm(void)
{
  g_algorithms[SOFT_START_ALG_S_CURVE].init_ramp_up      = _S_curve_init_ramp_up;
  g_algorithms[SOFT_START_ALG_S_CURVE].init_ramp_down    = _S_curve_init_ramp_down;
  g_algorithms[SOFT_START_ALG_S_CURVE].process_ramp_up   = _S_curve_process_ramp_up;
  g_algorithms[SOFT_START_ALG_S_CURVE].process_ramp_down = _S_curve_process_ramp_down;
}

/*-----------------------------------------------------------------------------------------------------
  Check if motor direction conflicts with currently running motors

  Parameters:
    motor_num - motor number that wants to start (1-4)
    new_direction - direction of new motor

  Return:
    0 - no conflict, 1 - conflict detected
-----------------------------------------------------------------------------------------------------*/
static uint8_t _Check_direction_conflict(uint8_t motor_num, uint8_t new_direction)
{
  uint8_t conflicting_motor = _Get_conflicting_motor(motor_num);

  if (conflicting_motor == 0)
  {
    return 0;  // No conflicting motor
  }

  // Check if conflicting motor is active
  if (!Motor_soft_start_is_active(conflicting_motor))
  {
    return 0;  // No conflict - conflicting motor is not active
  }

  T_motor_extended_state *conflicting_motor_state = Motor_get_extended_state(conflicting_motor);

  if (conflicting_motor_state == NULL)
  {
    return 0;  // Cannot access conflicting motor state
  }

  // Get actual directions considering inversion flags
  uint8_t new_actual_direction     = _Get_actual_direction(motor_num, new_direction);
  uint8_t current_actual_direction = _Get_actual_direction(conflicting_motor, conflicting_motor_state->target_direction);

  // Check if actual directions match
  if (new_actual_direction == current_actual_direction)
  {
    return 0;  // No conflict - same actual direction
  }
  else
  {
    return 1;  // Conflict - different actual directions
  }
}

/*-----------------------------------------------------------------------------------------------------
  Get actual motor direction considering inversion flag

  Parameters:
    motor_num - motor number (1-4)
    direction - requested direction

  Return:
    Actual direction after applying inversion
-----------------------------------------------------------------------------------------------------*/
static uint8_t _Get_actual_direction(uint8_t motor_num, uint8_t direction)
{
  if (motor_num < 1 || motor_num > MOTOR_SOFT_START_MAX_MOTORS)
  {
    return direction;
  }

  // Get motor parameters to check inversion flag
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
  Get conflicting motor number for given motor

  Parameters:
    motor_num - motor number (1-4)

  Return:
    Conflicting motor number (1-4) or 0 if no conflict
-----------------------------------------------------------------------------------------------------*/
static uint8_t _Get_conflicting_motor(uint8_t motor_num)
{
  // Motor pairs that share phases:
  // Motor 1 (Traction) and Motor 2 ( Motor 2) share V1
  // Motor 3 ( Motor 3) and Motor 4 ( Motor 2) share V2
  switch (motor_num)
  {
    case 1:
      return 2;  // Motor 1 conflicts with Motor 2
    case 2:
      return 1;  // Motor 2 conflicts with Motor 1
    case 3:
      return 4;  // Motor 3 conflicts with Motor 4
    case 4:
      return 3;  // Motor 4 conflicts with Motor 3
    default:
      return 0;  // No conflict
  }
}

// ================================================================================================
// LINEAR ALGORITHM IMPLEMENTATION
// ================================================================================================

/*-----------------------------------------------------------------------------------------------------
  Initialize linear ramp up

  Parameters:
    motor_index - motor index (0-3)
    motor - pointer to motor structure

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Linear_init_ramp_up(uint8_t motor_index, T_motor_extended_state *motor)
{
  (void)motor_index;  // Unused parameter

  // Initialize linear acceleration - no step size calculation needed
  // We'll calculate PWM based on elapsed time ratio
  motor->step_counter = 0;  // Will track elapsed time in milliseconds
}

/*-----------------------------------------------------------------------------------------------------
  Initialize linear ramp down

  Parameters:
    motor_index - motor index (0-3)
    motor - pointer to motor structure

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Linear_init_ramp_down(uint8_t motor_index, T_motor_extended_state *motor)
{
  (void)motor_index;  // Unused parameter

  // Initialize linear deceleration - no step size calculation needed
  // We'll calculate PWM based on elapsed time ratio
  motor->step_counter = 0;  // Will track elapsed time in milliseconds
}

/*-----------------------------------------------------------------------------------------------------
  Process linear ramp up step

  Parameters:
    motor_index - motor index (0-3)
    motor - pointer to motor structure

  Return:
    true - state changed, false - no state change
-----------------------------------------------------------------------------------------------------*/
static bool _Linear_process_ramp_up(uint8_t motor_index, T_motor_extended_state *motor)
{
  // Get motor parameters for time calculation
  T_motor_parameters motor_params;
  Get_motor_parameters(motor_index + 1, &motor_params);

  motor->step_counter++;  // Increment elapsed time in milliseconds

  // Calculate current PWM based on elapsed time ratio
  if (motor_params.accel_time_ms > 0)
  {
    // Calculate linear progression: current_pwm = (elapsed_time / total_time) * target_pwm
    uint32_t elapsed_time_ms = motor->step_counter;
    uint32_t target_pwm_x100 = motor->target_pwm * 100;
    if (elapsed_time_ms >= motor_params.accel_time_ms)
    {
      // Acceleration complete
      motor->current_pwm_x100 = target_pwm_x100;
      motor->soft_start_state = MOTOR_STATE_RUNNING;
      Motor_set_run_phase_start_time(motor_index + 1);  // Set RUN phase start time for current tracking
      return true;  // State changed
    }
    else
    {
      // Calculate current PWM: (elapsed_time * target_pwm) / total_time
      motor->current_pwm_x100 = (uint16_t)((elapsed_time_ms * target_pwm_x100) / motor_params.accel_time_ms);
    }
  }
  else
  {
    // Instant acceleration
    motor->current_pwm_x100 = motor->target_pwm * 100;
    motor->soft_start_state = MOTOR_STATE_RUNNING;
    Motor_set_run_phase_start_time(motor_index + 1);  // Set RUN phase start time for current tracking
    return true;  // State changed
  }

  return false;   // No state change
}

/*-----------------------------------------------------------------------------------------------------
  Process linear ramp down step

  Parameters:
    motor_index - motor index (0-3)
    motor - pointer to motor structure

  Return:
    true - state changed, false - no state change
-----------------------------------------------------------------------------------------------------*/
static bool _Linear_process_ramp_down(uint8_t motor_index, T_motor_extended_state *motor)
{
  // Get motor parameters for time calculation
  T_motor_parameters motor_params;
  Get_motor_parameters(motor_index + 1, &motor_params);

  motor->step_counter++;  // Increment elapsed time in milliseconds

  // Calculate current PWM based on elapsed time ratio
  if (motor_params.decel_time_ms > 0)
  {
    uint32_t elapsed_time_ms = motor->step_counter;
    uint16_t initial_pwm     = motor->original_target_pwm;  // Use original target PWM
    if (elapsed_time_ms >= motor_params.decel_time_ms)
    {
      // Deceleration complete
      motor->current_pwm_x100       = 0;
      motor->soft_start_state       = MOTOR_STATE_IDLE;
      motor->target_direction       = MOTOR_DIRECTION_STOP;
      motor->soft_start_initialized = false;
      motor->enabled                = 0;
      return true;  // State changed
    }
    else
    {
      // Calculate current PWM: initial_pwm * (1 - elapsed_time / total_time)
      uint32_t remaining_ratio = motor_params.decel_time_ms - elapsed_time_ms;
      motor->current_pwm_x100  = (uint16_t)((remaining_ratio * initial_pwm * 100) / motor_params.decel_time_ms);
    }
  }
  else
  {
    // Instant deceleration
    motor->current_pwm_x100       = 0;
    motor->soft_start_state       = MOTOR_STATE_IDLE;
    motor->target_direction       = MOTOR_DIRECTION_STOP;
    motor->soft_start_initialized = false;
    motor->enabled                = 0;
    return true;  // State changed
  }

  return false;   // No state change
}

// ================================================================================================
// INSTANT ALGORITHM IMPLEMENTATION
// ================================================================================================

/*-----------------------------------------------------------------------------------------------------
  Initialize instant ramp up - immediately set target PWM without ramping

  Parameters:
    motor_index - motor index (0-3)
    motor - pointer to motor structure

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Instant_init_ramp_up(uint8_t motor_index, T_motor_extended_state *motor)
{
  (void)motor_index;  // Unused parameter

  // Instantly set PWM to target level
  motor->current_pwm_x100 = motor->target_pwm * 100;
  motor->step_counter     = 0;
}

/*-----------------------------------------------------------------------------------------------------
  Initialize instant ramp down - immediately set PWM to zero

  Parameters:
    motor_index - motor index (0-3)
    motor - pointer to motor structure

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Instant_init_ramp_down(uint8_t motor_index, T_motor_extended_state *motor)
{
  (void)motor_index;  // Unused parameter

  // Instantly set PWM to zero
  motor->current_pwm_x100 = 0;
  motor->step_counter     = 0;
}

/*-----------------------------------------------------------------------------------------------------
  Process instant ramp up step - motor is already at target level

  Parameters:
    motor_index - motor index (0-3)
    motor - pointer to motor structure

  Return:
    true - state changed, false - no state change
-----------------------------------------------------------------------------------------------------*/
static bool _Instant_process_ramp_up(uint8_t motor_index, T_motor_extended_state *motor)
{
  (void)motor_index;  // Unused parameter

  // Motor is already at target level, transition to running state
  motor->soft_start_state = MOTOR_STATE_RUNNING;
  Motor_set_run_phase_start_time(motor_index + 1);  // Set RUN phase start time for current tracking
  // Update main motor state for display
  motor->pwm_level        = motor->target_pwm;
  motor->direction        = motor->target_direction;
  return true;  // State changed
}

/*-----------------------------------------------------------------------------------------------------
  Process instant ramp down step - motor is already stopped

  Parameters:
    motor_index - motor index (0-3)
    motor - pointer to motor structure

  Return:
    true - state changed, false - no state change
-----------------------------------------------------------------------------------------------------*/
static bool _Instant_process_ramp_down(uint8_t motor_index, T_motor_extended_state *motor)
{
  (void)motor_index;  // Unused parameter

  // Motor is already stopped, transition to idle state
  motor->soft_start_state       = MOTOR_STATE_IDLE;
  motor->target_direction       = MOTOR_DIRECTION_STOP;
  motor->soft_start_initialized = false;
  // Update main motor state for display
  motor->enabled                = 0;
  motor->pwm_level              = 0;
  motor->direction              = MOTOR_DIRECTION_STOP;
  return true;  // State changed
}

// ================================================================================================
// S-CURVE ALGORITHM IMPLEMENTATION
// ================================================================================================

/*-----------------------------------------------------------------------------------------------------
  Initialize S-curve ramp up

  Parameters:
    motor_index - motor index (0-3)
    motor - pointer to motor structure

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _S_curve_init_ramp_up(uint8_t motor_index, T_motor_extended_state *motor)
{
  T_motor_parameters motor_params;
  Get_motor_parameters(motor_index + 1, &motor_params);

  // Initialize S-curve acceleration parameters
  motor->step_counter       = 0;

  // For S-curve, step_counter tracks elapsed time in milliseconds
  // pwm_step_size_x100 stores target PWM * 100 for direct calculation
  motor->pwm_step_size_x100 = motor->target_pwm * 100;
}

/*-----------------------------------------------------------------------------------------------------
  Initialize S-curve ramp down

  Parameters:
    motor_index - motor index (0-3)
    motor - pointer to motor structure

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _S_curve_init_ramp_down(uint8_t motor_index, T_motor_extended_state *motor)
{
  T_motor_parameters motor_params;
  Get_motor_parameters(motor_index + 1, &motor_params);

  // Initialize S-curve deceleration parameters
  motor->step_counter       = 0;

  // Store current PWM level for deceleration calculation
  uint16_t initial_pwm_x100 = motor->current_pwm_x100;
  motor->pwm_step_size_x100 = initial_pwm_x100;
}

/*-----------------------------------------------------------------------------------------------------
  Process S-curve ramp up step

  Parameters:
    motor_index - motor index (0-3)
    motor - pointer to motor structure

  Return:
    true - state changed, false - no state change
-----------------------------------------------------------------------------------------------------*/
static bool _S_curve_process_ramp_up(uint8_t motor_index, T_motor_extended_state *motor)
{  // Get acceleration time for S-curve calculation
  T_motor_parameters motor_params;
  Get_motor_parameters(motor_index + 1, &motor_params);

  motor->step_counter++;  // Increment elapsed time (milliseconds)
  if (motor_params.accel_time_ms == 0)
  {
    // Instant acceleration
    motor->current_pwm_x100 = motor->pwm_step_size_x100;
    motor->soft_start_state = MOTOR_STATE_RUNNING;
    Motor_set_run_phase_start_time(motor_index + 1);  // Set RUN phase start time for current tracking
    // Update main motor state for display
    motor->pwm_level        = motor->target_pwm;
    motor->direction        = motor->target_direction;
    return true;  // State changed
  }

  // Calculate S-curve coefficient using polynomial function
  float t                 = (float)motor->step_counter / 1000.0f;         // Current time in seconds
  float T                 = (float)motor_params.accel_time_ms / 1000.0f;  // Total time in seconds
  float s_coeff           = _S_curve_accel(t, T);

  // Apply S-curve coefficient to target PWM
  motor->current_pwm_x100 = (uint16_t)(motor->pwm_step_size_x100 * s_coeff);
  if (s_coeff >= 1.0f)
  {
    motor->current_pwm_x100 = motor->pwm_step_size_x100;  // Ensure exact target
    motor->soft_start_state = MOTOR_STATE_RUNNING;
    Motor_set_run_phase_start_time(motor_index + 1);     // Set RUN phase start time for current tracking
    // Update main motor state for display
    motor->pwm_level        = motor->target_pwm;
    motor->direction        = motor->target_direction;
    return true;  // Target reached
  }
  return false;   // Still ramping up
}

/*-----------------------------------------------------------------------------------------------------
  Process S-curve ramp down step

  Parameters:
    motor_index - motor index (0-3)
    motor - pointer to motor structure

  Return:
    true - state changed, false - no state change
-----------------------------------------------------------------------------------------------------*/
static bool _S_curve_process_ramp_down(uint8_t motor_index, T_motor_extended_state *motor)
{
  // Get deceleration time for S-curve calculation
  T_motor_parameters motor_params;
  Get_motor_parameters(motor_index + 1, &motor_params);

  motor->step_counter++;  // Increment elapsed time (milliseconds)

  if (motor_params.decel_time_ms == 0)
  {
    // Instant deceleration
    motor->current_pwm_x100       = 0;
    motor->soft_start_state       = MOTOR_STATE_IDLE;
    motor->target_direction       = MOTOR_DIRECTION_STOP;
    motor->soft_start_initialized = false;
    return true;  // State changed
  }
  // Calculate S-curve coefficient using polynomial function
  float t = (float)motor->step_counter / 1000.0f;         // Current time in seconds
  float T = (float)motor_params.decel_time_ms / 1000.0f;  // Total time in seconds
  // Calculate proportional deceleration time based on current PWM level
  // Use original_target_pwm since target_pwm is set to 0 during soft stop
  if (motor->original_target_pwm > 0)
  {
    uint16_t initial_pwm = motor->pwm_step_size_x100 / 100;
    if (initial_pwm > 0)
    {
      uint32_t effective_decel_time_ms = (initial_pwm * motor_params.decel_time_ms) / motor->original_target_pwm;

      // Ensure minimum deceleration time for safety
      if (effective_decel_time_ms < 100)
      {
        effective_decel_time_ms = 100;               // Minimum 100ms
      }

      T = (float)effective_decel_time_ms / 1000.0f;  // Use proportional time
    }
  }
  float s_coeff           = _S_curve_decel(t, T);

  // Apply S-curve coefficient (deceleration from initial PWM level)
  motor->current_pwm_x100 = (uint16_t)(motor->pwm_step_size_x100 * s_coeff);
  if (s_coeff <= 0.0f)
  {
    motor->current_pwm_x100       = 0;
    motor->soft_start_state       = MOTOR_STATE_IDLE;
    motor->target_direction       = MOTOR_DIRECTION_STOP;
    motor->soft_start_initialized = false;
    // Update main motor state for display
    motor->enabled                = 0;
    motor->pwm_level              = 0;
    motor->direction              = MOTOR_DIRECTION_STOP;
    return true;  // Stopped
  }

  return false;   // Still ramping down
}

// ================================================================================================
// S-CURVE MATHEMATICAL FUNCTIONS (3-2 POLYNOMIAL VARIANT)
// ================================================================================================

/*-----------------------------------------------------------------------------------------------------
  S-curve acceleration function (smooth acceleration 0 → 1)

  Uses 3τ² - 2τ³ polynomial for smooth acceleration curve with zero derivative at endpoints

  Parameters:
    t - current time from start of acceleration phase (seconds)
    T - total duration of acceleration phase (seconds)

  Return:
    Acceleration coefficient [0.0 ; 1.0]
-----------------------------------------------------------------------------------------------------*/
static float _S_curve_accel(float t, float T)
{
  if (t <= 0.0f)
  {
    return 0.0f;
  }
  if (t >= T)
  {
    return 1.0f;
  }

  float tau = t / T;                       // Normalized time 0‥1
  return tau * tau * (3.0f - 2.0f * tau);  // Formula 3τ² − 2τ³ = τ²·(3 − 2τ)
}

/*-----------------------------------------------------------------------------------------------------
  S-curve deceleration function (smooth deceleration 1 → 0)

  Symmetric to acceleration: 1 - s_curve_accel

  Parameters:
    t - current time from start of deceleration phase (seconds)
    T - total duration of deceleration phase (seconds)

  Return:
    Deceleration coefficient [0.0 ; 1.0]
-----------------------------------------------------------------------------------------------------*/
static float _S_curve_decel(float t, float T)
{
  return 1.0f - _S_curve_accel(t, T);
}
