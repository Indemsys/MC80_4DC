#ifndef ADC_DRIVER_H
#define ADC_DRIVER_H

#define ADC_SAMPLE_STATES_COUNT      0x20  // 32 ticks (0.267 us at 120 MHz PCLKD)
#define ADC_CHANNELS_COUNT           13
#define ADC_MULTIPLEXED_CHANNELS     9     // Channels controlled by multiplexers

// ADC channel assignment constants
#define ADC_CHAN_IU_MOTOR            0   // AN000 - U phase current (via U9 mux)
#define ADC_CHAN_IV_MOTOR            1   // AN001 - V phase current (via U9 mux)
#define ADC_CHAN_IW_MOTOR            2   // AN002 - W phase current (via U9 mux)
#define ADC_CHAN_V24V_MON            3   // AN004 - System +24V input monitoring
#define ADC_CHAN_VOLTAGE_M1          4   // AN005 - Motor 1 phase voltages (via U12 mux)
#define ADC_CHAN_VOLTAGE_M2          5   // AN006 - Motor 2 phase voltages (via U12 mux)
#define ADC_CHAN_IPWR_MOTOR          6   // AN100 - Power supply current (via U11 mux)
#define ADC_CHAN_SPEED_SENS          7   // AN101 - Speed sensor (via U11 mux)
#define ADC_CHAN_POS_SENS            8   // AN102 - Position sensor (via U11 mux)
#define ADC_CHAN_V5V_MON             9   // AN104 - +5V supply monitoring
#define ADC_CHAN_V3V3_MON            10  // AN105 - +3.3V reference/supply monitoring
#define ADC_CHAN_THERMISTOR_M1       11  // AN016 - Motor 1 power transistors thermistor
#define ADC_CHAN_THERMISTOR_M2       12  // AN117 - Motor 2 power transistors thermistor

// Motor selection for multiplexed channels
#define ADC_MOTOR_1                  0
#define ADC_MOTOR_2                  1

// Phase selection for voltage measurement
#define ADC_PHASE_U                  0
#define ADC_PHASE_V                  1
#define ADC_PHASE_W                  2

// Thermistor NCP21XV103J03RA calculation constants
#define THERMISTOR_R25               10000.0f  // Resistance at 25°C (Ohm)
#define THERMISTOR_T25               298.15f   // 25°C in Kelvin
#define THERMISTOR_B_CONSTANT        3984.0f   // B25/85 parameter (K)
#define THERMISTOR_PULLUP_R          10000.0f  // Pull-up resistor value (Ohm)
#define THERMISTOR_SUPPLY_V          3.322f    // ADC supply voltage (V)

// Thermistor temperature limits
#define THERMISTOR_MIN_TEMP          -40.0f   // Minimum expected temperature (°C)
#define THERMISTOR_MAX_TEMP          150.0f   // Maximum expected temperature (°C)
#define THERMISTOR_MIN_VOLTAGE       0.01f    // Minimum voltage threshold (V)

#define KELVIN_TO_CELSIUS            273.15f  // Kelvin to Celsius conversion constant

// ADC scaling factors constants
#define ADC_REF_VOLTAGE              3.322f   // ADC reference voltage (V)
#define ADC_RESOLUTION               4096.0f  // 12-bit ADC resolution (counts)

// Amplifier gain constants
#define INA186A2_GAIN                50.0f  // INA186A2IDDFR current sense amplifier gain (V/V)
#define TMC6200_GAIN                 20.0f  // TMC6200-TA driver amplifier gain (V/V)

// Voltage divider resistor values for phase voltage measurement (kΩ)
#define PHASE_VOLTAGE_UPPER_RESISTOR 100.0f  // Upper resistor in phase voltage divider (kΩ)
#define PHASE_VOLTAGE_LOWER_RESISTOR 10.0f   // Lower resistor in phase voltage divider (kΩ)

// Voltage divider resistor values for 5V monitoring (kΩ)
#define V5V_UPPER_RESISTOR           10.0f  // Upper resistor in 5V voltage divider (kΩ)
#define V5V_LOWER_RESISTOR           5.1f   // Lower resistor in 5V voltage divider (kΩ)

// Voltage divider resistor values for 3.3V monitoring (kΩ)
#define V3V3_UPPER_RESISTOR          100.0f  // Upper resistor in 3.3V voltage divider (kΩ)
#define V3V3_LOWER_RESISTOR          100.0f  // Lower resistor in 3.3V voltage divider (kΩ)

