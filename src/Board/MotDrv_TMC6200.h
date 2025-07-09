#ifndef MOTDRV_TMC6200_H
#define MOTDRV_TMC6200_H

// =============================================================================
// General Register Addresses
// =============================================================================

#define TMC6200_REG_GCONF           0x00  // Global configuration
#define TMC6200_REG_GSTAT           0x01  // Global status flags
#define TMC6200_REG_IOIN            0x04  // Input pin status
#define TMC6200_REG_OTP_PROG        0x06  // OTP programming register
#define TMC6200_REG_OTP_READ        0x07  // OTP read register
#define TMC6200_REG_FACTORY_CONF    0x08  // Factory configuration
#define TMC6200_REG_SHORT_CONF      0x09  // Short circuit configuration
#define TMC6200_REG_DRV_CONF        0x0A  // Driver configuration

// =============================================================================
// GCONF Register Bits (0x00)
// =============================================================================

#define TMC6200_GCONF_DISABLE       (1 << 0)  // Disable driver
#define TMC6200_GCONF_SINGLELINE    (1 << 1)  // Use single-line control interface
#define TMC6200_GCONF_FAULTDIRECT   (1 << 2)  // FAULT pin reacts to all protection events
#define TMC6200_GCONF_AMPLIF_MASK   (3 << 4)  // Amplifier gain mask
#define TMC6200_GCONF_AMPLIF_5X     (0 << 4)  // Amplifier gain x5
#define TMC6200_GCONF_AMPLIF_10X    (1 << 4)  // Amplifier gain x10
#define TMC6200_GCONF_AMPLIF_20X    (3 << 4)  // Amplifier gain x20
#define TMC6200_GCONF_AMPLIFIER_OFF (1 << 6)  // Disable current sense amplifiers
#define TMC6200_GCONF_TEST_MODE     (1 << 7)  // Enable test mode (use 0 in normal use)

// =============================================================================
// GSTAT Register Bits (0x01)
// =============================================================================
// GSTAT – Global Status Flags (address 0x01, RW/clear-on-write, width 16 bit)
typedef struct
{
  uint16_t reset : 1;       // [0]  Indicates that the IC has been reset. All registers
                            //      are back to defaults. Write ‘1’ to clear.
                            //      Note: DRV_EN must be HIGH to allow clearing.

  uint16_t drv_otpw : 1;    // [1]  Driver-temperature pre-warning (OTPW) has triggered.
                            //      Informational only, no protective action. Latched.

  uint16_t drv_ot : 1;      // [2]  Over-temperature shutdown (OT) is active.  Driver is off
                            //      until the temperature falls below the limit and the bit
                            //      is cleared by writing ‘1’. Latched.

  uint16_t uv_cp : 1;       // [3]  Charge-pump undervoltage detected.  Driver is disabled
                            //      during undervoltage. Latched.

  uint16_t shortdet_u : 1;  // [4]  Short-detection counter on phase U has triggered at least once.
                            //      ORed to STATUS output. Latched.

  uint16_t s2gu : 1;        // [5]  Phase U short to GND detected.  Driver is disabled
                            //      until flag is cleared. ORed to STATUS output. Latched.

  uint16_t s2vsu : 1;       // [6]  Phase U short to VS detected.  Driver is disabled
                            //      until flag is cleared. ORed to STATUS output. Latched.

  uint16_t _res7 : 1;       // [7]  Reserved

  uint16_t shortdet_v : 1;  // [8]  Short-detection counter on phase V triggered. Latched.

  uint16_t s2gv : 1;        // [9]  Phase V short to GND detected. Driver disabled. Latched.

  uint16_t s2vsv : 1;       // [10] Phase V short to VS detected. Driver disabled. Latched.

  uint16_t _res11 : 1;      // [11] Reserved

  uint16_t shortdet_w : 1;  // [12] Short-detection counter on phase W triggered. Latched.

  uint16_t s2gw : 1;        // [13] Phase W short to GND detected. Driver disabled. Latched.

  uint16_t s2vsw : 1;       // [14] Phase W short to VS detected. Driver disabled. Latched.

  uint16_t _res15 : 1;      // [15] Reserved
} T_tmc6200_gstat_bits;

// =============================================================================
// IOIN Register Bits (0x04)
// =============================================================================

