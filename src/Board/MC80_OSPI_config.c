/*-----------------------------------------------------------------------------------------------------
  MC80 OSPI Configuration File

  This file contains the complete configuration for the MC80 OSPI (Octal Serial Peripheral Interface)
  driver, including timing settings, command sets, and protocol configurations.

-----------------------------------------------------------------------------------------------------*/

#include "App.h"
#include "mx25um25645g.h"
#include "MC80_OSPI_drv.h"

// External DMA callback function declaration
extern void OSPI_dma_callback(dmac_callback_args_t *p_args);

// === OSPI Transfer (DMA) Configuration ===
#if OSPI_B_CFG_DMAC_SUPPORT_ENABLE
dmac_instance_ctrl_t g_transfer_OSPI_ctrl;
transfer_info_t      g_transfer_OSPI_info = {
       .transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED,
       .transfer_settings_word_b.repeat_area    = TRANSFER_REPEAT_AREA_SOURCE,
       .transfer_settings_word_b.irq            = TRANSFER_IRQ_END,
       .transfer_settings_word_b.chain_mode     = TRANSFER_CHAIN_MODE_DISABLED,
       .transfer_settings_word_b.src_addr_mode  = TRANSFER_ADDR_MODE_INCREMENTED,
       .transfer_settings_word_b.size           = TRANSFER_SIZE_1_BYTE,
       .transfer_settings_word_b.mode           = TRANSFER_MODE_BLOCK,
       .p_dest                                  = (void *)NULL,
       .p_src                                   = (void const *)NULL,
       .num_blocks                              = 1,
       .length                                  = 64,
};

const dmac_extended_cfg_t g_transfer_OSPI_extend = {
  .offset          = 0,
  .src_buffer_size = 0,
  #if defined(VECTOR_NUMBER_DMAC3_INT)
  .irq = VECTOR_NUMBER_DMAC3_INT,
  #else
  .irq = FSP_INVALID_VECTOR,
  #endif
  .ipl               = (10),
  .channel           = 3,
  .p_callback        = OSPI_dma_callback,
  .p_context         = NULL,
  .activation_source = ELC_EVENT_NONE,
};

const transfer_cfg_t g_transfer_OSPI_cfg = {
  .p_info   = &g_transfer_OSPI_info,
  .p_extend = &g_transfer_OSPI_extend,
};

const transfer_instance_t g_transfer_OSPI = {
  .p_ctrl = &g_transfer_OSPI_ctrl,
  .p_cfg  = &g_transfer_OSPI_cfg,
  .p_api  = &g_transfer_on_dmac
};
#endif

// Control instance for MC80 OSPI
T_mc80_ospi_instance_ctrl g_OSPI_ctrl;

// OSPI timing settings
static T_mc80_ospi_timing_setting g_OSPI_timing_settings = {
  .command_to_command_interval = MC80_OSPI_COMMAND_INTERVAL_CLOCKS_2,
  .cs_pullup_lag               = MC80_OSPI_COMMAND_CS_PULLUP_CLOCKS_NO_EXTENSION,
  .cs_pulldown_lead            = MC80_OSPI_COMMAND_CS_PULLDOWN_CLOCKS_NO_EXTENSION,
  .sdr_drive_timing            = MC80_OSPI_SDR_DRIVE_TIMING_BEFORE_CK,
  .sdr_sampling_edge           = MC80_OSPI_CK_EDGE_FALLING,
  .sdr_sampling_delay          = MC80_OSPI_SDR_SAMPLING_DELAY_NONE,
  .ddr_sampling_extension      = MC80_OSPI_DDR_SAMPLING_EXTENSION_NONE,
};

// === Erase Commands Configuration ===

// Erase commands for Standard SPI mode (1S-1S-1S)
static const T_mc80_ospi_erase_command g_OSPI_command_set_initial_erase_commands[] = {
  { .command = MX25_CMD_SE4B, .size = MX25UM25645G_SECTOR_SIZE },       // 4KB Sector Erase with 4-byte address
  { .command = MX25_CMD_BE4B, .size = MX25UM25645G_BLOCK_SIZE },        // 64KB Block Erase with 4-byte address
  { .command = MX25_CMD_CE, .size = MC80_OSPI_ERASE_SIZE_CHIP_ERASE },  // Chip Erase
};

