// This file is auto-generated. Do not edit manually.
#include "App.h"
#include "MC80_Params.h"

#define WVAR_SIZE 45
#define SELECTORS_NUM 5

WVAR_TYPE wvar;

static const T_parmenu parmenu[11] =
{
  { MC80_0                      , MC80_main                   , "Parameters and settings                 ", "Основная категория  ",   1 }, // Parameters and settings
  { MC80_main                   , MC80_General                , "Device general settings                 ", "                    ",   1 }, // Device general settings
  { MC80_main                   , MC80_DriverIC               , "Driver IC settings                      ", "                    ",   1 }, // Driver IC settings
  { MC80_main                   , MC80_Motor_Control          , "Motor control settings                  ", "                    ",   1 }, // Motor control settings
  { MC80_main                   , MC80_Display                , "Display settings                        ", "                    ",   1 }, // Display settings
  { MC80_main                   , MC80_FreeMaster             , "FreeMaster communication settings       ", "                    ",   1 }, // FreeMaster communication settings
  { MC80_main                   , MC80_USB_Interface          , "USB Interface settings                  ", "                    ",   1 }, // USB Interface settings
  { MC80_Motor_Control          , MC80_Motor_1                , "Traction motor settings                 ", "                    ",   1 }, // Traction motor settings
  { MC80_Motor_Control          , MC80_Motor_2                , "Motor 2  settings               ", "                    ",   1 }, //  Motor 2ing settings
  { MC80_Motor_Control          , MC80_Motor_3                , " settings                   ", "                    ",   1 }, //  settings
  { MC80_Motor_Control          , MC80_Motor_4                , " settings                  ", "                    ",   1 }, //  settings
};

// Parameter hash table for fast parameter lookup by CRC16 hash
// Table is sorted by hash values for binary search
typedef struct
{
  uint16_t hash;     // CRC16 hash of parameter name
  uint16_t index;    // Index in parameter array
} T_param_hash_entry;

static const T_param_hash_entry param_hash_table[WVAR_SIZE] =
{
  {0x0624, 42},  // motor_4_decel_time_ms
  {0x0D84, 10},  // pwm_frequency
  {0x1002, 14},  // short_det_spike_filter
  {0x1473, 32},  // motor_2_max_current_a
  {0x1D1F, 11},  // usb_mode
  {0x1E01, 34},  // motor_3_direction_invert
  {0x1F32, 39},  // motor_4_max_pwm_percent
  {0x21D2, 31},  // motor_2_algorithm
  {0x31C5, 16},  // enable_short_to_gnd_prot
  {0x336A, 24},  // motor_1_decel_time_ms
  {0x3543, 35},  // motor_3_accel_time_ms
  {0x36CC,  0},  // display_orientation
  {0x4C58, 21},  // motor_1_max_pwm_percent
  {0x4DD4, 41},  // motor_4_accel_time_ms
  {0x502C,  5},  // hardware_version
  {0x5161, 18},  // gate_driver_current_param
  {0x5626,  4},  // software_version
  {0x5928, 25},  // motor_1_algorithm
  {0x6040, 20},  // input_shunt_resistor
  {0x646B,  9},  // en_formated_settings
  {0x6D9C, 33},  // motor_3_max_pwm_percent
  {0x6ECE,  7},  // en_log_to_file
  {0x7714, 17},  // enable_short_to_vs_prot
  {0x789A, 23},  // motor_1_accel_time_ms
  {0x7D7E, 27},  // motor_2_max_pwm_percent
  {0x7EB3, 36},  // motor_3_decel_time_ms
  {0x9416, 12},  // short_vs_det_level
  {0x9BBF, 29},  // motor_2_accel_time_ms
  {0xA7E8, 13},  // short_gnd_det_level
  {0xA9C5,  1},  // en_freemaster
  {0xAEC4, 19},  // shunt_resistor
  {0xB8EB,  8},  // en_compress_settins
  {0xBA8F, 38},  // motor_3_max_current_a
  {0xC218, 44},  // motor_4_max_current_a
  {0xD026, 43},  // motor_4_algorithm
  {0xD04F, 30},  // motor_2_decel_time_ms
  {0xD47E,  3},  // product_name
  {0xDC64,  6},  // enable_log
  {0xEE30, 28},  // motor_2_direction_invert
  {0xEE42, 22},  // motor_1_direction_invert
  {0xEED4, 40},  // motor_4_direction_invert
  {0xF40E, 15},  // short_det_delay_param
  {0xF756, 26},  // motor_1_max_current_a
  {0xF85A,  2},  // en_log_to_freemaster
  {0xF99B, 37}  // motor_3_algorithm
};

