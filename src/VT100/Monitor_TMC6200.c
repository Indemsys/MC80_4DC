#include "App.h"

// Global variables for saving EN signal states when entering TMC6200 diagnostic mode
static uint8_t g_tmc6200_saved_drv1_en_state = 0;
static uint8_t g_tmc6200_saved_drv2_en_state = 0;

#define MAX_ROWCOUNT           40
#define TMC6200_DIAG_HEADER    "================ TMC6200 Motor Driver Diagnostic ================\r\n"
#define TMC6200_DIAG_SUBMENU   "[1] Print all registers\r\n[2] Continuous IOIN (0x04) monitoring (ESC to exit)\r\n[ESC] Exit\r\n\r\n"
#define TMC6200_DIAG_PROMPT    "Select function: "
#define TMC6200_DIAG_MONITOR   "Continuous IOIN (0x04) monitoring (ESC to exit)\r\n\r\n"

#define TMC6200_KEY_REG_DUMP   '1'
#define TMC6200_KEY_MONITOR    '2'
#define TMC6200_KEY_EXIT       VT100_ESC
#define TMC6200_KEY_TOGGLE_EN1 '1'
#define TMC6200_KEY_TOGGLE_EN2 '2'

// Global counters for register read statistics
uint32_t g_tmc6200_read_success[2] = { 0, 0 };  // g_tmc6200_read_success[0] for chip 1, [1] for chip 2
uint32_t g_tmc6200_read_fail[2]    = { 0, 0 };  // g_tmc6200_read_fail[0] for chip 1, [1] for chip 2
// Global counter for incorrect VERSION field read (unexpected VERSION or read error)
uint32_t g_tmc6200_version_fail[2] = { 0, 0 };  // [0] for chip 1, [1] for chip 2

static void _Print_tmc6200_registers(uint8_t driver_num);
static void _Print_tmc6200_menu(void);
static void _TMC6200_mode_reg_dump(void);
static void _TMC6200_mode_monitor(void);
static int  _TMC6200_input_all(uint32_t input_row_base, uint8_t *chip, uint8_t *reg, uint32_t *val);

