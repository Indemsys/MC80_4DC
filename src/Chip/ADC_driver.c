#include "App.h"
/*
| АЦП| Аналоговый вход | Куда физически подключен (пин + компонент)                                      | Что измеряем / назначение                                                                                   |
| -- | --------------- | ------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------ |
| 1  | AN000           | D1 (вход) мультиплексора **U9 TMUX4053**                                        | Фазный ток **U-фазы**; по выходам S1A/S1B того же мультиплексора попадает на линии **MD1_I_U / MD2_I_U**     |
| 2  | AN001           | D2 U9 TMUX4053                                                                  | Фазный ток **V-фазы** (MD1_I_V / MD2_I_V)                                                                    |
| 3  | AN002           | D3 U9 TMUX4053                                                                  | Фазный ток **W-фазы** (MD1_I_W / MD2_I_W)                                                                    |
| 4  | AN004           | Буфер-усилитель **U3 TLV9101** → RC-фильтр (R10 100 Ω, C26 1 nF)                | Мониторинг напряжения питания +24 V                                                                          |
| 5  | AN005           | Вход DA мультиплексора **U12 TMUX4052**                                         | Фазные напряжения двигателя **АЦП 1**: MD1_V_U / _V_V / _V_W                                                 |
| 6  | AN006           | Вход DB U12 TMUX4052                                                            | Фазные напряжения двигателя **АЦП 2**: MD2_V_U / _V_V / _V_W                                                 |
| 7  | AN100           | D1 мультиплексора **U11 TMUX4053**                                              | Шинный ток питания драйверов (линии **MD1_I_PWR / MD2_I_PWR**)                                               |
| 8  | AN101           | D2 U11 TMUX4053                                                                 | Выход датчика скорости ротора (MD1_SPEED / MD2_SPEED)                                                        |
| 9  | AN102           | D3 U11 TMUX4053                                                                 | Аналоговый датчик положения (MD1_POS / MD2_POS)                                                              |
| 10 | AN104           | Буфер-усилитель **U4 TLV9101** → RC-фильтр (R12 100 Ω, C27 1 nF)                | Контроль напряжения **+5V**                                                                                  |
| 11 | AN105           | Выход операционного усилителя **U8 THS4281** (R22‒R23, C45‒C46 фильтр)          | Напряжение пиатния АЦП **+3.3ADCV**                                                                          |
| 12 | AN016           | Прямо на разъем **M1** через делитель R=10кОм + термистор NCP21XV103J03RA       | Температура силовых транзисторов двигателя №1 (термистор к земле, резистор к +3.3V)                          |
| 13 | AN117           | Прямо на разъем **M2** через делитель R=10кОм + термистор NCP21XV103J03RA       | Температура силовых транзисторов двигателя №2 (термистор к земле, резистор к +3.3V)                          |
  -------------------------------------------------------------
    АЦП: карта входов и управление мультиплексорами
  -------------------------------------------------------------
   1. Тройные 2-к-1 мультиплексоры U9 и U11 (TMUX4053)
      ──────────────────────────────────────────────────────
      • Линия SEL  (MCU вывод P413) одна на все три канала.
          SEL = 0 → подключен Двигатель №1  (линии SxA)
          SEL = 1 → подключен Двигатель №2  (линии SxB)
          ┌────────┬─────────────────────────┬─────────────────────────┐
          │ ADCвход│  Двигатель №1 (SEL=0)   │ Двигатель №2 (SEL=1)    │
          ├────────┼─────────────────────────┼─────────────────────────┤
          │ AN000  │ MD1_I_U  – ток фазы U   │ MD2_I_U  – ток фазы U   │
          │ AN001  │ MD1_I_V  – ток фазы V   │ MD2_I_V  – ток фазы V   │
          │ AN002  │ MD1_I_W  – ток фазы W   │ MD2_I_W  – ток фазы W   │
          │ AN100  │ MD1_I_PWR – шинный ток  │ MD2_I_PWR – шинный ток  │
          │ AN101  │ MD1_SPEED  – датч.скор. │ MD2_SPEED  – датч.скор. │
          │ AN102  │ MD1_POS    – датч.позиц.│ MD2_POS    – датч.позиц.│
          └────────┴─────────────────────────┴─────────────────────────┘
   2. Двойной 4-к-1 мультиплексор U12 (TMUX4052)
      ──────────────────────────────────────────────────────
      • Линии выбора:  A1 = MCU P614 (старший бит)
                       A0 = MCU P613 (младший бит)
          Код A1:A0  →  AN005 (двиг.1) / AN006 (двиг.2)
          -------------------------------------------------
            00  →  MDx_V_U   – напряжение фазы U
            01  →  MDx_V_V   – напряжение фазы V
            10  →  MDx_V_W   – напряжение фазы W
            11  →  GND       – обнуление / калибровка нуля
   3. Входы АЦП без переключения
      ──────────────────────────────────────────────────────
         AN004  – монитор системного  +24 V
         AN104  – монитор +5 V (питание системы)
         AN105  – монитор +3.3 V (аналоговое напряжение питания)
         AN016  – термистор силовых транзисторов двигателя №1 (NCP21XV103J03RA)
         AN117  – термистор силовых транзисторов двигателя №2 (NCP21XV103J03RA)
  -----------------------------------------------------------
*/

// ADC interrupt vector definition
void Adc_scan_end_isr(void);

#define ADC_AVERAGING_DONE BIT(0)

T_adc_cbl adc;

// Static function declarations
static void            _Adc_init_registers(void);
static void            _Adc_configure_channels(void);
static void            _Adc_configure_interrupts(void);
FORCE_INLINE_ATTR void _Adc_sampling_data_collection(void);
FORCE_INLINE_ATTR void _Adc_multiplexer_control(void);
FORCE_INLINE_ATTR void _Adc_apply_ema_filtering(void);

// Global PWM frequency and dynamic EMA coefficients
uint32_t g_adc_pwm_frequency          = 16000;  // Default PWM frequency in Hz
uint16_t g_ema_alpha_10hz_direct      = 0;      // Dynamic coefficient for direct channels, 10Hz
uint16_t g_ema_alpha_1hz_direct       = 0;      // Dynamic coefficient for direct channels, 1Hz
uint16_t g_ema_alpha_10hz_2ch         = 0;      // Dynamic coefficient for 2-ch mux, 10Hz
uint16_t g_ema_alpha_1hz_2ch          = 0;      // Dynamic coefficient for 2-ch mux, 1Hz
uint16_t g_ema_alpha_10hz_3ch         = 0;      // Dynamic coefficient for 3-ch mux, 10Hz
uint16_t g_ema_alpha_1hz_3ch          = 0;      // Dynamic coefficient for 3-ch mux, 1Hz
uint16_t g_ema_alpha_current_fast_2ch = 0;      // Dynamic coefficient for fast current filtering, 2-ch mux
uint16_t g_ema_alpha_current_slow_2ch = 0;      // Dynamic coefficient for slow current filtering, 2-ch mux

/*-----------------------------------------------------------------------------------------------------
  ADC scan end interrupt service routine
  Called at PWM frequency for synchronized sampling

  Parameters: void

  Return: void
-----------------------------------------------------------------------------------------------------*/
void Adc_scan_end_isr(void)
{
  _Adc_sampling_data_collection();

  if (adc.isr_callback)
  {
    adc.isr_callback();
  }

  R_ICU->IELSR_b[ADC0_SCAN_END_IRQn].IR = 0;  // Clear interrupt flag in ICU
}