// Binary search function to find parameter index by CRC16 hash
// Returns parameter index or 0xFFFF if not found
uint16_t Find_param_by_hash(uint16_t hash)
{
  int left = 0;
  int right = WVAR_SIZE - 1;

  while (left <= right)
  {
    int mid = (left + right) / 2;
    if (param_hash_table[mid].hash == hash)
    {
      return param_hash_table[mid].index;
    }
    if (param_hash_table[mid].hash < hash)
    {
      left = mid + 1;
    }
    else
    {
      right = mid - 1;
    }
  }
  return 0xFFFF; // Parameter not found
}

// Parameter index-to-hash table for CAN command transmission
// Array index corresponds to parameter index, value is CRC16 hash
static const uint16_t param_index_to_hash_table[WVAR_SIZE] =
{
  0x36CC,  // [ 0] display_orientation
  0xA9C5,  // [ 1] en_freemaster
  0xF85A,  // [ 2] en_log_to_freemaster
  0xD47E,  // [ 3] product_name
  0x5626,  // [ 4] software_version
  0x502C,  // [ 5] hardware_version
  0xDC64,  // [ 6] enable_log
  0x6ECE,  // [ 7] en_log_to_file
  0xB8EB,  // [ 8] en_compress_settins
  0x646B,  // [ 9] en_formated_settings
  0x0D84,  // [10] pwm_frequency
  0x1D1F,  // [11] usb_mode
  0x9416,  // [12] short_vs_det_level
  0xA7E8,  // [13] short_gnd_det_level
  0x1002,  // [14] short_det_spike_filter
  0xF40E,  // [15] short_det_delay_param
  0x31C5,  // [16] enable_short_to_gnd_prot
  0x7714,  // [17] enable_short_to_vs_prot
  0x5161,  // [18] gate_driver_current_param
  0xAEC4,  // [19] shunt_resistor
  0x6040,  // [20] input_shunt_resistor
  0x4C58,  // [21] motor_1_max_pwm_percent
  0xEE42,  // [22] motor_1_direction_invert
  0x789A,  // [23] motor_1_accel_time_ms
  0x336A,  // [24] motor_1_decel_time_ms
  0x5928,  // [25] motor_1_algorithm
  0xF756,  // [26] motor_1_max_current_a
  0x7D7E,  // [27] motor_2_max_pwm_percent
  0xEE30,  // [28] motor_2_direction_invert
  0x9BBF,  // [29] motor_2_accel_time_ms
  0xD04F,  // [30] motor_2_decel_time_ms
  0x21D2,  // [31] motor_2_algorithm
  0x1473,  // [32] motor_2_max_current_a
  0x6D9C,  // [33] motor_3_max_pwm_percent
  0x1E01,  // [34] motor_3_direction_invert
  0x3543,  // [35] motor_3_accel_time_ms
  0x7EB3,  // [36] motor_3_decel_time_ms
  0xF99B,  // [37] motor_3_algorithm
  0xBA8F,  // [38] motor_3_max_current_a
  0x1F32,  // [39] motor_4_max_pwm_percent
  0xEED4,  // [40] motor_4_direction_invert
  0x4DD4,  // [41] motor_4_accel_time_ms
  0x0624,  // [42] motor_4_decel_time_ms
  0xD026,  // [43] motor_4_algorithm
  0xC218  // [44] motor_4_max_current_a
};

// Function to get parameter hash by index for CAN transmission
// Returns parameter hash or 0x0000 if index is out of range
uint16_t Get_param_hash_by_index(uint16_t index)
{
  if (index >= WVAR_SIZE)
  {
    return 0x0000; // Invalid index
  }
  return param_index_to_hash_table[index];
}