// 24V monitoring differential amplifier attenuation factor
#define V24V_DIFF_AMP_ATTENUATION    0.082f  // Differential amplifier attenuation factor

// Voltage divider ratios calculated from resistor values
#define PHASE_VOLTAGE_DIVIDER_RATIO  ((PHASE_VOLTAGE_UPPER_RESISTOR + PHASE_VOLTAGE_LOWER_RESISTOR) / PHASE_VOLTAGE_LOWER_RESISTOR)
#define V5V_DIVIDER_RATIO            ((V5V_UPPER_RESISTOR + V5V_LOWER_RESISTOR) / V5V_LOWER_RESISTOR)
#define V3V3_DIVIDER_RATIO           ((V3V3_UPPER_RESISTOR + V3V3_LOWER_RESISTOR) / V3V3_LOWER_RESISTOR)
#define V24V_DIVIDER_RATIO           (1.0f / V24V_DIFF_AMP_ATTENUATION)  // Reciprocal of attenuation factor

// CPU temperature calculation scale factor for RA8M1 factory calibration
#define CPU_TEMP_SCALE_FACTOR        ((ADC_REF_VOLTAGE * 1000000.0f) / (ADC_RESOLUTION * (float)BSP_FEATURE_ADC_TSN_SLOPE))

// EMA filter constants for optimized integer arithmetic
// Using 16-bit shift for alpha coefficient calculation (65536 = 2^16)
// Formula: α = 2π * fc / fs, where fc=cutoff freq, fs=sampling freq
// EMA_ALPHA = α * 65536 (Q16 fixed-point format for alpha coefficients)
// Note: Filtered values are also stored in Q16 format and require >> EMA_FILTER_SHIFT before use
#define EMA_FILTER_SHIFT             16
#define EMA_FILTER_SCALE             (1 << EMA_FILTER_SHIFT)  // 65536

// Cutoff frequencies in Hz (easily configurable)
#define FC_CONTROL_10HZ              10  // Control signals cutoff frequency
#define FC_MONITOR_1HZ               1   // Monitoring signals cutoff frequency

// Current control filter frequencies for dual EMA approach
#define FC_CURRENT_FAST              1500  // Fast current filter for control (Hz)
#define FC_CURRENT_SLOW              300   // Slow current filter for monitoring (Hz)

// Global PWM frequency and dynamic EMA coefficients
extern uint32_t g_adc_pwm_frequency;
extern uint16_t g_ema_alpha_10hz_direct;  // Dynamic coefficient for direct channels, 10Hz
extern uint16_t g_ema_alpha_1hz_direct;   // Dynamic coefficient for direct channels, 1Hz
extern uint16_t g_ema_alpha_10hz_2ch;     // Dynamic coefficient for 2-ch mux, 10Hz
extern uint16_t g_ema_alpha_1hz_2ch;      // Dynamic coefficient for 2-ch mux, 1Hz
extern uint16_t g_ema_alpha_10hz_3ch;     // Dynamic coefficient for 3-ch mux, 10Hz
extern uint16_t g_ema_alpha_1hz_3ch;      // Dynamic coefficient for 3-ch mux, 1Hz

// Current filtering coefficients for dual EMA approach
extern uint16_t g_ema_alpha_current_fast_2ch;  // Dynamic coefficient for fast current filtering, 2-ch mux
extern uint16_t g_ema_alpha_current_slow_2ch;  // Dynamic coefficient for slow current filtering, 2-ch mux

// Optimized EMA filter macro to avoid 64-bit arithmetic operations
// Formula: filtered = filtered + ((sample - (filtered >> shift)) * alpha)
// Note: Filtered values are stored in Q16 format and must be shifted before use
// The alpha coefficient is in Q16 format for precision, filtered data requires >> EMA_FILTER_SHIFT
#define EMA_FILTER_UPDATE(filtered, sample, alpha) \
  ((filtered) + (uint32_t)(((int32_t)(sample) - ((int32_t)(filtered) >> EMA_FILTER_SHIFT)) * (alpha)))

//  Полный перечень кодов триггеров для ADSTRGR
//  RA8M1, R01UH0994EJ0120, таблицы 45-21 / 45-22 (Rev.1.20, 22-Nov-2024)

