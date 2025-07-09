#include "App.h"

// Global system error flags structure - volatile to ensure thread-safe access
volatile T_system_error_flags g_system_error_flags = { 0 };

/*-----------------------------------------------------------------------------------------------------
  Initialize system error flags with default startup values

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void App_init_error_flags(void)
{
  TX_INTERRUPT_SAVE_AREA

  TX_DISABLE
  // Initialize all flags to 0
  *(uint32_t*)&g_system_error_flags = 0;

  // Set CAN error flag to 1 by default (no communication at startup)
  g_system_error_flags.can_bus_error = 1;
  TX_RESTORE

  APPLOG("System error flags initialized - CAN communication error set by default");
}

/*-----------------------------------------------------------------------------------------------------
  Get current system error flags as uint32_t for CAN protocol reporting

  Parameters:
    None

  Return:
    Current error flags as 32-bit value
-----------------------------------------------------------------------------------------------------*/
uint32_t App_get_error_flags(void)
{
  return *(uint32_t*)&g_system_error_flags;
}

/*-----------------------------------------------------------------------------------------------------
  Check if any system errors are currently active

  Parameters:
    None

  Return:
    true if any error flag is set, false if no errors
-----------------------------------------------------------------------------------------------------*/
bool App_has_any_errors(void)
{
  return (*(uint32_t*)&g_system_error_flags) != 0;
}

