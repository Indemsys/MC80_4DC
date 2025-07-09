#ifndef CAN_PROTOCOL_H
#define CAN_PROTOCOL_H

/*-----------------------------------------------------------------------------------------------------
  MC80 4DC CAN COMMUNICATION PROTOCOL SPECIFICATION
  ====================================================

  This protocol defines the communication interface between the central controller and
  four motor controller nodes in the MC80 4DC system. The protocol supports real-time
  motor control, status monitoring, and telemetry data exchange over CAN bus.

  SYSTEM ARCHITECTURE:
  - Central Controller: Main system controller managing overall operation
  - 4 Motor Controller Nodes: Individual controllers for each motor subsystem
    * Node 1 (MOT3_ID): Motor 3 (Motor 3: U2-V2)
    * Node 2 (MOT4_ID): Motor 4 (Motor 4: V2-W2)
    * Node 3 (MOT2_ID): Motor 2 (Motor 2: U1-W1)
    * Node 4 (TRACTION_MOT_ID): Traction motor (Motor 1: V1-U1)

  COMMUNICATION PATTERN:
  1. Central controller broadcasts REQUEST_STATE_TO_ALL command every 1ms
  2. Each motor node responds with 3 sequential packets:
     - Status packet (7 bytes): Error flags, movement info, PWM state
     - Sensor packet (8 bytes): Position sensor, RPM, temperature
     - Power packet (8 bytes): Voltage, current, power consumption
  3. Central controller can send system control commands for motor operation

  CAN ID STRUCTURE:
  - Base message types: 0x1A01FFFF (request), 0x1A02FFFF (response),
    0x1A03FFFF (sensor), 0x1A04FFFF (power)
  - Node ID embedded in bits 23-20: (node_id << 20) | base_message_type
  - Extended CAN format (29-bit identifiers)

  DATA ENCODING:
  - All analog values scaled by factor of 10 for fixed-point integer transmission
  - Temperature: °C × 10 (25.6°C → 256)
  - Voltage: V × 10 (12.5V → 125)
  - Current: A × 10 (1.23A → 12)
  - Power: W × 10 (15.7W → 157)

  MOTOR CONTROL:
  - Soft start/stop with configurable acceleration/deceleration times
  - PWM-based speed control with maximum duty cycle limits
  - Direction inversion support via configuration parameters
  - Emergency stop functionality with immediate motor shutdown
-----------------------------------------------------------------------------------------------------*/

// Motor node identifiers for CAN message routing
#define MOT3_ID                       1  // Motor 3 controller
#define MOT4_ID                       2  // Motor 4 controller
#define MOT2_ID                       3  // Motor 2 controller
#define TRACTION_MOT_ID               4  // Traction motor controller

// Protocol magic words for operation validation
#define PARAM_SAVE_MAGIC_WORD         0x53415645  // "SAVE" in ASCII
#define RESET_MAGIC_WORD              0x52535420  // "RST " in ASCII
#define CLEAR_MOTOR_ERRORS_MAGIC_WORD 0x434C5253  // "CLRS" in ASCII

// Base CAN message type identifiers
#define MC80_REQ                      0x1A01FFFF  // Request command from central controller
#define MC80_ANS                      0x1A02FFFF  // Status response from motor node
#define MC80_SENS_INFO                0x1A03FFFF  // Sensor data response from motor node
#define MC80_PWR_INFO                 0x1A04FFFF  // Power data response from motor node
#define MC80_PARAM_WR                 0x1A05FFFF  // Parameter write request to MC80 4DC
#define MC80_PARAM_RD                 0x1A06FFFF  // Parameter read request from MC80 4DC
#define MC80_PARAM_ANS                0x1A07FFFF  // Parameter response from MC80 4DC
#define MC80_PARAM_SAVE               0x1A08FFFF  // Parameter save to NV memory request
#define MC80_RESET                    0x1A09FFFF  // Controller reset command
#define MC80_CLEAR_MOTOR_ERRORS       0x1A0AFFFF  // Clear motor overcurrent and emergency stop errors