// Array items are sorted by CategoryName followed by SubNumber
static const T_NV_parameters arr_wvar[WVAR_SIZE] =
{
  // var_name                            , var_description                                                       , var_alias , val                                   , vartype, defval, minval, maxval, attr, parmnlev          , pdefval    , format , func, varlen                                , menu_pos, selector_id
  { /* 00 */ "display_orientation"      , "Display_orientation"                                                 , "SPLSHSC" , (void*)&wvar.display_orientation      , tint8u , 0     , 0     , 3     , 0   , MC80_Display      , ""         , "%d"   , 0   , sizeof(wvar.display_orientation)      , 1       , 0           },
  { /* 01 */ "en_freemaster"            , "Enable FreeMaster protocol"                                          , "NFRMSTR" , (void*)&wvar.en_freemaster            , tint8u , 0     , 0     , 1     , 0   , MC80_FreeMaster   , ""         , "%d"   , 0   , sizeof(wvar.en_freemaster)            , 1       , 1           },
  { /* 02 */ "en_log_to_freemaster"     , "Enable logging to FreeMaster pipe"                                   , "NLGTFRM" , (void*)&wvar.en_log_to_freemaster     , tint8u , 0     , 0     , 1     , 0   , MC80_FreeMaster   , ""         , "%d"   , 0   , sizeof(wvar.en_log_to_freemaster)     , 2       , 1           },
  { /* 03 */ "product_name"             , "Product name"                                                        , "PRDCTNM" , (void*)&wvar.product_name             , tstring, 0     , 0     , 0     , 0   , MC80_General      , "MC80"     , "%s"   , 0   , sizeof(wvar.product_name)-1           , 2       , 0           },
  { /* 04 */ "software_version"         , "Software version"                                                    , "SFTWRVR" , (void*)&wvar.software_version         , tstring, 0     , 0     , 0     , 0   , MC80_General      , "MC80"     , "%s"   , 0   , sizeof(wvar.software_version)-1       , 3       , 0           },
  { /* 05 */ "hardware_version"         , "Hardware version"                                                    , "HRDWRVR" , (void*)&wvar.hardware_version         , tstring, 0     , 0     , 0     , 0   , MC80_General      , "MC80 v1.0", "%s"   , 0   , sizeof(wvar.hardware_version)-1       , 4       , 0           },
  { /* 06 */ "enable_log"               , "Enable log"                                                          , "NNNNNNN" , (void*)&wvar.enable_log               , tint8u , 1     , 0     , 1     , 0   , MC80_General      , ""         , "%d"   , 0   , sizeof(wvar.enable_log)               , 5       , 1           },
  { /* 07 */ "en_log_to_file"           , "Enable logging to file"                                              , "NLGTFLN" , (void*)&wvar.en_log_to_file           , tint8u , 1     , 0     , 1     , 0   , MC80_General      , ""         , "%d"   , 0   , sizeof(wvar.en_log_to_file)           , 7       , 1           },
  { /* 08 */ "en_compress_settins"      , "Enable settings compression"                                         , "NCMPRSS" , (void*)&wvar.en_compress_settins      , tint8u , 1     , 0     , 1     , 0   , MC80_General      , ""         , "%d"   , 0   , sizeof(wvar.en_compress_settins)      , 8       , 1           },
  { /* 09 */ "en_formated_settings"     , "Enable formatting in settings file"                                  , "NFRMTNG" , (void*)&wvar.en_formated_settings     , tint8u , 1     , 0     , 1     , 0   , MC80_General      , ""         , "%d"   , 0   , sizeof(wvar.en_formated_settings)     , 9       , 1           },
  { /* 10 */ "pwm_frequency"            , "PWM frequency for motor control (Hz)"                                , "PWMFRQHZ", (void*)&wvar.pwm_frequency            , tint32u, 8000  , 2000  , 16000 , 0   , MC80_General      , ""         , "%d"   , 0   , sizeof(wvar.pwm_frequency)            , 12      , 0           },
  { /* 11 */ "usb_mode"                 , "USB mode"                                                            , "SBMDSBM" , (void*)&wvar.usb_mode                 , tint32u, 1     , 0     , 7     , 0   , MC80_USB_Interface, ""         , "%d"   , 0   , sizeof(wvar.usb_mode)                 , 1       , 2           },
  { /* 12 */ "short_vs_det_level"       , "Short to VS detector level for lowside FETs (1- highest..15-lowest)" , "SHRTVSD" , (void*)&wvar.short_vs_det_level       , tint32u, 12    , 1     , 15    , 0   , MC80_DriverIC     , ""         , "%d"   , 0   , sizeof(wvar.short_vs_det_level)       , 1       , 0           },
  { /* 13 */ "short_gnd_det_level"      , "Short to GND detector level for highside FETs (2- highest..15lowest)", "SHRTGND" , (void*)&wvar.short_gnd_det_level      , tint32u, 12    , 2     , 15    , 0   , MC80_DriverIC     , ""         , "%d"   , 0   , sizeof(wvar.short_gnd_det_level)      , 2       , 0           },
  { /* 14 */ "short_det_spike_filter"   , "Spike filtering bandwidth for short detection (0-100ns..3-3us)"      , "SHRTDTS" , (void*)&wvar.short_det_spike_filter   , tint32u, 0     , 0     , 3     , 0   , MC80_DriverIC     , ""         , "%d"   , 0   , sizeof(wvar.short_det_spike_filter)   , 3       , 0           },
  { /* 15 */ "short_det_delay_param"    , "Short detection delay parameter (0..1)"                              , "SHRTDTD" , (void*)&wvar.short_det_delay_param    , tint32u, 0     , 0     , 1     , 0   , MC80_DriverIC     , ""         , "%d"   , 0   , sizeof(wvar.short_det_delay_param)    , 4       , 0           },
  { /* 16 */ "enable_short_to_gnd_prot" , "Enable short to GND protection"                                      , "NBLSHRT" , (void*)&wvar.enable_short_to_gnd_prot , tint8u , 0     , 0     , 1     , 0   , MC80_DriverIC     , ""         , "%d"   , 0   , sizeof(wvar.enable_short_to_gnd_prot) , 5       , 1           },
  { /* 17 */ "enable_short_to_vs_prot"  , "Enable short to VS protection"                                       , "NBLSHRA" , (void*)&wvar.enable_short_to_vs_prot  , tint8u , 0     , 0     , 1     , 0   , MC80_DriverIC     , ""         , "%d"   , 0   , sizeof(wvar.enable_short_to_vs_prot)  , 6       , 1           },
  { /* 18 */ "gate_driver_current_param", "Gate driver current parameter (0-weak..4-strong)"                    , "GTDRVRC" , (void*)&wvar.gate_driver_current_param, tint32u, 2     , 0     , 3     , 0   , MC80_DriverIC     , ""         , "%d"   , 0   , sizeof(wvar.gate_driver_current_param), 7       , 0           },
  { /* 19 */ "shunt_resistor"           , "Shunt resistor (Ohm)"                                                , "SHNTRSS" , (void*)&wvar.shunt_resistor           , tfloat , 0.002 , 0     , 1     , 0   , MC80_DriverIC     , ""         , "%0.6f", 0   , sizeof(wvar.shunt_resistor)           , 8       , 0           },
  { /* 20 */ "input_shunt_resistor"     , "Input shunt resistor (Ohm)"                                          , "NPTSHNT" , (void*)&wvar.input_shunt_resistor     , tfloat , 0.001 , 0     , 1     , 0   , MC80_DriverIC     , ""         , "%0.6f", 0   , sizeof(wvar.input_shunt_resistor)     , 9       , 0           },
  { /* 21 */ "motor_1_max_pwm_percent"  , "Maximum PWM level (percent)"                                         , "MOTR1PWM", (void*)&wvar.motor_1_max_pwm_percent  , tint8u , 100   , 1     , 100   , 0   , MC80_Motor_1      , ""         , "%d"   , 0   , sizeof(wvar.motor_1_max_pwm_percent)  , 1       , 0           },
  { /* 22 */ "motor_1_direction_invert" , "Direction invert flag"                                               , "MOTR1INV", (void*)&wvar.motor_1_direction_invert , tint8u , 0     , 0     , 1     , 0   , MC80_Motor_1      , ""         , "%d"   , 0   , sizeof(wvar.motor_1_direction_invert) , 2       , 1           },
  { /* 23 */ "motor_1_accel_time_ms"    , "Acceleration time (ms)"                                              , "MOTR1ACC", (void*)&wvar.motor_1_accel_time_ms    , tint32u, 1000  , 0     , 10000 , 0   , MC80_Motor_1      , ""         , "%d"   , 0   , sizeof(wvar.motor_1_accel_time_ms)    , 3       , 0           },
  { /* 24 */ "motor_1_decel_time_ms"    , "Deceleration time (ms)"                                              , "MOTR1DEC", (void*)&wvar.motor_1_decel_time_ms    , tint32u, 500   , 0     , 10000 , 0   , MC80_Motor_1      , ""         , "%d"   , 0   , sizeof(wvar.motor_1_decel_time_ms)    , 4       , 0           },
  { /* 25 */ "motor_1_algorithm"        , "Acceleration/Deceleration algorithm"                                 , "MOTR1ALG", (void*)&wvar.motor_1_algorithm        , tint8u , 2     , 0     , 2     , 0   , MC80_Motor_1      , ""         , "%d"   , 0   , sizeof(wvar.motor_1_algorithm)        , 5       , 3           },
  { /* 26 */ "motor_1_max_current_a"    , "Maximum current for emergency stop (A)"                              , "MOTR1CUR", (void*)&wvar.motor_1_max_current_a    , tfloat , 25.0  , 0.1   , 100.0 , 0   , MC80_Motor_1      , ""         , "%0.1f", 0   , sizeof(wvar.motor_1_max_current_a)    , 6       , 0           },
  { /* 27 */ "motor_2_max_pwm_percent"  , "Maximum PWM level (percent)"                                         , "MOTR2PWM", (void*)&wvar.motor_2_max_pwm_percent  , tint8u , 100   , 1     , 100   , 0   , MC80_Motor_2      , ""         , "%d"   , 0   , sizeof(wvar.motor_2_max_pwm_percent)  , 1       , 0           },
  { /* 28 */ "motor_2_direction_invert" , "Direction invert flag"                                               , "MOTR2INV", (void*)&wvar.motor_2_direction_invert , tint8u , 0     , 0     , 1     , 0   , MC80_Motor_2      , ""         , "%d"   , 0   , sizeof(wvar.motor_2_direction_invert) , 2       , 1           },
  { /* 29 */ "motor_2_accel_time_ms"    , "Acceleration time (ms)"                                              , "MOTR2ACC", (void*)&wvar.motor_2_accel_time_ms    , tint32u, 1000  , 0     , 10000 , 0   , MC80_Motor_2      , ""         , "%d"   , 0   , sizeof(wvar.motor_2_accel_time_ms)    , 3       , 0           },
  { /* 30 */ "motor_2_decel_time_ms"    , "Deceleration time (ms)"                                              , "MOTR2DEC", (void*)&wvar.motor_2_decel_time_ms    , tint32u, 100   , 0     , 10000 , 0   , MC80_Motor_2      , ""         , "%d"   , 0   , sizeof(wvar.motor_2_decel_time_ms)    , 4       , 0           },
  { /* 31 */ "motor_2_algorithm"        , "Acceleration/Deceleration algorithm"                                 , "MOTR2ALG", (void*)&wvar.motor_2_algorithm        , tint8u , 2     , 0     , 2     , 0   , MC80_Motor_2      , ""         , "%d"   , 0   , sizeof(wvar.motor_2_algorithm)        , 5       , 3           },
  { /* 32 */ "motor_2_max_current_a"    , "Maximum current for emergency stop (A)"                              , "MOTR2CUR", (void*)&wvar.motor_2_max_current_a    , tfloat , 5.0   , 0.1   , 100.0 , 0   , MC80_Motor_2      , ""         , "%0.1f", 0   , sizeof(wvar.motor_2_max_current_a)    , 6       , 0           },
  { /* 33 */ "motor_3_max_pwm_percent"  , "Maximum PWM level (percent)"                                         , "MOTR3PWM", (void*)&wvar.motor_3_max_pwm_percent  , tint8u , 100   , 1     , 100   , 0   , MC80_Motor_3      , ""         , "%d"   , 0   , sizeof(wvar.motor_3_max_pwm_percent)  , 1       , 0           },
  { /* 34 */ "motor_3_direction_invert" , "Direction invert flag"                                               , "MOTR3INV", (void*)&wvar.motor_3_direction_invert , tint8u , 0     , 0     , 1     , 0   , MC80_Motor_3      , ""         , "%d"   , 0   , sizeof(wvar.motor_3_direction_invert) , 2       , 1           },
  { /* 35 */ "motor_3_accel_time_ms"    , "Acceleration time (ms)"                                              , "MOTR3ACC", (void*)&wvar.motor_3_accel_time_ms    , tint32u, 1000  , 0     , 10000 , 0   , MC80_Motor_3      , ""         , "%d"   , 0   , sizeof(wvar.motor_3_accel_time_ms)    , 3       , 0           },
  { /* 36 */ "motor_3_decel_time_ms"    , "Deceleration time (ms)"                                              , "MOTR3DEC", (void*)&wvar.motor_3_decel_time_ms    , tint32u, 100   , 0     , 10000 , 0   , MC80_Motor_3      , ""         , "%d"   , 0   , sizeof(wvar.motor_3_decel_time_ms)    , 4       , 0           },
  { /* 37 */ "motor_3_algorithm"        , "Acceleration/Deceleration algorithm"                                 , "MOTR3ALG", (void*)&wvar.motor_3_algorithm        , tint8u , 2     , 0     , 2     , 0   , MC80_Motor_3      , ""         , "%d"   , 0   , sizeof(wvar.motor_3_algorithm)        , 5       , 3           },
  { /* 38 */ "motor_3_max_current_a"    , "Maximum current for emergency stop (A)"                              , "MOTR3CUR", (void*)&wvar.motor_3_max_current_a    , tfloat , 4.0   , 0.1   , 100.0 , 0   , MC80_Motor_3      , ""         , "%0.1f", 0   , sizeof(wvar.motor_3_max_current_a)    , 6       , 0           },
  { /* 39 */ "motor_4_max_pwm_percent"  , "Maximum PWM level (percent)"                                         , "MOTR4PWM", (void*)&wvar.motor_4_max_pwm_percent  , tint8u , 100   , 1     , 100   , 0   , MC80_Motor_4      , ""         , "%d"   , 0   , sizeof(wvar.motor_4_max_pwm_percent)  , 1       , 0           },
  { /* 40 */ "motor_4_direction_invert" , "Direction invert flag"                                               , "MOTR4INV", (void*)&wvar.motor_4_direction_invert , tint8u , 0     , 0     , 1     , 0   , MC80_Motor_4      , ""         , "%d"   , 0   , sizeof(wvar.motor_4_direction_invert) , 2       , 1           },
  { /* 41 */ "motor_4_accel_time_ms"    , "Acceleration time (ms)"                                              , "MOTR4ACC", (void*)&wvar.motor_4_accel_time_ms    , tint32u, 1000  , 0     , 10000 , 0   , MC80_Motor_4      , ""         , "%d"   , 0   , sizeof(wvar.motor_4_accel_time_ms)    , 3       , 0           },
  { /* 42 */ "motor_4_decel_time_ms"    , "Deceleration time (ms)"                                              , "MOTR4DEC", (void*)&wvar.motor_4_decel_time_ms    , tint32u, 100   , 0     , 10000 , 0   , MC80_Motor_4      , ""         , "%d"   , 0   , sizeof(wvar.motor_4_decel_time_ms)    , 4       , 0           },
  { /* 43 */ "motor_4_algorithm"        , "Acceleration/Deceleration algorithm"                                 , "MOTR4ALG", (void*)&wvar.motor_4_algorithm        , tint8u , 2     , 0     , 2     , 0   , MC80_Motor_4      , ""         , "%d"   , 0   , sizeof(wvar.motor_4_algorithm)        , 5       , 3           },
  { /* 44 */ "motor_4_max_current_a"    , "Maximum current for emergency stop (A)"                              , "MOTR4CUR", (void*)&wvar.motor_4_max_current_a    , tfloat , 4.0   , 0.1   , 50.0  , 0   , MC80_Motor_4      , ""         , "%0.1f", 0   , sizeof(wvar.motor_4_max_current_a)    , 6       , 0           }
};

