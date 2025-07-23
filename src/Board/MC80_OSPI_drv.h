#ifndef MC80_OSPI_DRV_H
#define MC80_OSPI_DRV_H

// Required includes for basic types only
#include <stdint.h>
#include <stdbool.h>
#include "r_transfer_api.h"

// Configuration defines - embedded in header
#define MC80_OSPI_CFG_PARAM_CHECKING_ENABLE          (1)
#define MC80_OSPI_CFG_DMAC_SUPPORT_ENABLE            (1)
#define MC80_OSPI_CFG_AUTOCALIBRATION_SUPPORT_ENABLE (1)
#define MC80_OSPI_CFG_PREFETCH_FUNCTION              (1)
#define MC80_OSPI_CFG_COMBINATION_FUNCTION           MC80_OSPI_COMBINATION_FUNCTION_64BYTE
#define MC80_OSPI_CFG_DOTF_SUPPORT_ENABLE            (0)
#define MC80_OSPI_CFG_ROW_ADDRESSING_SUPPORT_ENABLE  (0)

// Maximum number of status polling checks after enabling memory writes
#define MC80_OSPI_MAX_WRITE_ENABLE_POLLING_LOOPS     (5)

// Hardware-specific constants (independent from FSP BSP)
#define MC80_OSPI_PERIPHERAL_CHANNEL_MASK            (0x03U)  // OSPI units 0 and 1 available
#define MC80_OSPI_DEVICE_0_START_ADDRESS             (0x80000000UL)  // Memory-mapped address for device 0 (must match BSP_FEATURE_OSPI_B_DEVICE_0_START_ADDRESS)
#define MC80_OSPI_DEVICE_1_START_ADDRESS             (0x90000000UL)  // Memory-mapped address for device 1 (must match BSP_FEATURE_OSPI_B_DEVICE_1_START_ADDRESS)

// OSPI hardware base addresses
#define MC80_OSPI0_BASE_ADDRESS                      (0x40268000UL)  // OSPI0 base address
#define MC80_OSPI1_BASE_ADDRESS                      (0x40269000UL)  // OSPI1 base address
#define MC80_OSPI_UNIT_ADDRESS_OFFSET                (MC80_OSPI1_BASE_ADDRESS - MC80_OSPI0_BASE_ADDRESS)

// MC80 OSPI constants
#define MC80_OSPI_ERASE_SIZE_CHIP_ERASE              (0xFFFFFFFFU)  // Special value for chip erase

// Default preamble patterns for auto-calibration (based on FSP library)
#define MC80_OSPI_DEFAULT_PREAMBLE_PATTERN_0         (0xFFFF0000U)
#define MC80_OSPI_DEFAULT_PREAMBLE_PATTERN_1         (0x000800FFU)
#define MC80_OSPI_DEFAULT_PREAMBLE_PATTERN_2         (0x00FFF700U)
#define MC80_OSPI_DEFAULT_PREAMBLE_PATTERN_3         (0xF700F708U)

// Auto-calibration parameter values
#define MC80_OSPI_CALIBRATION_INTERVAL_MAX           (0x04U)  // CAITV: Interval between calibration patterns (2^(4+1) = 32 cycles)
#define MC80_OSPI_CALIBRATION_NO_OVERWRITE_ENABLE    (0x1U)   // CANOWR: Skip write command during calibration
#define MC80_OSPI_CALIBRATION_SHIFT_START_MIN        (0x0U)   // CASFTSTA: Minimum OM_DQS shift start value (0)
#define MC80_OSPI_CALIBRATION_SHIFT_END_MAX          (0x1FU)  // CASFTEND: Maximum OM_DQS shift value (31)

/*-----------------------------------------------------------------------------------------------------
  OSPI Flash chip select
-----------------------------------------------------------------------------------------------------*/
typedef enum e_mc80_ospi_chip_select
{
  MC80_OSPI_DEVICE_NUMBER_0 = 0U,  // Device connected to Chip-Select 0
  MC80_OSPI_DEVICE_NUMBER_1,       // Device connected to Chip-Select 1
} T_mc80_ospi_device_number;

/*-----------------------------------------------------------------------------------------------------
  OSPI flash number of command code bytes
-----------------------------------------------------------------------------------------------------*/
typedef enum e_mc80_ospi_command_bytes
{
  MC80_OSPI_COMMAND_BYTES_1 = 1U,  // Command codes are 1 byte long
  MC80_OSPI_COMMAND_BYTES_2 = 2U,  // Command codes are 2 bytes long
} T_mc80_ospi_command_bytes;