#define TMC6200_IOIN_UL                (1 << 0)       // U low-side input state
#define TMC6200_IOIN_UH                (1 << 1)       // U high-side input state
#define TMC6200_IOIN_VL                (1 << 2)       // V low-side input state
#define TMC6200_IOIN_VH                (1 << 3)       // V high-side input state
#define TMC6200_IOIN_WL                (1 << 4)       // W low-side input state
#define TMC6200_IOIN_WH                (1 << 5)       // W high-side input state
#define TMC6200_IOIN_DRV_EN            (1 << 6)       // DRV_EN pin status
#define TMC6200_IOIN_OTPW              (1 << 8)       // Overtemperature prewarning active
#define TMC6200_IOIN_OT136             (1 << 9)       // Temperature ≥ 136 °C
#define TMC6200_IOIN_OT143             (1 << 10)      // Temperature ≥ 143 °C
#define TMC6200_IOIN_OT150             (1 << 11)      // Temperature ≥ 150 °C
#define TMC6200_IOIN_VERSION_MASK      (0xFFU << 24)  // Chip version
#define TMC6200_IOIN_VERSION_SHIFT     24             // Chip version bit shift
#define TMC6200_EXPECTED_VERSION       0x10           // Expected TMC6200 chip version

// =============================================================================
// Error Codes for TMC6200 Driver
// =============================================================================

#define TMC6200_ERROR_NONE             0  // No error
#define TMC6200_ERROR_SPI_READ_FAIL    1  // SPI read operation failed
#define TMC6200_ERROR_SPI_WRITE_FAIL   2  // SPI write operation failed
#define TMC6200_ERROR_WRONG_VERSION    3  // Wrong chip version
#define TMC6200_ERROR_GCONF_WRITE_FAIL 4  // Failed to write GCONF register
#define TMC6200_ERROR_SHORT_WRITE_FAIL 5  // Failed to write SHORT_CONF register
#define TMC6200_ERROR_DRV_WRITE_FAIL   6  // Failed to write DRV_CONF register
#define TMC6200_ERROR_GSTAT_READ_FAIL  7  // Failed to read GSTAT register

// =============================================================================
// Global Error Flags
// =============================================================================

extern uint8_t g_tmc6200_driver1_error_code;  // Error code for driver 1
extern uint8_t g_tmc6200_driver2_error_code;  // Error code for driver 2

// =============================================================================
// SHORT_CONF Register Fields (0x09)
// =============================================================================

#define TMC6200_SHORT_S2VS_LEVEL(x)        ((x) & 0xF)          // Sensitivity for LS short to VS (1=high, 15=low)
#define TMC6200_SHORT_S2G_LEVEL(x)         (((x) & 0xF) << 8)   // Sensitivity for HS short to GND (2=high, 15=low)
#define TMC6200_SHORT_FILTER(x)            (((x) & 0x3) << 16)  // Spike filter time: 0=100ns, 1=1us, 2=2us, 3=3us
#define TMC6200_SHORT_DELAY                (1 << 20)            // Delay before short detection: 0=750ns, 1=1500ns
#define TMC6200_SHORT_RETRY(x)             (((x) & 0x3) << 24)  // Retry count after short: 0=none, 1..3=chopper cycles
#define TMC6200_SHORT_PROTECT_PARALLEL     (1 << 28)            // Shutdown all bridges on short
#define TMC6200_SHORT_DISABLE_S2G          (1 << 29)            // Disable HS short to GND protection
#define TMC6200_SHORT_DISABLE_S2VS         (1 << 30)            // Disable LS short to VS protection

// =============================================================================
// DRV_CONF Register Fields (0x0A)
// =============================================================================

#define TMC6200_DRVCONF_BBMCLKS(x)         ((x) & 0x1F)         // BBM delay in clocks (typ. 42ns per clk)
#define TMC6200_DRVCONF_OTSELECT(x)        (((x) & 0x3) << 16)  // Overtemperature shutdown threshold
#define TMC6200_DRVCONF_OT_150C            (0 << 16)
#define TMC6200_DRVCONF_OT_143C            (1 << 16)
#define TMC6200_DRVCONF_OT_136C            (2 << 16)
#define TMC6200_DRVCONF_OT_120C            (3 << 16)
#define TMC6200_DRVCONF_DRVSTRENGTH(x)     (((x) & 0x3) << 18)  // Driver strength
#define TMC6200_DRVCONF_STRENGTH_WEAK      (0 << 18)
#define TMC6200_DRVCONF_STRENGTH_WEAK_TC   (1 << 18)
#define TMC6200_DRVCONF_STRENGTH_MEDIUM    (2 << 18)
#define TMC6200_DRVCONF_STRENGTH_STRONG    (3 << 18)

