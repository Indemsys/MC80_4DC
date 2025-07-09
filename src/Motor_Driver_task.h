#ifndef MOTOR_DRIVER_TASK_H
#define MOTOR_DRIVER_TASK_H

// Temperature threshold macros for overtemperature protection (in Celsius)
// Motor and CPU emergency stop thresholds
#define MOTOR_TEMP_EMERGENCY_THRESHOLD    90.0f  // Emergency stop threshold for motor temperature
#define CPU_TEMP_EMERGENCY_THRESHOLD      90.0f  // Emergency stop threshold for CPU temperature

// Motor current offset calibration constants
#define CALIBRATION_SAMPLES_COUNT         1000  // Number of samples to collect per motor for calibration
#define CALIBRATION_TIMEOUT_MS            5000  // Maximum timeout for calibration in milliseconds
#define CALIBRATION_SUCCESS               0     // Calibration successful return code
#define CALIBRATION_ERROR_TIMEOUT         1     // Calibration timeout error code
#define CALIBRATION_ERROR_DRIVERS_ON      2     // Motor drivers not disabled error code

// Motor calibration event flags for inter-thread communication
#define MOTOR_EVENT_CALIBRATION_REQUEST   0x00000001  // Request calibration from diagnostic terminal
#define MOTOR_EVENT_CALIBRATION_DONE      0x00000002  // Calibration completed acknowledgment
#define MOTOR_EVENT_CALIBRATION_ERROR     0x00000004  // Calibration error occurred

// Motor conflict management policy macros
#define MOTORS_1_2_MUTUALLY_EXCLUSIVE     0  // 0 = Motors 1&2 can work together, 1 = mutually exclusive
#define MOTORS_3_4_MUTUALLY_EXCLUSIVE     0  // 0 = Motors 3&4 can work together, 1 = mutually exclusive

#define MOTOR_COMMAND_QUEUE_SIZE          16

// Maximum current tracking constants
#define MAX_CURRENT_RUN_PHASE_DELAY_TICKS (TX_TIMER_TICKS_PER_SECOND / 2)  // 0.5 second delay for RUN phase current tracking

#define MOTOR_CMD_COAST                   1                                // Coast stop (free wheeling)
#define MOTOR_CMD_SOFT_START              2
#define MOTOR_CMD_SOFT_STOP               3
#define MOTOR_CMD_EMERGENCY_STOP          4                                // Emergency stop with dynamic braking

// Motor identification constants
#define MOTOR_1_                          1  //
#define MOTOR_2_                          2  //
#define MOTOR_3_                          3  //
#define MOTOR_4_                          4  //

// Driver identification constants (TMC6200 drivers)
#define DRIVER_1                          1  // TMC6200 driver 1 (motors 1 and 3)
#define DRIVER_2                          2  // TMC6200 driver 2 (motors 2 and 4)

// Motor array indices (0-based for array access)
#define MOT_1                             0  // Motor 1 array index
#define MOT_2                             1  // Motor 2 array index
#define MOT_3                             2  // Motor 3 array index
#define MOT_4                             3  // Motor 4 array index

// Phase array indices (0-based for array access)
#define PH_U                              0  // Phase U array index
#define PH_V                              1  // Phase V array index
#define PH_W                              2  // Phase W array index

// Operation phase indices for max current tracking
#define PHASE_ACCEL                       0  // Acceleration phase
#define PHASE_RUN                         1  // Constant speed phase
#define PHASE_BRAKE                       2  // Braking phase
#define MAX_CURRENT_PHASES                3  // Number of operation phases tracked

// Direction constants
#define MOTOR_DIRECTION_FORWARD           1
#define MOTOR_DIRECTION_REVERSE           2
#define MOTOR_DIRECTION_STOP              0

// Soft start/stop states for motor state tracking
#define MOTOR_STATE_IDLE                  0  // Motor stopped with dynamic braking
#define MOTOR_STATE_COASTING              1  // Motor stopped with free coasting
#define MOTOR_STATE_SOFT_RAMPING_UP       2  // Gradually increasing PWM
#define MOTOR_STATE_RUNNING               3  // Motor running at target level
#define MOTOR_STATE_SOFT_RAMPING_DOWN     4  // Gradually decreasing PWM