/* --- 0x00 … 0x08 ---------------------------------------------------- */
#define ADC_TRG_ADTRG_PIN     0x00U /* ADTRGn вход – асинхронный пуск (TRSA) */

/* --- 0x09 … 0x0B : ELC события -------------------------------------- */
#define ADC_TRG_ELC_AD00      0x09U /* ELC_AD00  | ELC_AD10 (unit0/1)        */
#define ADC_TRG_ELC_AD01      0x0AU /* ELC_AD01  | ELC_AD11 (unit0/1)        */
#define ADC_TRG_ELC_AD00_AD01 0x0BU /* «OR» всех ELC_AD0x (AD00+AD01+...11)  */

/* --- 0x11 … 0x18 : одиночные события GPT ---------------------------- */
/*  Пара «unit0 / unit1» указывает на одинаковые события в двух блоках таймера (RA8M1 имеет два сдвоенных 32-битных GPT блока).           */
#define ADC_TRG_GPT0_A        0x11U /* GTCIADA0 (u0) & GTCIADA4 (u1) */
#define ADC_TRG_GPT0_B        0x12U /* GTCIADB0 (u0) & GTCIADB4 (u1) */
#define ADC_TRG_GPT1_A        0x13U /* GTCIADA1 (u0) & GTCIADA5 (u1) */
#define ADC_TRG_GPT1_B        0x14U /* GTCIADB1 (u0) & GTCIADB5 (u1) */
#define ADC_TRG_GPT2_A        0x15U /* GTCIADA2 (u0) & GTCIADA6 (u1) */
#define ADC_TRG_GPT2_B        0x16U /* GTCIADB2 (u0) & GTCIADB6 (u1) */
#define ADC_TRG_GPT3_A        0x17U /* GTCIADA3 (u0) & GTCIADA7 (u1) */
#define ADC_TRG_GPT3_B        0x18U /* GTCIADB3 (u0) & GTCIADB7 (u1) */

/* --- 0x19 … 0x1C : «A OR B» (оба выхода того же GPT-канала) --------- */
#define ADC_TRG_GPT0_4_AB     0x19U /* ADA0 || ADB0  +  ADA4 || ADB4 */
#define ADC_TRG_GPT1_AB       0x1AU /* ADA1 || ADB1  +  ADA5 || ADB5 */
#define ADC_TRG_GPT2_AB       0x1BU /* ADA2 || ADB2  +  ADA6 || ADB6 */
#define ADC_TRG_GPT3_AB       0x1CU /* ADA3 || ADB3  +  ADA7 || ADB7 */

/* --- 0x3F : полное отключение источника ----------------------------- */
#define ADC_TRG_DISABLED      0x3FU /* «Trigger source deselected state» */

// ADC register bitfield type definitions

typedef struct
{
  uint16_t DBLANS : 5;  // [4:0]  Double-trigger channel select
  uint16_t : 1;         // [5]    Reserved
  uint16_t GBADIE : 1;  // [6]    Group-B scan-end / ELC-event interrupt enable
  uint16_t DBLE : 1;    // [7]    Double-trigger mode select
  uint16_t EXTRG : 1;   // [8]    Trigger source select (0 = sync, 1 = ADTRG pin)
  uint16_t TRGE : 1;    // [9]    Trigger start enable
  uint16_t : 3;         // [12:10] Reserved
  uint16_t ADCS : 2;    // [14:13] Scan-mode select (00=single,01=group,10=cont.)
  uint16_t ADST : 1;    // [15]   Start/stop A/D conversion
} T_adcsr_bits;

typedef struct
{
  uint16_t : 1;          // [0]    Reserved (проч.-0, запис.-0)
  uint16_t ADPRC : 2;    // [2:1]  Conversion accuracy (00=12-bit, 01=10-bit, 10=8-bit, 11=запрещено)
  uint16_t : 2;          // [4:3]  Reserved
  uint16_t ACE : 1;      // [5]    Automatic clear of ADDRn/ADDRxx (0=off/1=on)
  uint16_t : 2;          // [7:6]  Reserved
  uint16_t DIAGVAL : 2;  // [9:8]  Self-diagnosis voltage (00=prohibit, 01=0 V, 10=VREF×½, 11=VREF)
  uint16_t DIAGLD : 1;   // [10]   Self-diagnosis mode (0=rotation, 1=mixed)
  uint16_t DIAGM : 1;    // [11]   Self-diagnosis enable (0=disable, 1=enable)
  uint16_t : 3;          // [14:12]Reserved
  uint16_t ADRFMT : 1;   // [15]   Data format (0=right-justified, 1=left)
} T_adcer_bits;

