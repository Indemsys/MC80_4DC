/*-----------------------------------------------------------------------------------------------------
  Description: OSPI Flash memory testing terminal interface
               Provides direct access to OSPI flash operations for debugging

  Parameters:

  Return:
-----------------------------------------------------------------------------------------------------*/

#include "App.h"

// Test data sizes
#define OSPI_TEST_PATTERN_SIZE           256
#define OSPI_TEST_SECTOR_SIZE            4096

// Read operation sizes
#define OSPI_DIRECT_READ_SIZE            4096  // Size for direct read test
#define OSPI_MEMORY_MAPPED_READ_SIZE     4096  // Size for memory-mapped read test
#define OSPI_CONTINUOUS_READ_SIZE        128   // Size for continuous read test
#define OSPI_JEDEC_ID_SIZE               3     // JEDEC ID read size

// Write operation sizes
#define OSPI_MEMORY_MAPPED_WRITE_SIZE    256  // Size for memory-mapped write test
#define OSPI_SECTOR_WRITE_SIZE           256  // Size for sector test write chunks
#define OSPI_PREAMBLE_PATTERN_SIZE       256  // Size for preamble pattern write

// Display sizes
#define OSPI_DISPLAY_PREVIEW_SIZE        64            // Size for data preview display
#define OSPI_DISPLAY_MAX_SIZE            (128 * 1024)  // Maximum size for data display (128KB)

// Custom operation limits
#define OSPI_MAX_CUSTOM_SIZE             (1024 * 1024)  // Maximum 1MB for custom operations
#define OSPI_MIN_CUSTOM_SIZE             1              // Minimum 1 byte
#define OSPI_MAX_FLASH_ADDRESS           0x01FFFFFF     // 32MB flash size - 1
#define OSPI_DISPLAY_BYTES_PER_LINE      16             // Bytes per line for hex display

// Comprehensive test limits
#define OSPI_COMPREHENSIVE_TEST_MAX_SIZE (150 * 1024)  // Maximum 150KB for comprehensive test
#define OSPI_COMPREHENSIVE_TEST_MIN_SIZE 1             // Minimum 1 byte for comprehensive test
#define OSPI_MAX_ERASE_RETRIES           10            // Maximum number of erase retry attempts

// Data pattern types
typedef enum
{
  OSPI_PATTERN_CONSTANT = 1,
  OSPI_PATTERN_INCREMENT,
  OSPI_PATTERN_RANDOM
} T_ospi_pattern_type;

// Address and size alignment types for comprehensive test
typedef enum
{
  OSPI_ALIGNMENT_UNALIGNED = 1,  // Unaligned addresses and random sizes
  OSPI_ALIGNMENT_64_BYTE,        // 64-byte aligned addresses and sizes
  OSPI_ALIGNMENT_256_BYTE        // 256-byte aligned addresses and sizes
} T_ospi_alignment_type;

// OSPI operation results structure
typedef struct
{
  uint32_t read_time_us;
  uint32_t write_time_us;
  uint32_t erase_time_us;
  uint32_t last_bytes_transferred;
  uint32_t last_checksum;
  bool     results_valid;
} T_ospi_operation_results;

// OSPI operation settings structure
typedef struct
{
  uint32_t             address;
  uint32_t             size;
  T_ospi_pattern_type  pattern_type;
  uint8_t              pattern_value;
  T_mc80_ospi_protocol protocol;
  bool                 settings_valid;
} T_ospi_operation_settings;

// Comprehensive test result structure
typedef struct
{
  uint32_t test_number;
  uint32_t address;
  uint32_t size;
  uint32_t erase_time_us;
  uint32_t write_time_us;
  uint32_t read_time_us;
  uint32_t write_checksum;
  uint32_t read_checksum;
  bool     checksum_match;
  bool     test_passed;
} T_ospi_comprehensive_test_result;

// Fixed seeds for reproducible random number generation
#define OSPI_FIXED_SEED_ADDRESS 0x12345678UL  // Fixed seed for address generation

// Simple Linear Congruential Generator (LCG) for reproducible random numbers
// Uses same constants as Microsoft Visual C++ RAND_MAX=32767
static uint32_t g_ospi_prng_state = 1;

/*-----------------------------------------------------------------------------------------------------
  Description: Initialize PRNG with seed value

  Parameters: seed - Seed value for PRNG

  Return:
-----------------------------------------------------------------------------------------------------*/
static void _Ospi_prng_seed(uint32_t seed)
{
  g_ospi_prng_state = seed;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Generate next pseudo-random number using LCG algorithm

  Parameters: None

  Return: Pseudo-random number (0 to RAND_MAX)
-----------------------------------------------------------------------------------------------------*/
static uint32_t _Ospi_prng_rand(void)
{
  // Linear Congruential Generator: next = (a * seed + c) % m
  // Using constants from Numerical Recipes: a=1664525, c=1013904223, m=2^32
  g_ospi_prng_state = (1664525UL * g_ospi_prng_state + 1013904223UL);
  return (g_ospi_prng_state >> 16) & 0x7FFF;  // Return 15-bit value (0-32767)
}

/*-----------------------------------------------------------------------------------------------------
  Description: Generate 32-bit pseudo-random number

  Parameters: None

  Return: 32-bit pseudo-random number
-----------------------------------------------------------------------------------------------------*/
static uint32_t _Ospi_prng_rand32(void)
{
  // Combine two 16-bit values to get full 32-bit range
  uint32_t high = _Ospi_prng_rand() << 16;
  uint32_t low  = _Ospi_prng_rand();
  return high | low;
}

// Function declarations
void OSPI_test_info(uint8_t keycode);
void OSPI_test_custom_operations(uint8_t keycode);
void OSPI_test_comprehensive_memory(uint8_t keycode);
void OSPI_chip_erase_full(uint8_t keycode);

// Internal helper functions
static fsp_err_t             _Ospi_ensure_driver_open(void);
static T_mc80_ospi_protocol  _Ospi_select_protocol(void);
static T_ospi_alignment_type _Ospi_select_alignment_type(void);
static uint32_t              _Ospi_get_address_input(void);
static uint32_t              _Ospi_get_size_input(uint32_t max_size);
static T_ospi_pattern_type   _Ospi_get_pattern_type(void);
static uint8_t               _Ospi_get_pattern_value(void);
static void                  _Ospi_generate_pattern(uint8_t *buffer, uint32_t size, T_ospi_pattern_type type, uint8_t base_value);
static void                  _Ospi_display_data(uint8_t *data, uint32_t size, uint32_t start_address);
static void                  _Ospi_display_speed(uint32_t bytes, uint32_t time_us);
static void                  _Ospi_display_custom_menu(T_ospi_operation_settings *settings, T_ospi_operation_results *results);
static bool                  _Ospi_verify_write_data(uint8_t *original_data, uint32_t address, uint32_t size);
static bool                  _Ospi_compare_buffers_detailed(uint8_t *write_buffer, uint8_t *read_buffer, uint32_t size, uint32_t address);
static bool                  _Ospi_verify_erase_data(uint8_t *buffer, uint32_t address, uint32_t size);
static uint32_t              _Ospi_calculate_checksum(uint8_t *data, uint32_t size);

const T_VT100_Menu_item MENU_OSPI_ITEMS[] = {
  { '1', OSPI_test_info, 0 },
  { '2', OSPI_test_comprehensive_memory, 0 },
  { '3', OSPI_test_custom_operations, 0 },
  { '4', OSPI_chip_erase_full, 0 },
  { 'R', 0, 0 },
  { 0 }
};

const T_VT100_Menu MENU_OSPI = {
  "OSPI Flash Testing",
  "\033[5C OSPI Flash memory testing menu\r\n"
  "\033[5C <1> - Flash information & status\r\n"
  "\033[5C <2> - Comprehensive memory test (reproducible random sequences)\r\n"
  "\033[5C <3> - Custom operations (read/write/erase)\r\n"
  "\033[5C <4> - Full chip erase (32MB)\r\n"
  "\033[5C <R> - Return to previous menu\r\n",
  MENU_OSPI_ITEMS,
};

/*-----------------------------------------------------------------------------------------------------
  Description: Display custom operations menu with current settings and results

  Parameters: settings - Current operation settings
              results - Last operation results

  Return:
-----------------------------------------------------------------------------------------------------*/
static void _Ospi_display_custom_menu(T_ospi_operation_settings *settings, T_ospi_operation_results *results)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF(" ===== Custom OSPI Operations =====\n\r");

  // Display current settings
  MPRINTF("\n\r===== Current Settings =====\n\r");
  if (settings->settings_valid)
  {
    MPRINTF("Address                       : 0x%08X\n\r", settings->address);
    MPRINTF("Size                          : %u bytes\n\r", settings->size);

    MPRINTF("Pattern type                  : ");
    switch (settings->pattern_type)
    {
      case OSPI_PATTERN_CONSTANT:
        MPRINTF("Constant (0x%02X)\n\r", settings->pattern_value);
        break;
      case OSPI_PATTERN_INCREMENT:
        MPRINTF("Increment (start: 0x%02X)\n\r", settings->pattern_value);
        break;
      case OSPI_PATTERN_RANDOM:
        MPRINTF("Random (custom PRNG)\n\r");
        break;
    }

    MPRINTF("Protocol                      : ");
    switch (settings->protocol)
    {
      case MC80_OSPI_PROTOCOL_1S_1S_1S:
        MPRINTF("Standard SPI (1S-1S-1S)\n\r");
        break;
      case MC80_OSPI_PROTOCOL_8D_8D_8D:
        MPRINTF("Octal DDR (8D-8D-8D)\n\r");
        break;
      default:
        MPRINTF("Unknown\n\r");
        break;
    }
  }
  else
  {
    MPRINTF("No settings configured\n\r");
  }

  // Display last operation results
  MPRINTF("\n\r===== Last Operation Results =====\n\r");
  if (results->results_valid)
  {
    if (results->read_time_us > 0)
    {
      MPRINTF("Read time                     : %u us\n\r", results->read_time_us);
      MPRINTF("Read speed                    : ");
      _Ospi_display_speed(results->last_bytes_transferred, results->read_time_us);
    }

    if (results->write_time_us > 0)
    {
      MPRINTF("Write time                    : %u us\n\r", results->write_time_us);
      MPRINTF("Write speed                   : ");
      _Ospi_display_speed(results->last_bytes_transferred, results->write_time_us);
    }

    if (results->erase_time_us > 0)
    {
      MPRINTF("Erase time                    : %u us\n\r", results->erase_time_us);
      MPRINTF("Erase speed                   : ");
      _Ospi_display_speed(results->last_bytes_transferred, results->erase_time_us);
    }

    MPRINTF("Last checksum (CRC32)         : 0x%08X\n\r", results->last_checksum);
  }
  else
  {
    MPRINTF("No operations performed yet\n\r");
  }

  // Display menu options
  MPRINTF("\n\r===== Operations Menu =====\n\r");
  MPRINTF("  <1> - Configure address\n\r");
  MPRINTF("  <2> - Configure size\n\r");
  MPRINTF("  <3> - Configure pattern\n\r");
  MPRINTF("  <4> - Read operation (memory-mapped)\n\r");
  MPRINTF("  <5> - Direct read operation\n\r");
  MPRINTF("  <6> - Fast read benchmark\n\r");
  MPRINTF("  <7> - Write operation\n\r");
  MPRINTF("  <8> - Erase operation\n\r");
  MPRINTF("  <9> - Switch protocol\n\r");
  MPRINTF("  <R> - Return to main menu\n\r");
  MPRINTF("Choice: ");
}