/*-----------------------------------------------------------------------------------------------------
  OSPI frame to frame interval
-----------------------------------------------------------------------------------------------------*/
typedef enum e_mc80_ospi_frame_interval_clocks
{
  MC80_OSPI_COMMAND_INTERVAL_CLOCKS_1 = 0U,  // 1 interval clocks
  MC80_OSPI_COMMAND_INTERVAL_CLOCKS_2,       // 2 interval clocks
  MC80_OSPI_COMMAND_INTERVAL_CLOCKS_3,       // 3 interval clocks
  MC80_OSPI_COMMAND_INTERVAL_CLOCKS_4,       // 4 interval clocks
  MC80_OSPI_COMMAND_INTERVAL_CLOCKS_5,       // 5 interval clocks
  MC80_OSPI_COMMAND_INTERVAL_CLOCKS_6,       // 6 interval clocks
  MC80_OSPI_COMMAND_INTERVAL_CLOCKS_7,       // 7 interval clocks
  MC80_OSPI_COMMAND_INTERVAL_CLOCKS_8,       // 8 interval clocks
  MC80_OSPI_COMMAND_INTERVAL_CLOCKS_9,       // 9 interval clocks
  MC80_OSPI_COMMAND_INTERVAL_CLOCKS_10,      // 10 interval clocks
  MC80_OSPI_COMMAND_INTERVAL_CLOCKS_11,      // 11 interval clocks
  MC80_OSPI_COMMAND_INTERVAL_CLOCKS_12,      // 12 interval clocks
  MC80_OSPI_COMMAND_INTERVAL_CLOCKS_13,      // 13 interval clocks
  MC80_OSPI_COMMAND_INTERVAL_CLOCKS_14,      // 14 interval clocks
  MC80_OSPI_COMMAND_INTERVAL_CLOCKS_15,      // 15 interval clocks
  MC80_OSPI_COMMAND_INTERVAL_CLOCKS_16,      // 16 interval clocks
} T_mc80_ospi_command_interval_clocks;

/*-----------------------------------------------------------------------------------------------------
  OSPI chip select de-assertion duration
-----------------------------------------------------------------------------------------------------*/
typedef enum e_mc80_ospi_cs_pullup_clocks
{
  MC80_OSPI_COMMAND_CS_PULLUP_CLOCKS_NO_EXTENSION = 0U,  // CS asserting No extension
  MC80_OSPI_COMMAND_CS_PULLUP_CLOCKS_1,                  // CS asserting Extend 1 cycle
} T_mc80_ospi_command_cs_pullup_clocks;

/*-----------------------------------------------------------------------------------------------------
  OSPI chip select assertion duration
-----------------------------------------------------------------------------------------------------*/
typedef enum e_mc80_ospi_cs_pulldown_clocks
{
  MC80_OSPI_COMMAND_CS_PULLDOWN_CLOCKS_NO_EXTENSION = 0U,  // CS negating No extension
  MC80_OSPI_COMMAND_CS_PULLDOWN_CLOCKS_1,                  // CS negating Extend 1 cycle
} T_mc80_ospi_command_cs_pulldown_clocks;

/*-----------------------------------------------------------------------------------------------------
  OSPI data strobe delay
-----------------------------------------------------------------------------------------------------*/
typedef enum e_mc80_ospi_ds_timing_delay
{
  MC80_OSPI_DS_TIMING_DELAY_NONE = 0,   // Sample without delay
  MC80_OSPI_DS_TIMING_DELAY_1    = 1,   // Delay sampling by 1 clock cell
  MC80_OSPI_DS_TIMING_DELAY_2    = 2,   // Delay sampling by 2 clock cells
  MC80_OSPI_DS_TIMING_DELAY_3    = 3,   // Delay sampling by 3 clock cells
  MC80_OSPI_DS_TIMING_DELAY_4    = 4,   // Delay sampling by 4 clock cells
  MC80_OSPI_DS_TIMING_DELAY_5    = 5,   // Delay sampling by 5 clock cells
  MC80_OSPI_DS_TIMING_DELAY_6    = 6,   // Delay sampling by 6 clock cells
  MC80_OSPI_DS_TIMING_DELAY_7    = 7,   // Delay sampling by 7 clock cells
  MC80_OSPI_DS_TIMING_DELAY_8    = 8,   // Delay sampling by 8 clock cells
  MC80_OSPI_DS_TIMING_DELAY_9    = 9,   // Delay sampling by 9 clock cells
  MC80_OSPI_DS_TIMING_DELAY_10   = 10,  // Delay sampling by 10 clock cells
  MC80_OSPI_DS_TIMING_DELAY_11   = 11,  // Delay sampling by 11 clock cells
  MC80_OSPI_DS_TIMING_DELAY_12   = 12,  // Delay sampling by 12 clock cells
  MC80_OSPI_DS_TIMING_DELAY_13   = 13,  // Delay sampling by 13 clock cells
  MC80_OSPI_DS_TIMING_DELAY_14   = 14,  // Delay sampling by 14 clock cells
  MC80_OSPI_DS_TIMING_DELAY_15   = 15,  // Delay sampling by 15 clock cells
  MC80_OSPI_DS_TIMING_DELAY_16   = 16,  // Delay sampling by 16 clock cells
  MC80_OSPI_DS_TIMING_DELAY_17   = 17,  // Delay sampling by 17 clock cells
  MC80_OSPI_DS_TIMING_DELAY_18   = 18,  // Delay sampling by 18 clock cells
  MC80_OSPI_DS_TIMING_DELAY_19   = 19,  // Delay sampling by 19 clock cells
  MC80_OSPI_DS_TIMING_DELAY_20   = 20,  // Delay sampling by 20 clock cells
  MC80_OSPI_DS_TIMING_DELAY_21   = 21,  // Delay sampling by 21 clock cells
  MC80_OSPI_DS_TIMING_DELAY_22   = 22,  // Delay sampling by 22 clock cells
  MC80_OSPI_DS_TIMING_DELAY_23   = 23,  // Delay sampling by 23 clock cells
  MC80_OSPI_DS_TIMING_DELAY_24   = 24,  // Delay sampling by 24 clock cells
  MC80_OSPI_DS_TIMING_DELAY_25   = 25,  // Delay sampling by 25 clock cells
  MC80_OSPI_DS_TIMING_DELAY_26   = 26,  // Delay sampling by 26 clock cells
  MC80_OSPI_DS_TIMING_DELAY_27   = 27,  // Delay sampling by 27 clock cells
  MC80_OSPI_DS_TIMING_DELAY_28   = 28,  // Delay sampling by 28 clock cells
  MC80_OSPI_DS_TIMING_DELAY_29   = 29,  // Delay sampling by 29 clock cells
  MC80_OSPI_DS_TIMING_DELAY_30   = 30,  // Delay sampling by 30 clock cells
  MC80_OSPI_DS_TIMING_DELAY_31   = 31,  // Delay sampling by 31 clock cells
} T_mc80_ospi_ds_timing_delay;