typedef struct
{
  uint16_t TSSAD : 1;  // [0]  Temp-sensor addition/average mode (0=disable,1=enable)
  uint16_t OCSAD : 1;  // [1]  VREF addition/average mode        (0=disable,1=enable)
  uint16_t : 6;        // [7:2] Reserved
  uint16_t TSSA : 1;   // [8]  Temp-sensor conversion for Group A (0=disable,1=enable)
  uint16_t OCSA : 1;   // [9]  VREF conversion for Group A       (0=disable,1=enable)
  uint16_t TSSB : 1;   // [10] Temp-sensor conversion for Group B (0=disable,1=enable)
  uint16_t OCSB : 1;   // [11] VREF conversion for Group B        (0=disable,1=enable)
  uint16_t : 4;        // [15:12] Reserved
} T_adexicr_bits;

typedef struct
{
  uint8_t SST : 8;  // Sampling State Table
} T_adsstrt_bits;

typedef struct
{
  uint8_t SST : 8;  // Sampling State Table
} T_adsstro_bits;

typedef struct
{
  uint32_t : 4;       // [3:0]  Reserved
  uint32_t TSOE : 1;  // [4]    Temp-sensor output → ADC12 (0=disable, 1=enable)
  uint32_t : 2;       // [6:5]  Reserved
  uint32_t TSEN : 1;  // [7]    Temp-sensor enable      (0=stop,1=start)
  uint32_t : 24;      // [31:8] Reserved
} T_tscr_bits;

typedef struct
{
  uint16_t SSTSH : 8;   // [7:0]  Sample-and-hold time
  uint16_t SHANS0 : 1;  // [8]    AN000 S/H enable (1 = use circuit)
  uint16_t SHANS1 : 1;  // [9]    AN001 S/H enable
  uint16_t SHANS2 : 1;  // [10]   AN002 S/H enable
  uint16_t : 5;         // [15:11] Reserved
} T_adshcr_bits;

typedef struct
{
  uint16_t ANSA0 : 1;   // AN000 A/D Converted Value Addition/Average Channel Select
  uint16_t ANSA1 : 1;   // AN001 A/D Converted Value Addition/Average Channel Select
  uint16_t ANSA2 : 1;   // AN002 A/D Converted Value Addition/Average Channel Select
  uint16_t ANSA3 : 1;   // AN003 A/D Converted Value Addition/Average Channel Select
  uint16_t ANSA4 : 1;   // AN004 A/D Converted Value Addition/Average Channel Select
  uint16_t ANSA5 : 1;   // AN005 A/D Converted Value Addition/Average Channel Select
  uint16_t ANSA6 : 1;   // AN006 A/D Converted Value Addition/Average Channel Select
  uint16_t ANSA7 : 1;   // AN007 A/D Converted Value Addition/Average Channel Select
  uint16_t ANSA8 : 1;   // AN008 A/D Converted Value Addition/Average Channel Select
  uint16_t ANSA9 : 1;   // AN009 A/D Converted Value Addition/Average Channel Select
  uint16_t ANSA10 : 1;  // AN010 A/D Converted Value Addition/Average Channel Select
  uint16_t ANSA11 : 1;  // AN011 A/D Converted Value Addition/Average Channel Select
  uint16_t ANSA12 : 1;  // AN012 A/D Converted Value Addition/Average Channel Select
  uint16_t ANSA13 : 1;  // AN013 A/D Converted Value Addition/Average Channel Select
  uint16_t ANSA14 : 1;  // AN014 A/D Converted Value Addition/Average Channel Select
  uint16_t ANSA15 : 1;  // AN015 A/D Converted Value Addition/Average Channel Select
} T_adansa_bits;

typedef struct
{
  uint8_t SST : 8;  // Sampling State Table n
} T_adsstr_bits;

typedef struct
{
  uint8_t SST : 8;  // Sampling State Table L
} T_adsstrl_bits;

typedef void (*T_adc_isr_callback)(void);

