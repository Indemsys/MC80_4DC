#include "App.h"

// === OSPI Transfer (DMA) Configuration ===
#if OSPI_B_CFG_DMAC_SUPPORT_ENABLE
dmac_instance_ctrl_t g_transfer_OSPI_ctrl;
transfer_info_t g_transfer_OSPI_info = {
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
  { .command = 0x21, .size = 4096 },                             // 4KB Sector Erase with 4-byte address
  { .command = 0xDC, .size = 262144 },                           // 256KB Block Erase with 4-byte address
  { .command = 0x60, .size = SPI_FLASH_ERASE_SIZE_CHIP_ERASE },  // Chip Erase
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
   .protocol               = SPI_FLASH_PROTOCOL_1S_1S_1S,              // Standard SPI: команда, адрес и данные по одной линии
   .frame_format           = OSPI_B_FRAME_FORMAT_STANDARD,             // Стандартный SPI формат без расширений
   .latency_mode           = OSPI_B_LATENCY_MODE_FIXED,                // Фиксированная задержка (постоянные dummy cycles)
   .command_bytes          = OSPI_B_COMMAND_BYTES_1,                   // 1 байт команды (стандартные SPI команды)
   .address_bytes          = SPI_FLASH_ADDRESS_BYTES_4,                // 4 байта адреса (для 32MB памяти)
   .address_msb_mask       = 0xF0,                                     // Маска старших 4 бит адреса
   .status_needs_address   = false,                                    // Команда статуса НЕ требует адреса
   .status_address         = 0U,                                       // Адрес для статуса (не используется)
   .status_address_bytes   = (spi_flash_address_bytes_t)0U,            // Размер адреса статуса (не используется)
   .p_erase_commands       = &g_OSPI_command_set_initial_erase_table,  // Таблица команд стирания для SPI
   .read_command           = 0x0C,                                     // Fast Read с 4-байтным адресом
   .read_dummy_cycles      = 8,                                        // 8 пустых тактов между адресом и данными
   .program_command        = 0x12,                                     // Page Program с 4-байтным адресом
   .program_dummy_cycles   = 0,                                        // Без пустых тактов для записи
   .row_load_command       = 0x00,                                     // Команда загрузки строки (не используется)
   .row_load_dummy_cycles  = 0,                                        // Пустые такты для row load
   .row_store_command      = 0x00,                                     // Команда сохранения строки (не используется)
   .row_store_dummy_cycles = 0,                                        // Пустые такты для row store
   .write_enable_command   = 0x06,                                     // Write Enable (WREN)
   .status_command         = 0x05,                                     // Read Status Register (RDSR)
   .status_dummy_cycles    = 0,                                        // Без пустых тактов для статуса
  },
  {
   // Octal DDR mode (8D-8D-8D) configuration
   .protocol               = SPI_FLASH_PROTOCOL_8D_8D_8D,                 // Octal DDR: 8 линий с двойной скоростью
   .frame_format           = OSPI_B_FRAME_FORMAT_XSPI_PROFILE_1,          // Расширенный XSPI формат для высоких скоростей
   .latency_mode           = OSPI_B_LATENCY_MODE_FIXED,                   // Фиксированная задержка
   .command_bytes          = OSPI_B_COMMAND_BYTES_2,                      // 2 байта команды (дублированные для DDR)
   .address_bytes          = SPI_FLASH_ADDRESS_BYTES_4,                   // 4 байта адреса
   .address_msb_mask       = 0xF0,                                        // Маска старших битов адреса
   .status_needs_address   = true,                                        // Команда статуса ТРЕБУЕТ адрес в Octal DDR
   .status_address         = 0x00,                                        // Адрес для чтения статуса
   .status_address_bytes   = SPI_FLASH_ADDRESS_BYTES_4,                   // 4 байта для адреса статуса
   .p_erase_commands       = &g_OSPI_command_set_high_speed_erase_table,  // Пустая таблица (стирание только в SPI)
   .read_command           = 0xEE13,                                      // Octal DDR Read (0xEE дублированная)
   .read_dummy_cycles      = 20,                                          // 20 пустых тактов (критично для 200MHz)
   .program_command        = 0x12ED,                                      // Octal DDR Page Program
   .program_dummy_cycles   = 0,                                           // Без пустых тактов для записи
   .row_load_command       = 0x00,                                        // Не используется в MX25UM25645G
   .row_load_dummy_cycles  = 0,                                           // Не используется
   .row_store_command      = 0x00,                                        // Не используется в MX25UM25645G
   .row_store_dummy_cycles = 0,                                           // Не используется
   .write_enable_command   = 0x06F9,                                      // Write Enable (0x06 + инверсия F9)
   .status_command         = 0x05FA,                                      // Read Status (0x05 + инверсия FA)
   .status_dummy_cycles    = 4,                                           // 4 пустых такта для статуса в DDR
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
  .page_size_bytes            = 256,                             // Page size for MX25UM25645G
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