/*-----------------------------------------------------------------------------------------------------
  OSPI SDR signal drive timing
-----------------------------------------------------------------------------------------------------*/
typedef enum e_mc80_ospi_sdr_drive_timing
{
  MC80_OSPI_SDR_DRIVE_TIMING_BEFORE_CK = 0,  // SDR is asserted 1/2 cycle before the rising-edge of CK
  MC80_OSPI_SDR_DRIVE_TIMING_AT_CK     = 1,  // SDR is asserted at the rising-edge of CK
} T_mc80_ospi_sdr_drive_timing;

/*-----------------------------------------------------------------------------------------------------
  Clock edge useed to sample data in SDR mode
-----------------------------------------------------------------------------------------------------*/
typedef enum e_mc80_ospi_ck_edge
{
  MC80_OSPI_CK_EDGE_FALLING = 0,  // Falling-edge of CK signal
  MC80_OSPI_CK_EDGE_RISING  = 1,  // Rising-edge of CK signal
} T_mc80_ospi_ck_edge;

/*-----------------------------------------------------------------------------------------------------
  SDR sampling window delay
-----------------------------------------------------------------------------------------------------*/
typedef enum e_mc80_ospi_sdr_sampling_delay
{
  MC80_OSPI_SDR_SAMPLING_DELAY_NONE = 0,  // No sampling delay
  MC80_OSPI_SDR_SAMPLING_DELAY_1    = 1,  // Delay sampling by 1 cycle
  MC80_OSPI_SDR_SAMPLING_DELAY_2    = 2,  // Delay sampling by 2 cycles
  MC80_OSPI_SDR_SAMPLING_DELAY_3    = 3,  // Delay sampling by 3 cycles
  MC80_OSPI_SDR_SAMPLING_DELAY_4    = 4,  // Delay sampling by 4 cycles
  MC80_OSPI_SDR_SAMPLING_DELAY_5    = 5,  // Delay sampling by 5 cycles
  MC80_OSPI_SDR_SAMPLING_DELAY_6    = 6,  // Delay sampling by 6 cycles
  MC80_OSPI_SDR_SAMPLING_DELAY_7    = 7,  // Delay sampling by 7 cycles
} T_mc80_ospi_sdr_sampling_delay;

/*-----------------------------------------------------------------------------------------------------
  DDR sampling window extension
-----------------------------------------------------------------------------------------------------*/
typedef enum e_mc80_ospi_ddr_sampling_extension
{
  MC80_OSPI_DDR_SAMPLING_EXTENSION_NONE = 0,  // No sampling extension
  MC80_OSPI_DDR_SAMPLING_EXTENSION_1    = 1,  // Sampling extended by 1 cycle
  MC80_OSPI_DDR_SAMPLING_EXTENSION_2    = 2,  // Sampling extended by 2 cycles
  MC80_OSPI_DDR_SAMPLING_EXTENSION_3    = 3,  // Sampling extended by 3 cycles
  MC80_OSPI_DDR_SAMPLING_EXTENSION_4    = 4,  // Sampling extended by 4 cycles
  MC80_OSPI_DDR_SAMPLING_EXTENSION_5    = 5,  // Sampling extended by 5 cycles
  MC80_OSPI_DDR_SAMPLING_EXTENSION_6    = 6,  // Sampling extended by 6 cycles
  MC80_OSPI_DDR_SAMPLING_EXTENSION_7    = 7,  // Sampling extended by 7 cycles
} T_mc80_ospi_ddr_sampling_extension;