typedef struct
{
  // Raw ADC samples - current measurements (multiplexed)
  uint16_t smpl_i_u_motor1;  // Motor 1 U phase current
  uint16_t smpl_i_v_motor1;  // Motor 1 V phase current
  uint16_t smpl_i_w_motor1;  // Motor 1 W phase current
  uint16_t smpl_i_u_motor2;  // Motor 2 U phase current
  uint16_t smpl_i_v_motor2;  // Motor 2 V phase current
  uint16_t smpl_i_w_motor2;  // Motor 2 W phase current

  // Raw ADC samples - voltage measurements (multiplexed)
  uint16_t smpl_v_u_motor1;  // Motor 1 U phase voltage
  uint16_t smpl_v_v_motor1;  // Motor 1 V phase voltage
  uint16_t smpl_v_w_motor1;  // Motor 1 W phase voltage
  uint16_t smpl_v_u_motor2;  // Motor 2 U phase voltage
  uint16_t smpl_v_v_motor2;  // Motor 2 V phase voltage
  uint16_t smpl_v_w_motor2;  // Motor 2 W phase voltage

  // Raw ADC samples - power and sensors (multiplexed)
  uint16_t smpl_ipwr_motor1;   // Motor 1 power supply current
  uint16_t smpl_ipwr_motor2;   // Motor 2 power supply current
  uint16_t smpl_speed_motor1;  // Motor 1 speed sensor
  uint16_t smpl_speed_motor2;  // Motor 2 speed sensor
  uint16_t smpl_pos_motor1;    // Motor 1 position sensor
  uint16_t smpl_pos_motor2;    // Motor 2 position sensor
  // Raw ADC samples - direct channels
  uint16_t smpl_v24v_mon;       // AN004 - System +24V input monitoring
  uint16_t smpl_v5v_mon;        // AN104 - +5V supply monitoring
  uint16_t smpl_v3v3_mon;       // AN105 - +3.3V reference/supply monitoring
  uint16_t smpl_thermistor_m1;  // AN016 - Motor 1 power transistors thermistor
  uint16_t smpl_thermistor_m2;  // AN117 - Motor 2 power transistors thermistor

  // EMA filtered values (32-bit for fixed-point arithmetic)
  // 10 Hz filtered channels
  uint32_t filt_v_u_motor1;    // Motor 1 U phase voltage (filtered)
  uint32_t filt_v_v_motor1;    // Motor 1 V phase voltage (filtered)
  uint32_t filt_v_w_motor1;    // Motor 1 W phase voltage (filtered)
  uint32_t filt_v_u_motor2;    // Motor 2 U phase voltage (filtered)
  uint32_t filt_v_v_motor2;    // Motor 2 V phase voltage (filtered)
  uint32_t filt_v_w_motor2;    // Motor 2 W phase voltage (filtered)
  uint32_t filt_speed_motor1;  // Motor 1 speed sensor (filtered)
  uint32_t filt_speed_motor2;  // Motor 2 speed sensor (filtered)
  uint32_t filt_pos_motor1;    // Motor 1 position sensor (filtered)
  uint32_t filt_pos_motor2;    // Motor 2 position sensor (filtered)

  // 1 Hz filtered channels
  uint32_t filt_v24v_mon;        // AN004 - System +24V input monitoring (filtered)
  uint32_t filt_v5v_mon;         // AN104 - +5V supply monitoring (filtered)
  uint32_t filt_v3v3_mon;        // AN105 - +3.3V reference/supply monitoring (filtered)
  uint32_t filt_thermistor_m1;   // AN016 - Motor 1 thermistor (filtered)
  uint32_t filt_thermistor_m2;   // AN117 - Motor 2 thermistor (filtered)
  uint32_t filt_ipwr_motor1;     // Motor 1 power supply current (filtered)
  uint32_t filt_ipwr_motor2;     // Motor 2 power supply current (filtered)
  uint32_t filt_cpu_temp;        // CPU temperature (filtered)
  bool     filters_initialized;  // Flag to indicate if EMA filters have been initialized

  // Motor current filtering for real-time control - dual EMA approach
  // Fast EMA filters for control (1.5 kHz cutoff frequency)
  uint32_t filt_i_u_motor1_fast;  // Motor 1 U phase current (fast filtered for control)
  uint32_t filt_i_v_motor1_fast;  // Motor 1 V phase current (fast filtered for control)
  uint32_t filt_i_w_motor1_fast;  // Motor 1 W phase current (fast filtered for control)
  uint32_t filt_i_u_motor2_fast;  // Motor 2 U phase current (fast filtered for control)
  uint32_t filt_i_v_motor2_fast;  // Motor 2 V phase current (fast filtered for control)
  uint32_t filt_i_w_motor2_fast;  // Motor 2 W phase current (fast filtered for control)

  // Slow EMA filters for monitoring and protection (300 Hz cutoff frequency)
  uint32_t filt_i_u_motor1_slow;  // Motor 1 U phase current (slow filtered for monitoring)
  uint32_t filt_i_v_motor1_slow;  // Motor 1 V phase current (slow filtered for monitoring)
  uint32_t filt_i_w_motor1_slow;  // Motor 1 W phase current (slow filtered for monitoring)
  uint32_t filt_i_u_motor2_slow;  // Motor 2 U phase current (slow filtered for monitoring)
  uint32_t filt_i_v_motor2_slow;  // Motor 2 V phase current (slow filtered for monitoring)
  uint32_t filt_i_w_motor2_slow;  // Motor 2 W phase current (slow filtered for monitoring)

  // Calibration and offset values
  uint16_t smpl_i_u_offs_m1;  // Motor 1 U-phase current offset
  uint16_t smpl_i_v_offs_m1;  // Motor 1 V-phase current offset
  uint16_t smpl_i_w_offs_m1;  // Motor 1 W-phase current offset
  uint16_t smpl_i_u_offs_m2;  // Motor 2 U-phase current offset
  uint16_t smpl_i_v_offs_m2;  // Motor 2 V-phase current offset
  uint16_t smpl_i_w_offs_m2;  // Motor 2 W-phase current offset

  // Accumulators for simultaneous calibration
  uint64_t smpl_i_u_m1_acc;                     // Accumulator for Motor 1 Phase U current samples
  uint64_t smpl_i_v_m1_acc;                     // Accumulator for Motor 1 Phase V current samples
  uint64_t smpl_i_w_m1_acc;                     // Accumulator for Motor 1 Phase W current samples
  uint64_t smpl_i_u_m2_acc;                     // Accumulator for Motor 2 Phase U current samples
  uint64_t smpl_i_v_m2_acc;                     // Accumulator for Motor 2 Phase V current samples
  uint64_t smpl_i_w_m2_acc;                     // Accumulator for Motor 2 Phase W current samples
  uint64_t smpl_v3v3_acc;                       // Accumulator for V3V3 voltage samples

  uint32_t total_calibration_cycles_remaining;  // Total ADC conversion cycles remaining for calibration
  uint32_t samples_per_motor_phase;             // Number of samples collected per motor phase during calibration
  bool     calibration_in_progress;             // Flag indicating if offset calibration is active

  // Multiplexer and control variables
  uint8_t  active_motor;        // Currently selected motor for multiplexed channels (ADC_MOTOR_1 or ADC_MOTOR_2)
  uint8_t  active_phase;        // Currently selected phase for voltage measurement (ADC_PHASE_U, _V, _W, or _GND)
  uint32_t scan_cycle_counter;  // Counter for multiplexer cycling

  // CPU internal sensor samples
  uint16_t smpl_cpu_temp;   // CPU temperature sensor sample
  uint16_t smpl_int_ref_v;  // Internal reference voltage sample

  // Processed ADC values (engineering units)
  // Current measurements - only filtered versions are used
  // Raw currents removed - use filtered versions instead:
  // - i_*_motor*_fast for control applications (~1.5kHz filter)
  // - i_*_motor*_slow for monitoring/protection applications (~300Hz filter)

  // Filtered current measurements for control (fast filtered)
  float i_u_motor1_fast;  // Motor 1 U phase current [A] (fast filtered for control)
  float i_v_motor1_fast;  // Motor 1 V phase current [A] (fast filtered for control)
  float i_w_motor1_fast;  // Motor 1 W phase current [A] (fast filtered for control)
  float i_u_motor2_fast;  // Motor 2 U phase current [A] (fast filtered for control)
  float i_v_motor2_fast;  // Motor 2 V phase current [A] (fast filtered for control)
  float i_w_motor2_fast;  // Motor 2 W phase current [A] (fast filtered for control)

  // Filtered current measurements for monitoring (slow filtered)
  float i_u_motor1_slow;  // Motor 1 U phase current [A] (slow filtered for monitoring)
  float i_v_motor1_slow;  // Motor 1 V phase current [A] (slow filtered for monitoring)
  float i_w_motor1_slow;  // Motor 1 W phase current [A] (slow filtered for monitoring)
  float i_u_motor2_slow;  // Motor 2 U phase current [A] (slow filtered for monitoring)
  float i_v_motor2_slow;  // Motor 2 V phase current [A] (slow filtered for monitoring)
  float i_w_motor2_slow;  // Motor 2 W phase current [A] (slow filtered for monitoring)

  // Voltage measurements
  float v_u_motor1;  // Motor 1 U phase voltage [V]
  float v_v_motor1;  // Motor 1 V phase voltage [V]
  float v_w_motor1;  // Motor 1 W phase voltage [V]
  float v_u_motor2;  // Motor 2 U phase voltage [V]
  float v_v_motor2;  // Motor 2 V phase voltage [V]
  float v_w_motor2;  // Motor 2 W phase voltage [V]

  // Power supply currents
  float ipwr_motor1;  // Motor 1 power supply current [A]
  float ipwr_motor2;  // Motor 2 power supply current [A]

  // Sensor readings
  float speed_motor1;  // Motor 1 speed sensor [rad/s or rpm]
  float speed_motor2;  // Motor 2 speed sensor [rad/s or rpm]
  float pos_motor1;    // Motor 1 position sensor [rad or deg]
  float pos_motor2;    // Motor 2 position sensor [rad or deg]

  // Monitoring voltages
  float v24v_supply;  // System +24V input voltage [V]
  float v5v_supply;   // System +5V supply voltage [V]
  float v3v3_supply;  // ADC +3.3V supply voltage [V]

  // Temperature readings
  float cpu_temp;     // CPU temperature [°C]
  float temp_motor1;  // Motor 1 power transistors temperature [°C]
  float temp_motor2;  // Motor 2 power transistors temperature [°C]

  // Scale factors for unit conversion
  float adc_scale;      // ADC counts to voltage scale factor
  float current_scale;  // Voltage to current scale factor for phase currents
  float ipwr_scale;     // Voltage to current scale factor for power supply currents
  float voltage_scale;  // Voltage divider scale factor
  float temp_scale;     // Temperature scale factor

  // Combined scale factors (to avoid double multiplication)
  float phase_current_scale;  // Combined ADC + amplifier scale for phase currents
  float power_current_scale;  // Combined ADC + amplifier scale for power currents
  float phase_voltage_scale;  // Combined ADC + divider scale for phase voltages
  float monitor_24v_scale;    // Combined ADC + divider scale for 24V monitoring (AN004)
  float monitor_5v_scale;     // Combined ADC + divider scale for 5V monitoring (AN104)
  float monitor_3v3_scale;    // Combined ADC + divider scale for 3.3V monitoring (AN105)

  // Interrupt control
  TX_EVENT_FLAGS_GROUP adc_flags;
  T_adc_isr_callback   isr_callback;

} T_adc_cbl;

