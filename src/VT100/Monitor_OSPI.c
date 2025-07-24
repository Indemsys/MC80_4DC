/*-----------------------------------------------------------------------------------------------------
  Description: OSPI Flash memory testing terminal interface
               Provides direct access to OSPI flash operations for debugging

  Parameters:

  Return:
-----------------------------------------------------------------------------------------------------*/

#include "App.h"

// Test data sizes
#define OSPI_TEST_PATTERN_SIZE        256
#define OSPI_TEST_SECTOR_SIZE         4096

// Read operation sizes
#define OSPI_DIRECT_READ_SIZE         4096  // Size for direct read test
#define OSPI_MEMORY_MAPPED_READ_SIZE  4096  // Size for memory-mapped read test
#define OSPI_CONTINUOUS_READ_SIZE     128   // Size for continuous read test
#define OSPI_JEDEC_ID_SIZE            3     // JEDEC ID read size

// Write operation sizes
#define OSPI_MEMORY_MAPPED_WRITE_SIZE 256  // Size for memory-mapped write test
#define OSPI_SECTOR_WRITE_SIZE        256  // Size for sector test write chunks
#define OSPI_PREAMBLE_PATTERN_SIZE    256  // Size for preamble pattern write

// Display sizes
#define OSPI_DISPLAY_PREVIEW_SIZE     64              // Size for data preview display
#define OSPI_DISPLAY_MAX_SIZE         (128 * 1024)    // Maximum size for data display (128KB)

// Custom operation limits
#define OSPI_MAX_CUSTOM_SIZE          (1024 * 1024)  // Maximum 1MB for custom operations
#define OSPI_MIN_CUSTOM_SIZE          1               // Minimum 1 byte
#define OSPI_MAX_FLASH_ADDRESS        0x01FFFFFF      // 32MB flash size - 1
#define OSPI_DISPLAY_BYTES_PER_LINE   16              // Bytes per line for hex display

// Data pattern types
typedef enum
{
  OSPI_PATTERN_CONSTANT = 1,
  OSPI_PATTERN_INCREMENT,
  OSPI_PATTERN_RANDOM
} T_ospi_pattern_type;