/*-----------------------------------------------------------------------------------------------------
  Format of data frames used for communicating with the target device
-----------------------------------------------------------------------------------------------------*/
typedef enum e_mc80_ospi_frame_format
{
  MC80_OSPI_FRAME_FORMAT_STANDARD                = 0x0,  // Standard frame with command, address, and data phases
  MC80_OSPI_FRAME_FORMAT_XSPI_PROFILE_1          = 0x1,  // JEDEC XSPI 8D-8D-8D Profile 1.0 frame
  MC80_OSPI_FRAME_FORMAT_XSPI_PROFILE_2          = 0x2,  // JEDEC XSPI 8D-8D-8D Profile 2.0 frame
  MC80_OSPI_FRAME_FORMAT_XSPI_PROFILE_2_EXTENDED = 0x3,  // JEDEC XSPI 8D-8D-8D Profile 2.0 extended 6-byte command-address frame, used with HyperRAM
} T_mc80_ospi_frame_format;

/*-----------------------------------------------------------------------------------------------------
  Variable or fixed latency selection for flash devices which can notify the host of requiring additional time
-----------------------------------------------------------------------------------------------------*/
typedef enum e_mc80_ospi_latency_mode
{
  MC80_OSPI_LATENCY_MODE_FIXED = 0,  // Latency is fixed to the number of dummy cycles for the command
  MC80_OSPI_LATENCY_MODE_VARIABLE,   // The flash target signifies additional latency (2x dummy cycles) by asserting the DQS line during the address phase
} T_mc80_ospi_latency_mode;

/*-----------------------------------------------------------------------------------------------------
  Prefetch function settings
-----------------------------------------------------------------------------------------------------*/
typedef enum e_mc80_ospi_prefetch_function
{
  MC80_OSPI_PREFETCH_FUNCTION_DISABLE = 0x00,  // Prefetch function disable
  MC80_OSPI_PREFETCH_FUNCTION_ENABLE  = 0x01,  // Prefetch function enable
} T_mc80_ospi_prefetch_function;

/*-----------------------------------------------------------------------------------------------------
  Combination function settings
-----------------------------------------------------------------------------------------------------*/
typedef enum e_mc80_ospi_combination_function
{
  MC80_OSPI_COMBINATION_FUNCTION_DISABLE = 0x00,   // Combination function disable
  MC80_OSPI_COMBINATION_FUNCTION_4BYTE   = 0x01,   // Combine up to 4 bytes
  MC80_OSPI_COMBINATION_FUNCTION_8BYTE   = 0x03,   // Combine up to 8 bytes
  MC80_OSPI_COMBINATION_FUNCTION_12BYTE  = 0x05,   // Combine up to 12 bytes
  MC80_OSPI_COMBINATION_FUNCTION_16BYTE  = 0x07,   // Combine up to 16 bytes
  MC80_OSPI_COMBINATION_FUNCTION_20BYTE  = 0x09,   // Combine up to 20 bytes
  MC80_OSPI_COMBINATION_FUNCTION_24BYTE  = 0x0B,   // Combine up to 24 bytes
  MC80_OSPI_COMBINATION_FUNCTION_28BYTE  = 0x0D,   // Combine up to 28 bytes
  MC80_OSPI_COMBINATION_FUNCTION_32BYTE  = 0x0F,   // Combine up to 32 bytes
  MC80_OSPI_COMBINATION_FUNCTION_36BYTE  = 0x11,   // Combine up to 36 bytes
  MC80_OSPI_COMBINATION_FUNCTION_40BYTE  = 0x13,   // Combine up to 40 bytes
  MC80_OSPI_COMBINATION_FUNCTION_44BYTE  = 0x15,   // Combine up to 44 bytes
  MC80_OSPI_COMBINATION_FUNCTION_48BYTE  = 0x17,   // Combine up to 48 bytes
  MC80_OSPI_COMBINATION_FUNCTION_52BYTE  = 0x19,   // Combine up to 52 bytes
  MC80_OSPI_COMBINATION_FUNCTION_56BYTE  = 0x1B,   // Combine up to 56 bytes
  MC80_OSPI_COMBINATION_FUNCTION_60BYTE  = 0x1D,   // Combine up to 60 bytes
  MC80_OSPI_COMBINATION_FUNCTION_64BYTE  = 0x1F,   // Combine up to 64 bytes
  MC80_OSPI_COMBINATION_FUNCTION_2BYTE   = 0x1FF,  // Combine up to 2 bytes
} T_mc80_ospi_combination_function;

/*-----------------------------------------------------------------------------------------------------
  MC80 OSPI Protocol modes (custom implementation)
-----------------------------------------------------------------------------------------------------*/
typedef enum e_mc80_ospi_protocol
{
  MC80_OSPI_PROTOCOL_1S_1S_1S = 0x000,  // Standard 1-bit serial mode
  MC80_OSPI_PROTOCOL_1S_2S_2S = 0x048,  // Dual address and data mode
  MC80_OSPI_PROTOCOL_1S_4S_4S = 0x090,  // Quad address and data mode
  MC80_OSPI_PROTOCOL_2S_2S_2S = 0x049,  // Dual command, address and data mode
  MC80_OSPI_PROTOCOL_4S_4S_4S = 0x092,  // Quad command, address and data mode
  MC80_OSPI_PROTOCOL_4S_4D_4D = 0x3B2,  // 4S-4D-4D mode
  MC80_OSPI_PROTOCOL_8D_8D_8D = 0x3FF,  // OctalFlash DTR mode
} T_mc80_ospi_protocol;