// Motor parameters structure
typedef struct
{
  uint32_t accel_time_ms;     // Acceleration time in milliseconds
  uint32_t decel_time_ms;     // Deceleration time in milliseconds
  uint16_t max_pwm_percent;   // Maximum PWM percentage (0-100%)
  uint8_t  direction_invert;  // Direction inversion flag
  uint8_t  algorithm;         // Acceleration/deceleration algorithm
} T_motor_parameters;

// Extended motor state structure that combines both regular and soft start states
// This structure replaces both T_motor_state and T_motor_soft_start_internal
typedef struct
{
  // Basic motor state (used by regular motor commands)
  uint8_t  direction;  // Current direction
  uint16_t pwm_level;  // Current PWM level (0-100%)
  uint8_t  enabled;    // Motor enabled flag
  // Soft start/stop state (used by Motor_Soft_Start system)
  uint8_t  soft_start_state;        // Current soft start state (MOTOR_STATE_*
  uint16_t target_pwm;              // Target PWM level for soft start (0-100%)
  uint16_t original_target_pwm;     // Original target PWM before soft stop (for proportional deceleration)
  uint16_t current_pwm_x100;        // Current PWM level scaled by 100 for precision
  uint32_t step_counter;            // Step counter for soft start algorithms
  uint16_t pwm_step_size_x100;      // PWM step size per millisecond scaled by 100
  uint8_t  target_direction;        // Target direction for soft start
  bool     soft_start_initialized;  // Soft start initialization flag
  bool     conflict_detected;       // Direction conflict flag
  uint32_t run_phase_start_time;    // Time when RUN phase started (for max current tracking delay)
  bool     max_current_logged;      // Flag indicating if max currents were logged after motor stop
} T_motor_extended_state;

// Motor command structure for ThreadX queue communication
typedef __packed struct
{
  uint8_t  cmd_type;    // Command type
  uint8_t  motor_num;   // Motor number (1-4 for specific motor, 0=all)
  uint16_t pwm_level;   // PWM level (0-100 percent)
  uint8_t  direction;   // Motor direction (MOTOR_DIRECTION_*)
  uint16_t ramp_time;   // Ramp time in milliseconds (for soft start/stop commands)
  uint8_t  padding[1];  // 1 byte to make total 8 bytes (aligned to ULONG boundary)
                        // Required for proper ThreadX queue operation
} T_motor_command;

// Maximum current tracking structure for all 4 motors and 3 operation phases
typedef struct
{
  float motor[4][MAX_CURRENT_PHASES];  // [motor_index][phase_index] - Maximum current values [A]
                                       // motor_index: 0=Motor1, 1=Motor2, 2=Motor3, 3=Motor4
                                       // phase_index: 0=Accel, 1=Run, 2=Brake
} T_max_current_tracking;

void    Motor_thread_create(void);
uint8_t Get_motor_driver_ready(void);

// Motor state access function
uint32_t Motor_get_state(uint8_t motor_num, uint8_t *enabled, uint16_t *pwm_level, uint8_t *direction);

// Extended motor state access functions - new unified interface
T_motor_extended_state *Motor_get_extended_state(uint8_t motor_num);  // Get pointer to motor's extended state

// Motor command functions for 4 DC motors
// Motor 1: - connected between V1-U1
// Motor 2: - connected between U1-W1
// Motor 3: - connected between U2-V2
// Motor 4: - connected between V2-W2
uint32_t Motor_command_coast(uint8_t motor_num);  // motor_num: 1-4 for specific motor, 0=all

// Soft start motor command functions
uint32_t Motor_command_soft_start(uint8_t motor_num, uint16_t target_pwm, uint8_t direction);  // Smooth motor start
uint32_t Motor_command_soft_stop(uint8_t motor_num);                                           // Smooth motor stop
uint32_t Motor_command_emergency_stop(uint8_t motor_num);                                      // Emergency stop without ramping (via queue)

