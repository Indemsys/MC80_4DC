//----------------------------------------------------------------------
// File created on 2025-01-27
//----------------------------------------------------------------------
#include "App.h"

// Global error codes for TMC6200 drivers
uint8_t g_tmc6200_driver1_error_code      = TMC6200_ERROR_NONE;
uint8_t g_tmc6200_driver2_error_code      = TMC6200_ERROR_NONE;

// Global TMC6200 monitoring structure
T_tmc6200_monitoring g_tmc6200_monitoring = { 0 };

#define TMC6200_WRITE_CMD   0x80
#define TMC6200_READ_CMD    0x00
#define TMC6200_REG_SIZE    4
#define TMC6200_SPI_TIMEOUT 100

static void _Select_driver(uint8_t driver_num, bool select);
static void _Set_driver_error_code(uint8_t driver_num, uint8_t error_code);
static void _Analyze_gstat_errors(uint8_t driver_num, uint32_t gstat_value);

/*-----------------------------------------------------------------------------------------------------
  Write 32-bit value to TMC6200 register.

  Parameters:
    driver_num - 1 or 2 (chip select)
    reg_addr   - register address
    value      - 32-bit value to write

  Return:
    RES_OK     - success
    RES_ERROR  - error
-----------------------------------------------------------------------------------------------------*/
uint32_t Motdrv_tmc6200_WriteRegister(uint8_t driver_num, uint8_t reg_addr, uint32_t value)
{
  uint8_t  tx_buf[1 + TMC6200_REG_SIZE];
  uint32_t status;

  tx_buf[0] = (reg_addr | TMC6200_WRITE_CMD);
  tx_buf[1] = (uint8_t)(value >> 24);
  tx_buf[2] = (uint8_t)(value >> 16);
  tx_buf[3] = (uint8_t)(value >> 8);
  tx_buf[4] = (uint8_t)(value);

  status    = tx_mutex_get(&g_spi0_bus_mutex, TX_WAIT_FOREVER);  // SPI bus protection
  if (status != TX_SUCCESS)
  {
    return RES_ERROR;
  }
  SPI0_set_speed(SPI0_SPEED_3MHZ, SPI_CLK_POLARITY_HIGH, SPI_CLK_PHASE_EDGE_EVEN);
  _Select_driver(driver_num, true);  // Activate CS

  status = R_SPI_B_Write(&g_SPI0_ctrl, tx_buf, sizeof(tx_buf), SPI_BIT_WIDTH_8_BITS);
  if (status == FSP_SUCCESS)
  {
    status = SPI0_wait_transfer_complete(TMC6200_SPI_TIMEOUT);
  }

  _Select_driver(driver_num, false);  // Deactivate CS
  tx_mutex_put(&g_spi0_bus_mutex);

  if (status == TX_SUCCESS)
  {
    return RES_OK;
  }
  else
  {
    return RES_ERROR;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Read 32-bit value from TMC6200 register.

  Parameters:
    driver_num - 1 or 2 (chip select)
    reg_addr   - register address
    value      - pointer to return value

  Return:
    RES_OK     - success
    RES_ERROR  - error
-----------------------------------------------------------------------------------------------------*/
uint32_t Motdrv_tmc6200_ReadRegister(uint8_t driver_num, uint8_t reg_addr, uint32_t* value)
{
  uint8_t  tx_buf[1 + TMC6200_REG_SIZE] = { 0 };
  uint8_t  rx_buf[1 + TMC6200_REG_SIZE] = { 0 };
  uint32_t status;

  tx_buf[0] = (reg_addr | TMC6200_READ_CMD);

  status    = tx_mutex_get(&g_spi0_bus_mutex, TX_WAIT_FOREVER);  // SPI bus protection
  if (status != TX_SUCCESS)
  {
    return RES_ERROR;
  }
  SPI0_set_speed(SPI0_SPEED_3MHZ, SPI_CLK_POLARITY_HIGH, SPI_CLK_PHASE_EDGE_EVEN);
  _Select_driver(driver_num, true);  // Activate CS

  status = R_SPI_B_WriteRead(&g_SPI0_ctrl, tx_buf, rx_buf, sizeof(tx_buf), SPI_BIT_WIDTH_8_BITS);
  if (status == FSP_SUCCESS)
  {
    status = SPI0_wait_transfer_complete(TMC6200_SPI_TIMEOUT);
  }

  _Select_driver(driver_num, false);  // Deactivate CS
  tx_mutex_put(&g_spi0_bus_mutex);

  if (status == TX_SUCCESS)
  {
    *value = ((uint32_t)rx_buf[1] << 24) |
             ((uint32_t)rx_buf[2] << 16) |
             ((uint32_t)rx_buf[3] << 8) |
             ((uint32_t)rx_buf[4]);
    return RES_OK;
  }
  else
  {
    return RES_ERROR;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Select TMC6200 chip by number.

  Parameters:
    driver_num - 1 or 2
    select     - true to activate CS, false to deactivate

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Select_driver(uint8_t driver_num, bool select)
{
  if (driver_num == 1)
  {
    if (select)
    {
      MOTOR_DRV1_CS = 0;  // Active level
    }
    else
    {
      MOTOR_DRV1_CS = 1;
    }
  }
  else if (driver_num == 2)
  {
    if (select)
    {
      MOTOR_DRV2_CS = 0;
    }
    else
    {
      MOTOR_DRV2_CS = 1;
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Set error code for specific TMC6200 driver.

  Parameters:
    driver_num - 1 or 2
    error_code - error code from TMC6200_ERROR_* definitions

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Set_driver_error_code(uint8_t driver_num, uint8_t error_code)
{
  if (driver_num == 1)
  {
    g_tmc6200_driver1_error_code = error_code;
  }
  else if (driver_num == 2)
  {
    g_tmc6200_driver2_error_code = error_code;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Convert TMC6200 error code to readable string.

  Parameters:
    error_code - error code from TMC6200_ERROR_* definitions

  Return:
    Pointer to constant string describing the error
-----------------------------------------------------------------------------------------------------*/
const char* Motdrv_tmc6200_GetErrorString(uint8_t error_code)
{
  // clang-format off
  switch (error_code)
  {
    case TMC6200_ERROR_NONE:             return "No error";
    case TMC6200_ERROR_SPI_READ_FAIL:    return "SPI read failed";
    case TMC6200_ERROR_SPI_WRITE_FAIL:   return "SPI write failed";
    case TMC6200_ERROR_WRONG_VERSION:    return "Wrong chip version";
    case TMC6200_ERROR_GCONF_WRITE_FAIL: return "GCONF write failed";
    case TMC6200_ERROR_SHORT_WRITE_FAIL: return "SHORT_CONF write failed";
    case TMC6200_ERROR_DRV_WRITE_FAIL:   return "DRV_CONF write failed";
    case TMC6200_ERROR_GSTAT_READ_FAIL:  return "GSTAT read failed";
    default:                             return "Unknown error";
  }
  // clang-format on
}

/*-----------------------------------------------------------------------------------------------------
  Initialize TMC6200 driver with parameters from MC80_DriverIC configuration.

  Parameters:
    driver_num - 1 or 2 (chip select)

  Return:
    RES_OK     - success
    RES_ERROR  - error
-----------------------------------------------------------------------------------------------------*/
uint32_t Motdrv_tmc6200_Initialize(uint8_t driver_num)
{
  uint32_t status;
  uint32_t reg_value;
  uint32_t ioin_value;
  uint8_t  chip_version;

  // Clear error code at start of initialization
  _Set_driver_error_code(driver_num, TMC6200_ERROR_NONE);

  // Ensure EN signal is LOW during initialization
  Motor_driver_enable_set(driver_num, 0);  // Disable driver during initialization

  // Read IOIN register to verify chip version
  status = Motdrv_tmc6200_ReadRegister(driver_num, TMC6200_REG_IOIN, &ioin_value);
  if (status != RES_OK)
  {
    APPLOG("TMC6200 Driver %d: Failed to read IOIN register for version check", driver_num);
    _Set_driver_error_code(driver_num, TMC6200_ERROR_SPI_READ_FAIL);
    return RES_ERROR;
  }

  // Extract version from bits 31:24
  chip_version = (uint8_t)((ioin_value & TMC6200_IOIN_VERSION_MASK) >> TMC6200_IOIN_VERSION_SHIFT);

  // Verify chip version matches expected value (0x10)
  if (chip_version != TMC6200_EXPECTED_VERSION)
  {
    APPLOG("TMC6200 Driver %d: Wrong chip version 0x%02X, expected 0x%02X",  driver_num, chip_version, TMC6200_EXPECTED_VERSION);
    _Set_driver_error_code(driver_num, TMC6200_ERROR_WRONG_VERSION);
    return RES_ERROR;
  }
  APPLOG("TMC6200 Driver %d: Chip version verified (0x%02X)", driver_num, chip_version);
  // Configure GCONF register (0x00)
  reg_value = 0;
  reg_value |= TMC6200_GCONF_AMPLIF_20X;  // Set amplifier gain to 20x

  status = Motdrv_tmc6200_WriteRegister(driver_num, TMC6200_REG_GCONF, reg_value);
  if (status != RES_OK)
  {
    APPLOG("TMC6200 Driver %d: Failed to write GCONF register", driver_num);
    _Set_driver_error_code(driver_num, TMC6200_ERROR_GCONF_WRITE_FAIL);
    return RES_ERROR;
  }

  // Configure SHORT_CONF register (0x09) with MC80_DriverIC parameters
  reg_value = 0;
  reg_value |= TMC6200_SHORT_S2VS_LEVEL(wvar.short_vs_det_level);  // Short to VS sensitivity for lowside FETs
  reg_value |= TMC6200_SHORT_S2G_LEVEL(wvar.short_gnd_det_level);  // Short to GND sensitivity for highside FETs
  reg_value |= TMC6200_SHORT_FILTER(wvar.short_det_spike_filter);  // Spike filter bandwidth

  if (wvar.short_det_delay_param != 0)                             // Delay parameter 0 or 1
  {
    reg_value |= TMC6200_SHORT_DELAY;
  }

  if (wvar.enable_short_to_gnd_prot == 0)  // Disable short to GND protection if parameter is 0
  {
    reg_value |= TMC6200_SHORT_DISABLE_S2G;
  }

  if (wvar.enable_short_to_vs_prot == 0)  // Disable short to VS protection if parameter is 0
  {
    reg_value |= TMC6200_SHORT_DISABLE_S2VS;
  }
  status = Motdrv_tmc6200_WriteRegister(driver_num, TMC6200_REG_SHORT_CONF, reg_value);
  if (status != RES_OK)
  {
    APPLOG("TMC6200 Driver %d: Failed to write SHORT_CONF register", driver_num);
    _Set_driver_error_code(driver_num, TMC6200_ERROR_SHORT_WRITE_FAIL);
    return RES_ERROR;
  }

  // Configure DRV_CONF register (0x0A) with driver strength from MC80_DriverIC parameters
  reg_value = 0;
  reg_value |= TMC6200_DRVCONF_BBMCLKS(10);  // BBM delay 10 clocks (420ns typ.)
  reg_value |= TMC6200_DRVCONF_OT_150C;      // Overtemperature threshold 150°C

  // Map gate driver current parameter (0-3) to TMC6200 driver strength
  if (wvar.gate_driver_current_param == 0)
  {
    reg_value |= TMC6200_DRVCONF_STRENGTH_WEAK;
  }
  else if (wvar.gate_driver_current_param == 1)
  {
    reg_value |= TMC6200_DRVCONF_STRENGTH_WEAK_TC;
  }
  else if (wvar.gate_driver_current_param == 2)
  {
    reg_value |= TMC6200_DRVCONF_STRENGTH_MEDIUM;
  }
  else if (wvar.gate_driver_current_param == 3)
  {
    reg_value |= TMC6200_DRVCONF_STRENGTH_STRONG;
  }
  status = Motdrv_tmc6200_WriteRegister(driver_num, TMC6200_REG_DRV_CONF, reg_value);
  if (status != RES_OK)
  {
    APPLOG("TMC6200 Driver %d: Failed to write DRV_CONF register", driver_num);
    _Set_driver_error_code(driver_num, TMC6200_ERROR_DRV_WRITE_FAIL);
    return RES_ERROR;
  }

  // Log FAULT signal state with descriptive text
  uint8_t fault_state;
  if (driver_num == 1)
  {
    fault_state = MOTOR_DRV1_FAULT_STATE;
  }
  else
  {
    fault_state = MOTOR_DRV2_FAULT_STATE;
  }

  if (fault_state == 0)
  {
    APPLOG("TMC6200 Driver %d: FAULT signal = %d (Normal operation)", driver_num, fault_state);
  }
  else
  {
    APPLOG("TMC6200 Driver %d: FAULT signal = %d (FAULT detected!)", driver_num, fault_state);
  }
  // Clear any status flags by reading GSTAT and analyze errors
  uint32_t gstat_value;
  status = Motdrv_tmc6200_ReadRegister(driver_num, TMC6200_REG_GSTAT, &gstat_value);
  if (status != RES_OK)
  {
    APPLOG("TMC6200 Driver %d: Failed to read GSTAT register", driver_num);
    _Set_driver_error_code(driver_num, TMC6200_ERROR_GSTAT_READ_FAIL);
    return RES_ERROR;
  }
  // Analyze GSTAT register and log detected errors
  _Analyze_gstat_errors(driver_num, gstat_value);

  // Clear GSTAT flags by writing the current value back to the register
  // EN signal must be HIGH to allow clearing RESET flag and other latched flags
  Motor_driver_enable_set(driver_num, 1);  // Set EN signal HIGH

  tx_thread_sleep(ms_to_ticks(1));  // Wait 1ms delay

  // Write current GSTAT value back to clear all flags (writing 1 to a bit clears it)
  status = Motdrv_tmc6200_WriteRegister(driver_num, TMC6200_REG_GSTAT, gstat_value);
  if (status == RES_OK)
  {
    APPLOG("TMC6200 Driver %d: GSTAT flags cleared (value: 0x%08X)", driver_num, (unsigned int)gstat_value);
  }
  else
  {
    APPLOG("TMC6200 Driver %d: Failed to clear GSTAT flags", driver_num);
  }

  // Clear EN signal
  Motor_driver_enable_set(driver_num, 0);  // Clear EN signal

  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Analyze GSTAT register value and output list of detected TMC6200 errors.

  Parameters:
    driver_num - 1 or 2 (chip select)
    gstat_value - 32-bit value read from GSTAT register

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Analyze_gstat_errors(uint8_t driver_num, uint32_t gstat_value)
{
  T_tmc6200_gstat_bits* gstat_bits = (T_tmc6200_gstat_bits*)&gstat_value;

  // Check if any error bits are set
  if (gstat_value == 0)
  {
    APPLOG("TMC6200 Driver %d: GSTAT = 0x%08lX (No errors detected)", driver_num, (unsigned long)gstat_value);
    return;
  }

  // Check if only reset flag is set (bit 0 = 0x00000001)
  if (gstat_value == 0x00000001)
  {
    APPLOG("TMC6200 Driver %d: GSTAT = 0x%08lX (IC reset detected - normal after power-on)", driver_num, (unsigned long)gstat_value);
    return;
  }

  APPLOG("TMC6200 Driver %d: GSTAT = 0x%08lX (Errors detected):", driver_num, (unsigned long)gstat_value);

  // Check individual error bits and log detected errors
  if (gstat_bits->reset)
  {
    APPLOG("  - IC has been reset (all registers back to defaults)");
  }

  if (gstat_bits->drv_otpw)
  {
    APPLOG("  - Driver over-temperature pre-warning (OTPW) triggered");
  }

  if (gstat_bits->drv_ot)
  {
    APPLOG("  - Driver over-temperature shutdown (OT) active - driver disabled");
  }

  if (gstat_bits->uv_cp)
  {
    APPLOG("  - Charge pump undervoltage detected - driver disabled");
  }

  if (gstat_bits->shortdet_u)
  {
    APPLOG("  - Phase U short detection counter triggered");
  }

  if (gstat_bits->s2gu)
  {
    APPLOG("  - Phase U short to GND detected - driver disabled");
  }

  if (gstat_bits->s2vsu)
  {
    APPLOG("  - Phase U short to VS detected - driver disabled");
  }

  if (gstat_bits->shortdet_v)
  {
    APPLOG("  - Phase V short detection counter triggered");
  }

  if (gstat_bits->s2gv)
  {
    APPLOG("  - Phase V short to GND detected - driver disabled");
  }

  if (gstat_bits->s2vsv)
  {
    APPLOG("  - Phase V short to VS detected - driver disabled");
  }

  if (gstat_bits->shortdet_w)
  {
    APPLOG("  - Phase W short detection counter triggered");
  }

  if (gstat_bits->s2gw)
  {
    APPLOG("  - Phase W short to GND detected - driver disabled");
  }

  if (gstat_bits->s2vsw)
  {
    APPLOG("  - Phase W short to VS detected - driver disabled");
  }
}

/*-----------------------------------------------------------------------------------------------------
  Get TMC6200 driver status for external access.

  Parameters:
    driver_num - driver number (1 or 2)
    status     - pointer to status structure to fill

  Return:
    RES_OK     - success
    RES_ERROR  - error (invalid driver number or null pointer)
-----------------------------------------------------------------------------------------------------*/
uint32_t Motdrv_tmc6200_GetDriverStatus(uint8_t driver_num, T_tmc6200_driver_status* status)
{
  if (driver_num < 1 || driver_num > 2 || status == NULL)
  {
    return RES_ERROR;
  }

  *status = g_tmc6200_monitoring.driver[driver_num - 1];
  return RES_OK;
}