/*-----------------------------------------------------------------------------------------------------
  Set motor overcurrent error flag - logs emergency message only on first detection

  Parameters:
    motor_number - Motor number (1-4)
    current_value - Current measured current value in Amperes
    threshold_value - Current threshold value in Amperes

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void App_set_motor_overcurrent_flag(uint8_t motor_number, float current_value, float threshold_value)
{
  if (motor_number == MOTOR_1_)
  {
    if (g_system_error_flags.motor1_overcurrent == 0)
    {
      APPLOG("EMERGENCY: Motor 1 overcurrent detected: %.1fA > %.1fA threshold - stopping all motors", current_value, threshold_value);
      g_system_error_flags.motor1_overcurrent = 1;
    }
  }
  else if (motor_number == MOTOR_2_)
  {
    if (g_system_error_flags.motor2_overcurrent == 0)
    {
      APPLOG("EMERGENCY: Motor 2 overcurrent detected: %.1fA > %.1fA threshold - stopping all motors", current_value, threshold_value);
      g_system_error_flags.motor2_overcurrent = 1;
    }
  }
  else if (motor_number == MOTOR_3_)
  {
    if (g_system_error_flags.motor3_overcurrent == 0)
    {
      APPLOG("EMERGENCY: Motor 3 overcurrent detected: %.1fA > %.1fA threshold - stopping all motors", current_value, threshold_value);
      g_system_error_flags.motor3_overcurrent = 1;
    }
  }
  else if (motor_number == MOTOR_4_)
  {
    if (g_system_error_flags.motor4_overcurrent == 0)
    {
      APPLOG("EMERGENCY: Motor 4 overcurrent detected: %.1fA > %.1fA threshold - stopping all motors", current_value, threshold_value);
      g_system_error_flags.motor4_overcurrent = 1;
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Set driver overtemperature error flag - logs emergency message only on first detection

  Parameters:
    driver_number - Driver number (1-2)
    temperature_value - Current measured temperature value in degrees Celsius
    threshold_value - Temperature threshold value in degrees Celsius

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void App_set_driver_overtemperature_flag(uint8_t driver_number, float temperature_value, float threshold_value)
{
  if (driver_number == DRIVER_1)
  {
    if (g_system_error_flags.driver1_overtemperature == 0)
    {
      APPLOG("EMERGENCY: Motor 1 driver overtemperature: %.1f°C > %.1f°C threshold - stopping all motors", temperature_value, threshold_value);
      g_system_error_flags.driver1_overtemperature = 1;
    }
  }
  else if (driver_number == DRIVER_2)
  {
    if (g_system_error_flags.driver2_overtemperature == 0)
    {
      APPLOG("EMERGENCY: Motor 2 driver overtemperature: %.1f°C > %.1f°C threshold - stopping all motors", temperature_value, threshold_value);
      g_system_error_flags.driver2_overtemperature = 1;
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Set TMC6200 driver fault error flag - logs emergency message only on first detection

  Parameters:
    driver_number - Driver number (1-2)

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void App_set_tmc6200_driver_fault_flag(uint8_t driver_number)
{
  if (driver_number == DRIVER_1)
  {
    if (g_system_error_flags.tmc6200_driver1_fault == 0)
    {
      APPLOG("EMERGENCY: TMC6200 driver 1 hardware fault detected - stopping all motors");
      g_system_error_flags.tmc6200_driver1_fault = 1;
    }
  }
  else if (driver_number == DRIVER_2)
  {
    if (g_system_error_flags.tmc6200_driver2_fault == 0)
    {
      APPLOG("EMERGENCY: TMC6200 driver 2 hardware fault detected - stopping all motors");
      g_system_error_flags.tmc6200_driver2_fault = 1;
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Set power supply fault error flag - logs emergency message only on first detection

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void App_set_power_supply_fault_flag(void)
{
  if (g_system_error_flags.power_supply_fault == 0)
  {
    APPLOG("EMERGENCY: Power supply voltage fault detected - stopping all motors");
    g_system_error_flags.power_supply_fault = 1;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Set CAN bus error flag - logs emergency message only on first detection

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void App_set_can_bus_error_flag(void)
{
  if (g_system_error_flags.can_bus_error == 0)
  {
    APPLOG("EMERGENCY: CAN bus communication error detected");
    g_system_error_flags.can_bus_error = 1;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Clear CAN bus error flag when communication resumes

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void App_clear_can_bus_error_flag(void)
{
  if (g_system_error_flags.can_bus_error == 1)
  {
    APPLOG("CAN bus communication recovered - clearing error flag");
    g_system_error_flags.can_bus_error = 0;
  }
}


/*-----------------------------------------------------------------------------------------------------
  Set CPU overtemperature error flag - logs emergency message only on first detection

  Parameters:
    temperature_value - Current measured CPU temperature value in degrees Celsius
    threshold_value - CPU temperature threshold value in degrees Celsius

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void App_set_cpu_overtemperature_flag(float temperature_value, float threshold_value)
{
  if (g_system_error_flags.cpu_overtemperature == 0)
  {
    APPLOG("EMERGENCY: CPU overtemperature: %.1f°C > %.1f°C threshold - stopping all motors", temperature_value, threshold_value);
    g_system_error_flags.cpu_overtemperature = 1;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Set emergency stop flag - blocks all motor control commands

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void App_set_emergency_stop_flag(void)
{
  if (g_system_error_flags.emergency_stop_active == 0)
  {
    APPLOG("EMERGENCY: Emergency stop activated - all motor control commands blocked");
    g_system_error_flags.emergency_stop_active = 1;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Check if emergency stop is active

  Parameters:
    None

  Return:
    1 if emergency stop is active, 0 if not active
-----------------------------------------------------------------------------------------------------*/
uint8_t App_is_emergency_stop_active(void)
{
  return g_system_error_flags.emergency_stop_active;
}

/*-----------------------------------------------------------------------------------------------------
  Check if CAN bus error flag is set

  Parameters:
    None

  Return:
    1 if CAN bus error flag is set, 0 otherwise
-----------------------------------------------------------------------------------------------------*/
uint8_t App_is_can_bus_error_set(void)
{
  return g_system_error_flags.can_bus_error;
}