// Selector description:  Выбор между Yes и No
static const T_selector_items selector_2[2] =
{
  { 0 , "No"                                        , 0},
  { 1 , "Yes"                                       , 1},
};

// Selector description:  USB mode
static const T_selector_items selector_3[7] =
{
  { 0 , "None"                                      , -1},
  { 1 , "VCOM port"                                 , -1},
  { 2 , "Mass storage "                             , -1},
  { 3 , "VCOM and Mass storage"                     , -1},
  { 4 , "VCOM and FreeMaster port"                  , -1},
  { 5 , "RNDIS"                                     , -1},
  { 6 , "Host ECM"                                  , -1},
};

// Selector description:  Acceleration/deceleration algorithm selection
static const T_selector_items selector_4[3] =
{
  { 0 , "Instant"                                   , -1},
  { 1 , "Linear"                                    , -1},
  { 2 , "S-curve"                                   , -1},
};

static const T_selectors_list selectors_list[4] =
{
  {"string                        ", 0   , 0           },
  {"binary                        ", 2   , selector_2  },
  {"usb_mode                      ", 7   , selector_3  },
  {"accel_decel_alg               ", 3   , selector_4  },
};

const T_NV_parameters_instance wvar_inst =
{
  WVAR_SIZE,
  arr_wvar,
  11,
  parmenu,
  SELECTORS_NUM,
  selectors_list
};