/*-----------------------------------------------------------------------------------------------------
  MC80 OSPI Address bytes configuration
-----------------------------------------------------------------------------------------------------*/
typedef enum e_mc80_ospi_address_bytes
{
  MC80_OSPI_ADDRESS_BYTES_NONE = 0,  // No address bytes (not used)
  MC80_OSPI_ADDRESS_BYTES_3 = 2,     // 3 address bytes
  MC80_OSPI_ADDRESS_BYTES_4 = 3,     // 4 address bytes
} T_mc80_ospi_address_bytes;

/*-----------------------------------------------------------------------------------------------------
  MC80 OSPI Direct transfer direction
-----------------------------------------------------------------------------------------------------*/
typedef enum e_mc80_ospi_direct_transfer_dir
{
  MC80_OSPI_DIRECT_TRANSFER_DIR_READ  = 0,  // Read transfer
  MC80_OSPI_DIRECT_TRANSFER_DIR_WRITE = 1,  // Write transfer
} T_mc80_ospi_direct_transfer_dir;

/*-----------------------------------------------------------------------------------------------------
  MC80 OSPI Status structure
-----------------------------------------------------------------------------------------------------*/
typedef struct st_mc80_ospi_status
{
  bool write_in_progress;  // Write operation in progress flag
} T_mc80_ospi_status;

/*-----------------------------------------------------------------------------------------------------
  MC80 OSPI Calibration data structure - stores timing parameters before and after calibration
-----------------------------------------------------------------------------------------------------*/
typedef struct st_mc80_ospi_calibration_data
{
  struct
  {
    uint32_t wrapcfg_dssft;     // DQS shift value from WRAPCFG register (bits 12:8 for CS0, 28:24 for CS1)
    uint32_t liocfg_sdrsmpsft;  // SDR sampling shift from LIOCFGCS register (bits 27:24)
    uint32_t liocfg_ddrsmpex;   // DDR sampling extension from LIOCFGCS register (bits 31:28)
    uint32_t casttcs_value;     // Calibration status - success flags for OM_DQS shift values
  } before_calibration;         // Parameters before calibration

  struct
  {
    uint32_t wrapcfg_dssft;     // DQS shift value from WRAPCFG register (bits 12:8 for CS0, 28:24 for CS1)
    uint32_t liocfg_sdrsmpsft;  // SDR sampling shift from LIOCFGCS register (bits 27:24)
    uint32_t liocfg_ddrsmpex;   // DDR sampling extension from LIOCFGCS register (bits 31:28)
    uint32_t casttcs_value;     // Calibration status - success flags for OM_DQS shift values
  } after_calibration;          // Parameters after calibration

  bool     calibration_success; // Overall calibration result (true = success, false = failed)
  uint8_t  channel;             // OSPI channel (0 or 1)
} T_mc80_ospi_calibration_data;

/*-----------------------------------------------------------------------------------------------------
  MC80 OSPI Register Snapshot structure - captures all OSPI peripheral registers for debugging
-----------------------------------------------------------------------------------------------------*/
typedef struct st_mc80_ospi_register_snapshot
{
  // Metadata
  uint32_t timestamp;                    // RTOS timestamp when snapshot was taken
  uint8_t  channel;                      // Current active channel (0 or 1)
  T_mc80_ospi_protocol current_protocol; // Current protocol setting

  // Control and Configuration Registers
  uint32_t lioctl;                       // Line I/O Control Register
  uint32_t wrapcfg;                      // Wrap Configuration Register
  uint32_t comcfg;                       // Common Configuration Register
  uint32_t bmcfgch[2];                   // Bridge Map Configuration Register for CH0/CH1
  uint32_t bmctl0;                       // Bus Mode Control Register 0
  uint32_t bmctl1;                       // Bus Mode Control Register 1
  uint32_t abmcfg;                       // AXI Bridge Map Configuration Register

  // Channel Configuration Registers (Both Channels)
  uint32_t liocfgcs[2];                  // Line I/O Configuration Register for CS0/CS1

  struct
  {
    uint32_t cmcfg0;                     // Command Configuration Register 0
    uint32_t cmcfg1;                     // Command Configuration Register 1 (Read)
    uint32_t cmcfg2;                     // Command Configuration Register 2 (Write)
  } cmcfgcs[2];                          // Command Configuration for CS0/CS1

  // Calibration Control Registers (Both Channels)
  struct
  {
    uint32_t ccctl0;                     // Calibration Control Register 0
    uint32_t ccctl1;                     // Calibration Control Register 1
    uint32_t ccctl2;                     // Calibration Control Register 2
    uint32_t ccctl3;                     // Calibration Control Register 3 (Address)
    uint32_t ccctl4;                     // Calibration Control Register 4 (Pattern 0)
    uint32_t ccctl5;                     // Calibration Control Register 5 (Pattern 1)
    uint32_t ccctl6;                     // Calibration Control Register 6 (Pattern 2)
    uint32_t ccctl7;                     // Calibration Control Register 7 (Pattern 3)
  } ccctlcs[2];                          // Calibration Control for CS0/CS1

  // Status and Interrupt Registers
  uint32_t ints;                         // Interrupt Status Register
  uint32_t intc;                         // Interrupt Clear Register (write-only, captured for reference)
  uint32_t inte;                         // Interrupt Enable Register
  uint32_t comstt;                       // Communication Status Register
  uint32_t verstt;                       // Version Register

  // Calibration Status Registers
  uint32_t casttcs[2];                   // Calibration Status Register for CS0/CS1

  // Command Buffer Registers (Both Channels)
  struct
  {
    uint32_t cdt;                        // Command Data Transaction Register
    uint32_t cda;                        // Command Data Address Register
    uint32_t cdd0;                       // Command Data Register 0 (Lower 32 bits)
    uint32_t cdd1;                       // Command Data Register 1 (Upper 32 bits)
  } cdbuf[2];                            // Command Buffer for CS0/CS1

  // Manual Command Control
  uint32_t cdctl0;                       // Command Control Register 0
  uint32_t cdctl1;                       // Command Control Register 1
  uint32_t cdctl2;                       // Command Control Register 2

  // Link Pattern Control Registers
  uint32_t lpctl0;                       // Link Pattern Control Register 0
  uint32_t lpctl1;                       // Link Pattern Control Register 1

  // XIP Control Registers
  uint32_t cmctlch[2];                   // Command Control Channel Register for CH0/CH1
} T_mc80_ospi_register_snapshot;