// Complete CAN identifiers for motor (Node 1)
#define MOT3_CMD                      (MC80_REQ | (MOT3_ID << 20))
#define MOT3_ANS                      (MC80_ANS | (MOT3_ID << 20))
#define MOT3_SENS_INFO                (MC80_SENS_INFO | (MOT3_ID << 20))
#define MOT3_PWR_INFO                 (MC80_PWR_INFO | (MOT3_ID << 20))

// Complete CAN identifiers for motor (Node 2)
#define MOT4_CMD                      (MC80_REQ | (MOT4_ID << 20))
#define MOT4_ANS                      (MC80_ANS | (MOT4_ID << 20))
#define MOT4_SENS_INFO                (MC80_SENS_INFO | (MOT4_ID << 20))
#define MOT4_PWR_INFO                 (MC80_PWR_INFO | (MOT4_ID << 20))

// Complete CAN identifiers for motor (Node 3)
#define MOT2_CMD                      (MC80_REQ | (MOT2_ID << 20))
#define MOT2_ANS                      (MC80_ANS | (MOT2_ID << 20))
#define MOT2_SENS_INFO                (MC80_SENS_INFO | (MOT2_ID << 20))
#define MOT2_PWR_INFO                 (MC80_PWR_INFO | (MOT2_ID << 20))

// Complete CAN identifiers for traction motor (Node 4)
#define TRACTION_MOT_CMD              (MC80_REQ | (TRACTION_MOT_ID << 20))
#define TRACTION_MOT_ANS              (MC80_ANS | (TRACTION_MOT_ID << 20))
#define TRACTION_MOT_SENS_INFO        (MC80_SENS_INFO | (TRACTION_MOT_ID << 20))
#define TRACTION_MOT_PWR_INFO         (MC80_PWR_INFO | (TRACTION_MOT_ID << 20))

// Central controller broadcast commands
#define REQUEST_STATE_TO_ALL          0x00000001  // Broadcast: request status from all nodes
#define REQUEST_SYS_CONTROL           0x00000002  // Broadcast: system control command

// Motor soft start/stop state values (defined in Motor_Driver_task.h):
// MOTOR_STATE_IDLE              = 0  // Motor not starting/stopping
// MOTOR_STATE_SOFT_RAMPING_UP   = 1  // Gradually increasing PWM
// MOTOR_STATE_RUNNING           = 2  // Motor running at target level
// MOTOR_STATE_SOFT_RAMPING_DOWN = 3  // Gradually decreasing PWM

// Response packet command codes
#define MC80_REQ_STATUS               0x01  // Status request command code

// System control command bit definitions
#define MOT3_UP_BIT                   (1 << 0)   // Motor 3 up command
#define MOT3_DOWN_BIT                 (1 << 1)   // Motor 3 down command
#define MOT3_STOP_BIT                 (1 << 2)   // Motor 3 soft stop
#define MOT3_HARD_STOP_BIT            (1 << 3)   // Motor 3 hard stop
#define MOT4_UP_BIT                   (1 << 4)   // Motor 4 up command
#define MOT4_DOWN_BIT                 (1 << 5)   // Motor 4 down command
#define MOT4_STOP_BIT                 (1 << 6)   // Motor 4 soft stop
#define MOT4_HARD_STOP_BIT            (1 << 7)   // Motor 4 hard stop
#define MOT2_UP_BIT                   (1 << 8)   // Motor 2 up command
#define MOT2_DOWN_BIT                 (1 << 9)   // Motor 2 down command
#define MOT2_STOP_BIT                 (1 << 10)  // Motor 2 soft stop
#define MOT2_HARD_STOP_BIT            (1 << 11)  // Motor 2 hard stop
#define MOT_UP_BIT                    (1 << 12)  // Traction motor clockwise
#define MOT_DOWN_BIT                  (1 << 13)  // Traction motor counter-clockwise
#define MOT_STOP_BIT                  (1 << 14)  // Traction motor soft stop
#define MOT_HARD_STOP_BIT             (1 << 15)  // Traction motor hard stop
#define ALARM_BIT                     (1 << 16)  // System alarm signal
#define COAST_ALL_BIT                 (1 << 17)  // Coast stop all motors (free wheeling)