// Erase table for Standard SPI mode
const T_mc80_ospi_table g_OSPI_command_set_initial_erase_table = {
  .p_table = (void *)g_OSPI_command_set_initial_erase_commands,
  .length  = sizeof(g_OSPI_command_set_initial_erase_commands) / sizeof(g_OSPI_command_set_initial_erase_commands[0]),
};

// Erase commands for Octal DDR mode (8D-8D-8D)
static const T_mc80_ospi_erase_command g_OSPI_command_set_high_speed_erase_commands[] = {
  { .command = MX25_OPI_SE4B_DTR, .size = MX25UM25645G_SECTOR_SIZE },       // 4KB Sector Erase with 4-byte address (DTR)
  { .command = MX25_OPI_BE4B_DTR, .size = MX25UM25645G_BLOCK_SIZE },        // 64KB Block Erase with 4-byte address (DTR)
  { .command = MX25_OPI_CE_DTR, .size = MC80_OSPI_ERASE_SIZE_CHIP_ERASE },  // Chip Erase (DTR)
};

// Erase table for Octal DDR mode
const T_mc80_ospi_table g_OSPI_command_set_high_speed_erase_table = {
  .p_table = (void *)g_OSPI_command_set_high_speed_erase_commands,
  .length  = sizeof(g_OSPI_command_set_high_speed_erase_commands) / sizeof(g_OSPI_command_set_high_speed_erase_commands[0]),
};

// === Command Set Configuration ===

// Main command set table containing protocol-specific configurations for different SPI modes.
// Used by _Mc80_ospi_command_set_get() to find matching command set for current protocol.
// Referenced through g_OSPI_command_set.p_table in Mc80_ospi_spi_protocol_set() function.
// Contains two configurations: Standard SPI (1S-1S-1S) for initialization and Octal DDR (8D-8D-8D) for high-speed operation.
const T_mc80_ospi_xspi_command_set g_OSPI_command_set_table[] = {
  {
   // Standard SPI mode (1S-1S-1S) configuration
   .protocol             = MC80_OSPI_PROTOCOL_1S_1S_1S,              // Standard SPI: command, address and data on single line
   .frame_format         = MC80_OSPI_FRAME_FORMAT_STANDARD,          // Standard SPI format without extensions
   .latency_mode         = MC80_OSPI_LATENCY_MODE_FIXED,             // Fixed latency (constant dummy cycles)
   .command_bytes        = MC80_OSPI_COMMAND_BYTES_1,                // 1 byte command (standard SPI commands)
   .address_bytes        = MC80_OSPI_ADDRESS_BYTES_4,                // 4 byte address (for 32MB memory)
   .address_msb_mask     = 0xF0,                                     // Mask for upper 4 bits of address
   .status_needs_address = false,                                    // Status command does NOT require address
   .status_address       = 0U,                                       // Status address (not used)
   .status_address_bytes = MC80_OSPI_ADDRESS_BYTES_NONE,             // Status address size (not used)
   .p_erase_commands     = &g_OSPI_command_set_initial_erase_table,  // Erase commands table for SPI
   .read_command         = MX25_CMD_FAST_READ4B,                     // Fast Read with 4-byte address
   .read_dummy_cycles    = 8,                                        // 8 dummy cycles between address and data
   .program_command      = MX25_CMD_PP4B,                            // Page Program with 4-byte address
   .program_dummy_cycles = 0,                                        // No dummy cycles for write
   .write_enable_command = MX25_CMD_WREN,                            // Write Enable
   .status_command       = MX25_CMD_RDSR,                            // Read Status Register
   .status_dummy_cycles  = 0,                                        // No dummy cycles for status
  },
  {
   // Octal DDR mode (8D-8D-8D) configuration
   .protocol             = MC80_OSPI_PROTOCOL_8D_8D_8D,                 // Octal DDR: 8 lines with double data rate
   .frame_format         = MC80_OSPI_FRAME_FORMAT_XSPI_PROFILE_1,       // Extended XSPI format for high speeds
   .latency_mode         = MC80_OSPI_LATENCY_MODE_FIXED,                // Fixed latency
   .command_bytes        = MC80_OSPI_COMMAND_BYTES_2,                   // 2 byte command (duplicated for DDR)
   .address_bytes        = MC80_OSPI_ADDRESS_BYTES_4,                   // 4 byte address
   .address_msb_mask     = 0xF0,                                        // Address upper bits mask
   .status_needs_address = true,                                        // Status command REQUIRES address in Octal DDR
   .status_address       = 0x00,                                        // Address for status read
   .status_address_bytes = MC80_OSPI_ADDRESS_BYTES_4,                   // 4 bytes for status address
   .p_erase_commands     = &g_OSPI_command_set_high_speed_erase_table,  // Empty table (erase only in SPI)
   .read_command         = MX25_OPI_8READ_DTR,                          // Octal DDR Read
   .read_dummy_cycles    = 6,                                           // 6 dummy cycles for 66MHz OSPI
   .program_command      = MX25_OPI_PP4B_DTR,                           // Octal STR Page Program
   .program_dummy_cycles = 0,                                           // No dummy cycles for write
   .write_enable_command = MX25_OPI_WREN_DTR,                           // Write Enable
   .status_command       = MX25_OPI_RDSR_DTR,                           // Read Status
   .status_dummy_cycles  = 4,                                           // 4 dummy cycles for status in DDR
  }
};

