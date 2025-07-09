#include "App.h"
#include "freemaster_cfg.h"
#include "freemaster.h"
#include "freemaster_tsa.h"



FMSTR_TSA_TABLE_BEGIN(app_vars)
FMSTR_TSA_RW_VAR(g_cpu_usage                            ,FMSTR_TSA_UINT32)
FMSTR_TSA_RW_VAR(g_aver_cpu_usage                       ,FMSTR_TSA_UINT32)

// CAN command processing status
FMSTR_TSA_RW_VAR(g_can_command_processing_enabled       ,FMSTR_TSA_UINT8)

// Motor driver enable states for monitoring
FMSTR_TSA_RW_VAR(g_drv1_en_state                        ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(g_drv2_en_state                        ,FMSTR_TSA_UINT8)

// System error flags for monitoring (exported as uint32_t since it's a bitfield structure)
FMSTR_TSA_RW_VAR(g_system_error_flags                   ,FMSTR_TSA_UINT32)

// TMC6200 driver error monitoring
FMSTR_TSA_RW_VAR(g_tmc6200_driver1_error_code           ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(g_tmc6200_driver2_error_code           ,FMSTR_TSA_UINT8)

// TMC6200 driver monitoring status - Driver 1
FMSTR_TSA_RW_VAR(g_tmc6200_monitoring.driver[0].init_error_code         ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(g_tmc6200_monitoring.driver[0].last_gstat_error        ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(g_tmc6200_monitoring.driver[0].last_gstat_value        ,FMSTR_TSA_UINT32)
FMSTR_TSA_RW_VAR(g_tmc6200_monitoring.driver[0].error_count             ,FMSTR_TSA_UINT32)
FMSTR_TSA_RW_VAR(g_tmc6200_monitoring.driver[0].communication_failures  ,FMSTR_TSA_UINT32)
FMSTR_TSA_RW_VAR(g_tmc6200_monitoring.driver[0].driver_operational      ,FMSTR_TSA_UINT8)

// TMC6200 driver monitoring status - Driver 2
FMSTR_TSA_RW_VAR(g_tmc6200_monitoring.driver[1].init_error_code         ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(g_tmc6200_monitoring.driver[1].last_gstat_error        ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(g_tmc6200_monitoring.driver[1].last_gstat_value        ,FMSTR_TSA_UINT32)
FMSTR_TSA_RW_VAR(g_tmc6200_monitoring.driver[1].error_count             ,FMSTR_TSA_UINT32)
FMSTR_TSA_RW_VAR(g_tmc6200_monitoring.driver[1].communication_failures  ,FMSTR_TSA_UINT32)
FMSTR_TSA_RW_VAR(g_tmc6200_monitoring.driver[1].driver_operational      ,FMSTR_TSA_UINT8)

// TMC6200 monitoring global status
FMSTR_TSA_RW_VAR(g_tmc6200_monitoring.poll_interval_ms                  ,FMSTR_TSA_UINT32)
FMSTR_TSA_RW_VAR(g_tmc6200_monitoring.monitoring_active                 ,FMSTR_TSA_UINT8)

// ADC samples - raw values
FMSTR_TSA_RW_VAR(adc.smpl_i_u_motor1                    ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(adc.smpl_i_v_motor1                    ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(adc.smpl_i_w_motor1                    ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(adc.smpl_i_u_motor2                    ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(adc.smpl_i_v_motor2                    ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(adc.smpl_i_w_motor2                    ,FMSTR_TSA_UINT16)

FMSTR_TSA_RW_VAR(adc.smpl_v_u_motor1                    ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(adc.smpl_v_v_motor1                    ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(adc.smpl_v_w_motor1                    ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(adc.smpl_v_u_motor2                    ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(adc.smpl_v_v_motor2                    ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(adc.smpl_v_w_motor2                    ,FMSTR_TSA_UINT16)

FMSTR_TSA_RW_VAR(adc.smpl_ipwr_motor1                   ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(adc.smpl_ipwr_motor2                   ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(adc.smpl_speed_motor1                  ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(adc.smpl_speed_motor2                  ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(adc.smpl_pos_motor1                    ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(adc.smpl_pos_motor2                    ,FMSTR_TSA_UINT16)

FMSTR_TSA_RW_VAR(adc.smpl_v24v_mon                      ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(adc.smpl_v5v_mon                       ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(adc.smpl_v3v3_mon                      ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(adc.smpl_thermistor_m1                 ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(adc.smpl_thermistor_m2                 ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(adc.smpl_cpu_temp                      ,FMSTR_TSA_UINT16)

// ADC offset values
FMSTR_TSA_RW_VAR(adc.smpl_i_u_offs_m1                   ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(adc.smpl_i_v_offs_m1                   ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(adc.smpl_i_w_offs_m1                   ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(adc.smpl_i_u_offs_m2                   ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(adc.smpl_i_v_offs_m2                   ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(adc.smpl_i_w_offs_m2                   ,FMSTR_TSA_UINT16)

// ADC processed values (engineering units) - voltages and other measurements
FMSTR_TSA_RW_VAR(adc.v_u_motor1                         ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(adc.v_v_motor1                         ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(adc.v_w_motor1                         ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(adc.v_u_motor2                         ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(adc.v_v_motor2                         ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(adc.v_w_motor2                         ,FMSTR_TSA_FLOAT)

FMSTR_TSA_RW_VAR(adc.ipwr_motor1                        ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(adc.ipwr_motor2                        ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(adc.speed_motor1                       ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(adc.speed_motor2                       ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(adc.pos_motor1                         ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(adc.pos_motor2                         ,FMSTR_TSA_FLOAT)

FMSTR_TSA_RW_VAR(adc.v24v_supply                        ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(adc.v5v_supply                         ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(adc.v3v3_supply                        ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(adc.temp_motor1                        ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(adc.temp_motor2                        ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(adc.cpu_temp                           ,FMSTR_TSA_FLOAT)

// ADC scale factors
FMSTR_TSA_RW_VAR(adc.adc_scale                          ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(adc.phase_current_scale                ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(adc.power_current_scale                ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(adc.phase_voltage_scale                ,FMSTR_TSA_FLOAT)

// Multiplexer control
FMSTR_TSA_RW_VAR(adc.active_motor                       ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(adc.active_phase                       ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(adc.scan_cycle_counter                 ,FMSTR_TSA_UINT32)

// System configuration parameters
FMSTR_TSA_RW_VAR(wvar.display_orientation               ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(wvar.en_freemaster                     ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(wvar.en_log_to_freemaster              ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(wvar.enable_log                        ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(wvar.en_log_to_file                    ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(wvar.en_compress_settins               ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(wvar.en_formated_settings              ,FMSTR_TSA_UINT8)

// Driver IC configuration
FMSTR_TSA_RW_VAR(wvar.pwm_frequency                     ,FMSTR_TSA_UINT32)
FMSTR_TSA_RW_VAR(wvar.usb_mode                          ,FMSTR_TSA_UINT32)
FMSTR_TSA_RW_VAR(wvar.short_vs_det_level                ,FMSTR_TSA_UINT32)
FMSTR_TSA_RW_VAR(wvar.short_gnd_det_level               ,FMSTR_TSA_UINT32)
FMSTR_TSA_RW_VAR(wvar.short_det_spike_filter            ,FMSTR_TSA_UINT32)
FMSTR_TSA_RW_VAR(wvar.short_det_delay_param             ,FMSTR_TSA_UINT32)
FMSTR_TSA_RW_VAR(wvar.enable_short_to_gnd_prot          ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(wvar.enable_short_to_vs_prot           ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(wvar.gate_driver_current_param         ,FMSTR_TSA_UINT32)
FMSTR_TSA_RW_VAR(wvar.shunt_resistor                    ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(wvar.input_shunt_resistor              ,FMSTR_TSA_FLOAT)

// Motor 1 parameters
FMSTR_TSA_RW_VAR(wvar.motor_1_max_pwm_percent           ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(wvar.motor_1_direction_invert          ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(wvar.motor_1_accel_time_ms             ,FMSTR_TSA_UINT32)
FMSTR_TSA_RW_VAR(wvar.motor_1_decel_time_ms             ,FMSTR_TSA_UINT32)
FMSTR_TSA_RW_VAR(wvar.motor_1_algorithm                 ,FMSTR_TSA_UINT8)

// Motor 2 parameters
FMSTR_TSA_RW_VAR(wvar.motor_2_max_pwm_percent           ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(wvar.motor_2_direction_invert          ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(wvar.motor_2_accel_time_ms             ,FMSTR_TSA_UINT32)
FMSTR_TSA_RW_VAR(wvar.motor_2_decel_time_ms             ,FMSTR_TSA_UINT32)
FMSTR_TSA_RW_VAR(wvar.motor_2_algorithm                 ,FMSTR_TSA_UINT8)

// Motor 3 parameters
FMSTR_TSA_RW_VAR(wvar.motor_3_max_pwm_percent           ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(wvar.motor_3_direction_invert          ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(wvar.motor_3_accel_time_ms             ,FMSTR_TSA_UINT32)
FMSTR_TSA_RW_VAR(wvar.motor_3_decel_time_ms             ,FMSTR_TSA_UINT32)
FMSTR_TSA_RW_VAR(wvar.motor_3_algorithm                 ,FMSTR_TSA_UINT8)

// Motor 4 parameters
FMSTR_TSA_RW_VAR(wvar.motor_4_max_pwm_percent           ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(wvar.motor_4_direction_invert          ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(wvar.motor_4_accel_time_ms             ,FMSTR_TSA_UINT32)
FMSTR_TSA_RW_VAR(wvar.motor_4_decel_time_ms             ,FMSTR_TSA_UINT32)
FMSTR_TSA_RW_VAR(wvar.motor_4_algorithm                 ,FMSTR_TSA_UINT8)

// Dual-EMA filtered current values (fast filter for control)
FMSTR_TSA_RW_VAR(adc.i_u_motor1_fast                    ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(adc.i_v_motor1_fast                    ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(adc.i_w_motor1_fast                    ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(adc.i_u_motor2_fast                    ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(adc.i_v_motor2_fast                    ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(adc.i_w_motor2_fast                    ,FMSTR_TSA_FLOAT)

// Dual-EMA filtered current values (slow filter for monitoring)
FMSTR_TSA_RW_VAR(adc.i_u_motor1_slow                    ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(adc.i_v_motor1_slow                    ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(adc.i_w_motor1_slow                    ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(adc.i_u_motor2_slow                    ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(adc.i_v_motor2_slow                    ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(adc.i_w_motor2_slow                    ,FMSTR_TSA_FLOAT)

// Dual-EMA filter coefficients for current filtering
FMSTR_TSA_RW_VAR(g_ema_alpha_current_fast_2ch           ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(g_ema_alpha_current_slow_2ch           ,FMSTR_TSA_UINT16)

// Maximum current tracking - individual variables for easy FreeMaster access
// Motor 1 (Traction) max currents
FMSTR_TSA_RW_VAR(g_max_current_motor1_accel             ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(g_max_current_motor1_run               ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(g_max_current_motor1_brake             ,FMSTR_TSA_FLOAT)

// Motor 2 ( Motor 2) max currents
FMSTR_TSA_RW_VAR(g_max_current_motor2_accel             ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(g_max_current_motor2_run               ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(g_max_current_motor2_brake             ,FMSTR_TSA_FLOAT)

// Motor 3 ( Motor 3) max currents
FMSTR_TSA_RW_VAR(g_max_current_motor3_accel             ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(g_max_current_motor3_run               ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(g_max_current_motor3_brake             ,FMSTR_TSA_FLOAT)

// Motor 4 ( Motor 2) max currents
FMSTR_TSA_RW_VAR(g_max_current_motor4_accel             ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(g_max_current_motor4_run               ,FMSTR_TSA_FLOAT)
FMSTR_TSA_RW_VAR(g_max_current_motor4_brake             ,FMSTR_TSA_FLOAT)

// Motor states array - all fields for all 4 motors accessible for monitoring
// Motor 1 (Traction) state
FMSTR_TSA_RW_VAR(g_motor_states[0].direction            ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(g_motor_states[0].pwm_level            ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(g_motor_states[0].enabled              ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(g_motor_states[0].soft_start_state     ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(g_motor_states[0].target_pwm           ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(g_motor_states[0].original_target_pwm  ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(g_motor_states[0].current_pwm_x100     ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(g_motor_states[0].step_counter         ,FMSTR_TSA_UINT32)
FMSTR_TSA_RW_VAR(g_motor_states[0].pwm_step_size_x100   ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(g_motor_states[0].target_direction     ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(g_motor_states[0].soft_start_initialized ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(g_motor_states[0].conflict_detected    ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(g_motor_states[0].run_phase_start_time ,FMSTR_TSA_UINT32)

// Motor 2 ( Motor 2) state
FMSTR_TSA_RW_VAR(g_motor_states[1].direction            ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(g_motor_states[1].pwm_level            ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(g_motor_states[1].enabled              ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(g_motor_states[1].soft_start_state     ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(g_motor_states[1].target_pwm           ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(g_motor_states[1].original_target_pwm  ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(g_motor_states[1].current_pwm_x100     ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(g_motor_states[1].step_counter         ,FMSTR_TSA_UINT32)
FMSTR_TSA_RW_VAR(g_motor_states[1].pwm_step_size_x100   ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(g_motor_states[1].target_direction     ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(g_motor_states[1].soft_start_initialized ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(g_motor_states[1].conflict_detected    ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(g_motor_states[1].run_phase_start_time ,FMSTR_TSA_UINT32)

// Motor 3 ( Motor 3) state
FMSTR_TSA_RW_VAR(g_motor_states[2].direction            ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(g_motor_states[2].pwm_level            ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(g_motor_states[2].enabled              ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(g_motor_states[2].soft_start_state     ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(g_motor_states[2].target_pwm           ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(g_motor_states[2].original_target_pwm  ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(g_motor_states[2].current_pwm_x100     ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(g_motor_states[2].step_counter         ,FMSTR_TSA_UINT32)
FMSTR_TSA_RW_VAR(g_motor_states[2].pwm_step_size_x100   ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(g_motor_states[2].target_direction     ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(g_motor_states[2].soft_start_initialized ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(g_motor_states[2].conflict_detected    ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(g_motor_states[2].run_phase_start_time ,FMSTR_TSA_UINT32)

// Motor 4 ( Motor 2) state
FMSTR_TSA_RW_VAR(g_motor_states[3].direction            ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(g_motor_states[3].pwm_level            ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(g_motor_states[3].enabled              ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(g_motor_states[3].soft_start_state     ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(g_motor_states[3].target_pwm           ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(g_motor_states[3].original_target_pwm  ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(g_motor_states[3].current_pwm_x100     ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(g_motor_states[3].step_counter         ,FMSTR_TSA_UINT32)
FMSTR_TSA_RW_VAR(g_motor_states[3].pwm_step_size_x100   ,FMSTR_TSA_UINT16)
FMSTR_TSA_RW_VAR(g_motor_states[3].target_direction     ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(g_motor_states[3].soft_start_initialized ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(g_motor_states[3].conflict_detected    ,FMSTR_TSA_UINT8)
FMSTR_TSA_RW_VAR(g_motor_states[3].run_phase_start_time ,FMSTR_TSA_UINT32)
FMSTR_TSA_TABLE_END();


FMSTR_TSA_TABLE_LIST_BEGIN()

FMSTR_TSA_TABLE(app_vars)

FMSTR_TSA_TABLE_LIST_END()
