#ifndef SYSTEM_ERROR_FLAGS_H
#define SYSTEM_ERROR_FLAGS_H

// System error flags structure is now defined in CAN/CAN_protocol.h
// as it's an integral part of the CAN communication protocol

// Global error flags structure declaration
extern volatile T_system_error_flags g_system_error_flags;

// System error flag management functions
void     App_init_error_flags(void);
uint32_t App_get_error_flags(void);
bool     App_has_any_errors(void);
void     App_format_error_list(char *buffer, uint32_t buffer_size);
void     App_set_motor_overcurrent_flag(uint8_t motor_number, float current_value, float threshold_value);
void     App_set_driver_overtemperature_flag(uint8_t driver_number, float temperature_value, float threshold_value);
void     App_set_tmc6200_driver_fault_flag(uint8_t driver_number);
void     App_update_tmc6200_detailed_errors(uint8_t driver_number, uint32_t gstat_value);
void     App_set_power_supply_fault_flag(void);
void     App_set_can_bus_error_flag(void);
void     App_clear_can_bus_error_flag(void);
uint8_t  App_is_can_bus_error_set(void);
void     App_set_cpu_overtemperature_flag(float temperature_value, float threshold_value);
void     App_set_emergency_stop_flag(void);
uint8_t  App_is_emergency_stop_active(void);
void     App_clear_all_error_flags(void);

#endif  // SYSTEM_ERROR_FLAGS_H