// OSPI operation results structure
typedef struct
{
  uint32_t read_time_us;
  uint32_t write_time_us;
  uint32_t erase_time_us;
  uint32_t last_bytes_transferred;
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

// Function declarations
void OSPI_test_info(uint8_t keycode);
void OSPI_test_custom_operations(uint8_t keycode);

// Internal helper functions
static fsp_err_t            _Ospi_ensure_driver_open(void);
static T_mc80_ospi_protocol _Ospi_select_protocol(void);
static uint32_t             _Ospi_get_address_input(void);
static uint32_t             _Ospi_get_size_input(uint32_t max_size);
static T_ospi_pattern_type  _Ospi_get_pattern_type(void);
static uint8_t              _Ospi_get_pattern_value(void);
static void                 _Ospi_generate_pattern(uint8_t *buffer, uint32_t size, T_ospi_pattern_type type, uint8_t base_value);
static void                 _Ospi_display_data(uint8_t *data, uint32_t size, uint32_t start_address);
static void                 _Ospi_display_speed(uint32_t bytes, uint32_t time_us);
static void                 _Ospi_display_custom_menu(T_ospi_operation_settings *settings, T_ospi_operation_results *results);
static bool                 _Ospi_verify_write_data(uint8_t *original_data, uint32_t address, uint32_t size);
static bool                 _Ospi_verify_erase_data(uint32_t address, uint32_t size);

const T_VT100_Menu_item MENU_OSPI_ITEMS[] = {
  { '1', OSPI_test_info, 0 },
  { '4', OSPI_test_custom_operations, 0 },
  { 'R', 0, 0 },
  { 0 }
};

const T_VT100_Menu MENU_OSPI = {
  "OSPI Flash Testing",
  "\033[5C OSPI Flash memory testing menu\r\n"
  "\033[5C <1> - Flash information & status\r\n"
  "\033[5C <4> - Custom operations (read/write/erase)\r\n"
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
        MPRINTF("Random (LFSR)\n\r");
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
  }
  else
  {
    MPRINTF("No operations performed yet\n\r");
  }

  // Display menu options
  MPRINTF("\n\r===== Operations Menu =====\n\r");
  MPRINTF("  <1> - Configure settings\n\r");
  MPRINTF("  <2> - Read operation\n\r");
  MPRINTF("  <3> - Write operation\n\r");
  MPRINTF("  <4> - Erase operation\n\r");
  MPRINTF("  <5> - Fast read benchmark\n\r");
  MPRINTF("  <6> - Switch protocol\n\r");
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

  uint8_t key;
  WAIT_CHAR(&key, ms_to_ticks(30000));

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
  Description: Custom OSPI operations menu - read, write, erase with user-defined parameters

  Parameters: keycode - Input key code from VT100 terminal

  Return:
-----------------------------------------------------------------------------------------------------*/
void OSPI_test_custom_operations(uint8_t keycode)
{
  GET_MCBL;

  // Static variables to preserve settings and results between calls
  static T_ospi_operation_settings settings = { 0 };
  static T_ospi_operation_results results = { 0 };

  // Initialize default settings on first run
  if (!settings.settings_valid)
  {
    settings.address = 0x00000000;
    settings.size = 4096;
    settings.pattern_type = OSPI_PATTERN_CONSTANT;
    settings.pattern_value = 0x55;
    settings.protocol = MC80_OSPI_PROTOCOL_1S_1S_1S;
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

    uint8_t operation;
    WAIT_CHAR(&operation, ms_to_ticks(30000));
    MPRINTF("%c\n\r", operation);

    switch (operation)
    {
      case '1': // Configure settings
      {
        MPRINTF("\n\r===== Configure Settings =====\n\r");

        // Get new address
        MPRINTF("Current address: 0x%08X\n\r", settings.address);
        MPRINTF("Enter new address or press ENTER to keep current: ");

        uint8_t first_key;
        WAIT_CHAR(&first_key, ms_to_ticks(30000));

        if (first_key == '\r' || first_key == '\n')
        {
          MPRINTF("ENTER - keeping current address\n\r");
        }
        else
        {
          // Put the first character back and get full address
          MPRINTF("0x%c", first_key);
          uint32_t new_address = _Ospi_get_address_input();
          if (new_address <= OSPI_MAX_FLASH_ADDRESS)
          {
            settings.address = new_address;
          }
          else
          {
            MPRINTF("ERROR: Address exceeds flash size, keeping current\n\r");
          }
        }

        // Get new size
        MPRINTF("Current size: %u bytes\n\r", settings.size);
        uint32_t new_size = _Ospi_get_size_input(OSPI_MAX_CUSTOM_SIZE);
        if (new_size > 0 && (settings.address + new_size - 1) <= OSPI_MAX_FLASH_ADDRESS)
        {
          settings.size = new_size;
        }
        else
        {
          MPRINTF("ERROR: Invalid size or address range, keeping current\n\r");
        }

        // Get new pattern
        settings.pattern_type = _Ospi_get_pattern_type();
        if (settings.pattern_type == OSPI_PATTERN_CONSTANT || settings.pattern_type == OSPI_PATTERN_INCREMENT)
        {
          settings.pattern_value = _Ospi_get_pattern_value();
        }

        MPRINTF("Settings updated successfully\n\r");
        MPRINTF("Press any key to continue...\n\r");
        uint8_t dummy_key;
        WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
        break;
      }

      case '2': // Read operation
      {
        MPRINTF("\n\r===== Read Operation =====\n\r");

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
        fsp_err_t err = Mc80_ospi_memory_mapped_read(g_mc80_ospi.p_ctrl, read_buffer, settings.address, settings.size);
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

          // Update results
          results.read_time_us = elapsed_us;
          results.last_bytes_transferred = settings.size;
          results.results_valid = true;
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

      case '3': // Write operation
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
        fsp_err_t err = Mc80_ospi_memory_mapped_write(g_mc80_ospi.p_ctrl, write_buffer, (uint8_t *)(MC80_OSPI_DEVICE_0_START_ADDRESS + settings.address), settings.size);
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
            results.write_time_us = elapsed_us;
            results.last_bytes_transferred = settings.size;
            results.results_valid = true;
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

      case '4': // Erase operation
      {
        MPRINTF("\n\r===== Erase Operation =====\n\r");
        MPRINTF("WARNING: This will erase %u bytes starting from address 0x%08X\n\r", settings.size, settings.address);
        MPRINTF("Continue? (Y/N): ");

        uint8_t confirm;
        WAIT_CHAR(&confirm, ms_to_ticks(10000));
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
        fsp_err_t err = Mc80_ospi_erase(g_mc80_ospi.p_ctrl, (uint8_t *)(MC80_OSPI_DEVICE_0_START_ADDRESS + settings.address), settings.size);
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
          bool verify_ok = _Ospi_verify_erase_data(settings.address, settings.size);
          if (verify_ok)
          {
            MPRINTF("Erase verification            : SUCCESS\n\r");

            // Update results only if verification passed
            results.erase_time_us = elapsed_us;
            results.last_bytes_transferred = settings.size;
            results.results_valid = true;
          }
          else
          {
            MPRINTF("Erase verification            : FAILED\n\r");
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

      case '5': // Fast read benchmark
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
        fsp_err_t err = Mc80_ospi_memory_mapped_read(g_mc80_ospi.p_ctrl, read_buffer, settings.address, benchmark_size);
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

          // Update results
          results.read_time_us = elapsed_us;
          results.last_bytes_transferred = benchmark_size;
          results.results_valid = true;
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

      case '6': // Switch protocol
      {
        MPRINTF("\n\r===== Protocol Switch =====\n\r");
        T_mc80_ospi_protocol new_protocol = _Ospi_select_protocol();

        MPRINTF("Switching protocol...\n\r");
        fsp_err_t err = Mc80_ospi_spi_protocol_switch_safe(g_mc80_ospi.p_ctrl, new_protocol);
        if (err == FSP_SUCCESS)
        {
          MPRINTF("Protocol switch               : SUCCESS\n\r");
          settings.protocol = new_protocol;
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

  MPRINTF("Enter address (hex, e.g., 1000 for 0x1000): 0x");

  char input_buffer[16];
  memset(input_buffer, 0, sizeof(input_buffer));

  uint8_t pos = 0;
  while (pos < 15)
  {
    uint8_t key;
    WAIT_CHAR(&key, ms_to_ticks(30000));

    if (key == '\r' || key == '\n')
    {
      break;
    }
    else if (key == '\b' || key == 0x7F) // Backspace
    {
      if (pos > 0)
      {
        pos--;
        input_buffer[pos] = 0;
        MPRINTF("\b \b");
      }
    }
    else if ((key >= '0' && key <= '9') || (key >= 'A' && key <= 'F') || (key >= 'a' && key <= 'f'))
    {
      input_buffer[pos] = key;
      pos++;
      MPRINTF("%c", key);
    }
  }

  MPRINTF("\n\r");

  // Convert hex string to number
  for (uint8_t i = 0; i < pos; i++)
  {
    address = address * 16;
    char c = input_buffer[i];
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

  MPRINTF("Enter size in bytes (decimal, max %u): ", max_size);

  char input_buffer[16];
  memset(input_buffer, 0, sizeof(input_buffer));

  uint8_t pos = 0;
  while (pos < 15)
  {
    uint8_t key;
    WAIT_CHAR(&key, ms_to_ticks(30000));

    if (key == '\r' || key == '\n')
    {
      break;
    }
    else if (key == '\b' || key == 0x7F) // Backspace
    {
      if (pos > 0)
      {
        pos--;
        input_buffer[pos] = 0;
        MPRINTF("\b \b");
      }
    }
    else if (key >= '0' && key <= '9')
    {
      input_buffer[pos] = key;
      pos++;
      MPRINTF("%c", key);
    }
  }

  MPRINTF("\n\r");

  // Convert decimal string to number
  for (uint8_t i = 0; i < pos; i++)
  {
    size = size * 10 + (input_buffer[i] - '0');
  }

  if (size > max_size)
  {
    MPRINTF("WARNING: Size %u exceeds maximum %u, using maximum\n\r", size, max_size);
    size = max_size;
  }

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

  uint8_t key;
  WAIT_CHAR(&key, ms_to_ticks(30000));
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

  MPRINTF("Enter pattern value (hex, e.g., FF): 0x");

  char input_buffer[4];
  memset(input_buffer, 0, sizeof(input_buffer));

  uint8_t pos = 0;
  while (pos < 2)
  {
    uint8_t key;
    WAIT_CHAR(&key, ms_to_ticks(30000));

    if (key == '\r' || key == '\n')
    {
      break;
    }
    else if (key == '\b' || key == 0x7F) // Backspace
    {
      if (pos > 0)
      {
        pos--;
        input_buffer[pos] = 0;
        MPRINTF("\b \b");
      }
    }
    else if ((key >= '0' && key <= '9') || (key >= 'A' && key <= 'F') || (key >= 'a' && key <= 'f'))
    {
      input_buffer[pos] = key;
      pos++;
      MPRINTF("%c", key);
    }
  }

  MPRINTF("\n\r");

  // Convert hex string to number
  for (uint8_t i = 0; i < pos; i++)
  {
    value = value * 16;
    char c = input_buffer[i];
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
      // Use simple LFSR for pseudo-random generation
      uint32_t lfsr = 0xACE1u; // Seed value
      for (uint32_t i = 0; i < size; i++)
      {
        // 16-bit LFSR with taps at positions 16, 15, 13, 4
        uint32_t bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5)) & 1u;
        lfsr = (lfsr >> 1) | (bit << 15);
        buffer[i] = (uint8_t)(lfsr & 0xFF);
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
    uint64_t speed_bps_64 = ((uint64_t)bytes * 1000000ul) / time_us;  // Bytes per second (64-bit calculation)
    uint32_t speed_bps = (uint32_t)speed_bps_64;                      // Bytes per second
    uint32_t speed_kbps = speed_bps / 1024;                           // Kilobytes per second (integer part)
    uint32_t speed_kbps_frac = ((speed_bps % 1024) * 1000) / 1024;    // Fractional part (3 digits)
    uint32_t time_ms = time_us / 1000;                                // Convert to milliseconds for display
    uint32_t time_us_frac = time_us % 1000;                           // Microseconds fractional part

    MPRINTF("%u.%03u KB/s (%u bytes/s)\n\r", speed_kbps, speed_kbps_frac, speed_bps);
    MPRINTF("Time                          : %u.%03u ms (%u us)\n\r", time_ms, time_us_frac, time_us);
  }
  else
  {
    MPRINTF("> 1000 KB/s (operation too fast to measure)\n\r");
  }
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

  Parameters: address - Flash address that was erased
              size - Size of erased area in bytes

  Return: true if verification successful (all bytes are 0xFF), false otherwise
-----------------------------------------------------------------------------------------------------*/
static bool _Ospi_verify_erase_data(uint32_t address, uint32_t size)
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

  // Check that all bytes are 0xFF (erased state)
  bool all_erased = true;
  for (uint32_t i = 0; i < size; i++)
  {
    if (verify_buffer[i] != 0xFF)
    {
      all_erased = false;
      break;
    }
  }

  App_free(verify_buffer);
  return all_erased;
}
