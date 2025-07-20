#include "App.h"
#include "mx25um25645g.h"

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
  .p_callback        = NULL,
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

// Control instance for OSPI_B
ospi_b_instance_ctrl_t g_OSPI_ctrl;

// OSPI timing settings
static ospi_b_timing_setting_t g_OSPI_timing_settings = {
  .command_to_command_interval = OSPI_B_COMMAND_INTERVAL_CLOCKS_2,
  .cs_pullup_lag               = OSPI_B_COMMAND_CS_PULLUP_CLOCKS_NO_EXTENSION,
  .cs_pulldown_lead            = OSPI_B_COMMAND_CS_PULLDOWN_CLOCKS_NO_EXTENSION,
  .sdr_drive_timing            = OSPI_B_SDR_DRIVE_TIMING_BEFORE_CK,
  .sdr_sampling_edge           = OSPI_B_CK_EDGE_FALLING,
  .sdr_sampling_delay          = OSPI_B_SDR_SAMPLING_DELAY_NONE,
  .ddr_sampling_extension      = OSPI_B_DDR_SAMPLING_EXTENSION_NONE,
};

// === Erase Commands Configuration ===

// Erase commands for Standard SPI mode (1S-1S-1S)
static const spi_flash_erase_command_t g_OSPI_command_set_initial_erase_commands[] = {
  { .command = MX25_CMD_SE4B, .size = MX25UM25645G_SECTOR_SIZE },       // 4KB Sector Erase with 4-byte address
  { .command = MX25_CMD_BE4B, .size = MX25UM25645G_BLOCK_SIZE },        // 64KB Block Erase with 4-byte address
  { .command = MX25_CMD_CE, .size = SPI_FLASH_ERASE_SIZE_CHIP_ERASE },  // Chip Erase
};

// Erase table for Standard SPI mode
const ospi_b_table_t g_OSPI_command_set_initial_erase_table = {
  .p_table = (void *)g_OSPI_command_set_initial_erase_commands,
  .length  = sizeof(g_OSPI_command_set_initial_erase_commands) / sizeof(g_OSPI_command_set_initial_erase_commands[0]),
};

// Erase commands for Octal DDR mode (8D-8D-8D) - empty (erase only in SPI mode)
static const spi_flash_erase_command_t g_OSPI_command_set_high_speed_erase_commands[] = {};

// Erase table for Octal DDR mode
const ospi_b_table_t g_OSPI_command_set_high_speed_erase_table                        = {
                         .p_table = (void *)g_OSPI_command_set_high_speed_erase_commands,
                         .length  = sizeof(g_OSPI_command_set_high_speed_erase_commands) / sizeof(g_OSPI_command_set_high_speed_erase_commands[0]),
};

// === Command Set Configuration ===

const ospi_b_xspi_command_set_t g_OSPI_command_set_table[] = {
  {
   // Standard SPI mode (1S-1S-1S) configuration
   .protocol               = SPI_FLASH_PROTOCOL_1S_1S_1S,              // Standard SPI: command, address and data on single line
   .frame_format           = OSPI_B_FRAME_FORMAT_STANDARD,             // Standard SPI format without extensions
   .latency_mode           = OSPI_B_LATENCY_MODE_FIXED,                // Fixed latency (constant dummy cycles)
   .command_bytes          = OSPI_B_COMMAND_BYTES_1,                   // 1 byte command (standard SPI commands)
   .address_bytes          = SPI_FLASH_ADDRESS_BYTES_4,                // 4 byte address (for 32MB memory)
   .address_msb_mask       = 0xF0,                                     // Mask for upper 4 bits of address
   .status_needs_address   = false,                                    // Status command does NOT require address
   .status_address         = 0U,                                       // Status address (not used)
   .status_address_bytes   = (spi_flash_address_bytes_t)0U,            // Status address size (not used)
   .p_erase_commands       = &g_OSPI_command_set_initial_erase_table,  // Erase commands table for SPI
   .read_command           = MX25_CMD_FAST_READ4B,                     // Fast Read with 4-byte address
   .read_dummy_cycles      = 8,                                        // 8 dummy cycles between address and data
   .program_command        = MX25_CMD_PP4B,                            // Page Program with 4-byte address
   .program_dummy_cycles   = 0,                                        // No dummy cycles for write
   .row_load_command       = 0x00,                                     // Row load command (not used)
   .row_load_dummy_cycles  = 0,                                        // Dummy cycles for row load
   .row_store_command      = 0x00,                                     // Row store command (not used)
   .row_store_dummy_cycles = 0,                                        // Dummy cycles for row store
   .write_enable_command   = MX25_CMD_WREN,                            // Write Enable
   .status_command         = MX25_CMD_RDSR,                            // Read Status Register
   .status_dummy_cycles    = 0,                                        // No dummy cycles for status
  },
  {
   // Octal DDR mode (8D-8D-8D) configuration
   .protocol               = SPI_FLASH_PROTOCOL_8D_8D_8D,                 // Octal DDR: 8 lines with double data rate
   .frame_format           = OSPI_B_FRAME_FORMAT_XSPI_PROFILE_1,          // Extended XSPI format for high speeds
   .latency_mode           = OSPI_B_LATENCY_MODE_FIXED,                   // Fixed latency
   .command_bytes          = OSPI_B_COMMAND_BYTES_2,                      // 2 byte command (duplicated for DDR)
   .address_bytes          = SPI_FLASH_ADDRESS_BYTES_4,                   // 4 byte address
   .address_msb_mask       = 0xF0,                                        // Address upper bits mask
   .status_needs_address   = true,                                        // Status command REQUIRES address in Octal DDR
   .status_address         = 0x00,                                        // Address for status read
   .status_address_bytes   = SPI_FLASH_ADDRESS_BYTES_4,                   // 4 bytes for status address
   .p_erase_commands       = &g_OSPI_command_set_high_speed_erase_table,  // Empty table (erase only in SPI)
   .read_command           = MX25_OPI_8READ_DTR,                          // Octal DDR Read
   .read_dummy_cycles      = 20,                                          // 20 dummy cycles (critical for 200MHz)
   .program_command        = MX25_OPI_PP4B_STR,                           // Octal STR Page Program
   .program_dummy_cycles   = 0,                                           // No dummy cycles for write
   .row_load_command       = 0x00,                                        // Not used in MX25UM25645G
   .row_load_dummy_cycles  = 0,                                           // Not used
   .row_store_command      = 0x00,                                        // Not used in MX25UM25645G
   .row_store_dummy_cycles = 0,                                           // Not used
   .write_enable_command   = MX25_OPI_WREN_STR,                           // Write Enable
   .status_command         = MX25_OPI_RDSR_STR,                           // Read Status
   .status_dummy_cycles    = 4,                                           // 4 dummy cycles for status in DDR
  }
};