// Packet 1: Main status response (7 bytes total)
// CAN ID: MC80_ANS | (node_id << 20)
typedef __packed struct
{
  uint8_t  command_code;   // Byte 0: Always MC80_REQ_STATUS (0x01)
  uint32_t error_flags;    // Bytes 1-4: Error flags from App_get_error_flags()
  uint16_t movement_info;  // Bytes 5-6: Movement state information
                           // Bit 7: Motor active state (1=active, 0=inactive)
                           // Bits 0-6: Motor soft start state (MOTOR_STATE_* constants)
                           // Bits 8-15: PWM duty cycle as signed int8_t (0-100%)
} T_can_status_packet1;

// Packet 2: Sensor information response (8 bytes total)
// CAN ID: MC80_SENS_INFO | (node_id << 20)
typedef __packed struct
{
  uint16_t position_sensor;  // Bytes 0-1: Position sensor ADC counts
                             // MOT3_ID: adc.filt_pos_motor1
                             // MOT4_ID: adc.filt_pos_motor2
                             // MOT2_ID: 0 (no position sensor)
                             // TRACTION_MOT_ID: 0 (no position sensor)

  uint32_t motor_rpm;        // Bytes 2-5: Motor RPM (always 0 in current implementation)

  int16_t driver_temp_x10;   // Bytes 6-7: Driver temperature in °C × 10
                             // MOT3_ID: adc.temp_motor2 × 10
                             // MOT4_ID: adc.temp_motor2 × 10
                             // MOT2_ID: adc.temp_motor1 × 10
                             // TRACTION_MOT_ID: adc.temp_motor1 × 10
} T_can_status_packet2;

// Packet 3: Power information response (8 bytes total)
// CAN ID: MC80_PWR_INFO | (node_id << 20)
typedef __packed struct
{
  int16_t input_voltage_x10;  // Bytes 0-1: Supply voltage in V × 10
                              // All nodes: adc.v24v_supply × 10

  int16_t input_current_x10;  // Bytes 2-3: Driver input current in A × 10
                              // MOT3_ID: adc.ipwr_motor2 × 10
                              // MOT4_ID: adc.ipwr_motor2 × 10
                              // MOT2_ID: adc.ipwr_motor1 × 10
                              // TRACTION_MOT_ID: adc.ipwr_motor1 × 10

  int16_t input_power_x10;    // Bytes 4-5: Input power in W × 10
                              // Calculated as: input_voltage × input_current

  int16_t motor_current_x10;  // Bytes 6-7: Motor current in A × 10
                              // Same mapping as input_current_x10
} T_can_status_packet3;

// System control command structure
// Used with REQUEST_SYS_CONTROL command
// clang-format off
typedef __packed struct
{
  uint32_t MOT3_UP         : 1;  // Bit 0  - Motor 3 up command
  uint32_t MOT3_DOWN       : 1;  // Bit 1  - Motor 3 down command
  uint32_t MOT3_STOP       : 1;  // Bit 2  - Motor 3 soft  stop
  uint32_t MOT3_HARD_STOP  : 1;  // Bit 3  - Motor 3 hard stop
  uint32_t MOT4_UP         : 1;  // Bit 4  - Motor 4 up command
  uint32_t MOT4_DOWN       : 1;  // Bit 5  - Motor 4 down command
  uint32_t MOT4_STOP       : 1;  // Bit 6  - Motor 4 soft stop
  uint32_t MOT4_HARD_STOP  : 1;  // Bit 7  - Motor 4 hard stop
  uint32_t MOT2_UP         : 1;  // Bit 8  - Motor 2 up command
  uint32_t MOT2_DOWN       : 1;  // Bit 9  - Motor 2 down command
  uint32_t MOT2_STOP       : 1;  // Bit 10 - Motor 2 soft stop
  uint32_t MOT2_HARD_STOP  : 1;  // Bit 11 - Motor 2 hard stop
  uint32_t MOT_UP          : 1;  // Bit 12 - Traction motor clockwise
  uint32_t MOT_DOWN        : 1;  // Bit 13 - Traction motor counter-clockwise
  uint32_t MOT_STOP        : 1;  // Bit 14 - Traction motor soft stop
  uint32_t MOT_HARD_STOP   : 1;  // Bit 15 - Traction motor hard stop
  uint32_t ALARM           : 1;  // Bit 16 - System alarm signal
  uint32_t COAST_ALL       : 1;  // Bit 17 - Coast stop all motors (free wheeling)
  uint32_t reserved        : 14; // Bits 18-31 - Reserved for future use
} T_sys_control;
// clang-format on