/*-----------------------------------------------------------------------------------------------------
  Apply EMA filtering to relevant ADC channels for noise reduction
  Uses optimized fixed-point arithmetic for real-time performance
  Applies filtering only to channels that were actually updated in current scan cycle:
  - Direct channels (16 kHz): thermistors, supply voltages, CPU temp - filtered every interrupt
  - Multiplexed channels: filtered only when the specific channel is active and updated
    * Phase voltages (5.33 kHz): U→V→W cycle, filtered when each phase is sampled
    * Motor sensors (8 kHz): speed/position/power, filtered when specific motor is active

  Parameters: void
  Return: void
-----------------------------------------------------------------------------------------------------*/
FORCE_INLINE_PRAGMA
FORCE_INLINE_ATTR void _Adc_apply_ema_filtering(void)
{
  if (!adc.filters_initialized)
  {
    // Initialize filtered values with current samples shifted to Q16 format
    // Note: Optimized EMA formula still uses Q16 format for data storage
    adc.filt_v_u_motor1      = (uint32_t)adc.smpl_v_u_motor1 << EMA_FILTER_SHIFT;
    adc.filt_v_v_motor1      = (uint32_t)adc.smpl_v_v_motor1 << EMA_FILTER_SHIFT;
    adc.filt_v_w_motor1      = (uint32_t)adc.smpl_v_w_motor1 << EMA_FILTER_SHIFT;
    adc.filt_v_u_motor2      = (uint32_t)adc.smpl_v_u_motor2 << EMA_FILTER_SHIFT;
    adc.filt_v_v_motor2      = (uint32_t)adc.smpl_v_v_motor2 << EMA_FILTER_SHIFT;
    adc.filt_v_w_motor2      = (uint32_t)adc.smpl_v_w_motor2 << EMA_FILTER_SHIFT;
    adc.filt_speed_motor1    = (uint32_t)adc.smpl_speed_motor1 << EMA_FILTER_SHIFT;
    adc.filt_speed_motor2    = (uint32_t)adc.smpl_speed_motor2 << EMA_FILTER_SHIFT;
    adc.filt_pos_motor1      = (uint32_t)adc.smpl_pos_motor1 << EMA_FILTER_SHIFT;
    adc.filt_pos_motor2      = (uint32_t)adc.smpl_pos_motor2 << EMA_FILTER_SHIFT;
    adc.filt_v24v_mon        = (uint32_t)adc.smpl_v24v_mon << EMA_FILTER_SHIFT;
    adc.filt_v5v_mon         = (uint32_t)adc.smpl_v5v_mon << EMA_FILTER_SHIFT;
    adc.filt_v3v3_mon        = (uint32_t)adc.smpl_v3v3_mon << EMA_FILTER_SHIFT;
    adc.filt_thermistor_m1   = (uint32_t)adc.smpl_thermistor_m1 << EMA_FILTER_SHIFT;
    adc.filt_thermistor_m2   = (uint32_t)adc.smpl_thermistor_m2 << EMA_FILTER_SHIFT;
    adc.filt_ipwr_motor1     = (uint32_t)adc.smpl_ipwr_motor1 << EMA_FILTER_SHIFT;
    adc.filt_ipwr_motor2     = (uint32_t)adc.smpl_ipwr_motor2 << EMA_FILTER_SHIFT;
    adc.filt_cpu_temp        = (uint32_t)adc.smpl_cpu_temp << EMA_FILTER_SHIFT;

    // Initialize dual-EMA filtered current values (fast and slow)
    adc.filt_i_u_motor1_fast = (uint32_t)adc.smpl_i_u_motor1 << EMA_FILTER_SHIFT;
    adc.filt_i_v_motor1_fast = (uint32_t)adc.smpl_i_v_motor1 << EMA_FILTER_SHIFT;
    adc.filt_i_w_motor1_fast = (uint32_t)adc.smpl_i_w_motor1 << EMA_FILTER_SHIFT;
    adc.filt_i_u_motor2_fast = (uint32_t)adc.smpl_i_u_motor2 << EMA_FILTER_SHIFT;
    adc.filt_i_v_motor2_fast = (uint32_t)adc.smpl_i_v_motor2 << EMA_FILTER_SHIFT;
    adc.filt_i_w_motor2_fast = (uint32_t)adc.smpl_i_w_motor2 << EMA_FILTER_SHIFT;

    adc.filt_i_u_motor1_slow = (uint32_t)adc.smpl_i_u_motor1 << EMA_FILTER_SHIFT;
    adc.filt_i_v_motor1_slow = (uint32_t)adc.smpl_i_v_motor1 << EMA_FILTER_SHIFT;
    adc.filt_i_w_motor1_slow = (uint32_t)adc.smpl_i_w_motor1 << EMA_FILTER_SHIFT;
    adc.filt_i_u_motor2_slow = (uint32_t)adc.smpl_i_u_motor2 << EMA_FILTER_SHIFT;
    adc.filt_i_v_motor2_slow = (uint32_t)adc.smpl_i_v_motor2 << EMA_FILTER_SHIFT;
    adc.filt_i_w_motor2_slow = (uint32_t)adc.smpl_i_w_motor2 << EMA_FILTER_SHIFT;

    adc.filters_initialized  = true;
  }
  else
  {  // Apply 1 Hz EMA filter to direct channels (sampled every interrupt at PWM frequency)
    // Using optimized formula to avoid 64-bit arithmetic operations
    adc.filt_v24v_mon      = EMA_FILTER_UPDATE(adc.filt_v24v_mon, adc.smpl_v24v_mon, g_ema_alpha_1hz_direct);
    adc.filt_v5v_mon       = EMA_FILTER_UPDATE(adc.filt_v5v_mon, adc.smpl_v5v_mon, g_ema_alpha_1hz_direct);
    adc.filt_v3v3_mon      = EMA_FILTER_UPDATE(adc.filt_v3v3_mon, adc.smpl_v3v3_mon, g_ema_alpha_1hz_direct);
    adc.filt_thermistor_m1 = EMA_FILTER_UPDATE(adc.filt_thermistor_m1, adc.smpl_thermistor_m1, g_ema_alpha_1hz_direct);
    adc.filt_thermistor_m2 = EMA_FILTER_UPDATE(adc.filt_thermistor_m2, adc.smpl_thermistor_m2, g_ema_alpha_1hz_direct);
    adc.filt_cpu_temp      = EMA_FILTER_UPDATE(adc.filt_cpu_temp, adc.smpl_cpu_temp, g_ema_alpha_1hz_direct);

    // Apply filtering to multiplexed channels only when they are active and updated
    // Phase voltage filtering: only filter the currently sampled phase for both motors
    if (adc.active_phase == ADC_PHASE_U)
    {  // Filter U phase voltage for both motors (PWM_freq/3 effective sampling rate)
      // Using optimized formula to avoid 64-bit arithmetic operations
      adc.filt_v_u_motor1 = EMA_FILTER_UPDATE(adc.filt_v_u_motor1, adc.smpl_v_u_motor1, g_ema_alpha_10hz_3ch);
      adc.filt_v_u_motor2 = EMA_FILTER_UPDATE(adc.filt_v_u_motor2, adc.smpl_v_u_motor2, g_ema_alpha_10hz_3ch);
    }
    else if (adc.active_phase == ADC_PHASE_V)
    {  // Filter V phase voltage for both motors (PWM_freq/3 effective sampling rate)
      // Using optimized formula to avoid 64-bit arithmetic operations
      adc.filt_v_v_motor1 = EMA_FILTER_UPDATE(adc.filt_v_v_motor1, adc.smpl_v_v_motor1, g_ema_alpha_10hz_3ch);
      adc.filt_v_v_motor2 = EMA_FILTER_UPDATE(adc.filt_v_v_motor2, adc.smpl_v_v_motor2, g_ema_alpha_10hz_3ch);
    }
    else if (adc.active_phase == ADC_PHASE_W)
    {  // Filter W phase voltage for both motors (PWM_freq/3 effective sampling rate)
      // Using optimized formula to avoid 64-bit arithmetic operations
      adc.filt_v_w_motor1 = EMA_FILTER_UPDATE(adc.filt_v_w_motor1, adc.smpl_v_w_motor1, g_ema_alpha_10hz_3ch);
      adc.filt_v_w_motor2 = EMA_FILTER_UPDATE(adc.filt_v_w_motor2, adc.smpl_v_w_motor2, g_ema_alpha_10hz_3ch);
    }
    // Phase voltage filtering complete: all three phases (U, V, W) processed

    // Motor sensor filtering: only filter sensors for the currently active motor
    if (adc.active_motor == ADC_MOTOR_1)
    {  // Filter Motor 1 sensors (PWM_freq/2 effective sampling rate)
      // Using optimized formula to avoid 64-bit arithmetic operations
      adc.filt_speed_motor1    = EMA_FILTER_UPDATE(adc.filt_speed_motor1, adc.smpl_speed_motor1, g_ema_alpha_10hz_2ch);
      adc.filt_pos_motor1      = EMA_FILTER_UPDATE(adc.filt_pos_motor1, adc.smpl_pos_motor1, g_ema_alpha_10hz_2ch);
      adc.filt_ipwr_motor1     = EMA_FILTER_UPDATE(adc.filt_ipwr_motor1, adc.smpl_ipwr_motor1, g_ema_alpha_1hz_2ch);

      // Apply dual-EMA filtering to Motor 1 phase currents (PWM_freq/2 effective sampling rate)
      // Fast filter for control (~1.5kHz cutoff)
      adc.filt_i_u_motor1_fast = EMA_FILTER_UPDATE(adc.filt_i_u_motor1_fast, adc.smpl_i_u_motor1, g_ema_alpha_current_fast_2ch);
      adc.filt_i_v_motor1_fast = EMA_FILTER_UPDATE(adc.filt_i_v_motor1_fast, adc.smpl_i_v_motor1, g_ema_alpha_current_fast_2ch);
      adc.filt_i_w_motor1_fast = EMA_FILTER_UPDATE(adc.filt_i_w_motor1_fast, adc.smpl_i_w_motor1, g_ema_alpha_current_fast_2ch);

      // Slow filter for monitoring/protection (~300Hz cutoff)
      adc.filt_i_u_motor1_slow = EMA_FILTER_UPDATE(adc.filt_i_u_motor1_slow, adc.smpl_i_u_motor1, g_ema_alpha_current_slow_2ch);
      adc.filt_i_v_motor1_slow = EMA_FILTER_UPDATE(adc.filt_i_v_motor1_slow, adc.smpl_i_v_motor1, g_ema_alpha_current_slow_2ch);
      adc.filt_i_w_motor1_slow = EMA_FILTER_UPDATE(adc.filt_i_w_motor1_slow, adc.smpl_i_w_motor1, g_ema_alpha_current_slow_2ch);
    }
    else  // ADC_MOTOR_2
    {
      // Filter Motor 2 sensors (PWM_freq/2 effective sampling rate)
      // Using optimized formula to avoid 64-bit arithmetic operations
      adc.filt_speed_motor2    = EMA_FILTER_UPDATE(adc.filt_speed_motor2, adc.smpl_speed_motor2, g_ema_alpha_10hz_2ch);
      adc.filt_pos_motor2      = EMA_FILTER_UPDATE(adc.filt_pos_motor2, adc.smpl_pos_motor2, g_ema_alpha_10hz_2ch);
      adc.filt_ipwr_motor2     = EMA_FILTER_UPDATE(adc.filt_ipwr_motor2, adc.smpl_ipwr_motor2, g_ema_alpha_1hz_2ch);

      // Apply dual-EMA filtering to Motor 2 phase currents (PWM_freq/2 effective sampling rate)
      // Fast filter for control (~1.5kHz cutoff)
      adc.filt_i_u_motor2_fast = EMA_FILTER_UPDATE(adc.filt_i_u_motor2_fast, adc.smpl_i_u_motor2, g_ema_alpha_current_fast_2ch);
      adc.filt_i_v_motor2_fast = EMA_FILTER_UPDATE(adc.filt_i_v_motor2_fast, adc.smpl_i_v_motor2, g_ema_alpha_current_fast_2ch);
      adc.filt_i_w_motor2_fast = EMA_FILTER_UPDATE(adc.filt_i_w_motor2_fast, adc.smpl_i_w_motor2, g_ema_alpha_current_fast_2ch);

      // Slow filter for monitoring/protection (~300Hz cutoff)
      adc.filt_i_u_motor2_slow = EMA_FILTER_UPDATE(adc.filt_i_u_motor2_slow, adc.smpl_i_u_motor2, g_ema_alpha_current_slow_2ch);
      adc.filt_i_v_motor2_slow = EMA_FILTER_UPDATE(adc.filt_i_v_motor2_slow, adc.smpl_i_v_motor2, g_ema_alpha_current_slow_2ch);
      adc.filt_i_w_motor2_slow = EMA_FILTER_UPDATE(adc.filt_i_w_motor2_slow, adc.smpl_i_w_motor2, g_ema_alpha_current_slow_2ch);
    }
  }
}
/*-----------------------------------------------------------------------------------------------------
  Collect ADC sampling data from registers and update control structure
  Execution time: ~2-3 us
  Parameters: void

  Return: void
-----------------------------------------------------------------------------------------------------*/
FORCE_INLINE_PRAGMA
FORCE_INLINE_ATTR void _Adc_sampling_data_collection(void)
{
  // Read direct ADC channels (non-multiplexed)
  adc.smpl_v24v_mon      = R_ADC0->ADDR[4];   // AN004 - System +24V input monitoring
  adc.smpl_v5v_mon       = R_ADC1->ADDR[4];   // AN104 - +5V supply monitoring
  adc.smpl_v3v3_mon      = R_ADC1->ADDR[5];   // AN105 - +3.3V reference/supply monitoring
  adc.smpl_thermistor_m1 = R_ADC0->ADDR[16];  // AN016 - Motor 1 thermistor
  adc.smpl_thermistor_m2 = R_ADC1->ADDR[17];  // AN117 - Motor 2 thermistor
  // Read multiplexed channels - assign directly to motor-specific variables
  if (adc.active_motor == ADC_MOTOR_1)
  {
    adc.smpl_i_u_motor1   = R_ADC0->ADDR[0];  // AN000 - Current motor 1 U phase
    adc.smpl_i_v_motor1   = R_ADC0->ADDR[1];  // AN001 - Current motor 1 V phase
    adc.smpl_i_w_motor1   = R_ADC0->ADDR[2];  // AN002 - Current motor 1 W phase
    adc.smpl_ipwr_motor1  = R_ADC1->ADDR[0];  // AN100 - Current motor 1 power
    adc.smpl_speed_motor1 = R_ADC1->ADDR[1];  // AN101 - Current motor 1 speed
    adc.smpl_pos_motor1   = R_ADC1->ADDR[2];  // AN102 - Current motor 1 position
  }
  else                                        // ADC_MOTOR_2
  {
    adc.smpl_i_u_motor2   = R_ADC0->ADDR[0];  // AN000 - Current motor 2 U phase
    adc.smpl_i_v_motor2   = R_ADC0->ADDR[1];  // AN001 - Current motor 2 V phase
    adc.smpl_i_w_motor2   = R_ADC0->ADDR[2];  // AN002 - Current motor 2 W phase
    adc.smpl_ipwr_motor2  = R_ADC1->ADDR[0];  // AN100 - Current motor 2 power
    adc.smpl_speed_motor2 = R_ADC1->ADDR[1];  // AN101 - Current motor 2 speed
    adc.smpl_pos_motor2   = R_ADC1->ADDR[2];  // AN102 - Current motor 2 position
  }

  // Read CPU internal sensors
  adc.smpl_cpu_temp  = R_ADC0->ADTSDR;  // CPU temperature
  adc.smpl_int_ref_v = R_ADC0->ADOCDR;  // Internal reference

  // Read phase voltages and assign directly to motor-specific variables based on active phase
  // AN005 and AN006 are always sampled and contain current phase data for both motors
  switch (adc.active_phase)
  {
    case ADC_PHASE_U:
      adc.smpl_v_u_motor1 = R_ADC0->ADDR[5];  // AN005 contains U phase voltage for Motor 1
      adc.smpl_v_u_motor2 = R_ADC0->ADDR[6];  // AN006 contains U phase voltage for Motor 2
      break;
    case ADC_PHASE_V:
      adc.smpl_v_v_motor1 = R_ADC0->ADDR[5];  // AN005 contains V phase voltage for Motor 1
      adc.smpl_v_v_motor2 = R_ADC0->ADDR[6];  // AN006 contains V phase voltage for Motor 2
      break;
    case ADC_PHASE_W:
      adc.smpl_v_w_motor1 = R_ADC0->ADDR[5];  // AN005 contains W phase voltage for Motor 1
      adc.smpl_v_w_motor2 = R_ADC0->ADDR[6];  // AN006 contains W phase voltage for Motor 2
      break;
  }

  // Apply EMA filtering to relevant channels
  _Adc_apply_ema_filtering();
  // Handle calibration accumulation
  if (adc.calibration_in_progress)
  {
    // Accumulate current samples for active motor only
    if (adc.active_motor == ADC_MOTOR_1)
    {
      adc.smpl_i_u_m1_acc += adc.smpl_i_u_motor1;
      adc.smpl_i_v_m1_acc += adc.smpl_i_v_motor1;
      adc.smpl_i_w_m1_acc += adc.smpl_i_w_motor1;
    }
    else  // ADC_MOTOR_2
    {
      adc.smpl_i_u_m2_acc += adc.smpl_i_u_motor2;
      adc.smpl_i_v_m2_acc += adc.smpl_i_v_motor2;
      adc.smpl_i_w_m2_acc += adc.smpl_i_w_motor2;
    }
    adc.smpl_v3v3_acc += adc.smpl_v3v3_mon;

    adc.total_calibration_cycles_remaining--;
    if (adc.total_calibration_cycles_remaining == 0)
    {
      adc.calibration_in_progress = false;  // Stop calibration
      Adc_driver_set_averaging_done();
    }
  }

  // Control multiplexer cycling
  _Adc_multiplexer_control();
}