extern T_adc_cbl adc;

// Function prototypes
void     Adc_driver_init(T_adc_isr_callback isr_callback);
void     Adc_driver_set_pwm_frequency(uint32_t pwm_freq);
void     Adc_driver_calculate_scaling_factors(void);
void     Adc_driver_start_scan(void);
void     Adc_driver_stop_scan(void);
uint32_t Adc_driver_set_averaging_done(void);
uint32_t Adc_driver_wait_averaging_done(uint32_t timeout_ms);
uint32_t Adc_driver_check_averaging_done(void);
void     Adc_driver_set_callback(T_adc_isr_callback isr_callback);
uint32_t Adc_driver_start_calibration(uint32_t samples_count);
void     Adc_driver_cancel_calibration(void);
void     Adc_driver_process_samples(void);
float    Adc_driver_calculate_thermistor_temperature(uint16_t adc_value);
uint16_t Adc_driver_get_position_sensor_value(uint8_t motor_id);
float    Adc_driver_get_dc_motor_current(uint8_t motor_id);
float    Adc_driver_get_supply_voltage_24v(void);

// Current filtering functions for motor control
float Adc_driver_get_motor_current_fast(uint8_t motor_id, uint8_t phase);
float Adc_driver_get_motor_current_slow(uint8_t motor_id, uint8_t phase);

#endif  // ADC_DRIVER_H