/*
  COMMAND PROCESSING PRIORITIES (highest to lowest):
  1. STOP commands - Immediate emergency stop
  2. DOWN commands - Reverse direction movement
  3. UP commands - Forward direction movement
  4. No bits set - Controlled deceleration and stop

  MOTOR CONTROL CHARACTERISTICS:
  - Acceleration: Linear ramp-up over configurable time period
  - Deceleration: Linear ramp-down over configurable time period
  - Maximum PWM: Configurable percentage limit per motor
  - Direction inversion: Configurable per motor via parameters
  - Soft start/stop: Prevents mechanical stress and current spikes
*/

// Error flag constants for T_system_error_flags structure
#define ERROR_MOTOR1_OVERCURRENT      (1 << 0)   // 0x00000001
#define ERROR_MOTOR2_OVERCURRENT      (1 << 1)   // 0x00000002
#define ERROR_MOTOR3_OVERCURRENT      (1 << 2)   // 0x00000004
#define ERROR_MOTOR4_OVERCURRENT      (1 << 3)   // 0x00000008
#define ERROR_DRIVER1_OVERTEMPERATURE (1 << 4)   // 0x00000010
#define ERROR_DRIVER2_OVERTEMPERATURE (1 << 5)   // 0x00000020
#define ERROR_TMC6200_DRIVER1_FAULT   (1 << 6)   // 0x00000040
#define ERROR_TMC6200_DRIVER2_FAULT   (1 << 7)   // 0x00000080
#define ERROR_POWER_SUPPLY_FAULT      (1 << 8)   // 0x00000100
#define ERROR_CAN_BUS_ERROR           (1 << 9)   // 0x00000200
#define ERROR_CPU_OVERTEMPERATURE     (1 << 10)  // 0x00000400
#define ERROR_EMERGENCY_STOP_ACTIVE   (1 << 11)  // 0x00000800

// TMC6200 Driver 1 detailed error constants
#define ERROR_TMC6200_DRV1_UV_CP      (1 << 12)  // 0x00001000
#define ERROR_TMC6200_DRV1_SHORTDET_U (1 << 13)  // 0x00002000
#define ERROR_TMC6200_DRV1_S2GU       (1 << 14)  // 0x00004000
#define ERROR_TMC6200_DRV1_S2VSU      (1 << 15)  // 0x00008000
#define ERROR_TMC6200_DRV1_SHORTDET_V (1 << 16)  // 0x00010000
#define ERROR_TMC6200_DRV1_S2GV       (1 << 17)  // 0x00020000
#define ERROR_TMC6200_DRV1_S2VSV      (1 << 18)  // 0x00040000
#define ERROR_TMC6200_DRV1_SHORTDET_W (1 << 19)  // 0x00080000
#define ERROR_TMC6200_DRV1_S2GW       (1 << 20)  // 0x00100000
#define ERROR_TMC6200_DRV1_S2VSW      (1 << 21)  // 0x00200000

// TMC6200 Driver 2 detailed error constants
#define ERROR_TMC6200_DRV2_UV_CP      (1 << 22)  // 0x00400000
#define ERROR_TMC6200_DRV2_SHORTDET_U (1 << 23)  // 0x00800000
#define ERROR_TMC6200_DRV2_S2GU       (1 << 24)  // 0x01000000
#define ERROR_TMC6200_DRV2_S2VSU      (1 << 25)  // 0x02000000
#define ERROR_TMC6200_DRV2_SHORTDET_V (1 << 26)  // 0x04000000
#define ERROR_TMC6200_DRV2_S2GV       (1 << 27)  // 0x08000000
#define ERROR_TMC6200_DRV2_S2VSV      (1 << 28)  // 0x10000000
#define ERROR_TMC6200_DRV2_SHORTDET_W (1 << 29)  // 0x20000000
#define ERROR_TMC6200_DRV2_S2GW       (1 << 30)  // 0x40000000
#define ERROR_TMC6200_DRV2_S2VSW      (1 << 31)  // 0x80000000

// System error flags structure matching CAN Protocol Specification