/*-----------------------------------------------------------------------------------------------------
  MC80 OSPI Direct transfer structure
-----------------------------------------------------------------------------------------------------*/
typedef struct st_mc80_ospi_direct_transfer
{
  union
  {
    uint64_t data_u64;     // Data as 64-bit value
    uint8_t  data_bytes[8]; // Data as byte array
    uint32_t data;         // Data as 32-bit value (for compatibility)
  };
  uint32_t address;        // Address for the transfer
  uint16_t command;        // Command code
  uint8_t  command_length; // Number of command bytes (1 or 2)
  uint8_t  address_length; // Number of address bytes
  uint8_t  data_length;    // Number of data bytes
  uint8_t  dummy_cycles;   // Number of dummy cycles
} T_mc80_ospi_direct_transfer;

/*-----------------------------------------------------------------------------------------------------
  MC80 OSPI Erase command structure
-----------------------------------------------------------------------------------------------------*/
typedef struct st_mc80_ospi_erase_command
{
  uint32_t size;     // Size of the erase operation in bytes
  uint16_t command;  // Command code for this erase operation
} T_mc80_ospi_erase_command;

/*-----------------------------------------------------------------------------------------------------
  Simple array length table structure
-----------------------------------------------------------------------------------------------------*/
typedef struct st_mc80_ospi_table
{
  void*   p_table;  // Pointer to the table array
  uint8_t length;   // Number of entries in the table
} T_mc80_ospi_table;

/*-----------------------------------------------------------------------------------------------------
  Fixed timing configuration for bus signals
-----------------------------------------------------------------------------------------------------*/
typedef struct st_mc80_ospi_timing_setting
{
  T_mc80_ospi_command_interval_clocks    command_to_command_interval;  // Interval between 2 consecutive commands
  T_mc80_ospi_command_cs_pullup_clocks   cs_pullup_lag;                // Duration to de-assert CS line after the last command
  T_mc80_ospi_command_cs_pulldown_clocks cs_pulldown_lead;             // Duration to assert CS line before the first command
  T_mc80_ospi_sdr_drive_timing           sdr_drive_timing;             // Data signal timing relative to the rising-edge of the CK signal
  T_mc80_ospi_ck_edge                    sdr_sampling_edge;            // Selects the clock edge to sample the data signal
  T_mc80_ospi_sdr_sampling_delay         sdr_sampling_delay;           // Number of cycles to delay before sampling the data signal
  T_mc80_ospi_ddr_sampling_extension     ddr_sampling_extension;       // Number of cycles to extending the data sampling window in DDR mode
} T_mc80_ospi_timing_setting;

/*-----------------------------------------------------------------------------------------------------
  Command set used for a protocol mode
-----------------------------------------------------------------------------------------------------*/
typedef struct st_mc80_ospi_xspi_command_set
{
  T_mc80_ospi_protocol      protocol;              // Protocol mode associated with this command set
  T_mc80_ospi_frame_format  frame_format;          // Frame format to use for this command set
  T_mc80_ospi_latency_mode  latency_mode;          // Configurable or variable latency, only valid for MC80_OSPI_FRAME_FORMAT_XSPI_PROFILE_2 and MC80_OSPI_FRAME_FORMAT_XSPI_PROFILE_2_EXTENDED
  T_mc80_ospi_command_bytes command_bytes;         // Number of command bytes for each command code
  T_mc80_ospi_address_bytes address_bytes;         // Number of bytes used during the address phase
  uint16_t                  read_command;          // Read command
  uint16_t                  program_command;       // Memory program/write command
  uint16_t                  write_enable_command;  // Command to enable write or erase, set to 0x00 to ignore
  uint16_t                  status_command;        // Command to read the write status, set to 0x00 to ignore
  uint8_t                   read_dummy_cycles;     // Dummy cycles to be inserted for read commands
  uint8_t                   program_dummy_cycles;  // Dummy cycles to be inserted for page program commands
  uint8_t                   status_dummy_cycles;   // Dummy cycles to be inserted for status read commands
  uint8_t                   address_msb_mask;      // Mask of bits to zero when using memory-mapped operations; only applies to the most-significant byte
  bool                      status_needs_address;  // Indicates that reading the status register requires an address stage
  uint32_t                  status_address;        // Address to use for reading the status register with "busy" and "write-enable" flags
  T_mc80_ospi_address_bytes status_address_bytes;  // Number of bytes used for status register addressing
  T_mc80_ospi_table const*  p_erase_commands;      // List of all erase commands and associated sizes
} T_mc80_ospi_xspi_command_set;