// Command set table
const T_mc80_ospi_table g_OSPI_command_set = {
  .p_table = (void *)g_OSPI_command_set_table,
  .length  = 2
};

// Default preamble patterns for auto-calibration
static const uint32_t g_OSPI_preamble_patterns[4] = {
  MC80_OSPI_DEFAULT_PREAMBLE_PATTERN_0,  // 0xFFFF0000U
  MC80_OSPI_DEFAULT_PREAMBLE_PATTERN_1,  // 0x000800FFU
  MC80_OSPI_DEFAULT_PREAMBLE_PATTERN_2,  // 0x00FFF700U
  MC80_OSPI_DEFAULT_PREAMBLE_PATTERN_3   // 0xF700F708U
};

// === OSPI Extended Configuration ===
const T_mc80_ospi_extended_cfg g_OSPI_extended_cfg = {
  .ospi_unit                               = 0,                                     // OSPI unit number (0 or 1)
  .channel                                 = (T_mc80_ospi_device_number)0,          // Device channel number
  .p_autocalibration_preamble_pattern_addr = (uint8_t *)0x80000000,                 // Auto-calibration pattern address (XIP base)
  .p_autocalibration_preamble_patterns     = (uint32_t *)g_OSPI_preamble_patterns,  // Preamble patterns for calibration
  .p_timing_settings                       = &g_OSPI_timing_settings,               // Timing settings
  .p_xspi_command_set                      = &g_OSPI_command_set,                   // Command set table
  .data_latch_delay_clocks                 = MC80_OSPI_DS_TIMING_DELAY_15,          // Data latch delay
  .p_lower_lvl_transfer                    = &g_transfer_OSPI,                      // DMA transfer instance
};

// === MC80 OSPI Configuration ===
const T_mc80_ospi_cfg g_OSPI_cfg = {
  .spi_protocol      = MC80_OSPI_PROTOCOL_1S_1S_1S,  // Starting protocol (Standard SPI)
  .page_size_bytes   = MX25UM25645G_PAGE_SIZE,       // Page size for MX25UM25645G
  .write_status_bit  = 0,                            // Status register write protection bit
  .write_enable_bit  = 1,                            // Write enable bit position
  .xip_enter_command = 0,                            // XIP enter command (handled by driver)
  .xip_exit_command  = 0,                            // XIP exit command (handled by driver)
  .p_extend          = &g_OSPI_extended_cfg,         // Extended configuration
};

// MC80 OSPI API interface
const T_mc80_ospi_api g_mc80_ospi_api = {
  .open                  = Mc80_ospi_open,
  .close                 = Mc80_ospi_close,
  .write                 = Mc80_ospi_memory_mapped_write,
  .erase                 = Mc80_ospi_erase,
  .statusGet             = Mc80_ospi_status_get,
  .spiProtocolSet        = Mc80_ospi_spi_protocol_set,
  .spiProtocolSwitchSafe = Mc80_ospi_spi_protocol_switch_safe,
  .xipEnter              = Mc80_ospi_xip_enter,
  .xipExit               = Mc80_ospi_xip_exit,
  .directWrite           = Mc80_ospi_direct_write,
  .directRead            = Mc80_ospi_direct_read,
  .directTransfer        = Mc80_ospi_direct_transfer,
  .bankSet               = Mc80_ospi_bank_set,
};

// === MC80 OSPI Instance ===
const T_mc80_ospi_instance g_mc80_ospi = {
  .p_ctrl = &g_OSPI_ctrl,      // Control instance
  .p_cfg  = &g_OSPI_cfg,       // Configuration
  .p_api  = &g_mc80_ospi_api,  // MC80 OSPI API interface
};