/*-----------------------------------------------------------------------------------------------------
  Description: Display OSPI flash information and status

  Parameters: keycode - Input key code from VT100 terminal

  Return:
-----------------------------------------------------------------------------------------------------*/
void OSPI_test_info(uint8_t keycode)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF(" ===== OSPI Flash Information & Status =====\n\r");

  // Ensure OSPI driver is open
  fsp_err_t init_err = _Ospi_ensure_driver_open();
  if (init_err != FSP_SUCCESS)
  {
    MPRINTF("ERROR: Failed to ensure OSPI driver is open (0x%X)\n\r", init_err);
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  // Try to get flash status to verify communication
  T_mc80_ospi_status status;
  fsp_err_t          err = Mc80_ospi_status_get(g_mc80_ospi.p_ctrl, &status);

  if (err == FSP_SUCCESS)
  {
    MPRINTF("OSPI Flash Communication       : OK\n\r");
    MPRINTF("Write in progress              : %s\n\r", status.write_in_progress ? "YES" : "NO");
  }
  else
  {
    MPRINTF("OSPI Flash Communication       : FAILED (error: 0x%X)\n\r", err);
  }

  // Read JEDEC ID using RDID command
  MPRINTF("\n\r===== JEDEC ID Information =====\n\r");
  uint8_t jedec_id[OSPI_JEDEC_ID_SIZE] = { 0 };
  err                                  = Mc80_ospi_read_id(g_mc80_ospi.p_ctrl, jedec_id, OSPI_JEDEC_ID_SIZE);
  if (FSP_SUCCESS == err)
  {
    MPRINTF("JEDEC ID                       : 0x%02X 0x%02X 0x%02X\n\r", jedec_id[0], jedec_id[1], jedec_id[2]);

    MPRINTF("Manufacturer ID                : 0x%02X", jedec_id[0]);
    if (jedec_id[0] == 0xC2)
    {
      MPRINTF(" (Macronix)\n\r");
    }
    else
    {
      MPRINTF(" (Unknown)\n\r");
    }

    MPRINTF("Memory Type                    : 0x%02X", jedec_id[1]);
    if (jedec_id[1] == 0x80)
    {
      MPRINTF(" (Octal SPI Flash)\n\r");
    }
    else
    {
      MPRINTF(" (Unknown)\n\r");
    }

    MPRINTF("Memory Density                 : 0x%02X", jedec_id[2]);
    if (jedec_id[2] == 0x39)
    {
      MPRINTF(" (256 Mbit)\n\r");
    }
    else
    {
      MPRINTF(" (Unknown)\n\r");
    }
  }
  else
  {
    MPRINTF("JEDEC ID read                  : FAILED (error: 0x%X)\n\r", err);
  }

  MPRINTF("\n\rFlash Memory Specifications (MX25UM25645G):\n\r");
  MPRINTF("Total capacity                 : 256 Mbit (32 MB)\n\r");
  MPRINTF("Page size                      : 256 bytes\n\r");
  MPRINTF("Sector size                    : 4 KB\n\r");
  MPRINTF("Block size                     : 64 KB\n\r");
  MPRINTF("Operating voltage              : 1.8V\n\r");
  MPRINTF("Max read frequency             : 200 MHz (SDR)\n\r");
  MPRINTF("Max program time               : 700 µs (page)\n\r");
  MPRINTF("Max erase time                 : 300 ms (sector)\n\r");

  MPRINTF("\n\rOSPI Address Mapping:\n\r");
  MPRINTF("Memory-mapped base address     : 0x80000000\n\r");
  MPRINTF("Memory-mapped address range    : 0x80000000 - 0x81FFFFFF\n\r");
  MPRINTF("Total addressable              : 32 MB\n\r");

  // Additional status checks
  MPRINTF("\n\r===== Status Tests =====\n\r");

  // Test status read again for detailed info
  err = Mc80_ospi_status_get(g_mc80_ospi.p_ctrl, &status);
  if (err == FSP_SUCCESS)
  {
    MPRINTF("Status read                    : SUCCESS\n\r");
    MPRINTF("Write in progress              : %s\n\r", status.write_in_progress ? "YES" : "NO");
  }
  else
  {
    MPRINTF("Status read                    : FAILED\n\r");
    MPRINTF("Error code                     : 0x%X\n\r", err);
  }

  // Test SPI protocol setting
  MPRINTF("\n\rTesting SPI protocol setting...\n\r");
  err = Mc80_ospi_spi_protocol_set(g_mc80_ospi.p_ctrl, MC80_OSPI_PROTOCOL_8D_8D_8D);
  if (err == FSP_SUCCESS)
  {
    MPRINTF("SPI Protocol Set               : SUCCESS\n\r");
  }
  else
  {
    MPRINTF("SPI Protocol Set               : FAILED (error: 0x%X)\n\r", err);
  }

  MPRINTF("\n\rPress any key to continue...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
}

/*-----------------------------------------------------------------------------------------------------
  Description: Helper function to select OSPI protocol (1S-1S-1S or 8D-8D-8D)

  Parameters: None

  Return: Selected protocol or MC80_OSPI_PROTOCOL_1S_1S_1S if user cancels
-----------------------------------------------------------------------------------------------------*/
static T_mc80_ospi_protocol _Ospi_select_protocol(void)
{
  GET_MCBL;
  MPRINTF("\n\r===== Protocol Selection =====\n\r");
  MPRINTF("Select OSPI protocol:\n\r");
  MPRINTF("  <1> - Standard SPI (1S-1S-1S)\n\r");
  MPRINTF("  <2> - Octal DDR (8D-8D-8D)\n\r");
  MPRINTF("  <ESC> - Cancel\n\r");
  MPRINTF("Choice: ");

  uint8_t key = 0;
  if (WAIT_CHAR(&key, ms_to_ticks(30000)) != RES_OK)
  {
    MPRINTF("TIMEOUT - using Standard SPI (1S-1S-1S)\n\r");
    return MC80_OSPI_PROTOCOL_1S_1S_1S;
  }

  switch (key)
  {
    case '1':
      MPRINTF("1 - Standard SPI (1S-1S-1S)\n\r");
      return MC80_OSPI_PROTOCOL_1S_1S_1S;

    case '2':
      MPRINTF("2 - Octal DDR (8D-8D-8D)\n\r");
      return MC80_OSPI_PROTOCOL_8D_8D_8D;

    case VT100_ESC:
      MPRINTF("ESC - Cancelled\n\r");
      return MC80_OSPI_PROTOCOL_1S_1S_1S;  // Default to safe protocol

    default:
      MPRINTF("Invalid choice, using Standard SPI (1S-1S-1S)\n\r");
      return MC80_OSPI_PROTOCOL_1S_1S_1S;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Select alignment type for comprehensive test

  Parameters: None

  Return: Selected alignment type
-----------------------------------------------------------------------------------------------------*/
static T_ospi_alignment_type _Ospi_select_alignment_type(void)
{
  GET_MCBL;
  MPRINTF("\n\r===== Alignment Type Selection =====\n\r");
  MPRINTF("Select address and size alignment:\n\r");
  MPRINTF("  <1> - Unaligned addresses and random sizes\n\r");
  MPRINTF("  <2> - 64-byte aligned addresses and sizes\n\r");
  MPRINTF("  <3> - 256-byte aligned addresses and sizes\n\r");
  MPRINTF("  <ESC> - Cancel\n\r");
  MPRINTF("Choice: ");

  uint8_t key = 0;
  if (WAIT_CHAR(&key, ms_to_ticks(30000)) != RES_OK)
  {
    MPRINTF("TIMEOUT - using unaligned addresses\n\r");
    return OSPI_ALIGNMENT_UNALIGNED;
  }

  switch (key)
  {
    case '1':
      MPRINTF("1 - Unaligned addresses and odd sizes\n\r");
      return OSPI_ALIGNMENT_UNALIGNED;

    case '2':
      MPRINTF("2 - 64-byte aligned addresses and sizes\n\r");
      return OSPI_ALIGNMENT_64_BYTE;

    case '3':
      MPRINTF("3 - 256-byte aligned addresses and sizes\n\r");
      return OSPI_ALIGNMENT_256_BYTE;

    case VT100_ESC:
      MPRINTF("ESC - Cancelled\n\r");
      return OSPI_ALIGNMENT_UNALIGNED;  // Default to unaligned

    default:
      MPRINTF("Invalid choice, using unaligned addresses\n\r");
      return OSPI_ALIGNMENT_UNALIGNED;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Ensure OSPI driver is open, open it if not already open

  Parameters: None

  Return: FSP_SUCCESS if driver is open, error code otherwise
-----------------------------------------------------------------------------------------------------*/
static fsp_err_t _Ospi_ensure_driver_open(void)
{
  fsp_err_t err;

  // Try to open the driver - if already open, will return FSP_ERR_ALREADY_OPEN
  err = Mc80_ospi_open(g_mc80_ospi.p_ctrl, g_mc80_ospi.p_cfg);

  if (err == FSP_SUCCESS)
  {
    // Perform hardware reset of OSPI flash memory to ensure known state
    err = Mc80_ospi_hardware_reset(g_mc80_ospi.p_ctrl);
    if (err != FSP_SUCCESS)
    {
      return err;
    }

    // After hardware reset, flash memory is in Standard SPI mode (1S-1S-1S)
    // Set driver protocol to match the flash device state
    err = Mc80_ospi_spi_protocol_set(g_mc80_ospi.p_ctrl, MC80_OSPI_PROTOCOL_1S_1S_1S);
    if (err != FSP_SUCCESS)
    {
      return err;
    }

    return FSP_SUCCESS;
  }
  else if (err == FSP_ERR_ALREADY_OPEN)
  {
    // Driver was already open - perform reset to ensure known state
    err = Mc80_ospi_hardware_reset(g_mc80_ospi.p_ctrl);
    if (err != FSP_SUCCESS)
    {
      return err;
    }

    // After hardware reset, flash memory is in Standard SPI mode (1S-1S-1S)
    // Set driver protocol to match the flash device state
    err = Mc80_ospi_spi_protocol_set(g_mc80_ospi.p_ctrl, MC80_OSPI_PROTOCOL_1S_1S_1S);
    if (err != FSP_SUCCESS)
    {
      return err;
    }

    return FSP_SUCCESS;
  }
  else
  {
    return err;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Full chip erase operation - erases entire 32MB flash memory in 64KB blocks

  This function performs complete flash memory erase using 64KB block erase operations.
  WARNING: This operation will destroy ALL data on the flash chip and cannot be undone.
  The operation may take several minutes to complete due to the large size (32MB).

  Features:
  - Erases chip in 64KB blocks for optimal performance
  - Real-time progress display with statistics
  - Tracks minimum, maximum, and average erase times per block
  - Calculates erase speed statistics in KB/s
  - Static display updates (no scrolling)

  Parameters: keycode - Key code (unused)

  Return: None
-----------------------------------------------------------------------------------------------------*/
void OSPI_chip_erase_full(uint8_t keycode)
{
  GET_MCBL;

  // Clear screen and display header
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF(" ===== OSPI Full Chip Erase (64KB Blocks) =====\n\r");
  MPRINTF("\n\r");

  // Display warning message
  MPRINTF("WARNING: This operation will erase the ENTIRE flash chip (32MB)!\n\r");
  MPRINTF("ALL data will be permanently lost and cannot be recovered.\n\r");
  MPRINTF("This operation may take several minutes to complete.\n\r");
  MPRINTF("Erase method: 64KB blocks for optimal performance\n\r");
  MPRINTF("\n\r");

  // Confirm operation
  MPRINTF("Are you absolutely sure you want to proceed? (Y/N): ");
  uint8_t confirm_key = 0;
  WAIT_CHAR(&confirm_key, ms_to_ticks(30000));
  MPRINTF("%c\n\r", confirm_key);

  if (confirm_key != 'Y' && confirm_key != 'y')
  {
    MPRINTF("Operation cancelled by user\n\r");
    MPRINTF("Press any key to return to menu...");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(10000));
    return;
  }

  // Final confirmation
  MPRINTF("\n\rFinal confirmation - Type 'ERASE' to proceed: ");
  char confirm_text[16];
  uint8_t i = 0;

  while (i < sizeof(confirm_text) - 1)
  {
    uint8_t ch = 0;
    if (WAIT_CHAR(&ch, ms_to_ticks(30000)) != RES_OK)
    {
      break;  // Timeout, exit loop
    }
    if (ch == '\r' || ch == '\n')
    {
      break;
    }
    if (ch == '\b' && i > 0)  // Backspace
    {
      i--;
      MPRINTF("\b \b");
      continue;
    }
    if (ch >= 32 && ch <= 126)  // Printable characters
    {
      confirm_text[i] = ch;
      MPRINTF("%c", ch);
      i++;
    }
  }
  confirm_text[i] = '\0';
  MPRINTF("\n\r");

  if (strcmp(confirm_text, "ERASE") != 0)
  {
    MPRINTF("Incorrect confirmation text. Operation cancelled.\n\r");
    MPRINTF("Press any key to return to menu...");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(10000));
    return;
  }

  // Ensure driver is open
  fsp_err_t err = _Ospi_ensure_driver_open();
  if (err != FSP_SUCCESS)
  {
    MPRINTF("ERROR: Failed to initialize OSPI driver (error: 0x%X)\n\r", err);
    MPRINTF("Press any key to return to menu...");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(10000));
    return;
  }

  // Calculate erase parameters
  const uint32_t block_size = MX25UM25645G_BLOCK_SIZE;  // 64KB
  const uint32_t total_blocks = MX25UM25645G_TOTAL_SIZE / block_size;  // 512 blocks

  // Statistics variables
  uint32_t min_erase_time_us = UINT32_MAX;
  uint32_t max_erase_time_us = 0;
  uint64_t total_erase_time_us = 0;
  uint32_t min_speed_kbps = UINT32_MAX;
  uint32_t max_speed_kbps = 0;
  uint32_t blocks_erased = 0;

  MPRINTF("\n\r===== Starting 64KB Block Erase =====\n\r");
  MPRINTF("Flash size: %d MB (%d bytes)\n\r", MX25UM25645G_TOTAL_SIZE / (1024 * 1024), MX25UM25645G_TOTAL_SIZE);
  MPRINTF("Block size: %d KB (%d bytes)\n\r", block_size / 1024, block_size);
  MPRINTF("Total blocks: %d\n\r", total_blocks);
  MPRINTF("\n\r");

  // Setup static display area (save cursor position)
  MPRINTF("Progress: [                    ] 0%% (0/%d blocks)\n\r", total_blocks);
  MPRINTF("Current block: 0x00000000 - 0x0000FFFF\n\r");
  MPRINTF("Block erase time: --- ms\n\r");
  MPRINTF("Block erase speed: --- KB/s\n\r");
  MPRINTF("\n\r");
  MPRINTF("=== Statistics ===\n\r");
  MPRINTF("Min erase time: --- ms\n\r");
  MPRINTF("Max erase time: --- ms\n\r");
  MPRINTF("Avg erase time: --- ms\n\r");
  MPRINTF("Min erase speed: --- KB/s\n\r");
  MPRINTF("Max erase speed: --- KB/s\n\r");

  // Record total start time
  uint32_t total_start_time = tx_time_get();

  // Erase each 64KB block
  for (uint32_t block_num = 0; block_num < total_blocks; block_num++)
  {
    uint32_t block_address = block_num * block_size;

    // Record block start time
    uint32_t block_start_time = tx_time_get();

    // Perform block erase
    err = Mc80_ospi_erase(g_mc80_ospi.p_ctrl,
                         (uint8_t *)(MC80_OSPI_DEVICE_0_START_ADDRESS + block_address),
                         block_size);

    // Calculate block erase time
    uint32_t block_end_time = tx_time_get();
    uint32_t block_erase_ticks = block_end_time - block_start_time;
    uint32_t block_erase_ms = TICKS_TO_MS(block_erase_ticks);
    uint32_t block_erase_us = block_erase_ms * 1000;

    if (err != FSP_SUCCESS)
    {
      MPRINTF("\n\r\n\rERROR: Failed to erase block %d at address 0x%08X (error: 0x%X)\n\r",
              block_num, block_address, err);
      MPRINTF("Press any key to return to menu...");
      uint8_t dummy_key;
      WAIT_CHAR(&dummy_key, ms_to_ticks(10000));
      return;
    }

    blocks_erased++;

    // Calculate block erase speed (KB/s)
    uint32_t block_speed_kbps = 0;
    if (block_erase_us > 0)
    {
      block_speed_kbps = ((block_size / 1024) * 1000000) / block_erase_us;
    }

    // Update statistics
    if (block_erase_us < min_erase_time_us)
      min_erase_time_us = block_erase_us;
    if (block_erase_us > max_erase_time_us)
      max_erase_time_us = block_erase_us;
    total_erase_time_us += block_erase_us;

    if (block_speed_kbps < min_speed_kbps)
      min_speed_kbps = block_speed_kbps;
    if (block_speed_kbps > max_speed_kbps)
      max_speed_kbps = block_speed_kbps;

    // Calculate progress
    uint32_t progress_percent = ((block_num + 1) * 100) / total_blocks;
    uint32_t progress_chars = ((block_num + 1) * 20) / total_blocks;

    // Calculate average erase time
    uint32_t avg_erase_time_us = (uint32_t)(total_erase_time_us / blocks_erased);

    // Update static display (move cursor back and overwrite)
    MPRINTF("\033[11A");  // Move cursor up 11 lines to overwrite display

    // Progress bar
    MPRINTF("Progress: [");
    for (uint32_t j = 0; j < 20; j++)
    {
      if (j < progress_chars)
        MPRINTF("=");
      else
        MPRINTF(" ");
    }
    MPRINTF("] %d%% (%d/%d blocks)\n\r", progress_percent, block_num + 1, total_blocks);

    // Current block info
    uint32_t block_end_addr = block_address + block_size - 1;
    MPRINTF("Current block: 0x%08X - 0x%08X\n\r", block_address, block_end_addr);
    MPRINTF("Block erase time: %d ms\n\r", block_erase_ms);
    MPRINTF("Block erase speed: %d KB/s\n\r", block_speed_kbps);
    MPRINTF("\n\r");

    // Statistics
    MPRINTF("=== Statistics ===\n\r");
    MPRINTF("Min erase time: %d ms\n\r", min_erase_time_us / 1000);
    MPRINTF("Max erase time: %d ms\n\r", max_erase_time_us / 1000);
    MPRINTF("Avg erase time: %d ms\n\r", avg_erase_time_us / 1000);
    MPRINTF("Min erase speed: %d KB/s\n\r", min_speed_kbps);
    MPRINTF("Max erase speed: %d KB/s\n\r", max_speed_kbps);
  }

  // Calculate total elapsed time
  uint32_t total_end_time = tx_time_get();
  uint32_t total_elapsed_ticks = total_end_time - total_start_time;
  uint32_t total_elapsed_ms = TICKS_TO_MS(total_elapsed_ticks);
  uint32_t total_elapsed_seconds = total_elapsed_ms / 1000;
  uint32_t total_elapsed_minutes = total_elapsed_seconds / 60;

  MPRINTF("\n\r\n\r===== Block Erase Results =====\n\r");

  if (blocks_erased == total_blocks)
  {
    MPRINTF("Block erase operation         : SUCCESS\n\r");
    MPRINTF("Total blocks erased           : %d / %d\n\r", blocks_erased, total_blocks);
    MPRINTF("Total size erased             : %d MB (%d bytes)\n\r",
            MX25UM25645G_TOTAL_SIZE / (1024 * 1024), MX25UM25645G_TOTAL_SIZE);
    MPRINTF("Total elapsed time            : %d minutes %d seconds (%d ms)\n\r",
            total_elapsed_minutes, total_elapsed_seconds % 60, total_elapsed_ms);

    // Calculate overall erase speed
    if (total_elapsed_ms > 0)
    {
      uint32_t overall_speed_kbps = (MX25UM25645G_TOTAL_SIZE / 1024) * 1000 / total_elapsed_ms;
      MPRINTF("Overall erase speed           : %d KB/s\n\r", overall_speed_kbps);
    }

    MPRINTF("\n\r=== Final Statistics ===\n\r");
    MPRINTF("Minimum block erase time      : %d ms\n\r", min_erase_time_us / 1000);
    MPRINTF("Maximum block erase time      : %d ms\n\r", max_erase_time_us / 1000);
    MPRINTF("Average block erase time      : %d ms\n\r", (uint32_t)(total_erase_time_us / blocks_erased) / 1000);
    MPRINTF("Minimum block erase speed     : %d KB/s\n\r", min_speed_kbps);
    MPRINTF("Maximum block erase speed     : %d KB/s\n\r", max_speed_kbps);

    // Verification phase - check that entire chip is erased
    MPRINTF("\n\r===== Erase Verification =====\n\r");
    MPRINTF("Verifying that entire chip is erased...\n\r");
    MPRINTF("This will read all 32MB and verify data is 0xFF.\n\r");
    MPRINTF("Please wait, this may take a few minutes...\n\r");

    // Use 128KB verification buffer for optimal memory usage and efficiency
    const uint32_t verify_buffer_size = 128 * 1024;  // 128KB buffer
    uint8_t *verify_buffer = (uint8_t *)App_malloc(verify_buffer_size);

    if (verify_buffer != NULL)
    {
      bool     verification_passed = true;
      uint32_t total_verified      = 0;
      uint32_t error_count         = 0;
      uint32_t first_error_address = 0;
      uint32_t verify_start_time   = tx_time_get();

      // Verify entire chip in 128KB chunks
      for (uint32_t offset = 0; offset < MX25UM25645G_TOTAL_SIZE && verification_passed; offset += verify_buffer_size)
      {
        uint32_t current_chunk_size = verify_buffer_size;
        if ((offset + verify_buffer_size) > MX25UM25645G_TOTAL_SIZE)
        {
          current_chunk_size = MX25UM25645G_TOTAL_SIZE - offset;
        }

        // Read current chunk
        fsp_err_t read_err = Mc80_ospi_memory_mapped_read(g_mc80_ospi.p_ctrl, verify_buffer, offset, current_chunk_size);
        if (read_err != FSP_SUCCESS)
        {
          MPRINTF("ERROR: Failed to read verification data at offset 0x%08X (error: 0x%X)\n\r", offset, read_err);
          verification_passed = false;
          break;
        }

        // Verify all bytes in current chunk are 0xFF
        for (uint32_t i = 0; i < current_chunk_size; i++)
        {
          if (verify_buffer[i] != 0xFF)
          {
            if (error_count == 0)
            {
              first_error_address = offset + i;  // Remember first error location
            }
            error_count++;

            // Stop verification after finding too many errors (likely systematic failure)
            if (error_count >= 1000)
            {
              verification_passed = false;
              break;
            }
          }
        }

        total_verified += current_chunk_size;

        // Progress indicator every 4MB
        if ((offset % (4 * 1024 * 1024)) == 0)
        {
          uint32_t progress_percent = (offset * 100) / MX25UM25645G_TOTAL_SIZE;
          MPRINTF("Verification progress: %d%% (%d MB / %d MB)\n\r",
                  progress_percent, offset / (1024 * 1024), MX25UM25645G_TOTAL_SIZE / (1024 * 1024));
        }
      }

      uint32_t verify_end_time     = tx_time_get();
      uint32_t verify_elapsed_ticks = verify_end_time - verify_start_time;
      uint32_t verify_elapsed_ms   = TICKS_TO_MS(verify_elapsed_ticks);
      uint32_t verify_seconds      = verify_elapsed_ms / 1000;
      uint32_t verify_minutes      = verify_seconds / 60;

      // Display verification results
      MPRINTF("\n\r===== Verification Results =====\n\r");
      MPRINTF("Verification time             : %d minutes %d seconds (%d ms)\n\r",
              verify_minutes, verify_seconds % 60, verify_elapsed_ms);
      MPRINTF("Data verified                 : %d MB (%d bytes)\n\r",
              total_verified / (1024 * 1024), total_verified);

      if (verify_elapsed_ms > 0)
      {
        uint32_t verify_speed_kbps = (total_verified / 1024) * 1000 / verify_elapsed_ms;
        MPRINTF("Verification read speed       : %d KB/s\n\r", verify_speed_kbps);
      }

      if (verification_passed && error_count == 0)
      {
        MPRINTF("Erase verification            : SUCCESS\n\r");
        MPRINTF("All %d MB verified as 0xFF    : CONFIRMED\n\r", MX25UM25645G_TOTAL_SIZE / (1024 * 1024));
        MPRINTF("\n\rFull chip erase and verification completed successfully!\n\r");
        MPRINTF("The entire flash chip is confirmed to be in erased state (0xFF).\n\r");
      }
      else
      {
        MPRINTF("Erase verification            : FAILED\n\r");
        MPRINTF("Error count                   : %d bytes\n\r", error_count);
        if (error_count > 0)
        {
          MPRINTF("First error at address        : 0x%08X\n\r", first_error_address);
        }
        MPRINTF("\n\rWARNING: Erase verification failed!\n\r");
        MPRINTF("Some areas of the flash chip are not properly erased.\n\r");
        MPRINTF("You may need to retry the erase operation.\n\r");
      }

      App_free(verify_buffer);
    }
    else
    {
      MPRINTF("ERROR: Failed to allocate %d KB for verification buffer\n\r", verify_buffer_size / 1024);
      MPRINTF("Skipping erase verification due to memory allocation failure\n\r");
      MPRINTF("\n\rThe erase operation completed, but verification was skipped.\n\r");
      MPRINTF("Manual verification is recommended.\n\r");
    }
  }
  else
  {
    MPRINTF("Block erase operation         : INCOMPLETE\n\r");
    MPRINTF("Blocks erased                 : %d / %d\n\r", blocks_erased, total_blocks);
    MPRINTF("Total elapsed time            : %d minutes %d seconds (%d ms)\n\r",
            total_elapsed_minutes, total_elapsed_seconds % 60, total_elapsed_ms);
    MPRINTF("\n\rThe erase operation was incomplete. Flash memory state is unknown.\n\r");
    MPRINTF("You may need to retry the operation or check hardware connections.\n\r");
  }

  MPRINTF("\n\rPress any key to return to menu...");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
}

/*-----------------------------------------------------------------------------------------------------
  Description: Custom OSPI operations menu - read, write, erase with user-defined parameters

  Parameters: keycode - Input key code from VT100 terminal

  Return:
-----------------------------------------------------------------------------------------------------*/
void OSPI_test_custom_operations(uint8_t keycode)
{
  GET_MCBL;

  // Static variables to preserve settings and results between calls
  static T_ospi_operation_settings settings = { 0 };
  static T_ospi_operation_results  results  = { 0 };

  // Initialize default settings on first run
  if (!settings.settings_valid)
  {
    settings.address        = 0x00000000;
    settings.size           = 4096;
    settings.pattern_type   = OSPI_PATTERN_CONSTANT;
    settings.pattern_value  = 0x55;
    settings.protocol       = MC80_OSPI_PROTOCOL_1S_1S_1S;
    settings.settings_valid = true;
  }

  // Ensure OSPI driver is open
  fsp_err_t init_err = _Ospi_ensure_driver_open();
  if (init_err != FSP_SUCCESS)
  {
    MPRINTF(VT100_CLEAR_AND_HOME);
    MPRINTF("ERROR: Failed to ensure OSPI driver is open (0x%X)\n\r", init_err);
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  while (1)
  {
    _Ospi_display_custom_menu(&settings, &results);

    uint8_t operation = 0;
    if (WAIT_CHAR(&operation, ms_to_ticks(30000)) != RES_OK)
    {
      continue;  // Skip processing on timeout
    }
    MPRINTF("%c\n\r", operation);

    switch (operation)
    {
      case '1':  // Configure address
      {
        MPRINTF("\n\r===== Configure Address =====\n\r");
        MPRINTF("Current address: 0x%08X\n\r", settings.address);
        MPRINTF("Press ENTER to edit address, ESC to keep current: ");

        uint8_t key = 0;
        if (WAIT_CHAR(&key, ms_to_ticks(30000)) != RES_OK)
        {
          MPRINTF("TIMEOUT - keeping current address\n\r");
        }
        else if (key == '\r' || key == '\n')
        {
          MPRINTF("ENTER - entering address edit mode\n\r");
          uint32_t new_address = _Ospi_get_address_input();
          if (new_address <= OSPI_MAX_FLASH_ADDRESS)
          {
            // Auto-align address to even boundary for 8D-8D-8D protocol
            if (settings.protocol == MC80_OSPI_PROTOCOL_8D_8D_8D)
            {
              new_address &= 0xFFFFFFFE;  // Clear LSB to make address even
            }
            settings.address = new_address;
            MPRINTF("Address updated successfully\n\r");
          }
          else
          {
            MPRINTF("ERROR: Address exceeds flash size, keeping current\n\r");
          }
        }
        else if (key == VT100_ESC)
        {
          MPRINTF("ESC - keeping current address\n\r");
        }
        else
        {
          MPRINTF("Invalid key - keeping current address\n\r");
        }

        MPRINTF("Press any key to continue...\n\r");
        uint8_t dummy_key;
        WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
        break;
      }

      case '2':  // Configure size
      {
        MPRINTF("\n\r===== Configure Size =====\n\r");
        MPRINTF("Current size: %u bytes\n\r", settings.size);
        MPRINTF("Press ENTER to edit size, ESC to keep current: ");

        uint8_t key = 0;
        if (WAIT_CHAR(&key, ms_to_ticks(30000)) != RES_OK)
        {
          MPRINTF("TIMEOUT - keeping current size\n\r");
        }
        else if (key == '\r' || key == '\n')
        {
          MPRINTF("ENTER - entering size edit mode\n\r");
          uint32_t new_size = _Ospi_get_size_input(OSPI_MAX_CUSTOM_SIZE);
          if (new_size > 0 && (settings.address + new_size - 1) <= OSPI_MAX_FLASH_ADDRESS)
          {
            // Auto-align size to even boundary for 8D-8D-8D protocol
            if (settings.protocol == MC80_OSPI_PROTOCOL_8D_8D_8D)
            {
              new_size = (new_size + 1) & 0xFFFFFFFE;  // Round up to even size
            }
            settings.size = new_size;
            MPRINTF("Size updated successfully\n\r");
          }
          else
          {
            MPRINTF("ERROR: Invalid size or address range, keeping current\n\r");
          }
        }
        else if (key == VT100_ESC)
        {
          MPRINTF("ESC - keeping current size\n\r");
        }
        else
        {
          MPRINTF("Invalid key - keeping current size\n\r");
        }

        MPRINTF("Press any key to continue...\n\r");
        uint8_t dummy_key;
        WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
        break;
      }

      case '3':  // Configure pattern
      {
        MPRINTF("\n\r===== Configure Pattern =====\n\r");
        MPRINTF("Current pattern: %s\n\r",
                (settings.pattern_type == OSPI_PATTERN_CONSTANT) ? "Constant" : (settings.pattern_type == OSPI_PATTERN_INCREMENT) ? "Increment"
                                                                                                                                  : "Random");
        if (settings.pattern_type == OSPI_PATTERN_CONSTANT || settings.pattern_type == OSPI_PATTERN_INCREMENT)
        {
          MPRINTF("Current pattern value: 0x%02X\n\r", settings.pattern_value);
        }
        MPRINTF("Press ENTER to edit pattern, ESC to keep current: ");

        uint8_t key = 0;
        if (WAIT_CHAR(&key, ms_to_ticks(30000)) != RES_OK)
        {
          MPRINTF("TIMEOUT - keeping current pattern\n\r");
        }
        else if (key == '\r' || key == '\n')
        {
          MPRINTF("ENTER - entering pattern edit mode\n\r");
          settings.pattern_type = _Ospi_get_pattern_type();
          if (settings.pattern_type == OSPI_PATTERN_CONSTANT || settings.pattern_type == OSPI_PATTERN_INCREMENT)
          {
            settings.pattern_value = _Ospi_get_pattern_value();
          }
          MPRINTF("Pattern updated successfully\n\r");
        }
        else if (key == VT100_ESC)
        {
          MPRINTF("ESC - keeping current pattern\n\r");
        }
        else
        {
          MPRINTF("Invalid key - keeping current pattern\n\r");
        }

        MPRINTF("Press any key to continue...\n\r");
        uint8_t dummy_key;
        WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
        break;
      }

      case '4':  // Read operation (memory-mapped)
      {
        MPRINTF("\n\r===== Read Operation (Memory-Mapped) =====\n\r");

        // Allocate buffer
        uint8_t *read_buffer = (uint8_t *)App_malloc(settings.size);
        if (read_buffer == NULL)
        {
          MPRINTF("ERROR: Failed to allocate %u bytes for read buffer\n\r", settings.size);
          MPRINTF("Press any key to continue...\n\r");
          uint8_t dummy_key;
          WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
          break;
        }

        // Clear buffer
        memset(read_buffer, 0x00, settings.size);

        // Measure time and perform read
        T_sys_timestump start_time;
        Get_hw_timestump(&start_time);
        fsp_err_t       err = Mc80_ospi_memory_mapped_read(g_mc80_ospi.p_ctrl, read_buffer, settings.address, settings.size);
        T_sys_timestump end_time;
        Get_hw_timestump(&end_time);
        uint32_t elapsed_us = Timestump_diff_to_usec(&start_time, &end_time);

        if (err == FSP_SUCCESS)
        {
          MPRINTF("Read operation                : SUCCESS\n\r");
          MPRINTF("Address                       : 0x%08X\n\r", settings.address);
          MPRINTF("Size                          : %u bytes\n\r", settings.size);
          MPRINTF("Time elapsed                  : %u us\n\r", elapsed_us);
          MPRINTF("Transfer speed                : ");
          _Ospi_display_speed(settings.size, elapsed_us);

          // Display data (limited to prevent excessive output)
          _Ospi_display_data(read_buffer, settings.size, settings.address);

          // Calculate and display checksum
          uint32_t checksum = _Ospi_calculate_checksum(read_buffer, settings.size);
          MPRINTF("Data checksum (CRC32)         : 0x%08X\n\r", checksum);

          // Update results
          results.read_time_us           = elapsed_us;
          results.last_bytes_transferred = settings.size;
          results.last_checksum          = checksum;
          results.results_valid          = true;
        }
        else
        {
          MPRINTF("Read operation                : FAILED (error: 0x%X)\n\r", err);
        }

        App_free(read_buffer);
        MPRINTF("\nPress any key to continue...\n\r");
        uint8_t dummy_key;
        WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
        break;
      }

      case '5':  // Direct read operation
      {
        MPRINTF("\n\r===== Direct Read Operation =====\n\r");

        // Allocate buffer
        uint8_t *read_buffer = (uint8_t *)App_malloc(settings.size);
        if (read_buffer == NULL)
        {
          MPRINTF("ERROR: Failed to allocate %u bytes for read buffer\n\r", settings.size);
          MPRINTF("Press any key to continue...\n\r");
          uint8_t dummy_key;
          WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
          break;
        }

        // Clear buffer
        memset(read_buffer, 0x00, settings.size);

        // Measure time and perform direct read
        T_sys_timestump start_time;
        Get_hw_timestump(&start_time);
        fsp_err_t       err = Mc80_ospi_direct_read(g_mc80_ospi.p_ctrl, read_buffer, settings.address, settings.size);
        T_sys_timestump end_time;
        Get_hw_timestump(&end_time);
        uint32_t elapsed_us = Timestump_diff_to_usec(&start_time, &end_time);

        if (err == FSP_SUCCESS)
        {
          MPRINTF("Direct read operation         : SUCCESS\n\r");
          MPRINTF("Address                       : 0x%08X\n\r", settings.address);
          MPRINTF("Size                          : %u bytes\n\r", settings.size);
          MPRINTF("Time elapsed                  : %u us\n\r", elapsed_us);
          MPRINTF("Transfer speed                : ");
          _Ospi_display_speed(settings.size, elapsed_us);

          // Display data (limited to prevent excessive output)
          _Ospi_display_data(read_buffer, settings.size, settings.address);

          // Calculate and display checksum
          uint32_t checksum = _Ospi_calculate_checksum(read_buffer, settings.size);
          MPRINTF("Data checksum (CRC32)         : 0x%08X\n\r", checksum);

          // Update results
          results.read_time_us           = elapsed_us;
          results.last_bytes_transferred = settings.size;
          results.last_checksum          = checksum;
          results.results_valid          = true;
        }
        else
        {
          MPRINTF("Direct read operation         : FAILED (error: 0x%X)\n\r", err);
        }

        App_free(read_buffer);
        MPRINTF("\nPress any key to continue...\n\r");
        uint8_t dummy_key;
        WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
        break;
      }

      case '6':  // Fast read benchmark
      {
        MPRINTF("\n\r===== Fast Read Benchmark =====\n\r");
        MPRINTF("Using current settings: address 0x%08X, size %u bytes\n\r", settings.address, settings.size);

        // Use current settings size for benchmark
        uint32_t benchmark_size = settings.size;
        if ((settings.address + benchmark_size - 1) > OSPI_MAX_FLASH_ADDRESS)
        {
          MPRINTF("ERROR: Address range exceeds flash size\n\r");
          MPRINTF("Press any key to continue...\n\r");
          uint8_t dummy_key;
          WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
          break;
        }

        // Allocate buffer
        uint8_t *read_buffer = (uint8_t *)App_malloc(benchmark_size);
        if (read_buffer == NULL)
        {
          MPRINTF("ERROR: Failed to allocate %u bytes for benchmark buffer\n\r", benchmark_size);
          MPRINTF("Press any key to continue...\n\r");
          uint8_t dummy_key;
          WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
          break;
        }

        MPRINTF("Performing fast read benchmark...\n\r");

        // Measure time and perform read (no data display)
        T_sys_timestump start_time;
        Get_hw_timestump(&start_time);
        fsp_err_t       err = Mc80_ospi_memory_mapped_read(g_mc80_ospi.p_ctrl, read_buffer, settings.address, benchmark_size);
        T_sys_timestump end_time;
        Get_hw_timestump(&end_time);
        uint32_t elapsed_us = Timestump_diff_to_usec(&start_time, &end_time);

        if (err == FSP_SUCCESS)
        {
          MPRINTF("Fast read benchmark           : SUCCESS\n\r");
          MPRINTF("Address                       : 0x%08X\n\r", settings.address);
          MPRINTF("Size                          : %u bytes\n\r", benchmark_size);
          MPRINTF("Time elapsed                  : %u us\n\r", elapsed_us);
          MPRINTF("Maximum read speed            : ");
          _Ospi_display_speed(benchmark_size, elapsed_us);

          // Calculate and display checksum for verification
          uint32_t checksum = _Ospi_calculate_checksum(read_buffer, benchmark_size);
          MPRINTF("Data checksum (CRC32)         : 0x%08X\n\r", checksum);

          // Update results
          results.read_time_us           = elapsed_us;
          results.last_bytes_transferred = benchmark_size;
          results.last_checksum          = checksum;
          results.results_valid          = true;
        }
        else
        {
          MPRINTF("Fast read benchmark           : FAILED (error: 0x%X)\n\r", err);
        }

        App_free(read_buffer);
        MPRINTF("\nPress any key to continue...\n\r");
        uint8_t dummy_key;
        WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
        break;
      }

      case '7':  // Write operation
      {
        MPRINTF("\n\r===== Write Operation =====\n\r");

        // Allocate write buffer
        uint8_t *write_buffer = (uint8_t *)App_malloc(settings.size);
        if (write_buffer == NULL)
        {
          MPRINTF("ERROR: Failed to allocate %u bytes for write buffer\n\r", settings.size);
          MPRINTF("Press any key to continue...\n\r");
          uint8_t dummy_key;
          WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
          break;
        }

        // Generate pattern
        _Ospi_generate_pattern(write_buffer, settings.size, settings.pattern_type, settings.pattern_value);

        // Measure time and perform write
        T_sys_timestump start_time;
        Get_hw_timestump(&start_time);
        fsp_err_t       err = Mc80_ospi_memory_mapped_write(g_mc80_ospi.p_ctrl, write_buffer, (uint8_t *)(MC80_OSPI_DEVICE_0_START_ADDRESS + settings.address), settings.size);
        T_sys_timestump end_time;
        Get_hw_timestump(&end_time);
        uint32_t elapsed_us = Timestump_diff_to_usec(&start_time, &end_time);

        if (err == FSP_SUCCESS)
        {
          MPRINTF("Write operation               : SUCCESS\n\r");
          MPRINTF("Time elapsed                  : %u us\n\r", elapsed_us);
          MPRINTF("Transfer speed                : ");
          _Ospi_display_speed(settings.size, elapsed_us);

          // Verify write data
          MPRINTF("Verifying written data...\n\r");
          bool verify_ok = _Ospi_verify_write_data(write_buffer, settings.address, settings.size);
          if (verify_ok)
          {
            MPRINTF("Data verification             : SUCCESS\n\r");

            // Update results only if verification passed
            results.write_time_us          = elapsed_us;
            results.last_bytes_transferred = settings.size;
            results.results_valid          = true;
          }
          else
          {
            MPRINTF("Data verification             : FAILED\n\r");
          }
        }
        else
        {
          MPRINTF("Write operation               : FAILED (error: 0x%X)\n\r", err);
        }

        App_free(write_buffer);
        MPRINTF("\nPress any key to continue...\n\r");
        uint8_t dummy_key;
        WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
        break;
      }

      case '8':  // Erase operation
      {
        MPRINTF("\n\r===== Erase Operation =====\n\r");
        MPRINTF("WARNING: This will erase %u bytes starting from address 0x%08X\n\r", settings.size, settings.address);
        MPRINTF("Continue? (Y/N): ");

        uint8_t confirm = 0;
        if (WAIT_CHAR(&confirm, ms_to_ticks(10000)) != RES_OK)
        {
          MPRINTF("TIMEOUT - Erase operation cancelled\n\r");
          MPRINTF("Press any key to continue...\n\r");
          uint8_t dummy_key;
          WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
          break;
        }
        MPRINTF("%c\n\r", confirm);

        if (confirm != 'Y' && confirm != 'y')
        {
          MPRINTF("Erase operation cancelled\n\r");
          MPRINTF("Press any key to continue...\n\r");
          uint8_t dummy_key;
          WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
          break;
        }

        // Measure time and perform erase
        T_sys_timestump start_time;
        Get_hw_timestump(&start_time);
        fsp_err_t       err = Mc80_ospi_erase(g_mc80_ospi.p_ctrl, (uint8_t *)(MC80_OSPI_DEVICE_0_START_ADDRESS + settings.address), settings.size);
        T_sys_timestump end_time;
        Get_hw_timestump(&end_time);
        uint32_t elapsed_us = Timestump_diff_to_usec(&start_time, &end_time);

        if (err == FSP_SUCCESS)
        {
          MPRINTF("Erase operation               : SUCCESS\n\r");
          MPRINTF("Time elapsed                  : %u us\n\r", elapsed_us);
          MPRINTF("Erase speed                   : ");
          _Ospi_display_speed(settings.size, elapsed_us);

          // Verify erase
          MPRINTF("Verifying erased data...\n\r");

          // Allocate temporary buffer for verification
          uint8_t *verify_buffer = (uint8_t *)App_malloc(settings.size);
          if (verify_buffer != NULL)
          {
            bool verify_ok = _Ospi_verify_erase_data(verify_buffer, settings.address, settings.size);
            App_free(verify_buffer);

            if (verify_ok)
            {
              MPRINTF("Erase verification            : SUCCESS\n\r");

              // Update results only if verification passed
              results.erase_time_us          = elapsed_us;
              results.last_bytes_transferred = settings.size;
              results.results_valid          = true;
            }
            else
            {
              MPRINTF("Erase verification            : FAILED\n\r");
            }
          }
          else
          {
            MPRINTF("Erase verification            : SKIPPED (failed to allocate buffer)\n\r");

            // Update results even without verification
            results.erase_time_us          = elapsed_us;
            results.last_bytes_transferred = settings.size;
            results.results_valid          = true;
          }
        }
        else
        {
          MPRINTF("Erase operation               : FAILED (error: 0x%X)\n\r", err);
        }

        MPRINTF("\nPress any key to continue...\n\r");
        uint8_t dummy_key;
        WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
        break;
      }

      case '9':  // Switch protocol
      {
        MPRINTF("\n\r===== Protocol Switch =====\n\r");
        T_mc80_ospi_protocol new_protocol = _Ospi_select_protocol();

        MPRINTF("Switching protocol...\n\r");
        fsp_err_t err = Mc80_ospi_spi_protocol_switch_safe(g_mc80_ospi.p_ctrl, new_protocol);
        if (err == FSP_SUCCESS)
        {
          MPRINTF("Protocol switch               : SUCCESS\n\r");
          settings.protocol = new_protocol;

          // Auto-align existing address and size when switching to 8D-8D-8D protocol
          if (new_protocol == MC80_OSPI_PROTOCOL_8D_8D_8D)
          {
            settings.address &= 0xFFFFFFFE;  // Clear LSB to make address even
            settings.size = (settings.size + 1) & 0xFFFFFFFE;  // Round up to even size
          }
        }
        else
        {
          MPRINTF("Protocol switch               : FAILED (error: 0x%X)\n\r", err);
        }

        MPRINTF("Press any key to continue...\n\r");
        uint8_t dummy_key;
        WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
        break;
      }

      case 'R':
      case 'r':
        return;

      default:
        MPRINTF("Invalid choice\n\r");
        MPRINTF("Press any key to continue...\n\r");
        uint8_t dummy_key;
        WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
        break;
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Get memory address input from user

  Parameters: None

  Return: Address value entered by user
-----------------------------------------------------------------------------------------------------*/
static uint32_t _Ospi_get_address_input(void)
{
  GET_MCBL;
  uint32_t address = 0;

  MPRINTF("Quick addresses:\n\r");
  MPRINTF("  <1> - 0x00000000 (Start of flash)\n\r");
  MPRINTF("  <2> - 0x00001000 (4KB offset)\n\r");
  MPRINTF("  <3> - 0x00010000 (64KB offset)\n\r");
  MPRINTF("  <4> - 0x00100000 (1MB offset)\n\r");
  MPRINTF("  <5> - 0x01000000 (16MB offset)\n\r");
  MPRINTF("  <C> - Custom address\n\r");
  MPRINTF("Choice: ");

  uint8_t choice = 0;
  if (WAIT_CHAR(&choice, ms_to_ticks(30000)) != RES_OK)
  {
    MPRINTF("TIMEOUT - using custom input\n\r");
    choice = 'c';  // Default to custom
  }
  MPRINTF("%c\n\r", choice);

  switch (choice)
  {
    case '1':
      return 0x00000000;
    case '2':
      return 0x00001000;
    case '3':
      return 0x00010000;
    case '4':
      return 0x00100000;
    case '5':
      return 0x01000000;
    case 'C':
    case 'c':
      break;
    default:
      MPRINTF("Invalid choice, using custom input\n\r");
      break;
  }

  MPRINTF("Enter address (with or without 0x prefix, e.g., 1000 or 0x1000): ");

  char input_buffer[16];
  memset(input_buffer, 0, sizeof(input_buffer));

  uint8_t pos = 0;
  while (pos < 15)
  {
    uint8_t key;
    if (WAIT_CHAR(&key, ms_to_ticks(30000)) != RES_OK)
    {
      break;  // Exit on timeout
    }

    if (key == '\r' || key == '\n')
    {
      break;
    }
    else if (key == '\b' || key == 0x7F)  // Backspace
    {
      if (pos > 0)
      {
        pos--;
        input_buffer[pos] = 0;
        MPRINTF("\b \b");
      }
    }
    else if ((key >= '0' && key <= '9') || (key >= 'A' && key <= 'F') || (key >= 'a' && key <= 'f') || key == 'x' || key == 'X')
    {
      input_buffer[pos] = key;
      pos++;
      MPRINTF("%c", key);
    }
  }

  MPRINTF("\n\r");

  // Parse input - handle both "0x1000" and "1000" formats
  char   *hex_start  = input_buffer;
  uint8_t hex_length = pos;

  if (pos >= 2 && (input_buffer[0] == '0') && (input_buffer[1] == 'x' || input_buffer[1] == 'X'))
  {
    hex_start  = &input_buffer[2];
    hex_length = pos - 2;
  }

  // Check if we have any hex digits to process
  if (hex_length == 0)
  {
    MPRINTF("No valid hex digits entered, using 0x00000000\n\r");
    return 0;
  }

  // Convert hex string to number with overflow check
  for (uint8_t i = 0; i < hex_length; i++)
  {
    char c = hex_start[i];
    if (c == 0) break;  // End of string

    // Check for potential overflow (address > 32-bit max)
    if (address > (0xFFFFFFFF / 16))
    {
      MPRINTF("WARNING: Address value too large, truncating\n\r");
      break;
    }

    address = address * 16;
    if (c >= '0' && c <= '9')
    {
      address += c - '0';
    }
    else if (c >= 'A' && c <= 'F')
    {
      address += c - 'A' + 10;
    }
    else if (c >= 'a' && c <= 'f')
    {
      address += c - 'a' + 10;
    }
  }

  MPRINTF("Parsed address: 0x%08X\n\r", address);
  return address;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Get size input from user

  Parameters: max_size - Maximum allowed size

  Return: Size value entered by user
-----------------------------------------------------------------------------------------------------*/
static uint32_t _Ospi_get_size_input(uint32_t max_size)
{
  GET_MCBL;
  uint32_t size = 0;

  MPRINTF("Quick sizes:\n\r");
  MPRINTF("  <1> - 256 bytes (0x100 - Page size)\n\r");
  MPRINTF("  <2> - 4096 bytes (0x1000 - Sector size)\n\r");
  MPRINTF("  <3> - 65536 bytes (0x10000 - Block size)\n\r");
  MPRINTF("  <4> - 1048576 bytes (0x100000 - 1MB)\n\r");
  MPRINTF("  <5> - 16777216 bytes (0x1000000 - 16MB)\n\r");
  MPRINTF("  <C> - Custom size\n\r");
  MPRINTF("Choice: ");

  uint8_t choice = 0;
  if (WAIT_CHAR(&choice, ms_to_ticks(30000)) != RES_OK)
  {
    MPRINTF("TIMEOUT - using custom input\n\r");
    choice = 'c';  // Default to custom
  }
  MPRINTF("%c\n\r", choice);

  switch (choice)
  {
    case '1':
      return 256;
    case '2':
      return 4096;
    case '3':
      return 65536;
    case '4':
      return (1048576 <= max_size) ? 1048576 : max_size;
    case '5':
      return (16777216 <= max_size) ? 16777216 : max_size;
    case 'C':
    case 'c':
      break;
    default:
      MPRINTF("Invalid choice, using custom input\n\r");
      break;
  }

  MPRINTF("Enter size in bytes (decimal or hex with 0x prefix, max %u): ", max_size);

  char input_buffer[16];
  memset(input_buffer, 0, sizeof(input_buffer));

  uint8_t pos = 0;
  while (pos < 15)
  {
    uint8_t key;
    if (WAIT_CHAR(&key, ms_to_ticks(30000)) != RES_OK)
    {
      break;  // Exit on timeout
    }

    if (key == '\r' || key == '\n')
    {
      break;
    }
    else if (key == '\b' || key == 0x7F)  // Backspace
    {
      if (pos > 0)
      {
        pos--;
        input_buffer[pos] = 0;
        MPRINTF("\b \b");
      }
    }
    else if ((key >= '0' && key <= '9') || (key >= 'A' && key <= 'F') || (key >= 'a' && key <= 'f') || key == 'x' || key == 'X')
    {
      input_buffer[pos] = key;
      pos++;
      MPRINTF("%c", key);
    }
  }

  MPRINTF("\n\r");

  // Check if input is hexadecimal (starts with 0x or 0X)
  bool    is_hex       = false;
  char   *parse_start  = input_buffer;
  uint8_t parse_length = pos;

  if (pos >= 2 && (input_buffer[0] == '0') && (input_buffer[1] == 'x' || input_buffer[1] == 'X'))
  {
    is_hex       = true;
    parse_start  = &input_buffer[2];
    parse_length = pos - 2;
  }

  // Check if we have any valid digits to process
  if (parse_length == 0)
  {
    MPRINTF("No valid digits entered, using 0\n\r");
    return 0;
  }

  // Convert string to number
  if (is_hex)
  {
    // Convert hex string to number with overflow check
    for (uint8_t i = 0; i < parse_length; i++)
    {
      char c = parse_start[i];
      if (c == 0) break;  // End of string

      // Check for potential overflow
      if (size > (0xFFFFFFFF / 16))
      {
        MPRINTF("WARNING: Size value too large, truncating\n\r");
        break;
      }

      size = size * 16;
      if (c >= '0' && c <= '9')
      {
        size += c - '0';
      }
      else if (c >= 'A' && c <= 'F')
      {
        size += c - 'A' + 10;
      }
      else if (c >= 'a' && c <= 'f')
      {
        size += c - 'a' + 10;
      }
    }
  }
  else
  {
    // Convert decimal string to number
    for (uint8_t i = 0; i < parse_length; i++)
    {
      char c = parse_start[i];
      if (c < '0' || c > '9') break;  // Only process decimal digits

      // Check for potential overflow
      if (size > (0xFFFFFFFF / 10))
      {
        MPRINTF("WARNING: Size value too large, truncating\n\r");
        break;
      }

      size = size * 10 + (c - '0');
    }
  }

  if (size > max_size)
  {
    MPRINTF("WARNING: Size %u exceeds maximum %u, using maximum\n\r", size, max_size);
    size = max_size;
  }

  MPRINTF("Selected size: %u bytes\n\r", size);
  return size;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Get data pattern type from user

  Parameters: None

  Return: Selected pattern type
-----------------------------------------------------------------------------------------------------*/
static T_ospi_pattern_type _Ospi_get_pattern_type(void)
{
  GET_MCBL;
  MPRINTF("Select data pattern:\n\r");
  MPRINTF("  <1> - Constant value\n\r");
  MPRINTF("  <2> - Incrementing values\n\r");
  MPRINTF("  <3> - Random values\n\r");
  MPRINTF("Choice: ");

  uint8_t key = 0;
  if (WAIT_CHAR(&key, ms_to_ticks(30000)) != RES_OK)
  {
    MPRINTF("TIMEOUT - using constant pattern\n\r");
    return OSPI_PATTERN_CONSTANT;
  }
  MPRINTF("%c\n\r", key);

  switch (key)
  {
    case '1':
      return OSPI_PATTERN_CONSTANT;
    case '2':
      return OSPI_PATTERN_INCREMENT;
    case '3':
      return OSPI_PATTERN_RANDOM;
    default:
      MPRINTF("Invalid choice, using constant pattern\n\r");
      return OSPI_PATTERN_CONSTANT;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Get pattern value from user

  Parameters: None

  Return: Pattern value entered by user
-----------------------------------------------------------------------------------------------------*/
static uint8_t _Ospi_get_pattern_value(void)
{
  GET_MCBL;
  uint32_t value = 0;

  MPRINTF("Quick pattern values:\n\r");
  MPRINTF("  <1> - 0x00 (All zeros)\n\r");
  MPRINTF("  <2> - 0x55 (Alternating 01010101)\n\r");
  MPRINTF("  <3> - 0xAA (Alternating 10101010)\n\r");
  MPRINTF("  <4> - 0xFF (All ones)\n\r");
  MPRINTF("  <C> - Custom value\n\r");
  MPRINTF("Choice: ");

  uint8_t choice = 0;
  if (WAIT_CHAR(&choice, ms_to_ticks(30000)) != RES_OK)
  {
    MPRINTF("TIMEOUT - using 0x55\n\r");
    return 0x55;
  }
  MPRINTF("%c\n\r", choice);

  switch (choice)
  {
    case '1':
      return 0x00;
    case '2':
      return 0x55;
    case '3':
      return 0xAA;
    case '4':
      return 0xFF;
    case 'C':
    case 'c':
      break;
    default:
      MPRINTF("Invalid choice, using custom input\n\r");
      break;
  }

  MPRINTF("Enter pattern value (hex, with or without 0x prefix, e.g., FF or 0xFF): ");

  char input_buffer[4];
  memset(input_buffer, 0, sizeof(input_buffer));

  uint8_t pos = 0;
  while (pos < 3)
  {
    uint8_t key;
    if (WAIT_CHAR(&key, ms_to_ticks(30000)) != RES_OK)
    {
      break;  // Exit on timeout
    }

    if (key == '\r' || key == '\n')
    {
      break;
    }
    else if (key == '\b' || key == 0x7F)  // Backspace
    {
      if (pos > 0)
      {
        pos--;
        input_buffer[pos] = 0;
        MPRINTF("\b \b");
      }
    }
    else if ((key >= '0' && key <= '9') || (key >= 'A' && key <= 'F') || (key >= 'a' && key <= 'f') || key == 'x' || key == 'X')
    {
      input_buffer[pos] = key;
      pos++;
      MPRINTF("%c", key);
    }
  }

  MPRINTF("\n\r");

  // Parse input - handle both "0xFF" and "FF" formats
  char   *hex_start  = input_buffer;
  uint8_t hex_length = pos;

  if (pos >= 2 && (input_buffer[0] == '0') && (input_buffer[1] == 'x' || input_buffer[1] == 'X'))
  {
    hex_start  = &input_buffer[2];
    hex_length = pos - 2;
  }

  // Check if we have any hex digits to process
  if (hex_length == 0)
  {
    MPRINTF("No valid hex digits entered, using 0x00\n\r");
    return 0;
  }

  // Convert hex string to number with range check
  for (uint8_t i = 0; i < hex_length; i++)
  {
    char c = hex_start[i];
    if (c == 0) break;  // End of string

    // Check for byte overflow (value > 255)
    if (value > (0xFF / 16))
    {
      MPRINTF("WARNING: Value too large for byte, using 0xFF\n\r");
      return 0xFF;
    }

    value = value * 16;
    if (c >= '0' && c <= '9')
    {
      value += c - '0';
    }
    else if (c >= 'A' && c <= 'F')
    {
      value += c - 'A' + 10;
    }
    else if (c >= 'a' && c <= 'f')
    {
      value += c - 'a' + 10;
    }
  }

  MPRINTF("Selected pattern value: 0x%02X\n\r", (uint8_t)value);
  return (uint8_t)value;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Generate data pattern in buffer

  Parameters: buffer - Buffer to fill with pattern
              size - Size of buffer
              type - Pattern type
              base_value - Base value for pattern

  Return:
-----------------------------------------------------------------------------------------------------*/
static void _Ospi_generate_pattern(uint8_t *buffer, uint32_t size, T_ospi_pattern_type type, uint8_t base_value)
{
  switch (type)
  {
    case OSPI_PATTERN_CONSTANT:
      memset(buffer, base_value, size);
      break;

    case OSPI_PATTERN_INCREMENT:
      for (uint32_t i = 0; i < size; i++)
      {
        buffer[i] = (uint8_t)(base_value + i);
      }
      break;

    case OSPI_PATTERN_RANDOM:
    {
      // Use our own PRNG for reproducible pseudo-random generation
      // Initialize with a fixed seed to ensure reproducible results
      _Ospi_prng_seed(0xACE1u + base_value);  // Use base_value to add variation
      for (uint32_t i = 0; i < size; i++)
      {
        buffer[i] = (uint8_t)_Ospi_prng_rand();
      }
      break;
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Display data in hex format with ASCII representation

  Parameters: data - Data buffer to display
              size - Size of data to display
              start_address - Starting address for display offset

  Return:
-----------------------------------------------------------------------------------------------------*/
static void _Ospi_display_data(uint8_t *data, uint32_t size, uint32_t start_address)
{
  GET_MCBL;

  // Limit display size to prevent excessive output
  uint32_t display_size = size;
  if (display_size > OSPI_DISPLAY_MAX_SIZE)
  {
    display_size = OSPI_DISPLAY_MAX_SIZE;
    MPRINTF("Displaying first %u bytes of %u total bytes:\n\r", OSPI_DISPLAY_MAX_SIZE, size);
  }

  MPRINTF("\n\rData contents:\n\r");

  // Display data in hex format (16 bytes per line)
  for (uint32_t i = 0; i < display_size; i += OSPI_DISPLAY_BYTES_PER_LINE)
  {
    // Print address
    MPRINTF("0x%08X: ", start_address + i);

    // Print hex values
    for (uint32_t j = 0; j < OSPI_DISPLAY_BYTES_PER_LINE; j++)
    {
      if (i + j < display_size)
      {
        MPRINTF("%02X ", data[i + j]);
      }
      else
      {
        MPRINTF("   ");
      }
    }

    MPRINTF(" | ");

    // Print ASCII representation
    for (uint32_t j = 0; j < OSPI_DISPLAY_BYTES_PER_LINE; j++)
    {
      if (i + j < display_size)
      {
        uint8_t byte_val = data[i + j];
        if (byte_val >= 32 && byte_val <= 126)  // Printable ASCII
        {
          MPRINTF("%c", byte_val);
        }
        else
        {
          MPRINTF(".");
        }
      }
    }

    MPRINTF("\n\r");
  }

  if (size > display_size)
  {
    MPRINTF("... (%u more bytes not shown)\n\r", size - display_size);
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Display operation speed information

  Parameters: bytes - Number of bytes transferred
              time_us - Time elapsed in microseconds

  Return:
-----------------------------------------------------------------------------------------------------*/
static void _Ospi_display_speed(uint32_t bytes, uint32_t time_us)
{
  GET_MCBL;

  if (time_us > 0)
  {
    uint64_t speed_bps_64    = ((uint64_t)bytes * 1000000ul) / time_us;  // Bytes per second (64-bit calculation)
    uint32_t speed_bps       = (uint32_t)speed_bps_64;                   // Bytes per second
    uint32_t speed_kbps      = speed_bps / 1024;                         // Kilobytes per second (integer part)
    uint32_t speed_kbps_frac = ((speed_bps % 1024) * 1000) / 1024;       // Fractional part (3 digits)
    uint32_t time_ms         = time_us / 1000;                           // Convert to milliseconds for display
    uint32_t time_us_frac    = time_us % 1000;                           // Microseconds fractional part

    MPRINTF("%u.%03u KB/s (%u bytes/s)\n\r", speed_kbps, speed_kbps_frac, speed_bps);
    MPRINTF("Time                          : %u.%03u ms (%u us)\n\r", time_ms, time_us_frac, time_us);
  }
  else
  {
    MPRINTF("> 1000 KB/s (operation too fast to measure)\n\r");
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Compare two data buffers and display detailed differences

  Parameters: write_buffer - Original write data buffer
              read_buffer - Read back data buffer
              size - Size of buffers in bytes
              address - Starting address for display offset

  Return: true if buffers match, false otherwise
-----------------------------------------------------------------------------------------------------*/
static bool _Ospi_compare_buffers_detailed(uint8_t *write_buffer, uint8_t *read_buffer, uint32_t size, uint32_t address)
{
  GET_MCBL;
  bool     match                 = true;
  uint32_t error_count           = 0;
  uint32_t max_errors_to_display = 20;  // Limit error display to prevent terminal overflow

  MPRINTF("\n\r--- Detailed Data Comparison ---\n\r");

  // Compare all bytes
  for (uint32_t i = 0; i < size; i++)
  {
    if (write_buffer[i] != read_buffer[i])
    {
      match = false;
      error_count++;

      // Display first few errors in detail
      if (error_count <= max_errors_to_display)
      {
        MPRINTF("Mismatch at offset 0x%08X (addr 0x%08X): wrote 0x%02X, read 0x%02X\n\r",
                i, address + i, write_buffer[i], read_buffer[i]);
      }
    }
  }

  if (match)
  {
    MPRINTF("Data comparison               : SUCCESS - All %u bytes match\n\r", size);
  }
  else
  {
    MPRINTF("Data comparison               : FAILED - %u bytes differ\n\r", error_count);
    if (error_count > max_errors_to_display)
    {
      MPRINTF("... and %u more mismatches (not shown)\n\r", error_count - max_errors_to_display);
    }

    // Show error statistics
    double error_percentage = ((double)error_count / (double)size) * 100.0;
    MPRINTF("Error rate                    : %.2f%% (%u errors out of %u bytes)\n\r",
            error_percentage, error_count, size);
  }

  return match;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Verify written data by reading back and comparing

  Parameters: original_data - Original data that was written
              address - Flash address where data was written
              size - Size of data in bytes

  Return: true if verification successful, false otherwise
-----------------------------------------------------------------------------------------------------*/
static bool _Ospi_verify_write_data(uint8_t *original_data, uint32_t address, uint32_t size)
{
  // Allocate buffer for read-back
  uint8_t *verify_buffer = (uint8_t *)App_malloc(size);
  if (verify_buffer == NULL)
  {
    GET_MCBL;
    MPRINTF("ERROR: Failed to allocate verification buffer\n\r");
    return false;
  }

  // Read back the data
  fsp_err_t err = Mc80_ospi_memory_mapped_read(g_mc80_ospi.p_ctrl, verify_buffer, address, size);
  if (err != FSP_SUCCESS)
  {
    GET_MCBL;
    MPRINTF("ERROR: Failed to read back data for verification (0x%X)\n\r", err);
    App_free(verify_buffer);
    return false;
  }

  // Compare data
  bool match = true;
  for (uint32_t i = 0; i < size; i++)
  {
    if (original_data[i] != verify_buffer[i])
    {
      match = false;
      break;
    }
  }

  App_free(verify_buffer);
  return match;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Verify erased data by reading back and checking for 0xFF pattern

  Parameters: buffer - Buffer to use for reading data (must be at least size bytes)
              address - Flash address that was erased
              size - Size of erased area in bytes

  Return: true if verification successful (all bytes are 0xFF), false otherwise
-----------------------------------------------------------------------------------------------------*/
static bool _Ospi_verify_erase_data(uint8_t *buffer, uint32_t address, uint32_t size)
{
  // Read back the data using provided buffer
  fsp_err_t err = Mc80_ospi_memory_mapped_read(g_mc80_ospi.p_ctrl, buffer, address, size);
  if (err != FSP_SUCCESS)
  {
    GET_MCBL;
    MPRINTF("ERROR: Failed to read back data for verification (0x%X)\n\r", err);
    return false;
  }

  // Check that all bytes are 0xFF (erased state)
  for (uint32_t i = 0; i < size; i++)
  {
    if (buffer[i] != 0xFF)
    {
      return false;
    }
  }

  return true;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Calculate CRC32 checksum for data buffer

  Parameters: data - Data buffer to calculate checksum for
              size - Size of data in bytes

  Return: CRC32 checksum value
-----------------------------------------------------------------------------------------------------*/
static uint32_t _Ospi_calculate_checksum(uint8_t *data, uint32_t size)
{
  // CRC32 polynomial: 0x04C11DB7 (Ethernet CRC)
  const uint32_t polynomial = 0xEDB88320;  // Reflected form of 0x04C11DB7
  uint32_t       crc        = 0xFFFFFFFF;  // Initial value

  for (uint32_t i = 0; i < size; i++)
  {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; bit++)
    {
      if (crc & 1)
      {
        crc = (crc >> 1) ^ polynomial;
      }
      else
      {
        crc = crc >> 1;
      }
    }
  }

  return ~crc;  // Final XOR with 0xFFFFFFFF
}

/*-----------------------------------------------------------------------------------------------------
  Description: Comprehensive memory test with reproducible random address and size selection
               Uses custom PRNG to ensure same sequence after reset (independent of system rand)

  Parameters: keycode - Input key code from VT100 terminal

  Return: None
-----------------------------------------------------------------------------------------------------*/
void OSPI_test_comprehensive_memory(uint8_t keycode)
{
  GET_MCBL;

  // Ensure OSPI driver is open
  fsp_err_t init_err = _Ospi_ensure_driver_open();
  if (init_err != FSP_SUCCESS)
  {
    MPRINTF(VT100_CLEAR_AND_HOME);
    MPRINTF("ERROR: Failed to ensure OSPI driver is open (0x%X)\n\r", init_err);
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  // Select protocol for testing
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF(" ===== Comprehensive OSPI Memory Test =====\n\r");
  MPRINTF("\n\rThis test performs comprehensive memory testing\n\r");
  MPRINTF("with different address alignment and size options\n\r");
  MPRINTF("\n\rSelect OSPI protocol for testing:\n\r");
  T_mc80_ospi_protocol test_protocol = _Ospi_select_protocol();

  // Switch to selected protocol
  MPRINTF("Switching to selected protocol...\n\r");
  fsp_err_t protocol_err = Mc80_ospi_spi_protocol_switch_safe(g_mc80_ospi.p_ctrl, test_protocol);
  if (protocol_err != FSP_SUCCESS)
  {
    MPRINTF("ERROR: Failed to switch protocol (0x%X)\n\r", protocol_err);
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }
  MPRINTF("Protocol switch successful\n\r");

  // Select alignment type for testing
  T_ospi_alignment_type alignment_type = _Ospi_select_alignment_type();

  // Initialize our PRNG once with fixed seed for reproducible sequences
  _Ospi_prng_seed(OSPI_FIXED_SEED_ADDRESS);

  uint32_t test_number = 1;

  while (1)
  {
    uint32_t test_address;
    uint32_t test_size;

    // Generate address and size based on selected alignment type
    switch (alignment_type)
    {
      case OSPI_ALIGNMENT_UNALIGNED:
        // Generate completely unaligned address and random size
        test_address = _Ospi_prng_rand32() % (OSPI_MAX_FLASH_ADDRESS + 1);
        // Make address unaligned by adding 1-63 offset
        test_address = (test_address & ~0x3F) + ((_Ospi_prng_rand32() % 63) + 1);
        if (test_address > OSPI_MAX_FLASH_ADDRESS)
        {
          test_address = OSPI_MAX_FLASH_ADDRESS - 63;
        }

        // Calculate maximum possible size and generate random size
        {
          uint32_t max_possible_size = OSPI_MAX_FLASH_ADDRESS - test_address + 1;
          if (max_possible_size > OSPI_COMPREHENSIVE_TEST_MAX_SIZE)
          {
            max_possible_size = OSPI_COMPREHENSIVE_TEST_MAX_SIZE;
          }

          uint32_t size_range = max_possible_size - OSPI_COMPREHENSIVE_TEST_MIN_SIZE + 1;
          test_size           = (_Ospi_prng_rand32() % size_range) + OSPI_COMPREHENSIVE_TEST_MIN_SIZE;
        }
        break;

      case OSPI_ALIGNMENT_64_BYTE:
        // Generate 64-byte aligned address
        test_address = (_Ospi_prng_rand32() % ((OSPI_MAX_FLASH_ADDRESS + 1) / 64)) * 64;

        // Calculate maximum possible size and make it multiple of 64
        {
          uint32_t max_possible_size = OSPI_MAX_FLASH_ADDRESS - test_address + 1;
          if (max_possible_size > OSPI_COMPREHENSIVE_TEST_MAX_SIZE)
          {
            max_possible_size = OSPI_COMPREHENSIVE_TEST_MAX_SIZE;
          }

          // Align max size down to 64-byte boundary
          max_possible_size = (max_possible_size / 64) * 64;

          // Ensure minimum size is at least 64 bytes
          uint32_t min_size = 64;
          if (min_size > max_possible_size)
          {
            min_size = max_possible_size;
          }

          uint32_t size_range = (max_possible_size - min_size) / 64 + 1;
          test_size           = ((_Ospi_prng_rand32() % size_range) * 64) + min_size;
        }
        break;

      case OSPI_ALIGNMENT_256_BYTE:
        // Generate 256-byte aligned address
        test_address = (_Ospi_prng_rand32() % ((OSPI_MAX_FLASH_ADDRESS + 1) / 256)) * 256;

        // Calculate maximum possible size and make it multiple of 256
        {
          uint32_t max_possible_size = OSPI_MAX_FLASH_ADDRESS - test_address + 1;
          if (max_possible_size > OSPI_COMPREHENSIVE_TEST_MAX_SIZE)
          {
            max_possible_size = OSPI_COMPREHENSIVE_TEST_MAX_SIZE;
          }

          // Align max size down to 256-byte boundary
          max_possible_size = (max_possible_size / 256) * 256;

          // Ensure minimum size is at least 256 bytes
          uint32_t min_size = 256;
          if (min_size > max_possible_size)
          {
            min_size = max_possible_size;
          }

          uint32_t size_range = (max_possible_size - min_size) / 256 + 1;
          test_size           = ((_Ospi_prng_rand32() % size_range) * 256) + min_size;
        }
        break;

      default:
        // Fallback to unaligned
        test_address = _Ospi_prng_rand32() % (OSPI_MAX_FLASH_ADDRESS + 1);
        {
          uint32_t max_possible_size = OSPI_MAX_FLASH_ADDRESS - test_address + 1;
          if (max_possible_size > OSPI_COMPREHENSIVE_TEST_MAX_SIZE)
          {
            max_possible_size = OSPI_COMPREHENSIVE_TEST_MAX_SIZE;
          }

          uint32_t size_range = max_possible_size - OSPI_COMPREHENSIVE_TEST_MIN_SIZE + 1;
          test_size           = (_Ospi_prng_rand32() % size_range) + OSPI_COMPREHENSIVE_TEST_MIN_SIZE;
        }
        break;
    }

    // Auto-align address and size for 8D-8D-8D protocol
    bool address_aligned = false;
    bool size_aligned = false;
    if (test_protocol == MC80_OSPI_PROTOCOL_8D_8D_8D)
    {
      uint32_t orig_address = test_address;
      uint32_t orig_size = test_size;
      test_address &= 0xFFFFFFFE;  // Clear LSB to make address even
      test_size = (test_size + 1) & 0xFFFFFFFE;  // Round up to even size

      // Store alignment info for display
      address_aligned = (orig_address != test_address);
      size_aligned = (orig_size != test_size);
    }

    // Initialize test result structure
    T_ospi_comprehensive_test_result result = { 0 };
    result.test_number                      = test_number;
    result.address                          = test_address;
    result.size                             = test_size;
    result.test_passed                      = false;

    MPRINTF(VT100_CLEAR_AND_HOME);
    MPRINTF(" ===== Comprehensive OSPI Memory Test =====\n\r");
    MPRINTF("\n\rTest #%u (Reproducible sequence)\n\r", test_number);

    // Display alignment type information
    const char *alignment_str;
    switch (alignment_type)
    {
      case OSPI_ALIGNMENT_UNALIGNED:
        alignment_str = "Unaligned addresses and random sizes";
        break;
      case OSPI_ALIGNMENT_64_BYTE:
        alignment_str = "64-byte aligned addresses and sizes";
        break;
      case OSPI_ALIGNMENT_256_BYTE:
        alignment_str = "256-byte aligned addresses and sizes";
        break;
      default:
        alignment_str = "Unknown alignment";
        break;
    }

    MPRINTF("Alignment type                : %s\n\r", alignment_str);
    MPRINTF("Protocol                      : %s\n\r", (test_protocol == MC80_OSPI_PROTOCOL_8D_8D_8D) ? "Octal DDR (8D-8D-8D)" : "Standard SPI (1S-1S-1S)");
    MPRINTF("Generated address             : 0x%08X\n\r", test_address);
    MPRINTF("Generated size                : %u bytes\n\r", test_size);

    // Show auto-alignment info for 8D-8D-8D protocol
    if (test_protocol == MC80_OSPI_PROTOCOL_8D_8D_8D && (address_aligned || size_aligned))
    {
      MPRINTF("Note: Auto-aligned for 8D-8D-8D protocol ");
      if (address_aligned && size_aligned)
      {
        MPRINTF("(address & size)\n\r");
      }
      else if (address_aligned)
      {
        MPRINTF("(address only)\n\r");
      }
      else
      {
        MPRINTF("(size only)\n\r");
      }
    }
    MPRINTF("\n\rPress ENTER to continue, ESC to exit: ");

    // Wait for user confirmation
    uint8_t key = 0;
    if (WAIT_CHAR(&key, ms_to_ticks(600000)) != RES_OK)  // Wait up to 10 minutes
    {
      MPRINTF("TIMEOUT - continuing automatically\n\r");
      key = '\r';                                        // Auto-continue on timeout
    }
    MPRINTF("%c\n\r", key);

    if (key == VT100_ESC)
    {
      MPRINTF("Test sequence terminated by user\n\r");
      MPRINTF("Press any key to return to menu...\n\r");
      uint8_t dummy_key;
      WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
      return;
    }

    // Allocate buffer for random data
    uint8_t *write_buffer = (uint8_t *)App_malloc(test_size);
    if (write_buffer == NULL)
    {
      MPRINTF("ERROR: Failed to allocate %u bytes for write buffer\n\r", test_size);
      MPRINTF("Skipping this test...\n\r");
      MPRINTF("Press any key to continue...\n\r");
      uint8_t dummy_key;
      WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
      test_number++;
      continue;
    }

    // Allocate buffer for read verification
    uint8_t *read_buffer = (uint8_t *)App_malloc(test_size);
    if (read_buffer == NULL)
    {
      MPRINTF("ERROR: Failed to allocate %u bytes for read buffer\n\r");
      App_free(write_buffer);
      MPRINTF("Skipping this test...\n\r");
      MPRINTF("Press any key to continue...\n\r");
      uint8_t dummy_key;
      WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
      test_number++;
      continue;
    }

    bool test_success = true;

    // Step 1: Erase the test area (with retry attempts)
    MPRINTF("\n\r--- Step 1: Erasing test area ---\n\r");

    fsp_err_t erase_err           = FSP_ERR_ASSERTION;
    bool      erase_verified      = false;
    uint32_t  total_erase_time_us = 0;

    for (uint32_t retry_count = 1; retry_count <= OSPI_MAX_ERASE_RETRIES; retry_count++)
    {
      if (retry_count > 1)
      {
        MPRINTF("Erase attempt #%u (retry %u)...\n\r", retry_count, retry_count - 1);
      }
      else
      {
        MPRINTF("Erase attempt #%u...\n\r", retry_count);
      }

      T_sys_timestump start_time;
      Get_hw_timestump(&start_time);
      erase_err = Mc80_ospi_erase(g_mc80_ospi.p_ctrl, (uint8_t *)(MC80_OSPI_DEVICE_0_START_ADDRESS + test_address), test_size);
      T_sys_timestump end_time;
      Get_hw_timestump(&end_time);
      uint32_t current_erase_time_us = Timestump_diff_to_usec(&start_time, &end_time);
      total_erase_time_us += current_erase_time_us;

      if (erase_err == FSP_SUCCESS)
      {
        MPRINTF("Erase operation               : SUCCESS\n\r");
        MPRINTF("Erase time (attempt #%u)      : %u us\n\r", retry_count, current_erase_time_us);

        // Verify that erase was successful (all bytes should be 0xFF)
        MPRINTF("Verifying erase operation     : ");
        erase_verified = _Ospi_verify_erase_data(read_buffer, test_address, test_size);
        if (erase_verified)
        {
          MPRINTF("SUCCESS (all bytes are 0xFF)\n\r");
          break;  // Successful erase and verification, exit retry loop
        }
        else
        {
          MPRINTF("FAILED (some bytes are not 0xFF)\n\r");
          if (retry_count < OSPI_MAX_ERASE_RETRIES)
          {
            MPRINTF("Will retry erase operation...\n\r");
          }
        }
      }
      else
      {
        MPRINTF("Erase operation               : FAILED (error: 0x%X)\n\r", erase_err);
        if (retry_count < OSPI_MAX_ERASE_RETRIES)
        {
          MPRINTF("Will retry erase operation...\n\r");
        }
      }
    }

    // Final erase result evaluation
    result.erase_time_us = total_erase_time_us;
    if (erase_err == FSP_SUCCESS && erase_verified)
    {
      MPRINTF("Total erase time              : %u us\n\r", total_erase_time_us);
      MPRINTF("Erase speed                   : ");
      _Ospi_display_speed(test_size, total_erase_time_us);
    }
    else
    {
      MPRINTF("Erase operation FAILED after %u attempts\n\r", OSPI_MAX_ERASE_RETRIES);
      test_success = false;
    }

    // Step 2: Generate random data and write
    if (test_success)
    {
      MPRINTF("\n\r--- Step 2: Writing random data ---\n\r");

      // Generate random data using our PRNG - continues the same sequence
      for (uint32_t i = 0; i < test_size; i++)
      {
        write_buffer[i] = (uint8_t)_Ospi_prng_rand();
      }

      result.write_checksum = _Ospi_calculate_checksum(write_buffer, test_size);
      MPRINTF("Generated data checksum       : 0x%08X\n\r", result.write_checksum);

      T_sys_timestump start_time;
      Get_hw_timestump(&start_time);
      fsp_err_t       err = Mc80_ospi_memory_mapped_write(g_mc80_ospi.p_ctrl, write_buffer, (uint8_t *)(MC80_OSPI_DEVICE_0_START_ADDRESS + test_address), test_size);
      T_sys_timestump end_time;
      Get_hw_timestump(&end_time);
      result.write_time_us = Timestump_diff_to_usec(&start_time, &end_time);

      if (err != FSP_SUCCESS)
      {
        MPRINTF("Write operation               : FAILED (error: 0x%X)\n\r", err);
        test_success = false;
      }
      else
      {
        MPRINTF("Write operation               : SUCCESS\n\r");
        MPRINTF("Write time                    : %u us\n\r", result.write_time_us);
        MPRINTF("Write speed                   : ");
        _Ospi_display_speed(test_size, result.write_time_us);
      }
    }

    // Step 3: Read back and verify
    if (test_success)
    {
      MPRINTF("\n\r--- Step 3: Reading and verifying data ---\n\r");
      memset(read_buffer, 0x00, test_size);  // Clear read buffer

      T_sys_timestump start_time;
      Get_hw_timestump(&start_time);
      fsp_err_t       err = Mc80_ospi_memory_mapped_read(g_mc80_ospi.p_ctrl, read_buffer, test_address, test_size);
      T_sys_timestump end_time;
      Get_hw_timestump(&end_time);
      result.read_time_us = Timestump_diff_to_usec(&start_time, &end_time);

      if (err != FSP_SUCCESS)
      {
        MPRINTF("Read operation                : FAILED (error: 0x%X)\n\r", err);
        test_success = false;
      }
      else
      {
        MPRINTF("Read operation                : SUCCESS\n\r");
        MPRINTF("Read time                     : %u us\n\r", result.read_time_us);
        MPRINTF("Read speed                    : ");
        _Ospi_display_speed(test_size, result.read_time_us);

        result.read_checksum = _Ospi_calculate_checksum(read_buffer, test_size);
        MPRINTF("Read data checksum            : 0x%08X\n\r", result.read_checksum);
      }
    }

    // Step 4: Compare data buffers in detail
    if (test_success)
    {
      MPRINTF("\n\r--- Step 4: Detailed data comparison ---\n\r");
      result.checksum_match = (result.write_checksum == result.read_checksum);

      // Always perform detailed comparison even if checksums match
      bool data_match       = _Ospi_compare_buffers_detailed(write_buffer, read_buffer, test_size, test_address);

      if (result.checksum_match && data_match)
      {
        MPRINTF("Overall verification          : SUCCESS (checksums and data match)\n\r");
        result.test_passed = true;
      }
      else if (result.checksum_match && !data_match)
      {
        MPRINTF("Overall verification          : UNEXPECTED - checksums match but data differs\n\r");
        MPRINTF("This may indicate a CRC calculation issue\n\r");
        test_success = false;
      }
      else if (!result.checksum_match && data_match)
      {
        MPRINTF("Overall verification          : UNEXPECTED - data matches but checksums differ\n\r");
        MPRINTF("This may indicate a CRC calculation issue\n\r");
        test_success = false;
      }
      else
      {
        MPRINTF("Overall verification          : FAILED (both checksums and data differ)\n\r");
        test_success = false;
      }
    }

    // Clean up buffers
    App_free(write_buffer);
    App_free(read_buffer);

    // Brief pause before next test
    MPRINTF("\n\rPress any key for next test, ESC to exit...\n\r");
    uint8_t next_key = 0;
    if (WAIT_CHAR(&next_key, ms_to_ticks(600000)) == RES_OK)  // Wait up to 10 minutes
    {
      if (next_key == VT100_ESC)
      {
        MPRINTF("Test sequence terminated by user\n\r");
        MPRINTF("Press any key to return to menu...\n\r");
        uint8_t dummy_key;
        WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
        return;
      }
    }
    else
    {
      MPRINTF("TIMEOUT - continuing automatically\n\r");
    }

    test_number++;
  }
}