/*-----------------------------------------------------------------------------------------------------
  MC80 OSPI Extended configuration
-----------------------------------------------------------------------------------------------------*/
typedef struct st_mc80_ospi_extended_cfg
{
  uint8_t                           ospi_unit;                                // The OSPI unit corresponding to the selected channel
  T_mc80_ospi_device_number         channel;                                  // Device number to be used for memory device
  T_mc80_ospi_timing_setting const* p_timing_settings;                        // Fixed protocol timing settings
  T_mc80_ospi_table const*          p_xspi_command_set;                       // Additional protocol command sets; if additional protocol commands set are not used set this to NULL
  T_mc80_ospi_ds_timing_delay       data_latch_delay_clocks;                  // Delay after assertion of the DS signal where data should be latched
  uint8_t*                          p_autocalibration_preamble_pattern_addr;  // OctaFlash memory address holding the preamble pattern
  uint32_t*                         p_autocalibration_preamble_patterns;      // Pointer to array of 4 preamble patterns for calibration

  transfer_instance_t const* p_lower_lvl_transfer;                            // DMA Transfer instance used for data transmission
} T_mc80_ospi_extended_cfg;

/*-----------------------------------------------------------------------------------------------------
  MC80 OSPI Configuration structure (replaces spi_flash_cfg_t)
-----------------------------------------------------------------------------------------------------*/
typedef struct st_mc80_ospi_cfg
{
  T_mc80_ospi_protocol              spi_protocol;         // SPI protocol to use for memory device operations
  uint32_t                          page_size_bytes;      // Page size for memory device
  uint8_t                           write_status_bit;     // Bit position for write-in-progress status
  uint8_t                           write_enable_bit;     // Bit position for write-enable status
  uint16_t                          xip_enter_command;    // Command to enter XIP mode
  uint16_t                          xip_exit_command;     // Command to exit XIP mode
  T_mc80_ospi_extended_cfg const*   p_extend;             // Extension configuration
} T_mc80_ospi_cfg;

/*-----------------------------------------------------------------------------------------------------
  Instance control block. DO NOT INITIALIZE.  Initialization occurs when Mc80_ospi_open is called
-----------------------------------------------------------------------------------------------------*/
typedef struct st_mc80_ospi_instance_ctrl
{
  T_mc80_ospi_cfg const*              p_cfg;         // Pointer to initial configuration
  uint32_t                            open;          // Whether or not driver is open
  T_mc80_ospi_protocol                spi_protocol;  // Current OSPI protocol selected
  T_mc80_ospi_device_number           channel;       // Device number to be used for memory device
  uint8_t                             ospi_unit;     // OSPI instance number
  T_mc80_ospi_xspi_command_set const* p_cmd_set;     // Command set for the active protocol mode
  R_XSPI0_Type*                       p_reg;         // Address for the OSPI peripheral associated with this channel
} T_mc80_ospi_instance_ctrl;

/*-----------------------------------------------------------------------------------------------------
  MC80 OSPI API structure - independent from FSP SPI Flash API
-----------------------------------------------------------------------------------------------------*/
typedef struct st_mc80_ospi_api
{
  fsp_err_t (* open)(T_mc80_ospi_instance_ctrl * const p_ctrl, T_mc80_ospi_cfg const * const p_cfg);
  fsp_err_t (* close)(T_mc80_ospi_instance_ctrl * const p_ctrl);
  fsp_err_t (* write)(T_mc80_ospi_instance_ctrl * const p_ctrl, uint8_t const * const p_src, uint8_t * const p_dest, uint32_t byte_count);
  fsp_err_t (* erase)(T_mc80_ospi_instance_ctrl * const p_ctrl, uint8_t * const p_device_address, uint32_t byte_count);
  fsp_err_t (* statusGet)(T_mc80_ospi_instance_ctrl * const p_ctrl, T_mc80_ospi_status * const p_status);
  fsp_err_t (* spiProtocolSet)(T_mc80_ospi_instance_ctrl * const p_ctrl, T_mc80_ospi_protocol spi_protocol);
  fsp_err_t (* xipEnter)(T_mc80_ospi_instance_ctrl * const p_ctrl);
  fsp_err_t (* xipExit)(T_mc80_ospi_instance_ctrl * const p_ctrl);
  fsp_err_t (* directWrite)(T_mc80_ospi_instance_ctrl * const p_ctrl, uint8_t const * const p_src, uint32_t const address, uint32_t const bytes, bool const read_after_write);
  fsp_err_t (* directRead)(T_mc80_ospi_instance_ctrl * const p_ctrl, uint8_t * const p_dest, uint32_t const address, uint32_t const bytes);
  fsp_err_t (* directTransfer)(T_mc80_ospi_instance_ctrl * const p_ctrl, T_mc80_ospi_direct_transfer * const p_transfer, T_mc80_ospi_direct_transfer_dir direction);
  fsp_err_t (* bankSet)(T_mc80_ospi_instance_ctrl * const p_ctrl, uint32_t bank);
} T_mc80_ospi_api;