/*-----------------------------------------------------------------------------------------------------
  Control multiplexer cycling for independent motor and phase voltage sampling

  Two independent multiplexer control strategies:
    1. Motor Multiplexer (AN000-002, AN100-102): switches every 2 scans
     Scan Cycle | Motor | Action
     -----------|-------|--------------------------------------------------
         1      |   1   | Motor 1 (scan 1 of 2)
         2      |   1   | Motor 1 (scan 2 of 2)
         3      |   2   | Motor 2 (scan 1 of 2)
         4      |   2   | Motor 2 (scan 2 of 2)
         5      |   1   | Motor 1 (scan 1 of 2)
         6      |   1   | Motor 1 (scan 2 of 2)
         7      |   2   | Motor 2 (scan 1 of 2)
     ...and so on

  2. Phase Voltage Multiplexer (AN005, AN006): cycles U→V→W→U independently
     Scan Cycle | Phase | Action
     -----------|-------|--------------------------------------------------
         1      |   U   | Phase U voltage for both motors
         2      |   V   | Phase V voltage for both motors
         3      |   W   | Phase W voltage for both motors
         4      |   U   | Phase U voltage for both motors
         5      |   V   | Phase V voltage for both motors
         6      |   W   | Phase W voltage for both motors
     ...and so on
  Parameters: void

  Return: void
-----------------------------------------------------------------------------------------------------*/
FORCE_INLINE_PRAGMA
FORCE_INLINE_ATTR void _Adc_multiplexer_control(void)
{
  adc.scan_cycle_counter++;
  // Motor multiplexer: switch every 2 scans (M1→M1→M2→M2→M1→M1→...)
  if ((adc.scan_cycle_counter % 2) == 0)
  {
    // Switch to next motor after 2 scans
    if (adc.active_motor == ADC_MOTOR_1)
    {
      adc.active_motor = ADC_MOTOR_2;
      ADC_SELECT_MOTOR2();  // P413 = 1 - Inline motor selection
    }
    else
    {
      adc.active_motor = ADC_MOTOR_1;
      ADC_SELECT_MOTOR1();  // P413 = 0 - Inline motor selection
    }
  }

  // Phase voltage multiplexer: cycle U→V→W independently every scan
  adc.active_phase = (adc.active_phase + 1) % 3;  // 0=U, 1=V, 2=W

  // Inline phase selection logic
  switch (adc.active_phase)
  {
    case ADC_PHASE_U:
      ADC_SELECT_PHASE_U();  // P614:P613 = 00
      break;
    case ADC_PHASE_V:
      ADC_SELECT_PHASE_V();  // P614:P613 = 01
      break;
    case ADC_PHASE_W:
      ADC_SELECT_PHASE_W();  // P614:P613 = 10
      break;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Set averaging done flag

  Parameters: void

  Return: uint32_t - TX result code
-----------------------------------------------------------------------------------------------------*/
uint32_t Adc_driver_set_averaging_done(void)
{
  return tx_event_flags_set(&adc.adc_flags, ADC_AVERAGING_DONE, TX_OR);
}

/*-----------------------------------------------------------------------------------------------------
  Wait for averaging completion

  Parameters: timeout_ms - timeout in milliseconds

  Return: uint32_t - TX result code
-----------------------------------------------------------------------------------------------------*/
uint32_t Adc_driver_wait_averaging_done(uint32_t timeout_ms)
{
  ULONG actual_events;
  return tx_event_flags_get(&adc.adc_flags, ADC_AVERAGING_DONE, TX_OR_CLEAR, &actual_events, MS_TO_TICKS(timeout_ms));
}

/*-----------------------------------------------------------------------------------------------------
  Check if averaging is completed without waiting (non-blocking)

  Parameters: None

  Return:
    TX_SUCCESS - Averaging completed
    TX_NO_EVENTS - Averaging not completed yet
-----------------------------------------------------------------------------------------------------*/
uint32_t Adc_driver_check_averaging_done(void)
{
  ULONG actual_events;
  return tx_event_flags_get(&adc.adc_flags, ADC_AVERAGING_DONE, TX_OR_CLEAR, &actual_events, TX_NO_WAIT);
}

/*-----------------------------------------------------------------------------------------------------
  Initialize ADC registers for MC80 configuration

  Parameters: void

  Return: void
-----------------------------------------------------------------------------------------------------*/
static void _Adc_init_registers(void)
{
  // Enable ADC module clocks
  R_MSTP->MSTPCRD_b.MSTPD15 = 0;  // 12-Bit A/D Converter 1 Module
  R_MSTP->MSTPCRD_b.MSTPD16 = 0;  // 12-Bit A/D Converter 0 Module
  R_MSTP->MSTPCRD_b.MSTPD22 = 0;  // Temperature Sensor Module

  // Configure ADC0 control register
  T_adcsr_bits adcsr_temp   = *(T_adcsr_bits*)&R_ADC0->ADCSR;
  adcsr_temp.ADST           = 0;  // A/D Conversion Start
  adcsr_temp.ADCS           = 0;  // Single scan mode
  adcsr_temp.TRGE           = 1;  // Trigger Start Enable
  adcsr_temp.EXTRG          = 0;  // Synchronous trigger (GPT0 direct connection)
  adcsr_temp.DBLE           = 0;  // Double Trigger Mode disabled
  adcsr_temp.GBADIE         = 0;  // Group B Scan interrupt disabled
  adcsr_temp.DBLANS         = 0;  // Double Trigger Channel Select
  R_ADC0->ADCSR             = *(uint16_t*)&adcsr_temp;

  // Configure ADC1 control register (same settings)
  R_ADC1->ADCSR             = *(uint16_t*)&adcsr_temp;

  // Configure ADC conversion accuracy and format
  T_adcer_bits adcer_temp   = *(T_adcer_bits*)&R_ADC0->ADCER;
  adcer_temp.ADRFMT         = 0;  // Right-aligned format
  adcer_temp.DIAGM          = 0;  // Self-diagnosis disabled
  adcer_temp.DIAGLD         = 0;  // Self-diagnosis rotation mode
  adcer_temp.DIAGVAL        = 0;  // Self-diagnosis voltage select
  adcer_temp.ACE            = 0;  // Auto clearing disabled
  adcer_temp.ADPRC          = 0;  // 12-bit accuracy
  R_ADC0->ADCER             = *(uint16_t*)&adcer_temp;

  // Same configuration for ADC1
  R_ADC1->ADCER             = *(uint16_t*)&adcer_temp;  // Configure internal sensors
}

/*-----------------------------------------------------------------------------------------------------
  Configure ADC channels for scanning

  Parameters: void

  Return: void
-----------------------------------------------------------------------------------------------------*/
static void _Adc_configure_channels(void)
{
  // ADC0 channels configuration
  T_adansa_bits adansa0_temp = *(T_adansa_bits*)&R_ADC0->ADANSA[0];
  adansa0_temp.ANSA0         = 1;  // AN000 - Motor phase U current
  adansa0_temp.ANSA1         = 1;  // AN001 - Motor phase V current
  adansa0_temp.ANSA2         = 1;  // AN002 - Motor phase W current
  adansa0_temp.ANSA4         = 1;  // AN004 - System +24V monitoring (via differential amplifier)
  adansa0_temp.ANSA5         = 1;  // AN005 - Motor 1 phase voltages
  adansa0_temp.ANSA6         = 1;  // AN006 - Motor 2 phase voltages
  R_ADC0->ADANSA[0]          = *(uint16_t*)&adansa0_temp;

  T_adansa_bits adansa1_temp = *(T_adansa_bits*)&R_ADC0->ADANSA[1];
  adansa1_temp.ANSA0         = 1;  // AN016 - Motor 1 power transistors thermistor
  R_ADC0->ADANSA[1]          = *(uint16_t*)&adansa1_temp;

  // Configure sampling times for ADC0
  T_adsstr_bits adsstr_temp;
  adsstr_temp.SST                  = ADC_SAMPLE_STATES_COUNT;
  R_ADC0->ADSSTR[0]                = *(uint8_t*)&adsstr_temp;  // AN000 - Motor phase U current
  R_ADC0->ADSSTR[1]                = *(uint8_t*)&adsstr_temp;  // AN001 - Motor phase V current
  R_ADC0->ADSSTR[2]                = *(uint8_t*)&adsstr_temp;  // AN002 - Motor phase W current
  R_ADC0->ADSSTR[4]                = *(uint8_t*)&adsstr_temp;  // AN004 - System +24V monitoring (via differential amplifier)
  R_ADC0->ADSSTR[5]                = *(uint8_t*)&adsstr_temp;  // AN005 - Motor 1 phase voltages
  R_ADC0->ADSSTR[6]                = *(uint8_t*)&adsstr_temp;  // AN006 - Motor 2 phase voltages

  T_adsstrl_bits adsstrl_temp      = *(T_adsstrl_bits*)&R_ADC0->ADSSTRL;
  adsstrl_temp.SST                 = ADC_SAMPLE_STATES_COUNT;  // AN016+ sampling time (includes Motor 1 thermistor)
  R_ADC0->ADSSTRL                  = *(uint8_t*)&adsstrl_temp;

  // ADC1 channels configuration
  T_adansa_bits adansa1_adc1_temp  = *(T_adansa_bits*)&R_ADC1->ADANSA[0];
  adansa1_adc1_temp.ANSA0          = 1;  // AN100 - Motor power supply current
  adansa1_adc1_temp.ANSA1          = 1;  // AN101 - Motor speed sensor
  adansa1_adc1_temp.ANSA2          = 1;  // AN102 - Motor position sensor
  adansa1_adc1_temp.ANSA4          = 1;  // AN104 - +5V supply monitoring
  adansa1_adc1_temp.ANSA5          = 1;  // AN105 - +3.3V reference/supply monitoring
  R_ADC1->ADANSA[0]                = *(uint16_t*)&adansa1_adc1_temp;
  T_adansa_bits adansa1_adc1_temp2 = *(T_adansa_bits*)&R_ADC1->ADANSA[1];
  adansa1_adc1_temp2.ANSA1         = 1;  // AN117 - Motor 2 power transistors thermistor
  R_ADC1->ADANSA[1]                = *(uint16_t*)&adansa1_adc1_temp2;

  // Configure sampling times for ADC1
  adsstr_temp.SST                  = ADC_SAMPLE_STATES_COUNT;
  R_ADC1->ADSSTR[0]                = *(uint8_t*)&adsstr_temp;  // AN100 - Motor power supply current
  R_ADC1->ADSSTR[1]                = *(uint8_t*)&adsstr_temp;  // AN101 - Motor speed sensor
  R_ADC1->ADSSTR[2]                = *(uint8_t*)&adsstr_temp;  // AN102 - Motor position sensor
  R_ADC1->ADSSTR[4]                = *(uint8_t*)&adsstr_temp;  // AN104 - +5V supply monitoring
  R_ADC1->ADSSTR[5]                = *(uint8_t*)&adsstr_temp;  // AN105 - +3.3V reference/supply monitoring

  T_adsstrl_bits adsstrl_adc1_temp = *(T_adsstrl_bits*)&R_ADC1->ADSSTRL;
  adsstrl_adc1_temp.SST            = ADC_SAMPLE_STATES_COUNT;  // AN117+ sampling time (includes Motor 2 thermistor)
  R_ADC1->ADSSTRL                  = *(uint8_t*)&adsstrl_adc1_temp;

  // Configure sample and hold for simultaneous current measurements
  T_adshcr_bits adshcr_temp        = *(T_adshcr_bits*)&R_ADC0->ADSHCR;
  adshcr_temp.SSTSH                = ADC_SAMPLE_STATES_COUNT;  // Sample and hold time
  adshcr_temp.SHANS0               = 1;                        // AN000 sample-and-hold
  adshcr_temp.SHANS1               = 1;                        // AN001 sample-and-hold
  adshcr_temp.SHANS2               = 1;                        // AN002 sample-and-hold
  R_ADC0->ADSHCR                   = *(uint16_t*)&adshcr_temp;

  adshcr_temp                      = *(T_adshcr_bits*)&R_ADC1->ADSHCR;
  adshcr_temp.SSTSH                = ADC_SAMPLE_STATES_COUNT;  // Sample and hold time
  adshcr_temp.SHANS0               = 1;                        // AN100 sample-and-hold
  adshcr_temp.SHANS1               = 1;                        // AN101 sample-and-hold
  adshcr_temp.SHANS2               = 1;                        // AN102 sample-and-hold
  R_ADC1->ADSHCR                   = *(uint16_t*)&adshcr_temp;

  T_adexicr_bits adexicr_temp      = *(T_adexicr_bits*)&R_ADC0->ADEXICR;
  adexicr_temp.OCSB                = 1;  // Internal ref voltage group B enabled
  adexicr_temp.TSSB                = 0;  // Temperature sensor group B disabled
  adexicr_temp.OCSA                = 1;  // Internal ref voltage group A enabled
  adexicr_temp.TSSA                = 1;  // Temperature sensor group A enabled
  adexicr_temp.OCSAD               = 0;  // Internal ref addition/average disabled
  adexicr_temp.TSSAD               = 0;  // Temperature addition/average disabled
  R_ADC0->ADEXICR                  = *(uint16_t*)&adexicr_temp;

  adexicr_temp                     = *(T_adexicr_bits*)&R_ADC1->ADEXICR;
  adexicr_temp.OCSB                = 1;  // Internal ref voltage group B enabled
  adexicr_temp.TSSB                = 0;  // Temperature sensor group B disabled
  adexicr_temp.OCSA                = 1;  // Internal ref voltage group A enabled
  adexicr_temp.TSSA                = 1;  // Temperature sensor group A disabled
  adexicr_temp.OCSAD               = 0;  // Internal ref addition/average disabled
  adexicr_temp.TSSAD               = 0;  // Temperature addition/average disabled
  R_ADC1->ADEXICR                  = *(uint16_t*)&adexicr_temp;

  // Configure sample state registers for internal sensors
  T_adsstrt_bits adsstrt_temp      = *(T_adsstrt_bits*)&R_ADC0->ADSSTRT;
  adsstrt_temp.SST                 = 0x40;  // 64 ticks for temperature sensor
  R_ADC0->ADSSTRT                  = *(uint8_t*)&adsstrt_temp;
  R_ADC1->ADSSTRT                  = *(uint8_t*)&adsstrt_temp;

  T_adsstro_bits adsstro_temp      = *(T_adsstro_bits*)&R_ADC0->ADSSTRO;
  adsstro_temp.SST                 = 0x40;                      // 64 ticks for reference voltage
  R_ADC0->ADSSTRO                  = *(uint8_t*)&adsstro_temp;  // Enable and configure temperature sensor
  R_ADC1->ADSSTRO                  = *(uint8_t*)&adsstro_temp;  // Enable and configure temperature sensor

  T_tscr_bits tscr_temp            = *(T_tscr_bits*)&R_TSN_CTRL->TSCR;
  tscr_temp.TSEN                   = 1;                         // Enable temperature sensor
  R_TSN_CTRL->TSCR                 = *(uint32_t*)&tscr_temp;
  Wait_ms(1);                                                   // Stabilization delay

  tscr_temp        = *(T_tscr_bits*)&R_TSN_CTRL->TSCR;
  tscr_temp.TSOE   = 1;                                         // Enable temperature sensor output
  R_TSN_CTRL->TSCR = *(uint32_t*)&tscr_temp;
}

/*-----------------------------------------------------------------------------------------------------
  Configure ADC trigger sources and interrupts
  RA8M1 uses direct GPT0 to ADC connection, no ELC required

  Parameters: void

  Return: void
-----------------------------------------------------------------------------------------------------*/
static void _Adc_configure_interrupts(void)
{
  /*
   * ADSTRGR.TRSA - A/D Conversion Start Trigger Selection Register A (6-bit field: 0x00-0x3F)
   * This register selects the trigger source for starting A/D conversion.
   * RA8M1 GPT0 is directly connected to ADC modules without ELC routing.
   * ADC_TRG_GPT0_4_AB = 0x19U configures both GTADTRA and GTADTRB triggers.
   */
  R_ADC0->ADSTRGR_b.TRSA = ADC_TRG_GPT0_4_AB;
  R_ADC1->ADSTRGR_b.TRSA = ADC_TRG_GPT0_4_AB;

  NVIC_SetPriority(ADC0_SCAN_END_IRQn, 5);    // Set interrupt priority (0=highest, 15=lowest)

  R_ICU->IELSR_b[ADC0_SCAN_END_IRQn].IR = 0;  // Clear any pending interrupt flag
  NVIC_ClearPendingIRQ(ADC0_SCAN_END_IRQn);   // Clear NVIC pending interrupt
  NVIC_EnableIRQ(ADC0_SCAN_END_IRQn);         // Enable ADC0 scan end interrupt
}

/*-----------------------------------------------------------------------------------------------------
  Start ADC scanning operation

  Parameters: void

  Return: void
-----------------------------------------------------------------------------------------------------*/
void Adc_driver_start_scan(void)
{
  R_ADC0->ADCSR_b.ADST = 1;  // Start ADC0
  R_ADC1->ADCSR_b.ADST = 1;  // Start ADC1
}

/*-----------------------------------------------------------------------------------------------------
  Stop ADC scanning operation

  Parameters: void

  Return: void
-----------------------------------------------------------------------------------------------------*/
void Adc_driver_stop_scan(void)
{
  R_ADC0->ADCSR_b.ADST = 0;  // Stop ADC0
  R_ADC1->ADCSR_b.ADST = 0;  // Stop ADC1
}

/*-----------------------------------------------------------------------------------------------------
  Set ADC interrupt callback function

  Parameters: isr_callback - new callback function

  Return: void
-----------------------------------------------------------------------------------------------------*/
void Adc_driver_set_callback(T_adc_isr_callback isr_callback)
{
  __disable_interrupt();
  adc.isr_callback = isr_callback;
  __enable_interrupt();
}

/*-----------------------------------------------------------------------------------------------------
  Start calibration process for current measurement offsets

  Parameters: samples_count_per_motor - number of samples to accumulate per motor

  Return: uint32_t - TX_SUCCESS if calibration started successfully, TX_NOT_AVAILABLE if drivers are enabled
-----------------------------------------------------------------------------------------------------*/
uint32_t Adc_driver_start_calibration(uint32_t samples_count_per_motor)
{
  // Reset event flag before starting
  ULONG actual_events;
  tx_event_flags_get(&adc.adc_flags, ADC_AVERAGING_DONE, TX_OR_CLEAR, &actual_events, TX_NO_WAIT);

  TX_INTERRUPT_SAVE_AREA

  TX_DISABLE
  adc.calibration_in_progress            = true;
  adc.total_calibration_cycles_remaining = samples_count_per_motor * 2;
  adc.samples_per_motor_phase            = samples_count_per_motor;

  // Reset accumulators
  adc.smpl_i_u_m1_acc                    = 0;
  adc.smpl_i_v_m1_acc                    = 0;
  adc.smpl_i_w_m1_acc                    = 0;
  adc.smpl_i_u_m2_acc                    = 0;
  adc.smpl_i_v_m2_acc                    = 0;
  adc.smpl_i_w_m2_acc                    = 0;
  adc.smpl_v3v3_acc                      = 0;
  TX_RESTORE

  return TX_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Cancel calibration process for current measurement offsets

  Parameters: None

  Return: None
-----------------------------------------------------------------------------------------------------*/
void Adc_driver_cancel_calibration(void)
{
  TX_INTERRUPT_SAVE_AREA

  TX_DISABLE
  adc.calibration_in_progress            = false;
  adc.total_calibration_cycles_remaining = 0;
  TX_RESTORE
}

/*-----------------------------------------------------------------------------------------------------
  Process accumulated samples and convert to engineering units
  Uses filtered values for improved noise immunity and measurement accuracy

  Parameters: void

  Return: void
-----------------------------------------------------------------------------------------------------*/
void Adc_driver_process_samples(void)
{  // Process monitoring channels using filtered values and appropriate combined scale factors
  // Convert filtered values from Q16 format back to standard ADC counts
  adc.v24v_supply           = (float)(adc.filt_v24v_mon >> EMA_FILTER_SHIFT) * adc.monitor_24v_scale;  // AN004 - 24V input monitoring (filtered)
  adc.v5v_supply            = (float)(adc.filt_v5v_mon >> EMA_FILTER_SHIFT) * adc.monitor_5v_scale;    // AN104 - 5V supply monitoring (filtered)
  adc.v3v3_supply           = (float)(adc.filt_v3v3_mon >> EMA_FILTER_SHIFT) * adc.monitor_3v3_scale;  // AN105 - +3.3V reference/supply monitoring (filtered)
  // Process phase current measurements with offset correction using combined scale factors

  // Process dual-EMA filtered phase currents for control and monitoring
  // Fast filtered currents (for control applications)
  adc.i_u_motor1_fast       = ((float)(adc.filt_i_u_motor1_fast >> EMA_FILTER_SHIFT) - (float)adc.smpl_i_u_offs_m1) * adc.phase_current_scale;
  adc.i_v_motor1_fast       = ((float)(adc.filt_i_v_motor1_fast >> EMA_FILTER_SHIFT) - (float)adc.smpl_i_v_offs_m1) * adc.phase_current_scale;
  adc.i_w_motor1_fast       = ((float)(adc.filt_i_w_motor1_fast >> EMA_FILTER_SHIFT) - (float)adc.smpl_i_w_offs_m1) * adc.phase_current_scale;
  adc.i_u_motor2_fast       = ((float)(adc.filt_i_u_motor2_fast >> EMA_FILTER_SHIFT) - (float)adc.smpl_i_u_offs_m2) * adc.phase_current_scale;
  adc.i_v_motor2_fast       = ((float)(adc.filt_i_v_motor2_fast >> EMA_FILTER_SHIFT) - (float)adc.smpl_i_v_offs_m2) * adc.phase_current_scale;
  adc.i_w_motor2_fast       = ((float)(adc.filt_i_w_motor2_fast >> EMA_FILTER_SHIFT) - (float)adc.smpl_i_w_offs_m2) * adc.phase_current_scale;

  // Slow filtered currents (for monitoring/protection applications)
  adc.i_u_motor1_slow       = ((float)(adc.filt_i_u_motor1_slow >> EMA_FILTER_SHIFT) - (float)adc.smpl_i_u_offs_m1) * adc.phase_current_scale;
  adc.i_v_motor1_slow       = ((float)(adc.filt_i_v_motor1_slow >> EMA_FILTER_SHIFT) - (float)adc.smpl_i_v_offs_m1) * adc.phase_current_scale;
  adc.i_w_motor1_slow       = ((float)(adc.filt_i_w_motor1_slow >> EMA_FILTER_SHIFT) - (float)adc.smpl_i_w_offs_m1) * adc.phase_current_scale;
  adc.i_u_motor2_slow       = ((float)(adc.filt_i_u_motor2_slow >> EMA_FILTER_SHIFT) - (float)adc.smpl_i_u_offs_m2) * adc.phase_current_scale;
  adc.i_v_motor2_slow       = ((float)(adc.filt_i_v_motor2_slow >> EMA_FILTER_SHIFT) - (float)adc.smpl_i_v_offs_m2) * adc.phase_current_scale;
  adc.i_w_motor2_slow       = ((float)(adc.filt_i_w_motor2_slow >> EMA_FILTER_SHIFT) - (float)adc.smpl_i_w_offs_m2) * adc.phase_current_scale;
  // Process phase voltage measurements using filtered values and combined scale factors
  // Convert filtered values from Q16 format back to standard ADC counts
  adc.v_u_motor1            = (float)(adc.filt_v_u_motor1 >> EMA_FILTER_SHIFT) * adc.phase_voltage_scale;
  adc.v_v_motor1            = (float)(adc.filt_v_v_motor1 >> EMA_FILTER_SHIFT) * adc.phase_voltage_scale;
  adc.v_w_motor1            = (float)(adc.filt_v_w_motor1 >> EMA_FILTER_SHIFT) * adc.phase_voltage_scale;

  adc.v_u_motor2            = (float)(adc.filt_v_u_motor2 >> EMA_FILTER_SHIFT) * adc.phase_voltage_scale;
  adc.v_v_motor2            = (float)(adc.filt_v_v_motor2 >> EMA_FILTER_SHIFT) * adc.phase_voltage_scale;
  adc.v_w_motor2            = (float)(adc.filt_v_w_motor2 >> EMA_FILTER_SHIFT) * adc.phase_voltage_scale;
  // Process power supply current measurements using filtered values and combined scale factors
  // Convert filtered values from Q16 format and apply offset correction
  // Use signed arithmetic to handle negative currents properly (prevents uint16_t underflow)
  int32_t filtered_v3v3_ref = (int32_t)(adc.filt_v3v3_mon >> EMA_FILTER_SHIFT);
  adc.ipwr_motor1           = (float)((int32_t)(adc.filt_ipwr_motor1 >> EMA_FILTER_SHIFT) - filtered_v3v3_ref) * adc.power_current_scale;
  adc.ipwr_motor2           = (float)((int32_t)(adc.filt_ipwr_motor2 >> EMA_FILTER_SHIFT) - filtered_v3v3_ref) * adc.power_current_scale;
  // Process sensor measurements using filtered values
  // Convert filtered values from Q16 format back to standard ADC counts
  adc.speed_motor1          = (float)(adc.filt_speed_motor1 >> EMA_FILTER_SHIFT) * adc.adc_scale;
  adc.speed_motor2          = (float)(adc.filt_speed_motor2 >> EMA_FILTER_SHIFT) * adc.adc_scale;
  adc.pos_motor1            = (float)(adc.filt_pos_motor1 >> EMA_FILTER_SHIFT) * adc.adc_scale;
  adc.pos_motor2            = (float)(adc.filt_pos_motor2 >> EMA_FILTER_SHIFT) * adc.adc_scale;

/* 1 count → volts  */
#define TSN_K_V_PER_CNT    (ADC_REF_VOLTAGE / ADC_RESOLUTION) /* ≈ 0.00081103 V */

/* Convert voltage delta to °C.
   Slope in the HW manual is negative (sensor voltage falls as T rises): */
#define TSN_SLOPE_V_PER_C  (-1.0f * BSP_FEATURE_ADC_TSN_SLOPE * 1e-6f) /* = −0.004 V/°C */

/* Combined gain: counts → °C (≈ 0.00081103 / −0.004 = −0.2027575) */
#define TSN_GAIN_C_PER_CNT (TSN_K_V_PER_CNT / TSN_SLOPE_V_PER_C)

  uint16_t vs_code                = (uint16_t)(adc.filt_cpu_temp >> EMA_FILTER_SHIFT);  // live 12-bit sample
  uint16_t cal125                 = R_TSN_CAL->TSCDR_b.TSCDR;                           // factory code @ 125 °C
  adc.cpu_temp                    = ((float)vs_code - (float)cal125) * TSN_GAIN_C_PER_CNT - 45.0f;  // Установил опытным путем. Формуле из даташита не соответствеут

  // Process thermistor temperatures using filtered values for improved stability
  // Convert filtered values from Q16 format back to standard ADC counts
  uint16_t filtered_thermistor_m1 = (uint16_t)(adc.filt_thermistor_m1 >> EMA_FILTER_SHIFT);
  uint16_t filtered_thermistor_m2 = (uint16_t)(adc.filt_thermistor_m2 >> EMA_FILTER_SHIFT);
  adc.temp_motor1                 = Adc_driver_calculate_thermistor_temperature(filtered_thermistor_m1);
  adc.temp_motor2                 = Adc_driver_calculate_thermistor_temperature(filtered_thermistor_m2);
}

/*-----------------------------------------------------------------------------------------------------
  Calculate scaling factors for ADC measurements based on hardware configuration
  Sets up proper scaling for current, voltage and temperature measurements

  Parameters: none

  Return: void
-----------------------------------------------------------------------------------------------------*/
void Adc_driver_calculate_scaling_factors(void)
{
  // Base ADC scale factor: 12-bit ADC with 3.3V reference
  float adc_to_voltage         = ADC_REF_VOLTAGE / ADC_RESOLUTION;  // Power supply current scale factor for shunt resistor with INA186A2IDDFR amplifier
  // INA186A2IDDFR has gain of 50V/V
  // Current = V_adc / (R_shunt * G_amp)
  float power_shunt_resistance = wvar.input_shunt_resistor;  // Get shunt value from parameters
  float ina186_gain            = INA186A2_GAIN;              // INA186A2IDDFR gain

  // Use default value if parameter is not set or invalid
  if (power_shunt_resistance <= 0.0f)
  {
    power_shunt_resistance = 0.010f;  // Default 10mOhm shunt resistor
  }

  adc.current_scale            = adc_to_voltage / (power_shunt_resistance * ina186_gain);

  // Power supply current scale factor (same as current_scale for power measurements)
  adc.ipwr_scale               = adc.current_scale;
  // Phase current scale factor for shunt resistor with TMC6200-TA amplifier
  // TMC6200-TA driver has gain of 20x as configured in Motdrv_tmc6200_Initialize()
  // Current = V_adc / (R_shunt * G_tmc6200)
  float phase_shunt_resistance = wvar.shunt_resistor;  // Get correct phase shunt value
  float tmc6200_gain           = TMC6200_GAIN;         // TMC6200 amplifier gain
  float phase_current_scale_factor;

  // Use default value if parameter is not set or invalid
  if (phase_shunt_resistance <= 0.0f)
  {
    phase_shunt_resistance = 0.010f;                                                             // Default 10mOhm shunt resistor
  }

  phase_current_scale_factor        = adc_to_voltage / (phase_shunt_resistance * tmc6200_gain);  // Phase voltage scale factor with voltage divider compensation
  // Hardware: 100kΩ upper resistor, 10kΩ lower resistor (11:1 divider ratio)
  // V_real = V_adc * (R_upper + R_lower) / R_lower = V_adc * (10k + 5.1k) / 5.1k
  float phase_voltage_divider_ratio = PHASE_VOLTAGE_DIVIDER_RATIO;                   // Calculated from resistor values
  float phase_voltage_scale_factor  = adc_to_voltage * phase_voltage_divider_ratio;  // Voltage monitoring scale factors for different inputs
  // AN004 - System +24V input monitoring through differential amplifier
  // Differential amplifier reduces voltage by factor of 0.082
  // V_real = V_adc / 0.082 = V_adc * 12.195
  float v24_divider_ratio           = V24V_DIVIDER_RATIO;  // Calculated from differential amplifier gain

  // AN104 - +5V supply monitoring through voltage divider: 10kΩ upper, 5.1kΩ lower
  // V_real = V_adc * (R_upper + R_lower) / R_lower = V_adc * (10k + 5.1k) / 5.1k
  float v5v_divider_ratio           = (V5V_UPPER_RESISTOR + V5V_LOWER_RESISTOR) / V5V_LOWER_RESISTOR;

  // AN105 - +3.3V reference/supply monitoring through voltage divider: 100kΩ upper, 100kΩ lower
  // V_real = V_adc * (R_upper + R_lower) / R_lower = V_adc * (100k + 100k) / 100k = V_adc * 2
  float v3v3_divider_ratio          = V3V3_DIVIDER_RATIO;  // Calculated from resistor values

  // Store individual scale factors for compatibility
  adc.voltage_scale                 = phase_voltage_scale_factor;  // Updated with divider ratio
  // Combined scale factor for direct voltage measurements (monitoring channels)
  adc.adc_scale                     = adc_to_voltage;

  // Temperature scale factor is no longer needed - CPU temperature uses factory calibration
  // RA8M1 uses one-point calibration method with factory CAL125 value from TSCDR register
  // and precise slope value from BSP_FEATURE_ADC_TSN_SLOPE (4000 μV/°C)
  adc.temp_scale                    = 1.0f;  // Not used for calibrated temperature calculation

  // Calculate combined scale factors to eliminate double multiplication
  adc.phase_current_scale           = phase_current_scale_factor;  // Combined ADC + TMC6200 amplifier for phase currents
  adc.power_current_scale           = adc.ipwr_scale;              // Combined ADC + INA186 amplifier for power currents
  adc.phase_voltage_scale           = phase_voltage_scale_factor;  // Combined ADC + 11:1 divider for phase voltages

  // Calculate separate monitoring voltage scale factors for each channel
  adc.monitor_24v_scale             = adc_to_voltage * v24_divider_ratio;   // Combined ADC + 24V divider scale (AN004)
  adc.monitor_5v_scale              = adc_to_voltage * v5v_divider_ratio;   // Combined ADC + 5V divider scale (AN104)
  adc.monitor_3v3_scale             = adc_to_voltage * v3v3_divider_ratio;  // Combined ADC + 3.3V divider scale (AN105)
}

/*-----------------------------------------------------------------------------------------------------
  Initialize ADC driver for MC80 project
  Sets up ADC modules for synchronized sampling of all analog inputs

  Parameters: isr_callback - callback function for ADC interrupt

  Return: void
-----------------------------------------------------------------------------------------------------*/
void Adc_driver_init(T_adc_isr_callback isr_callback)
{
  // Initialize control structure
  memset(&adc, 0, sizeof(adc));

  adc.filters_initialized = false;  // Initialize the flag to false
  // Initialize multiplexer control variables
  adc.active_motor        = ADC_MOTOR_1;
  adc.active_phase        = ADC_PHASE_U;

  // Set initial multiplexer signals
  ADC_SELECT_MOTOR1();   // P413 = 0 - Select Motor 1 initially
  ADC_SELECT_PHASE_U();  // P614:P613 = 00 - Select U phase initially
  // Calculate and set proper ADC scaling factors before initializing ADC
  Adc_driver_calculate_scaling_factors();

  // Initialize EMA coefficients with default PWM frequency
  Adc_driver_set_pwm_frequency(g_adc_pwm_frequency);

  // Initialize hardware registers
  _Adc_init_registers();

  // Configure ADC channels
  _Adc_configure_channels();

  // Configure interrupts and triggers
  _Adc_configure_interrupts();

  // Initialize event flags
  tx_event_flags_create(&adc.adc_flags, "adc_flags");

  // Set callback
  adc.isr_callback = isr_callback;
}

/*-----------------------------------------------------------------------------------------------------
  Calculate temperature from thermistor ADC reading using Steinhart-Hart equation
  For NCP21XV103J03RA thermistor with 10kOhm pull-up resistor

  Parameters: adc_value - 12-bit ADC reading (0-4095)

  Return: float - temperature in Celsius degrees
-----------------------------------------------------------------------------------------------------*/
float Adc_driver_calculate_thermistor_temperature(uint16_t adc_value)
{
  // Convert ADC reading to voltage
  float v_adc = (float)adc_value * adc.adc_scale;

  // Handle boundary conditions
  if (v_adc <= THERMISTOR_MIN_VOLTAGE)
  {
    return THERMISTOR_MAX_TEMP;  // Maximum expected temperature when thermistor resistance is very low
  }
  if (v_adc >= (THERMISTOR_SUPPLY_V - THERMISTOR_MIN_VOLTAGE))
  {
    return THERMISTOR_MIN_TEMP;  // Minimum expected temperature when thermistor resistance is very high
  }

  // Calculate thermistor resistance from voltage divider
  // V_adc = V_supply * R_thermistor / (R_pullup + R_thermistor)
  // Solving for R_thermistor: R_thermistor = (V_adc * R_pullup) / (V_supply - V_adc)
  float r_thermistor = (v_adc * THERMISTOR_PULLUP_R) / (THERMISTOR_SUPPLY_V - v_adc);

  // Apply Steinhart-Hart equation (simplified beta formula)
  // 1/T = 1/T0 + (1/B) * ln(R/R0)
  // T = 1 / (1/T0 + (1/B) * ln(R/R0))
  float ln_ratio     = logf(r_thermistor / THERMISTOR_R25);
  float temp_kelvin  = 1.0f / ((1.0f / THERMISTOR_T25) + (ln_ratio / THERMISTOR_B_CONSTANT));

  // Convert from Kelvin to Celsius
  float temp_celsius = temp_kelvin - KELVIN_TO_CELSIUS;

  // Limit temperature to reasonable range
  if (temp_celsius < THERMISTOR_MIN_TEMP)
  {
    temp_celsius = THERMISTOR_MIN_TEMP;
  }
  if (temp_celsius > THERMISTOR_MAX_TEMP)
  {
    temp_celsius = THERMISTOR_MAX_TEMP;
  }
  return temp_celsius;
}

/*-----------------------------------------------------------------------------------------------------
  Get filtered position sensor value as 16-bit ADC counts for specified motor.
  Converts from internal Q16 format to standard 16-bit ADC value.

  Parameters:
    motor_id - Motor ID (1 = motor 1, 2 = motor 2)

  Return:
    Position sensor ADC value (0-4095 for 12-bit ADC), 0 if motor has no position sensor
-----------------------------------------------------------------------------------------------------*/
uint16_t Adc_driver_get_position_sensor_value(uint8_t motor_id)
{
  switch (motor_id)
  {
    case 1:      // Motor 1 position sensor
      return (uint16_t)(adc.filt_pos_motor1 >> EMA_FILTER_SHIFT);
    case 2:      // Motor 2 position sensor
      return (uint16_t)(adc.filt_pos_motor2 >> EMA_FILTER_SHIFT);
    default:
      return 0;  // Invalid motor ID or no position sensor
  }
}

/*-----------------------------------------------------------------------------------------------------
  Get DC motor current for two-phase operation.
  Motor connections to physical drivers:
  - Motor 1 (Traction): U1-V1 phases (MD1 driver → motor1 ADC channels)
  - Motor 2 (Motor 2): V1-W1 phases (MD1 driver → motor1 ADC channels)
  - Motor 3 ( Motor 3): U2-V2 phases (MD2 driver → motor2 ADC channels)
  - Motor 4 ( Motor 2): V2-W2 phases (MD2 driver → motor2 ADC channels)

  Important: V1 phase is shared between Motor 1 and Motor 2, V2 phase is shared between Motor 3 and Motor 4.
  To get accurate individual motor current, only use phases that are exclusive to each motor:
  - Motor 1: U1 phase only (V1 is shared with Motor 2)
  - Motor 2: W1 phase only (V1 is shared with Motor 1)
  - Motor 3: U2 phase only (V2 is shared with Motor 4)
  - Motor 4: W2 phase only (V2 is shared with Motor 3)

  Parameters:
    motor_id - Motor number (1-4)

  Return:
    DC motor current in Amperes from exclusive phase, 0.0f if invalid motor ID
-----------------------------------------------------------------------------------------------------*/
float Adc_driver_get_dc_motor_current(uint8_t motor_id)
{
  float motor_current = 0.0f;

  switch (motor_id)
  {
    case 1:                                        // Motor 1 (Traction) - use only U1 phase (exclusive to Motor 1)
    {
      motor_current = fabsf(adc.i_u_motor1_slow);  // U1 phase current only (slow filtered for monitoring)
    }
    break;
    case 2:                                        // Motor 2 (Motor 2) - use only W1 phase (exclusive to Motor 2)
    {
      motor_current = fabsf(adc.i_w_motor1_slow);  // W1 phase current only (slow filtered for monitoring)
    }
    break;
    case 3:                                        // Motor 3 ( Motor 3) - use only U2 phase (exclusive to Motor 3)
    {
      motor_current = fabsf(adc.i_u_motor2_slow);  // U2 phase current only (slow filtered for monitoring)
    }
    break;
    case 4:                                        // Motor 4 ( Motor 2) - use only W2 phase (exclusive to Motor 4)
    {
      motor_current = fabsf(adc.i_w_motor2_slow);  // W2 phase current only (slow filtered for monitoring)
    }
    break;
    default:
      motor_current = 0.0f;  // Invalid motor ID
      break;
  }

  return motor_current;
}

/*-----------------------------------------------------------------------------------------------------
  Set PWM frequency for dynamic EMA coefficient calculation
  Must be called after PWM frequency is determined to synchronize ADC sampling rates

  Parameters:
    pwm_freq - PWM frequency in Hz

  Return:
    none
-----------------------------------------------------------------------------------------------------*/
void Adc_driver_set_pwm_frequency(uint32_t pwm_freq)
{
  g_adc_pwm_frequency          = pwm_freq;  // Store PWM frequency for dynamic calculations

  // Calculate dynamic sampling frequencies based on PWM frequency
  uint32_t fs_direct           = g_adc_pwm_frequency;      // Direct ADC channels
  uint32_t fs_2ch              = g_adc_pwm_frequency / 2;  // 2-channel multiplexed
  uint32_t fs_3ch              = g_adc_pwm_frequency / 3;  // 3-channel multiplexed

  // Calculate dynamic EMA coefficients
  // EMA coefficient calculation : α = 2π * fc / fs * scale
  // Using integer approximation: 2π ≈ 6.283 ≈ 6283/1000
  // EMA_ALPHA_CALC(fc, fs) = ((6283UL * (fc) * EMA_FILTER_SCALE) / (1000UL * (fs)))
  g_ema_alpha_10hz_direct      = (uint16_t)((6283UL * FC_CONTROL_10HZ * EMA_FILTER_SCALE) / (1000UL * fs_direct));
  g_ema_alpha_1hz_direct       = (uint16_t)((6283UL * FC_MONITOR_1HZ * EMA_FILTER_SCALE) / (1000UL * fs_direct));

  g_ema_alpha_10hz_2ch         = (uint16_t)((6283UL * FC_CONTROL_10HZ * EMA_FILTER_SCALE) / (1000UL * fs_2ch));
  g_ema_alpha_1hz_2ch          = (uint16_t)((6283UL * FC_MONITOR_1HZ * EMA_FILTER_SCALE) / (1000UL * fs_2ch));

  g_ema_alpha_10hz_3ch         = (uint16_t)((6283UL * FC_CONTROL_10HZ * EMA_FILTER_SCALE) / (1000UL * fs_3ch));
  g_ema_alpha_1hz_3ch          = (uint16_t)((6283UL * FC_MONITOR_1HZ * EMA_FILTER_SCALE) / (1000UL * fs_3ch));

  // Calculate dual-EMA coefficients for current filtering (2-channel multiplexed rate)
  g_ema_alpha_current_fast_2ch = (uint16_t)((6283UL * FC_CURRENT_FAST * EMA_FILTER_SCALE) / (1000UL * fs_2ch));
  g_ema_alpha_current_slow_2ch = (uint16_t)((6283UL * FC_CURRENT_SLOW * EMA_FILTER_SCALE) / (1000UL * fs_2ch));
}

/*-----------------------------------------------------------------------------------------------------
  Get fast filtered motor phase current for real-time control applications
  Fast filter provides ~1.5kHz cutoff frequency for responsive control with noise reduction

  Parameters:
    motor_id - Motor ID (1 = motor 1, 2 = motor 2)
    phase - Phase ID (0 = U phase, 1 = V phase, 2 = W phase)

  Return:
    Fast filtered phase current in Amperes with offset correction, 0.0f if invalid parameters
-----------------------------------------------------------------------------------------------------*/
float Adc_driver_get_motor_current_fast(uint8_t motor_id, uint8_t phase)
{
  // Return already processed filtered current values from process_samples function
  switch (motor_id)
  {
    case 1:  // Motor 1
      switch (phase)
      {
        case ADC_PHASE_U:
          return adc.i_u_motor1_fast;
        case ADC_PHASE_V:
          return adc.i_v_motor1_fast;
        case ADC_PHASE_W:
          return adc.i_w_motor1_fast;
        default:
          return 0.0f;  // Invalid phase
      }
      break;
    case 2:             // Motor 2
      switch (phase)
      {
        case ADC_PHASE_U:
          return adc.i_u_motor2_fast;
        case ADC_PHASE_V:
          return adc.i_v_motor2_fast;
        case ADC_PHASE_W:
          return adc.i_w_motor2_fast;
        default:
          return 0.0f;  // Invalid phase
      }
      break;
    default:
      return 0.0f;  // Invalid motor ID
  }
}

/*-----------------------------------------------------------------------------------------------------
  Get slow filtered motor phase current for monitoring and protection applications
  Slow filter provides ~300Hz cutoff frequency for stable monitoring with excellent noise rejection

  Parameters:
    motor_id - Motor ID (1 = motor 1, 2 = motor 2)
    phase - Phase ID (0 = U phase, 1 = V phase, 2 = W phase)

  Return:
    Slow filtered phase current in Amperes with offset correction, 0.0f if invalid parameters
-----------------------------------------------------------------------------------------------------*/
float Adc_driver_get_motor_current_slow(uint8_t motor_id, uint8_t phase)
{
  // Return already processed filtered current values from process_samples function
  switch (motor_id)
  {
    case 1:  // Motor 1
      switch (phase)
      {
        case ADC_PHASE_U:
          return adc.i_u_motor1_slow;
        case ADC_PHASE_V:
          return adc.i_v_motor1_slow;
        case ADC_PHASE_W:
          return adc.i_w_motor1_slow;
        default:
          return 0.0f;  // Invalid phase
      }
      break;
    case 2:             // Motor 2
      switch (phase)
      {
        case ADC_PHASE_U:
          return adc.i_u_motor2_slow;
        case ADC_PHASE_V:
          return adc.i_v_motor2_slow;
        case ADC_PHASE_W:
          return adc.i_w_motor2_slow;
        default:
          return 0.0f;  // Invalid phase
      }
      break;
    default:
      return 0.0f;  // Invalid motor ID
  }
}

/*-----------------------------------------------------------------------------------------------------
  Get system 24V supply voltage

  Parameters:
    None

  Return:
    24V supply voltage in Volts
-----------------------------------------------------------------------------------------------------*/
float Adc_driver_get_supply_voltage_24v(void)
{
  return adc.v24v_supply;
}