/*-----------------------------------------------------------------------------------------------------
  Print all main TMC6200 registers for a given driver.

  Parameters:
    driver_num   Driver number (1 or 2)

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Print_tmc6200_registers(uint8_t driver_num)
{
  GET_MCBL;
  uint32_t       reg_val = 0;
  uint32_t       status;
  const char    *reg_names[] = { "GCONF", "GSTAT", "IOIN", "OTP_PROG", "OTP_READ", "FACTORY_CONF", "SHORT_CONF", "DRV_CONF" };
  const uint8_t  reg_addrs[] = { 0x00, 0x01, 0x04, 0x06, 0x07, 0x08, 0x09, 0x0A };
  const uint32_t reg_count   = 8;
  MPRINTF("TMC6200 #%u:\r\n", (unsigned int)driver_num);

  // Check IOIN register (0x04) VERSION field
  status = Motdrv_tmc6200_ReadRegister(driver_num, 0x04, &reg_val);
  if (status == RES_OK)
  {
    uint8_t version = (uint8_t)((reg_val >> 24) & 0xFF);  // VERSION field is bits 31:24
    if (version == 0x10)
    {
      MPRINTF("  [OK] Chip identified as TMC6200 (VERSION=0x%02X)\r\n", version);
    }
    else
    {
      MPRINTF("  [WARNING] Unexpected chip VERSION=0x%02X (expected 0x10)\r\n", version);
      g_tmc6200_version_fail[driver_num - 1]++;  // Increment incorrect VERSION counter
    }
  }
  else
  {
    MPRINTF("  [ERROR] Failed to read IOIN register for chip identification\r\n");
    g_tmc6200_version_fail[driver_num - 1]++;  // Increment incorrect VERSION counter
  }
  for (uint32_t i = 0; i < reg_count; i++)
  {
    status            = Motdrv_tmc6200_ReadRegister(driver_num, reg_addrs[i], &reg_val);
    char bitdesc[128] = "";  // Increased buffer size for detailed register descriptions

    switch (reg_addrs[i])
    {
      case 0x00:  // GCONF
      {
        if (reg_val & (1 << 0)) strcat(bitdesc, "DISABLE ");
        if (reg_val & (1 << 1)) strcat(bitdesc, "SINGLELINE ");
        if (reg_val & (1 << 2)) strcat(bitdesc, "FAULTDIRECT ");
        // Amplifier gain (bits 4-5)
        uint32_t amplif_gain = (reg_val >> 4) & 0x3;
        if (amplif_gain == 0)
          strcat(bitdesc, "AMPLIF_5X ");
        else if (amplif_gain == 1)
          strcat(bitdesc, "AMPLIF_10X ");
        else if (amplif_gain == 3)
          strcat(bitdesc, "AMPLIF_20X ");
        if (reg_val & (1 << 6)) strcat(bitdesc, "AMPLIFIER_OFF ");
        if (reg_val & (1 << 7)) strcat(bitdesc, "TEST_MODE ");
      }
      break;
      case 0x01:  // GSTAT
      {
        if (reg_val & (1 << 0)) strcat(bitdesc, "RESET ");
        if (reg_val & (1 << 1)) strcat(bitdesc, "DRV_OTPW ");
        if (reg_val & (1 << 2)) strcat(bitdesc, "DRV_OT ");
        if (reg_val & (1 << 3)) strcat(bitdesc, "UV_CP ");
        if (reg_val & (1 << 4)) strcat(bitdesc, "SHORTDET_U ");
        if (reg_val & (1 << 5)) strcat(bitdesc, "S2GU ");
        if (reg_val & (1 << 6)) strcat(bitdesc, "S2VSU ");
        if (reg_val & (1 << 8)) strcat(bitdesc, "SHORTDET_V ");
        if (reg_val & (1 << 9)) strcat(bitdesc, "S2GV ");
        if (reg_val & (1 << 10)) strcat(bitdesc, "S2VSV ");
        if (reg_val & (1 << 12)) strcat(bitdesc, "SHORTDET_W ");
        if (reg_val & (1 << 13)) strcat(bitdesc, "S2GW ");
        if (reg_val & (1 << 14)) strcat(bitdesc, "S2VSW ");
      }
      break;
      case 0x04:  // IOIN
      {
        if (reg_val & (1 << 0)) strcat(bitdesc, "UL ");
        if (reg_val & (1 << 1)) strcat(bitdesc, "UH ");
        if (reg_val & (1 << 2)) strcat(bitdesc, "VL ");
        if (reg_val & (1 << 3)) strcat(bitdesc, "VH ");
        if (reg_val & (1 << 4)) strcat(bitdesc, "WL ");
        if (reg_val & (1 << 5)) strcat(bitdesc, "WH ");
        if (reg_val & (1 << 6)) strcat(bitdesc, "DRV_EN ");
        if (reg_val & (1 << 8)) strcat(bitdesc, "OTPW ");
        if (reg_val & (1 << 9)) strcat(bitdesc, "OT136 ");
        if (reg_val & (1 << 10)) strcat(bitdesc, "OT143 ");
        if (reg_val & (1 << 11)) strcat(bitdesc, "OT150 ");
        // Add chip version information
        uint32_t version = (reg_val >> 24) & 0xFF;
        char     version_str[20];
        snprintf(version_str, sizeof(version_str), " VER=0x%02lX", version);
        strcat(bitdesc, version_str);
      }
      break;
      case 0x06:  // OTP_PROG
      {
        uint32_t bit_num  = reg_val & 0x07;
        uint32_t byte_num = (reg_val >> 4) & 0x03;
        uint32_t magic    = (reg_val >> 8) & 0xFF;
        if (magic == 0xBD)
        {
          snprintf(bitdesc, sizeof(bitdesc), "BIT=%lu BYTE=%lu MAGIC=0x%02lX [PROG_MODE]", bit_num, byte_num, magic);
        }
        else
        {
          snprintf(bitdesc, sizeof(bitdesc), "BIT=%lu BYTE=%lu MAGIC=0x%02lX", bit_num, byte_num, magic);
        }
      }
      break;
      case 0x07:  // OTP_READ
      {
        snprintf(bitdesc, sizeof(bitdesc), "BBM=%lu FCLKTRIM=%lu", (reg_val >> 6) & 0x03, reg_val & 0x1F);
      }
      break;
      case 0x08:  // FACTORY_CONF
      {
        snprintf(bitdesc, sizeof(bitdesc), "FCLKTRIM=%lu", reg_val & 0x1F);
      }
      break;
      case 0x09:  // SHORT_CONF
      {
        uint32_t s2vs_level = reg_val & 0xF;
        uint32_t s2g_level  = (reg_val >> 8) & 0xF;
        uint32_t filter     = (reg_val >> 16) & 0x3;
        uint32_t delay      = (reg_val >> 20) & 0x1;
        uint32_t retry      = (reg_val >> 24) & 0x3;
        snprintf(bitdesc, sizeof(bitdesc), "S2VS=%lu S2G=%lu FILT=%lu DLY=%lu RETRY=%lu", s2vs_level, s2g_level, filter, delay, retry);
        if (reg_val & (1 << 28)) strcat(bitdesc, " PROT_PAR");
        if (reg_val & (1 << 29)) strcat(bitdesc, " DIS_S2G");
        if (reg_val & (1 << 30)) strcat(bitdesc, " DIS_S2VS");
      }
      break;
      case 0x0A:  // DRV_CONF
      {
        uint32_t    bbm_clks         = reg_val & 0x1F;
        uint32_t    ot_select        = (reg_val >> 16) & 0x3;
        uint32_t    drv_strength     = (reg_val >> 18) & 0x3;
        const char *ot_names[]       = { "150C", "143C", "136C", "120C" };
        const char *strength_names[] = { "WEAK", "WEAK_TC", "MEDIUM", "STRONG" };
        snprintf(bitdesc, sizeof(bitdesc), "BBM=%lu OT=%s STRENGTH=%s", bbm_clks, ot_names[ot_select], strength_names[drv_strength]);
      }
      break;
      default:
        break;
    }
    MPRINTF("  %-14s (0x%02X) : 0x%08lX   // %s\r\n", reg_names[i], reg_addrs[i], (unsigned long)reg_val, bitdesc);
    if (status == RES_OK)
    {
      g_tmc6200_read_success[driver_num - 1]++;  // Increment global counter
    }
    else
    {
      g_tmc6200_read_fail[driver_num - 1]++;  // Increment global counter
    }
  }

  // Print EN and FAULT states for this chip
  if (driver_num == DRIVER_1)
  {
    MPRINTF("  EN:    %u   (input, [%c]=toggle)\r\n", (unsigned int)Motor_driver_enable_get(DRIVER_1), TMC6200_KEY_TOGGLE_EN1);
    MPRINTF("  FAULT: %u   (input)\r\n", (unsigned int)MOTOR_DRV1_FAULT_STATE);
  }
  else if (driver_num == DRIVER_2)
  {
    MPRINTF("  EN:    %u   (input, [%c]=toggle)\r\n", (unsigned int)Motor_driver_enable_get(DRIVER_2), TMC6200_KEY_TOGGLE_EN2);
    MPRINTF("  FAULT: %u   (input)\r\n", (unsigned int)MOTOR_DRV2_FAULT_STATE);
  }
  MPRINTF("------------------------------------------------------------\r\n");
}

/*-----------------------------------------------------------------------------------------------------
  Print the TMC6200 diagnostic menu to the terminal.

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Print_tmc6200_menu(void)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF(TMC6200_DIAG_HEADER);
  MPRINTF("\r\n");
  MPRINTF(TMC6200_DIAG_SUBMENU);
  MPRINTF(TMC6200_DIAG_PROMPT);
}

/*-----------------------------------------------------------------------------------------------------
  Input chip number, register address and value for TMC6200 register write (all in one place)

  Parameters:
    input_row_base - first row for input area
    chip           - pointer to result chip number (1 or 2)
    reg            - pointer to result register address (uint8_t)
    val            - pointer to result 32-bit value

  Return:
    0 if all inputs are valid, -1 if cancelled or invalid
-----------------------------------------------------------------------------------------------------*/
static int _TMC6200_input_all(uint32_t input_row_base, uint8_t *chip, uint8_t *reg, uint32_t *val)
{
  GET_MCBL;
  uint32_t input_row_chip = input_row_base;
  uint32_t input_row_reg  = input_row_base + 1;
  uint32_t input_row_val  = input_row_base + 2;
  uint8_t  b              = 0;
  uint32_t chip_tmp       = 1;
  uint32_t reg_tmp        = 0;
  uint32_t val_tmp        = 0;
  // Clear input area
  for (uint32_t r = input_row_chip; r <= input_row_val + 1; r++)
  {
    VT100_set_cursor_pos(r, 0);
    MPRINTF(VT100_CLL_FM_CRSR);
  }
  // Chip number input
  VT100_set_cursor_pos(input_row_chip, 0);
  MPRINTF("Enter chip number (1 or 2):");
  if (VT100_edit_uinteger_val_mode(input_row_chip + 1, &chip_tmp, 1, 2, false) != RES_OK)
  {
    VT100_set_cursor_pos(input_row_chip, 0);
    MPRINTF("  Input cancelled. Press any key...\r\n");
    WAIT_CHAR(&b, ms_to_ticks(100000));
    goto input_cleanup;
  }
  VT100_set_cursor_pos(input_row_chip, 0);
  MPRINTF(VT100_CLL_FM_CRSR "  Chip number entered: %lu\r\n", (unsigned long)chip_tmp);
  *chip = (uint8_t)chip_tmp;

  // Register address input
  VT100_set_cursor_pos(input_row_reg, 0);
  MPRINTF("Enter register address (hex, 0x00-0xFF):");
  if (VT100_edit_uinteger_hex_val(input_row_reg + 1, &reg_tmp, 0x00, 0xFF) != RES_OK)
  {
    VT100_set_cursor_pos(input_row_reg, 0);
    MPRINTF("  Input cancelled. Press any key...\r\n");
    WAIT_CHAR(&b, ms_to_ticks(100000));
    goto input_cleanup;
  }
  VT100_set_cursor_pos(input_row_reg, 0);
  MPRINTF(VT100_CLL_FM_CRSR "  Register address entered: 0x%02X\r\n", (unsigned int)reg_tmp);
  *reg = (uint8_t)reg_tmp;

  // Value input
  VT100_set_cursor_pos(input_row_val, 0);
  MPRINTF("Enter 32-bit value (hex, 0x00000000-0xFFFFFFFF):");
  if (VT100_edit_uinteger_hex_val(input_row_val + 1, &val_tmp, 0x00000000, 0xFFFFFFFF) != RES_OK)
  {
    VT100_set_cursor_pos(input_row_val, 0);
    MPRINTF("  Input cancelled. Press any key...\r\n");
    WAIT_CHAR(&b, ms_to_ticks(100000));
    goto input_cleanup;
  }
  VT100_set_cursor_pos(input_row_val, 0);
  MPRINTF(VT100_CLL_FM_CRSR "  Value entered: 0x%08lX\r\n", (unsigned long)val_tmp);
  *val = val_tmp;

  // Wait for user confirmation before clearing
  VT100_set_cursor_pos(input_row_val + 2, 0);
  MPRINTF("Press any key to continue...\r\n");
  WAIT_CHAR(&b, ms_to_ticks(100000));
input_cleanup:
  // Clean up input area (including confirmations)
  for (uint32_t r = input_row_chip; r <= input_row_val + 2; r++)
  {
    VT100_set_cursor_pos(r, 0);
    MPRINTF(VT100_CLL_FM_CRSR);
  }
  if (chip_tmp != 1 && chip_tmp != 2)
    return -1;
  return 0;
}

