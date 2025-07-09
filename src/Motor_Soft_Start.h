//----------------------------------------------------------------------
// File created on 2025-01-03
//----------------------------------------------------------------------
#ifndef MOTOR_SOFT_START_H
#define MOTOR_SOFT_START_H

// Maximum number of motors supported
#define MOTOR_SOFT_START_MAX_MOTORS 4

// Soft start algorithm types
typedef enum
{
  SOFT_START_ALG_INSTANT = 0,  // Instant start/stop (no ramping)
  SOFT_START_ALG_LINEAR  = 1,  // Linear ramp up/down
  SOFT_START_ALG_S_CURVE = 2,  // S-curve (smooth acceleration/deceleration)
  SOFT_START_ALG_COUNT         // Number of available algorithms
} T_soft_start_algorithm;

// Soft start status information structure
typedef struct
{
  uint8_t  state;              // Current soft start state
  uint16_t target_pwm;         // Target PWM level (0-100%)
  uint16_t current_pwm;        // Current actual PWM level (0-100%)
  uint8_t  target_direction;   // Target direction
  uint8_t  progress_percent;   // Progress percentage (0-100%)
  bool     conflict_detected;  // Direction conflict flag
} T_soft_start_status;

/*-----------------------------------------------------------------------------------------------------
  Initialize soft start system

  Parameters:
    None

  Return:
    0 - success, non-zero - error
-----------------------------------------------------------------------------------------------------*/
uint32_t Motor_soft_start_init(void);

/*-----------------------------------------------------------------------------------------------------
  Set soft start algorithm for specific motor

  Parameters:
    motor_num - motor number (1-4), or 0 for all motors
    algorithm - algorithm type to use

  Return:
    0 - success, non-zero - error
-----------------------------------------------------------------------------------------------------*/
uint32_t Motor_soft_start_set_algorithm(uint8_t motor_num, T_soft_start_algorithm algorithm);

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
uint32_t Motor_soft_start_begin(uint8_t motor_num, uint16_t target_pwm, uint8_t direction);

/*-----------------------------------------------------------------------------------------------------
  Stop motor with soft stop

  Parameters:
    motor_num - motor number (1-4)

  Return:
    0 - success, non-zero - error
-----------------------------------------------------------------------------------------------------*/
uint32_t Motor_soft_start_stop(uint8_t motor_num);

/*-----------------------------------------------------------------------------------------------------
  Process soft start/stop for all motors (call every millisecond)

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Motor_soft_start_process(void);

/*-----------------------------------------------------------------------------------------------------
  Get soft start status for a motor

  Parameters:
    motor_num - motor number (1-4)
    status - pointer to status structure to fill

  Return:
    0 - success, non-zero - error
-----------------------------------------------------------------------------------------------------*/
uint32_t Motor_soft_start_get_status(uint8_t motor_num, T_soft_start_status *status);

/*-----------------------------------------------------------------------------------------------------
  Check if motor is active (in any state except IDLE)

  Parameters:
    motor_num - motor number (1-4)

  Return:
    true - motor is active, false - motor is idle
-----------------------------------------------------------------------------------------------------*/
bool Motor_soft_start_is_active(uint8_t motor_num);

/*-----------------------------------------------------------------------------------------------------
  Get current algorithm type for specific motor

  Parameters:
    motor_num - motor number (1-4)

  Return:
    Current algorithm type for the motor, or SOFT_START_ALG_LINEAR if invalid motor number
-----------------------------------------------------------------------------------------------------*/
T_soft_start_algorithm Motor_soft_start_get_algorithm(uint8_t motor_num);

/*-----------------------------------------------------------------------------------------------------
  Get algorithms for all motors

  Parameters:
    algorithms - array to store algorithms (must be at least MOTOR_SOFT_START_MAX_MOTORS elements)

  Return:
    0 - success, non-zero - error
-----------------------------------------------------------------------------------------------------*/
uint32_t Motor_soft_start_get_all_algorithms(T_soft_start_algorithm *algorithms);

#endif  // MOTOR_SOFT_START_H