// Command set table
const ospi_b_table_t g_OSPI_command_set = {
  .p_table = (void *)g_OSPI_command_set_table,
  .length  = 2
};

// === DOTF Configuration (if enabled) ===
#if OSPI_B_CFG_DOTF_SUPPORT_ENABLE
extern uint8_t g_ospi_dotf_iv[];
extern uint8_t g_ospi_dotf_key[];

static ospi_b_dotf_cfg_t g_ospi_dotf_cfg = {
  .key_type     = OSPI_B_DOTF_AES_KEY_TYPE_128,
  .format       = OSPI_B_DOTF_KEY_FORMAT_PLAINTEXT,
  .p_start_addr = (uint32_t *)0x90000000,
  .p_end_addr   = (uint32_t *)0x90001FFF,
  .p_key        = (uint32_t *)g_ospi_dotf_key,
  .p_iv         = (uint32_t *)g_ospi_dotf_iv,
};
#endif

// === OSPI Extended Configuration ===
const ospi_b_extended_cfg_t g_OSPI_extended_cfg = {
  .ospi_b_unit                             = 0,                          // OSPI unit number (0 or 1)
  .channel                                 = (ospi_b_device_number_t)0,  // Device channel number
  .p_autocalibration_preamble_pattern_addr = (uint8_t *)0x00,            // Auto-calibration pattern address
  .p_timing_settings                       = &g_OSPI_timing_settings,    // Timing settings
  .p_xspi_command_set                      = &g_OSPI_command_set,        // Command set table
#if OSPI_B_CFG_DMAC_SUPPORT_ENABLE
  .p_lower_lvl_transfer = &g_transfer_OSPI,                              // DMA transfer instance
#endif
#if OSPI_B_CFG_DOTF_SUPPORT_ENABLE
  .p_dotf_cfg = &g_ospi_dotf_cfg,                                        // DOTF configuration
#endif
#if OSPI_B_CFG_ROW_ADDRESSING_SUPPORT_ENABLE
  .row_index_bytes = 0xFF                                                // Row addressing (not used)
#endif
};

// === SPI Flash Configuration ===
const spi_flash_cfg_t g_OSPI_cfg = {
  .spi_protocol               = SPI_FLASH_PROTOCOL_1S_1S_1S,     // Starting protocol (Standard SPI)
  .read_mode                  = SPI_FLASH_READ_MODE_STANDARD,    // Unused by OSPI_B
  .address_bytes              = SPI_FLASH_ADDRESS_BYTES_4,       // 4-byte addressing for 32MB
  .dummy_clocks               = SPI_FLASH_DUMMY_CLOCKS_DEFAULT,  // Unused by OSPI_B
  .page_program_address_lines = (spi_flash_data_lines_t)0U,      // Unused by OSPI_B
  .page_size_bytes            = MX25UM25645G_PAGE_SIZE,          // Page size for MX25UM25645G
  .write_status_bit           = 0,                               // Status register write protection bit
  .write_enable_bit           = 1,                               // Write enable bit position
  .page_program_command       = 0,                               // OSPI_B uses command sets
  .write_enable_command       = 0,                               // OSPI_B uses command sets
  .status_command             = 0,                               // OSPI_B uses command sets
  .read_command               = 0,                               // OSPI_B uses command sets
#if OSPI_B_CFG_XIP_SUPPORT_ENABLE
  .xip_enter_command = 0,                                        // XIP enter command (handled by driver)
  .xip_exit_command  = 0,                                        // XIP exit command (handled by driver)
#else
  .xip_enter_command = 0U,
  .xip_exit_command  = 0U,
#endif
  .erase_command_list_length = 0U,                               // OSPI_B uses command sets
  .p_erase_command_list      = NULL,                             // OSPI_B uses command sets
  .p_extend                  = &g_OSPI_extended_cfg,             // Extended configuration
};

// === SPI Flash Instance ===
const spi_flash_instance_t g_OSPI = {
  .p_ctrl = &g_OSPI_ctrl,            // Control instance
  .p_cfg  = &g_OSPI_cfg,             // Configuration
  .p_api  = &g_ospi_b_on_spi_flash,  // API interface
};

// === RSIP Instance (if enabled) ===
#if defined OSPI_B_CFG_DOTF_PROTECTED_MODE_SUPPORT_ENABLE
rsip_instance_t const *const gp_rsip_instance = &RA_NOT_DEFINED;
#endif