// =============================================================================
// OTP_PROG Register (0x06) – One-Time Programmable Memory Programming
// =============================================================================

#define TMC6200_REG_OTP_PROG               0x06                 // OTP programming register

#define TMC6200_OTP_PROG_BIT(n)            ((n) & 0x07)         // Bit number to program (0..7)
#define TMC6200_OTP_PROG_BYTE(n)           (((n) & 0x03) << 4)  // Byte index (0..3), currently only 0 is used
#define TMC6200_OTP_PROG_MAGIC             (0xBD << 8)          // Magic key to enable OTP programming

// Note: Write TMC6200_OTP_PROG_MAGIC | TMC6200_OTP_PROG_BYTE(x) | TMC6200_OTP_PROG_BIT(y) to program one bit.
// Programming takes at least 10ms per bit. Bits can only be set, not cleared.

// =============================================================================
// OTP_READ Register (0x07) – Read Result of OTP Memory
// =============================================================================

#define TMC6200_REG_OTP_READ               0x07    // OTP memory read register

#define TMC6200_OTP_READ_BBM_MASK          (0xC0)  // Bits 6..7: Default BBM setting
#define TMC6200_OTP_READ_S2_LEVEL_MASK     (0x20)  // Bit 5: Short detection level (0=S2G/VS=6, 1=S2G/VS=12)
#define TMC6200_OTP_READ_FCLKTRIM_MASK     (0x1F)  // Bits 0..4: Clock trim (0=lowest freq, 31=highest)

// =============================================================================
// FACTORY_CONF Register (0x08) – Internal Clock Trim Configuration
// =============================================================================

#define TMC6200_REG_FACTORY_CONF           0x08    // Factory configuration register

#define TMC6200_FACTORY_CONF_FCLKTRIM_MASK (0x1F)  // Bits 0..4: Clock trim (use default unless tuning required)

// =============================================================================
// TMC6200 Driver Monitoring Structure
// =============================================================================

// Maximum number of error history entries for each driver
#define TMC6200_ERROR_HISTORY_SIZE         10

// TMC6200 driver monitoring status structure
typedef struct
{
  uint8_t  driver_num;                                 // Driver number (1 or 2)
  uint8_t  init_error_code;                            // Initialization error code (TMC6200_ERROR_*)
  uint8_t  last_gstat_error;                           // Last GSTAT error detected (0 = no error)
  uint32_t last_gstat_value;                           // Last GSTAT register value read
  uint32_t error_count;                                // Total number of errors detected
  uint32_t communication_failures;                     // SPI communication failure count
  uint32_t last_poll_time;                             // Last polling time (system ticks)
  bool     driver_operational;                         // Driver operational status
  uint32_t error_history[TMC6200_ERROR_HISTORY_SIZE];  // History of last GSTAT values with errors
  uint8_t  error_history_index;                        // Current index in error history (circular buffer)
} T_tmc6200_driver_status;

// Global TMC6200 monitoring structure
typedef struct
{
  T_tmc6200_driver_status driver[2];          // Status for both drivers (index 0=driver1, 1=driver2)
  uint32_t                poll_interval_ms;   // Polling interval in milliseconds
  uint32_t                last_monitor_time;  // Last monitoring time (system ticks)
  bool                    monitoring_active;  // Monitoring task active flag
} T_tmc6200_monitoring;

// Global monitoring structure
extern T_tmc6200_monitoring g_tmc6200_monitoring;

uint32_t    Motdrv_tmc6200_WriteRegister(uint8_t driver_num, uint8_t reg_addr, uint32_t value);
uint32_t    Motdrv_tmc6200_ReadRegister(uint8_t driver_num, uint8_t reg_addr, uint32_t* value);
uint32_t    Motdrv_tmc6200_Initialize(uint8_t driver_num);
const char* Motdrv_tmc6200_GetErrorString(uint8_t error_code);

// TMC6200 monitoring functions
uint32_t Motdrv_tmc6200_GetDriverStatus(uint8_t driver_num, T_tmc6200_driver_status* status);

#endif  // MOTDRV_TMC6200_H
