#ifndef MC80_OSPI_DRV_H
#define MC80_OSPI_DRV_H

// Required includes for basic types and SPI flash API
#include <stdint.h>
#include <stdbool.h>
#include "r_spi_flash_api.h"
#include "r_transfer_api.h"

// Configuration defines - embedded in header
#define MC80_OSPI_CFG_PARAM_CHECKING_ENABLE          (1)
#define MC80_OSPI_CFG_DMAC_SUPPORT_ENABLE            (1)
#define MC80_OSPI_CFG_XIP_SUPPORT_ENABLE             (1)
#define MC80_OSPI_CFG_AUTOCALIBRATION_SUPPORT_ENABLE (0)
#define MC80_OSPI_CFG_PREFETCH_FUNCTION              (1)
#define MC80_OSPI_CFG_COMBINATION_FUNCTION           MC80_OSPI_COMBINATION_FUNCTION_64BYTE
#define MC80_OSPI_CFG_DOTF_SUPPORT_ENABLE            (0)
#define MC80_OSPI_CFG_ROW_ADDRESSING_SUPPORT_ENABLE  (0)

// Maximum number of status polling checks after enabling memory writes
#define MC80_OSPI_MAX_WRITE_ENABLE_POLLING_LOOPS     (5)

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
  spi_flash_protocol_t      protocol;              // Protocol mode associated with this command set
  T_mc80_ospi_frame_format  frame_format;          // Frame format to use for this command set
  T_mc80_ospi_latency_mode  latency_mode;          // Configurable or variable latency, only valid for MC80_OSPI_FRAME_FORMAT_XSPI_PROFILE_2 and MC80_OSPI_FRAME_FORMAT_XSPI_PROFILE_2_EXTENDED
  T_mc80_ospi_command_bytes command_bytes;         // Number of command bytes for each command code
  spi_flash_address_bytes_t address_bytes;         // Number of bytes used during the address phase
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
  spi_flash_address_bytes_t status_address_bytes;  // Number of bytes used for status register addressing
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

  transfer_instance_t const* p_lower_lvl_transfer;                            // DMA Transfer instance used for data transmission
} T_mc80_ospi_extended_cfg;

/*-----------------------------------------------------------------------------------------------------
  Instance control block. DO NOT INITIALIZE.  Initialization occurs when Mc80_ospi_open is called
-----------------------------------------------------------------------------------------------------*/
typedef struct st_mc80_ospi_instance_ctrl
{
  spi_flash_cfg_t const*              p_cfg;         // Pointer to initial configuration
  uint32_t                            open;          // Whether or not driver is open
  spi_flash_protocol_t                spi_protocol;  // Current OSPI protocol selected
  T_mc80_ospi_device_number           channel;       // Device number to be used for memory device
  uint8_t                             ospi_unit;     // OSPI instance number
  T_mc80_ospi_xspi_command_set const* p_cmd_set;     // Command set for the active protocol mode
  R_XSPI0_Type*                       p_reg;         // Address for the OSPI peripheral associated with this channel
} T_mc80_ospi_instance_ctrl;

/*-----------------------------------------------------------------------------------------------------
  API function declarations
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_open(spi_flash_ctrl_t* const p_ctrl, spi_flash_cfg_t const* const p_cfg);
fsp_err_t Mc80_ospi_close(spi_flash_ctrl_t* const p_ctrl);
fsp_err_t Mc80_ospi_direct_write(spi_flash_ctrl_t* const p_ctrl, uint8_t const* const p_src, uint32_t const bytes, bool const read_after_write);
fsp_err_t Mc80_ospi_direct_read(spi_flash_ctrl_t* const p_ctrl, uint8_t* const p_dest, uint32_t const bytes);
fsp_err_t Mc80_ospi_direct_transfer(spi_flash_ctrl_t* const p_ctrl, spi_flash_direct_transfer_t* const p_transfer, spi_flash_direct_transfer_dir_t direction);
fsp_err_t Mc80_ospi_spi_protocol_set(spi_flash_ctrl_t* const p_ctrl, spi_flash_protocol_t spi_protocol);
fsp_err_t Mc80_ospi_xip_enter(spi_flash_ctrl_t* const p_ctrl);
fsp_err_t Mc80_ospi_xip_exit(spi_flash_ctrl_t* const p_ctrl);
fsp_err_t Mc80_ospi_write(spi_flash_ctrl_t* const p_ctrl, uint8_t const* const p_src, uint8_t* const p_dest, uint32_t byte_count);
fsp_err_t Mc80_ospi_erase(spi_flash_ctrl_t* const p_ctrl, uint8_t* const p_device_address, uint32_t byte_count);
fsp_err_t Mc80_ospi_status_get(spi_flash_ctrl_t* const p_ctrl, spi_flash_status_t* const p_status);
fsp_err_t Mc80_ospi_bank_set(spi_flash_ctrl_t* const p_ctrl, uint32_t bank);

#endif  // MC80_OSPI_DRV_H
