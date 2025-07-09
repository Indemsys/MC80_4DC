#ifndef MC80_PARAMS_H
#define MC80_PARAMS_H

// This file is auto-generated. Do not edit manually.

// Category constants
#define MC80_0             0
#define MC80_main          1
#define MC80_General       2
#define MC80_DriverIC      3
#define MC80_Motor_Control 4
#define MC80_Display       5
#define MC80_FreeMaster    6
#define MC80_USB_Interface 7
#define MC80_Motor_1       8
#define MC80_Motor_2       9
#define MC80_Motor_3       10
#define MC80_Motor_4       11

typedef struct
{
  uint8_t display_orientation;         // Display_orientation
  uint8_t en_freemaster;               // Enable FreeMaster protocol
  uint8_t en_log_to_freemaster;        // Enable logging to FreeMaster pipe
  uint8_t product_name[64];            // Product name
  uint8_t software_version[64];        // Software version
  uint8_t hardware_version[64];        // Hardware version
  uint8_t enable_log;                  // Enable log
  uint8_t en_log_to_file;              // Enable logging to file
  uint8_t en_compress_settins;         // Enable settings compression
  uint8_t en_formated_settings;        // Enable formatting in settings file
  uint32_t pwm_frequency;              // PWM frequency for motor control (Hz)
  uint32_t usb_mode;                   // USB mode
  uint32_t short_vs_det_level;         // Short to VS detector level for lowside FETs (1- highest..15-lowest)
  uint32_t short_gnd_det_level;        // Short to GND detector level for highside FETs (2- highest..15lowest)
  uint32_t short_det_spike_filter;     // Spike filtering bandwidth for short detection (0-100ns..3-3us)
  uint32_t short_det_delay_param;      // Short detection delay parameter (0..1)
  uint8_t enable_short_to_gnd_prot;    // Enable short to GND protection
  uint8_t enable_short_to_vs_prot;     // Enable short to VS protection
  uint32_t gate_driver_current_param;  // Gate driver current parameter (0-weak..4-strong)
  float shunt_resistor;                // Shunt resistor (Ohm)
  float input_shunt_resistor;          // Input shunt resistor (Ohm)
  uint8_t motor_1_max_pwm_percent;     // Maximum PWM level (percent)
  uint8_t motor_1_direction_invert;    // Direction invert flag
  uint32_t motor_1_accel_time_ms;      // Acceleration time (ms)
  uint32_t motor_1_decel_time_ms;      // Deceleration time (ms)
  uint8_t motor_1_algorithm;           // Acceleration/Deceleration algorithm
  float motor_1_max_current_a;         // Maximum current for emergency stop (A)
  uint8_t motor_2_max_pwm_percent;     // Maximum PWM level (percent)
  uint8_t motor_2_direction_invert;    // Direction invert flag
  uint32_t motor_2_accel_time_ms;      // Acceleration time (ms)
  uint32_t motor_2_decel_time_ms;      // Deceleration time (ms)
  uint8_t motor_2_algorithm;           // Acceleration/Deceleration algorithm
  float motor_2_max_current_a;         // Maximum current for emergency stop (A)
  uint8_t motor_3_max_pwm_percent;     // Maximum PWM level (percent)
  uint8_t motor_3_direction_invert;    // Direction invert flag
  uint32_t motor_3_accel_time_ms;      // Acceleration time (ms)
  uint32_t motor_3_decel_time_ms;      // Deceleration time (ms)
  uint8_t motor_3_algorithm;           // Acceleration/Deceleration algorithm
  float motor_3_max_current_a;         // Maximum current for emergency stop (A)
  uint8_t motor_4_max_pwm_percent;     // Maximum PWM level (percent)
  uint8_t motor_4_direction_invert;    // Direction invert flag
  uint32_t motor_4_accel_time_ms;      // Acceleration time (ms)
  uint32_t motor_4_decel_time_ms;      // Deceleration time (ms)
  uint8_t motor_4_algorithm;           // Acceleration/Deceleration algorithm
  float motor_4_max_current_a;         // Maximum current for emergency stop (A)
} WVAR_TYPE;

// Selector constants
// accel_decel_alg
#define ACCEL_DECEL_ALG_INSTANT 0
#define ACCEL_DECEL_ALG_LINEAR  1
#define ACCEL_DECEL_ALG_S_CURVE 2

// binary
#define BINARY_NO  0
#define BINARY_YES 1

// usb_mode
#define USB_MODE_NONE                     0
#define USB_MODE_VCOM_PORT                1
#define USB_MODE_MASS_STORAGE_            2
#define USB_MODE_VCOM_AND_MASS_STORAGE    3
#define USB_MODE_VCOM_AND_FREEMASTER_PORT 4
#define USB_MODE_RNDIS                    5
#define USB_MODE_HOST_ECM                 6

extern WVAR_TYPE wvar;
extern const T_NV_parameters_instance wvar_inst;

// Function for fast parameter lookup by CRC16 hash
uint16_t Find_param_by_hash(uint16_t hash);

// Function to get parameter hash by index for CAN transmission
uint16_t Get_param_hash_by_index(uint16_t index);

#endif // MC80_PARAMS_H