/*-----------------------------------------------------------------------------------------------------
  Register dump mode: print all registers for both TMC6200 chips, allow writing, continuous refresh,
  and show EN/FAULT states with EN control.

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _TMC6200_mode_reg_dump(void)
{
  GET_MCBL;
  uint8_t  b                = 0;
  uint8_t  chip             = 0;
  uint8_t  reg              = 0;
  uint32_t val              = 0;
  // input_row_base must be after the last output line
  // Calculated: 26 lines (2 chips) + 1 (empty) + 1 (statistics header) + 2 (statistics) + 1 (empty) + 1 (EN menu) + 1 (hint) = 33
  // input_row_base = 34
  uint32_t input_row_base   = 34;
  uint32_t wait_ms          = 500;
  // Reset counters at start of diagnostic session
  g_tmc6200_read_success[0] = 0;
  g_tmc6200_read_success[1] = 0;
  g_tmc6200_read_fail[0]    = 0;
  g_tmc6200_read_fail[1]    = 0;
  g_tmc6200_version_fail[0] = 0;
  g_tmc6200_version_fail[1] = 0;
  while (1)
  {
    MPRINTF(VT100_CLEAR_AND_HOME);
    MPRINTF(TMC6200_DIAG_HEADER);
    MPRINTF("--- Register Dump Mode ---\r\n");
    _Print_tmc6200_registers(DRIVER_1);
    _Print_tmc6200_registers(DRIVER_2);
    // Print cumulative statistics for both chips (only once)
    MPRINTF("\r\nTMC6200 register read statistics (accumulated):\r\n");
    MPRINTF("  Chip 1: OK=%lu, ERROR=%lu, VERSION_FAIL=%lu\r\n", (unsigned long)g_tmc6200_read_success[0], (unsigned long)g_tmc6200_read_fail[0], (unsigned long)g_tmc6200_version_fail[0]);
    MPRINTF("  Chip 2: OK=%lu, ERROR=%lu, VERSION_FAIL=%lu\r\n", (unsigned long)g_tmc6200_read_success[1], (unsigned long)g_tmc6200_read_fail[1], (unsigned long)g_tmc6200_version_fail[1]);
    MPRINTF("\r\n[1] Toggle EN for TMC6200#1   [2] Toggle EN for TMC6200#2\r\n");
    MPRINTF("Press [ESC] to return, 'w' to write to register (chip: 1 or 2)...\r\n");
    if (WAIT_CHAR(&b, ms_to_ticks(wait_ms)) == RES_OK)
    {
      if (b == 'w' || b == 'W')
      {
        // input_row_base is already correctly calculated above
        if (input_row_base + 2 >= MAX_ROWCOUNT)
        {
          input_row_base = (MAX_ROWCOUNT > 3) ? (MAX_ROWCOUNT - 3) : 0;
        }
        if (_TMC6200_input_all(input_row_base, &chip, &reg, &val) == 0)
        {
          uint32_t status = Motdrv_tmc6200_WriteRegister(chip, reg, val);
          if (status == RES_OK)
          {
            MPRINTF("\r\nWrite OK\r\n");
          }
          else
          {
            MPRINTF("\r\nWrite ERROR\r\n");
          }
          MPRINTF("\r\nPress any key to continue...\r\n");
          WAIT_CHAR(&b, ms_to_ticks(100000));
        }
        continue;
      }
      else if (b == TMC6200_KEY_TOGGLE_EN1)
      {
        Motor_driver_enable_toggle(DRIVER_1);  // Use centralized function
        continue;
      }
      else if (b == TMC6200_KEY_TOGGLE_EN2)
      {
        Motor_driver_enable_toggle(DRIVER_2);  // Use centralized function
        continue;
      }
      else if (b == TMC6200_KEY_EXIT)
      {
        break;
      }
    }
    // If no key pressed, just refresh every 0.5 sec
  }
}

/*-----------------------------------------------------------------------------------------------------
  Live IOIN & signal monitor mode: show IOIN and signal states, allow EN toggling.

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _TMC6200_mode_monitor(void)
{
  GET_MCBL;

  // Save current EN signal states before entering monitor mode
  g_tmc6200_saved_drv1_en_state = Motor_driver_enable_get(DRIVER_1);  // Use centralized function
  g_tmc6200_saved_drv2_en_state = Motor_driver_enable_get(DRIVER_2);  // Use centralized function

  uint32_t status = tx_mutex_get(&g_spi0_bus_mutex, TX_WAIT_FOREVER);
  if (status != TX_SUCCESS)
  {
    MPRINTF("[ERROR] Failed to acquire SPI0 mutex\r\n");
    return;
  }
  uint8_t b = 0;
  while (1)
  {
    MPRINTF(VT100_CLEAR_AND_HOME);
    MPRINTF(TMC6200_DIAG_HEADER);
    MPRINTF("--- Live IOIN & Signal Monitor Mode ---\r\n");
    MPRINTF(TMC6200_DIAG_MONITOR);
    uint32_t reg_val1 = 0, reg_val2 = 0;
    uint32_t st1 = Motdrv_tmc6200_ReadRegister(DRIVER_1, 0x04, &reg_val1);
    uint32_t st2 = Motdrv_tmc6200_ReadRegister(DRIVER_2, 0x04, &reg_val2);
    // Grouped output for chip 1
    MPRINTF("TMC6200#1 IOIN:  %08lX %s\r\n", (unsigned long)reg_val1, (st1 == RES_OK ? "" : "[ERROR]"));
    MPRINTF("  EN:    %u   (input, [%c]=toggle)\r\n", (unsigned int)Motor_driver_enable_get(DRIVER_1), TMC6200_KEY_TOGGLE_EN1);
    MPRINTF("  FAULT: %u   (input)\r\n\r\n", (unsigned int)MOTOR_DRV1_FAULT_STATE);
    // Grouped output for chip 2
    MPRINTF("TMC6200#2 IOIN:  %08lX %s\r\n", (unsigned long)reg_val2, (st2 == RES_OK ? "" : "[ERROR]"));
    MPRINTF("  EN:    %u   (input, [%c]=toggle)\r\n", (unsigned int)Motor_driver_enable_get(DRIVER_2), TMC6200_KEY_TOGGLE_EN2);
    MPRINTF("  FAULT: %u   (input)\r\n\r\n", (unsigned int)MOTOR_DRV2_FAULT_STATE);
    MPRINTF("[ESC] Exit\r\n");
    if (WAIT_CHAR(&b, ms_to_ticks(100)) == RES_OK)
    {
      switch (b)
      {
        case TMC6200_KEY_TOGGLE_EN1:
          Motor_driver_enable_toggle(DRIVER_1);  // Use centralized function
          break;
        case TMC6200_KEY_TOGGLE_EN2:
          Motor_driver_enable_toggle(DRIVER_2);  // Use centralized function
          break;        case TMC6200_KEY_EXIT:
          // Restore saved EN signal states
          Motor_driver_enable_set(DRIVER_1, g_tmc6200_saved_drv1_en_state);  // Use centralized function
          Motor_driver_enable_set(DRIVER_2, g_tmc6200_saved_drv2_en_state);  // Use centralized function
          tx_mutex_put(&g_spi0_bus_mutex);
          return;
        default:
          break;
      }
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Show diagnostic information for both TMC6200 motor driver chips.

  Parameters:
    keycode   Not used.

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Diagnostic_TMC6200(uint8_t keycode)
{
  GET_MCBL;
  _Print_tmc6200_menu();
  uint8_t b = 0;
  while (1)
  {
    if (WAIT_CHAR(&b, ms_to_ticks(100000)) == RES_OK)
    {
      switch (b)
      {
        case TMC6200_KEY_REG_DUMP:
          _TMC6200_mode_reg_dump();
          _Print_tmc6200_menu();
          break;
        case TMC6200_KEY_MONITOR:
          _TMC6200_mode_monitor();
          _Print_tmc6200_menu();
          break;
        case TMC6200_KEY_EXIT:
          return;
        default:
          break;
      }
    }
  }
}