/*-----------------------------------------------------------------------------------------------------
  MC80 OSPI Instance structure
-----------------------------------------------------------------------------------------------------*/
typedef struct st_mc80_ospi_instance
{
  T_mc80_ospi_instance_ctrl * p_ctrl;  // Pointer to the control structure
  T_mc80_ospi_cfg const *     p_cfg;   // Pointer to the configuration structure
  T_mc80_ospi_api const *     p_api;   // Pointer to the API function structure
} T_mc80_ospi_instance;

/*-----------------------------------------------------------------------------------------------------
  API function declarations
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_open(T_mc80_ospi_instance_ctrl* const p_ctrl, T_mc80_ospi_cfg const* const p_cfg);
fsp_err_t Mc80_ospi_close(T_mc80_ospi_instance_ctrl* const p_ctrl);
fsp_err_t Mc80_ospi_direct_write(T_mc80_ospi_instance_ctrl* const p_ctrl, uint8_t const* const p_src, uint32_t const address, uint32_t const bytes, bool const read_after_write);
fsp_err_t Mc80_ospi_direct_read(T_mc80_ospi_instance_ctrl* const p_ctrl, uint8_t* const p_dest, uint32_t const address, uint32_t const bytes);
fsp_err_t Mc80_ospi_direct_transfer(T_mc80_ospi_instance_ctrl* const p_ctrl, T_mc80_ospi_direct_transfer* const p_transfer, T_mc80_ospi_direct_transfer_dir direction);
fsp_err_t Mc80_ospi_spi_protocol_set(T_mc80_ospi_instance_ctrl* const p_ctrl, T_mc80_ospi_protocol spi_protocol);
fsp_err_t Mc80_ospi_xip_enter(T_mc80_ospi_instance_ctrl* const p_ctrl);
fsp_err_t Mc80_ospi_xip_exit(T_mc80_ospi_instance_ctrl* const p_ctrl);
fsp_err_t Mc80_ospi_memory_mapped_write(T_mc80_ospi_instance_ctrl* const p_ctrl, uint8_t const* const p_src, uint8_t* const p_dest, uint32_t byte_count);
fsp_err_t Mc80_ospi_erase(T_mc80_ospi_instance_ctrl* const p_ctrl, uint8_t* const p_device_address, uint32_t byte_count);
fsp_err_t Mc80_ospi_status_get(T_mc80_ospi_instance_ctrl* const p_ctrl, T_mc80_ospi_status* const p_status);
fsp_err_t Mc80_ospi_read_id(T_mc80_ospi_instance_ctrl* const p_ctrl, uint8_t* const p_id, uint32_t id_length);
fsp_err_t Mc80_ospi_bank_set(T_mc80_ospi_instance_ctrl* const p_ctrl, uint32_t bank);
fsp_err_t Mc80_ospi_auto_calibrate(T_mc80_ospi_instance_ctrl* const p_ctrl, T_mc80_ospi_calibration_data* const p_calibration_data);
fsp_err_t Mc80_ospi_hardware_reset(T_mc80_ospi_instance_ctrl* const p_ctrl);
fsp_err_t Mc80_ospi_memory_mapped_read(T_mc80_ospi_instance_ctrl* const p_ctrl, uint8_t* const p_dest, uint32_t const address, uint32_t const bytes);

// Debug and diagnostic functions
fsp_err_t Mc80_ospi_capture_register_snapshot(T_mc80_ospi_instance_ctrl* const p_ctrl, T_mc80_ospi_register_snapshot* const p_snapshot);

// DMA transfer control functions
void Mc80_ospi_dma_transfer_reset_flags(void);

// RTOS-based DMA synchronization functions (automatically initialized when driver opens)
fsp_err_t Mc80_ospi_dma_get_event_flags(TX_EVENT_FLAGS_GROUP **pp_event_flags);
fsp_err_t Mc80_ospi_dma_wait_for_completion(ULONG timeout_ticks);

// DMA event flag constants for task synchronization
#define OSPI_DMA_EVENT_TRANSFER_COMPLETE  (0x00000001UL)
#define OSPI_DMA_EVENT_TRANSFER_ERROR     (0x00000002UL)
#define OSPI_DMA_EVENT_ALL_EVENTS         (OSPI_DMA_EVENT_TRANSFER_COMPLETE | OSPI_DMA_EVENT_TRANSFER_ERROR)

#endif  // MC80_OSPI_DRV_H