// Direct motor control functions (bypass queue)
uint32_t Motor_emergency_stop_direct(uint8_t motor_num);  // Direct emergency stop (immediate, bypasses queue)

// Motor parameter access function
uint32_t Motor_get_parameters(uint8_t motor_num, T_motor_parameters *params);  // Get motor timing and configuration parameters

// Motor parameter getter and PWM setter functions for soft start module
void Get_motor_parameters(uint8_t motor_num, T_motor_parameters *params);                      // Get motor parameters (used by soft start)
void Motor_soft_start_pwm_setter(uint8_t motor_num, uint8_t direction, uint16_t pwm_percent);  // PWM setter for soft start

// Motor algorithm configuration functions

// Motor calibration functions for external access
uint32_t Motor_request_calibration(void);  // Request motor current offset calibration from external thread

// Motor calibration event flag group (global access for diagnostic terminal)
extern TX_EVENT_FLAGS_GROUP g_motor_calibration_events;

// Global variables for driver enable state monitoring (accessible from FreeMaster and terminal)
extern uint8_t g_drv1_en_state;  // Driver 1 enable state (0 = disabled, 1 = enabled)
extern uint8_t g_drv2_en_state;  // Driver 2 enable state (0 = disabled, 1 = enabled)

// Global structure for maximum current tracking (accessible from FreeMaster)
extern T_max_current_tracking g_max_current_tracking;

// Global motor states array (accessible from FreeMaster for monitoring all motor fields)
extern T_motor_extended_state g_motor_states[4];  // Motor states for all 4 motors (MOT_1 to MOT_4)

// Individual maximum current variables for easy FreeMaster access
// Motor 1 (Traction) max currents
extern float g_max_current_motor1_accel;  // Motor 1 maximum current during acceleration [A]
extern float g_max_current_motor1_run;    // Motor 1 maximum current during constant speed [A]
extern float g_max_current_motor1_brake;  // Motor 1 maximum current during braking [A]

// Motor 2 ( Motor 2) max currents
extern float g_max_current_motor2_accel;  // Motor 2 maximum current during acceleration [A]
extern float g_max_current_motor2_run;    // Motor 2 maximum current during constant speed [A]
extern float g_max_current_motor2_brake;  // Motor 2 maximum current during braking [A]

// Motor 3 ( Motor 3) max currents
extern float g_max_current_motor3_accel;  // Motor 3 maximum current during acceleration [A]
extern float g_max_current_motor3_run;    // Motor 3 maximum current during constant speed [A]
extern float g_max_current_motor3_brake;  // Motor 3 maximum current during braking [A]

// Motor 4 ( Motor 2) max currents
extern float g_max_current_motor4_accel;  // Motor 4 maximum current during acceleration [A]
extern float g_max_current_motor4_run;    // Motor 4 maximum current during constant speed [A]
extern float g_max_current_motor4_brake;  // Motor 4 maximum current during braking [A]

// Centralized driver enable control functions
void    Motor_driver_enable_set(uint8_t driver_num, uint8_t enable_state);  // Set driver enable state (0=disable, 1=enable)
void    Motor_driver_enable_toggle(uint8_t driver_num);                     // Toggle driver enable state
uint8_t Motor_driver_enable_get(uint8_t driver_num);                        // Get current driver enable state

// Maximum current tracking functions
void  Motor_max_current_reset(uint8_t motor_num, uint8_t phase);                        // Reset max current for specific motor and phase (0xFF for all)
void  Motor_max_current_update(uint8_t motor_num, uint8_t phase, float current_value);  // Update max current if new value is higher
void  Motor_max_current_reset_all(void);                                                // Reset all maximum current values
void  Motor_set_run_phase_start_time(uint8_t motor_num);                                // Set RUN phase start time for max current tracking delay
float Motor_max_current_get(uint8_t motor_num, uint8_t phase);                          // Get max current value for specific motor and phase

#endif                                                                                  // MOTOR_DRIVER_TASK_H