/*-----------------------------------------------------------------------------------------------------
  Clear all system error flags - allows retry of motor commands and resets all error states

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void App_clear_all_error_flags(void)
{
  TX_INTERRUPT_SAVE_AREA

  TX_DISABLE
  // Clear all error flags by resetting the entire structure
  *(uint32_t*)&g_system_error_flags = 0;
  TX_RESTORE

  APPLOG("All system error flags cleared - motor commands now allowed");
}

/*-----------------------------------------------------------------------------------------------------
  Update TMC6200 detailed error flags from GSTAT register value

  Parameters:
    driver_number - Driver number (1-2)
    gstat_value   - GSTAT register value from TMC6200

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void App_update_tmc6200_detailed_errors(uint8_t driver_number, uint32_t gstat_value)
{
  // Extract error bits from GSTAT (excluding first 3 bits: reset, drv_otpw, drv_ot)
  // Only process bits 3-14 (excluding reserved bits 7, 11, 15)

  if (driver_number == DRIVER_1)
  {
    g_system_error_flags.tmc6200_drv1_uv_cp = (gstat_value & (1 << 3)) ? 1 : 0;        // Charge pump undervoltage
    g_system_error_flags.tmc6200_drv1_shortdet_u = (gstat_value & (1 << 4)) ? 1 : 0;   // Phase U short detection
    g_system_error_flags.tmc6200_drv1_s2gu = (gstat_value & (1 << 5)) ? 1 : 0;         // Phase U short to GND
    g_system_error_flags.tmc6200_drv1_s2vsu = (gstat_value & (1 << 6)) ? 1 : 0;        // Phase U short to VS
    g_system_error_flags.tmc6200_drv1_shortdet_v = (gstat_value & (1 << 8)) ? 1 : 0;   // Phase V short detection
    g_system_error_flags.tmc6200_drv1_s2gv = (gstat_value & (1 << 9)) ? 1 : 0;         // Phase V short to GND
    g_system_error_flags.tmc6200_drv1_s2vsv = (gstat_value & (1 << 10)) ? 1 : 0;       // Phase V short to VS
    g_system_error_flags.tmc6200_drv1_shortdet_w = (gstat_value & (1 << 12)) ? 1 : 0;  // Phase W short detection
    g_system_error_flags.tmc6200_drv1_s2gw = (gstat_value & (1 << 13)) ? 1 : 0;        // Phase W short to GND
    g_system_error_flags.tmc6200_drv1_s2vsw = (gstat_value & (1 << 14)) ? 1 : 0;       // Phase W short to VS
  }
  else if (driver_number == DRIVER_2)
  {
    g_system_error_flags.tmc6200_drv2_uv_cp = (gstat_value & (1 << 3)) ? 1 : 0;        // Charge pump undervoltage
    g_system_error_flags.tmc6200_drv2_shortdet_u = (gstat_value & (1 << 4)) ? 1 : 0;   // Phase U short detection
    g_system_error_flags.tmc6200_drv2_s2gu = (gstat_value & (1 << 5)) ? 1 : 0;         // Phase U short to GND
    g_system_error_flags.tmc6200_drv2_s2vsu = (gstat_value & (1 << 6)) ? 1 : 0;        // Phase U short to VS
    g_system_error_flags.tmc6200_drv2_shortdet_v = (gstat_value & (1 << 8)) ? 1 : 0;   // Phase V short detection
    g_system_error_flags.tmc6200_drv2_s2gv = (gstat_value & (1 << 9)) ? 1 : 0;         // Phase V short to GND
    g_system_error_flags.tmc6200_drv2_s2vsv = (gstat_value & (1 << 10)) ? 1 : 0;       // Phase V short to VS
    g_system_error_flags.tmc6200_drv2_shortdet_w = (gstat_value & (1 << 12)) ? 1 : 0;  // Phase W short detection
    g_system_error_flags.tmc6200_drv2_s2gw = (gstat_value & (1 << 13)) ? 1 : 0;        // Phase W short to GND
    g_system_error_flags.tmc6200_drv2_s2vsw = (gstat_value & (1 << 14)) ? 1 : 0;       // Phase W short to VS
  }
}

/*-----------------------------------------------------------------------------------------------------
  Format list of active system errors into buffer for display

  Parameters:
    buffer - Buffer to write error list to
    buffer_size - Size of the buffer

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void App_format_error_list(char *buffer, uint32_t buffer_size)
{
  if (!buffer || buffer_size == 0) return;

  buffer[0] = '\0';  // Initialize empty string

  if (!App_has_any_errors())
  {
    strcat(buffer, "No system errors");
    return;
  }

  strcat(buffer, "System Errors:\n\n");

  // Check main error flags
  if (g_system_error_flags.motor1_overcurrent)
    strcat(buffer, "• Motor 1 overcurrent\n");
  if (g_system_error_flags.motor2_overcurrent)
    strcat(buffer, "• Motor 2 overcurrent\n");
  if (g_system_error_flags.motor3_overcurrent)
    strcat(buffer, "• Motor 3 overcurrent\n");
  if (g_system_error_flags.motor4_overcurrent)
    strcat(buffer, "• Motor 4 overcurrent\n");
  if (g_system_error_flags.driver1_overtemperature)
    strcat(buffer, "• Driver 1 overtemperature\n");
  if (g_system_error_flags.driver2_overtemperature)
    strcat(buffer, "• Driver 2 overtemperature\n");
  if (g_system_error_flags.tmc6200_driver1_fault)
    strcat(buffer, "• TMC6200 driver 1 fault\n");
  if (g_system_error_flags.tmc6200_driver2_fault)
    strcat(buffer, "• TMC6200 driver 2 fault\n");
  if (g_system_error_flags.power_supply_fault)
    strcat(buffer, "• Power supply fault\n");
  if (g_system_error_flags.can_bus_error)
    strcat(buffer, "• CAN bus error\n");
  if (g_system_error_flags.cpu_overtemperature)
    strcat(buffer, "• CPU overtemperature\n");
  if (g_system_error_flags.emergency_stop_active)
    strcat(buffer, "• Emergency stop active\n");

  // Check TMC6200 detailed errors for driver 1
  if (g_system_error_flags.tmc6200_drv1_uv_cp)
    strcat(buffer, "• Driver 1 charge pump UV\n");
  if (g_system_error_flags.tmc6200_drv1_shortdet_u)
    strcat(buffer, "• Driver 1 phase U short\n");
  if (g_system_error_flags.tmc6200_drv1_s2gu)
    strcat(buffer, "• Driver 1 phase U to GND\n");
  if (g_system_error_flags.tmc6200_drv1_s2vsu)
    strcat(buffer, "• Driver 1 phase U to VS\n");
  if (g_system_error_flags.tmc6200_drv1_shortdet_v)
    strcat(buffer, "• Driver 1 phase V short\n");
  if (g_system_error_flags.tmc6200_drv1_s2gv)
    strcat(buffer, "• Driver 1 phase V to GND\n");
  if (g_system_error_flags.tmc6200_drv1_s2vsv)
    strcat(buffer, "• Driver 1 phase V to VS\n");
  if (g_system_error_flags.tmc6200_drv1_shortdet_w)
    strcat(buffer, "• Driver 1 phase W short\n");
  if (g_system_error_flags.tmc6200_drv1_s2gw)
    strcat(buffer, "• Driver 1 phase W to GND\n");
  if (g_system_error_flags.tmc6200_drv1_s2vsw)
    strcat(buffer, "• Driver 1 phase W to VS\n");

  // Check TMC6200 detailed errors for driver 2
  if (g_system_error_flags.tmc6200_drv2_uv_cp)
    strcat(buffer, "• Driver 2 charge pump UV\n");
  if (g_system_error_flags.tmc6200_drv2_shortdet_u)
    strcat(buffer, "• Driver 2 phase U short\n");
  if (g_system_error_flags.tmc6200_drv2_s2gu)
    strcat(buffer, "• Driver 2 phase U to GND\n");
  if (g_system_error_flags.tmc6200_drv2_s2vsu)
    strcat(buffer, "• Driver 2 phase U to VS\n");
  if (g_system_error_flags.tmc6200_drv2_shortdet_v)
    strcat(buffer, "• Driver 2 phase V short\n");
  if (g_system_error_flags.tmc6200_drv2_s2gv)
    strcat(buffer, "• Driver 2 phase V to GND\n");
  if (g_system_error_flags.tmc6200_drv2_s2vsv)
    strcat(buffer, "• Driver 2 phase V to VS\n");
  if (g_system_error_flags.tmc6200_drv2_shortdet_w)
    strcat(buffer, "• Driver 2 phase W short\n");
  if (g_system_error_flags.tmc6200_drv2_s2gw)
    strcat(buffer, "• Driver 2 phase W to GND\n");
  if (g_system_error_flags.tmc6200_drv2_s2vsw)
    strcat(buffer, "• Driver 2 phase W to VS\n");
}