typedef struct
{
  uint32_t motor1_overcurrent : 1;       // Bit 0  - Motor 1 overcurrent protection triggered
  uint32_t motor2_overcurrent : 1;       // Bit 1  - Motor 2 overcurrent protection triggered
  uint32_t motor3_overcurrent : 1;       // Bit 2  - Motor 3 overcurrent protection triggered
  uint32_t motor4_overcurrent : 1;       // Bit 3  - Motor 4 overcurrent protection triggered
  uint32_t driver1_overtemperature : 1;  // Bit 4  - Driver 1 overtemperature protection triggered
  uint32_t driver2_overtemperature : 1;  // Bit 5  - Driver 2 overtemperature protection triggered
  uint32_t tmc6200_driver1_fault : 1;    // Bit 6  - TMC6200 driver 1 hardware fault
  uint32_t tmc6200_driver2_fault : 1;    // Bit 7  - TMC6200 driver 2 hardware fault
  uint32_t power_supply_fault : 1;       // Bit 8  - Power supply voltage fault
  uint32_t can_bus_error : 1;            // Bit 9  - CAN bus communication error
  uint32_t cpu_overtemperature : 1;      // Bit 10 - CPU overtemperature protection triggered
  uint32_t emergency_stop_active : 1;    // Bit 11 - Emergency stop active (blocks motor commands)
  // TMC6200 Driver 1 detailed error bits (bits 12-21)
  uint32_t tmc6200_drv1_uv_cp : 1;       // Bit 12 - TMC6200 driver 1 charge pump undervoltage
  uint32_t tmc6200_drv1_shortdet_u : 1;  // Bit 13 - TMC6200 driver 1 phase U short detection
  uint32_t tmc6200_drv1_s2gu : 1;        // Bit 14 - TMC6200 driver 1 phase U short to GND
  uint32_t tmc6200_drv1_s2vsu : 1;       // Bit 15 - TMC6200 driver 1 phase U short to VS
  uint32_t tmc6200_drv1_shortdet_v : 1;  // Bit 16 - TMC6200 driver 1 phase V short detection
  uint32_t tmc6200_drv1_s2gv : 1;        // Bit 17 - TMC6200 driver 1 phase V short to GND
  uint32_t tmc6200_drv1_s2vsv : 1;       // Bit 18 - TMC6200 driver 1 phase V short to VS
  uint32_t tmc6200_drv1_shortdet_w : 1;  // Bit 19 - TMC6200 driver 1 phase W short detection
  uint32_t tmc6200_drv1_s2gw : 1;        // Bit 20 - TMC6200 driver 1 phase W short to GND
  uint32_t tmc6200_drv1_s2vsw : 1;       // Bit 21 - TMC6200 driver 1 phase W short to VS
  // TMC6200 Driver 2 detailed error bits (bits 22-31)
  uint32_t tmc6200_drv2_uv_cp : 1;       // Bit 22 - TMC6200 driver 2 charge pump undervoltage
  uint32_t tmc6200_drv2_shortdet_u : 1;  // Bit 23 - TMC6200 driver 2 phase U short detection
  uint32_t tmc6200_drv2_s2gu : 1;        // Bit 24 - TMC6200 driver 2 phase U short to GND
  uint32_t tmc6200_drv2_s2vsu : 1;       // Bit 25 - TMC6200 driver 2 phase U short to VS
  uint32_t tmc6200_drv2_shortdet_v : 1;  // Bit 26 - TMC6200 driver 2 phase V short detection
  uint32_t tmc6200_drv2_s2gv : 1;        // Bit 27 - TMC6200 driver 2 phase V short to GND
  uint32_t tmc6200_drv2_s2vsv : 1;       // Bit 28 - TMC6200 driver 2 phase V short to VS
  uint32_t tmc6200_drv2_shortdet_w : 1;  // Bit 29 - TMC6200 driver 2 phase W short detection
  uint32_t tmc6200_drv2_s2gw : 1;        // Bit 30 - TMC6200 driver 2 phase W short to GND
  uint32_t tmc6200_drv2_s2vsw : 1;       // Bit 31 - TMC6200 driver 2 phase W short to VS
} T_system_error_flags;

#endif                                   // CAN_PROTOCOL_H
