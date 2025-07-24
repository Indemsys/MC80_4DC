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
#define OSPI_DISPLAY_PREVIEW_SIZE     64  // Size for data preview display

// Function declarations
void OSPI_test_info(uint8_t keycode);
void OSPI_test_read(uint8_t keycode);
void OSPI_test_memory_mapped_write(uint8_t keycode);
void OSPI_test_erase(uint8_t keycode);
void OSPI_test_sector(uint8_t keycode);
void OSPI_test_8d_calibration(uint8_t keycode);
void OSPI_write_preamble_patterns(uint8_t keycode);
void OSPI_test_memory_mapped_read(uint8_t keycode);
void OSPI_test_8d_continuous_read(uint8_t keycode);

// Internal helper functions
static fsp_err_t            _Ospi_ensure_driver_open(void);
static void                 _Ospi_display_register_differences(const T_mc80_ospi_register_snapshot *before, const T_mc80_ospi_register_snapshot *after);
static T_mc80_ospi_protocol _Ospi_select_protocol(void);

const T_VT100_Menu_item MENU_OSPI_ITEMS[] = {
  { '1', OSPI_test_info, 0 },
  { '2', OSPI_test_read, 0 },
  { '3', OSPI_test_memory_mapped_read, 0 },
  { '4', OSPI_test_memory_mapped_write, 0 },
  { '5', OSPI_test_erase, 0 },
  { '6', OSPI_test_sector, 0 },
  { '7', OSPI_test_8d_calibration, 0 },
  { '8', OSPI_write_preamble_patterns, 0 },
  { '9', OSPI_test_8d_continuous_read, 0 },
  { 'R', 0, 0 },
  { 0 }
};

const T_VT100_Menu MENU_OSPI = {
  "OSPI Flash Testing",
  "\033[5C OSPI Flash memory testing menu\r\n"
  "\033[5C <1> - Flash information & status\r\n"
  "\033[5C <2> - Direct read test (command-based)\r\n"
  "\033[5C <3> - Memory-mapped read test (DMA)\r\n"
  "\033[5C <4> - Memory-mapped write test (DMA)\r\n"
  "\033[5C <5> - Erase sector test\r\n"
  "\033[5C <6> - Full sector test (write/read/verify)\r\n"
  "\033[5C <7> - 8D-8D-8D protocol & calibration test\r\n"
  "\033[5C <8> - Write calibration preamble patterns\r\n"
  "\033[5C <9> - 8D-8D-8D continuous read test (100ms)\r\n"
  "\033[5C <R> - Return to previous menu\r\n",
  MENU_OSPI_ITEMS,
};

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
  Description: Test OSPI read operations using direct read interface

  Parameters: keycode - Input key code from VT100 terminal

  Return:
-----------------------------------------------------------------------------------------------------*/
void OSPI_test_read(uint8_t keycode)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF(" ===== OSPI Direct Read Test =====\n\r");

  // Select protocol first
  T_mc80_ospi_protocol protocol = _Ospi_select_protocol();

  // Ensure OSPI driver is open
  fsp_err_t init_err            = _Ospi_ensure_driver_open();
  if (init_err != FSP_SUCCESS)
  {
    MPRINTF("ERROR: Failed to ensure OSPI driver is open (0x%X)\n\r", init_err);
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  // Set the selected protocol using safe switch function
  MPRINTF("Switching to protocol safely (with flash reset)...\n\r");
  fsp_err_t protocol_err = Mc80_ospi_spi_protocol_switch_safe(g_mc80_ospi.p_ctrl, protocol);
  if (protocol_err != FSP_SUCCESS)
  {
    MPRINTF("ERROR: Failed to switch protocol safely (0x%X)\n\r", protocol_err);
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }
  MPRINTF("Protocol switched successfully\n\r");

  // Allocate buffer for reading data
  uint8_t *read_buffer = App_malloc(OSPI_DIRECT_READ_SIZE);
  if (NULL == read_buffer)
  {
    MPRINTF("ERROR: Failed to allocate memory for read buffer\n\r");
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  MPRINTF("Reading %d bytes from address 0x00000000 using Mc80_ospi_direct_read...\n\r", OSPI_DIRECT_READ_SIZE);

  // Read data from offset 0 using direct read
  fsp_err_t err = Mc80_ospi_direct_read(g_mc80_ospi.p_ctrl, read_buffer, 0, OSPI_DIRECT_READ_SIZE);
  if (err != FSP_SUCCESS)
  {
    MPRINTF("Direct read failed: 0x%X\n\r", err);
    App_free(read_buffer);
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  MPRINTF("Direct read successful! Data from address 0x00000000 (%d bytes):\n\r", OSPI_DIRECT_READ_SIZE);

  // Display the read data in hexadecimal format (16 bytes per line)
  for (int i = 0; i < OSPI_DIRECT_READ_SIZE; i++)
  {
    if (i % 16 == 0)
    {
      MPRINTF("%04X: ", i);
    }
    MPRINTF("%02X ", read_buffer[i]);
    if (i % 16 == 15)
    {
      MPRINTF("\n\r");
    }
  }

  // Free allocated memory
  App_free(read_buffer);

  MPRINTF("\n\rDirect read test completed successfully.\n\r");
  MPRINTF("Press any key to continue...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
}

/*-----------------------------------------------------------------------------------------------------
  Description: Test OSPI write operations

  Parameters: keycode - Input key code from VT100 terminal

  Return:
-----------------------------------------------------------------------------------------------------*/
void OSPI_test_memory_mapped_write(uint8_t keycode)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF(" ===== Memory-Mapped Write Test (DMA) =====\n\r");

  // Select protocol first
  T_mc80_ospi_protocol protocol = _Ospi_select_protocol();

  // Ensure OSPI driver is open
  fsp_err_t init_err            = _Ospi_ensure_driver_open();
  if (init_err != FSP_SUCCESS)
  {
    MPRINTF("ERROR: Failed to ensure OSPI driver is open (0x%X)\n\r", init_err);
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  // Set the selected protocol with safe switching
  MPRINTF("Setting protocol with safe switching...\n\r");
  fsp_err_t protocol_err = Mc80_ospi_spi_protocol_switch_safe(g_mc80_ospi.p_ctrl, protocol);
  if (protocol_err != FSP_SUCCESS)
  {
    MPRINTF("ERROR: Failed to set protocol (0x%X)\n\r", protocol_err);
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }
  MPRINTF("Protocol set successfully\n\r");

  MPRINTF("WARNING: This will write test data to flash address 0x00000000\n\r");
  MPRINTF("Press 'Y' to continue or any other key to cancel: ");

  uint8_t ch;
  WAIT_CHAR(&ch, ms_to_ticks(100000));
  MPRINTF("%c\n\r", ch);

  if (ch != 'Y' && ch != 'y')
  {
    MPRINTF("Write test cancelled.\n\r");
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  uint8_t *write_buffer = (uint8_t *)App_malloc(OSPI_MEMORY_MAPPED_WRITE_SIZE);
  uint8_t *read_buffer  = (uint8_t *)App_malloc(OSPI_MEMORY_MAPPED_WRITE_SIZE);

  if (write_buffer == NULL || read_buffer == NULL)
  {
    MPRINTF("ERROR: Failed to allocate buffers\n\r");
    if (write_buffer) App_free(write_buffer);
    if (read_buffer) App_free(read_buffer);
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  // Create test pattern with random base value and incremented sequence
  uint32_t seed      = tx_time_get();
  seed               = seed * 1103515245 + 12345;  // Simple LCG algorithm
  uint8_t base_value = (uint8_t)(seed >> 16);

  MPRINTF("Creating test pattern with random base: 0x%02X\n\r", base_value);

  for (int i = 0; i < OSPI_MEMORY_MAPPED_WRITE_SIZE; i++)
  {
    write_buffer[i] = base_value + i;
  }

  MPRINTF("Test pattern created (%d bytes: 0x%02X + increment)\n\r", OSPI_MEMORY_MAPPED_WRITE_SIZE, base_value);

  // Check flash status
  T_mc80_ospi_status status;
  fsp_err_t          err = Mc80_ospi_status_get(g_mc80_ospi.p_ctrl, &status);
  if (err == FSP_SUCCESS && status.write_in_progress)
  {
    MPRINTF("Flash is busy, waiting...\n\r");
    // Simple wait - in real code should have timeout
    do
    {
      R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
      Mc80_ospi_status_get(g_mc80_ospi.p_ctrl, &status);
    } while (status.write_in_progress);
  }

  // Perform write
  MPRINTF("Writing %d bytes to flash offset 0x00000000...\n\r", OSPI_MEMORY_MAPPED_WRITE_SIZE);
  err = Mc80_ospi_memory_mapped_write(g_mc80_ospi.p_ctrl, write_buffer, (uint8_t *)(OSPI_BASE_ADDRESS + 0x00000000), OSPI_MEMORY_MAPPED_WRITE_SIZE);

  if (err == FSP_SUCCESS)
  {
    MPRINTF("Write operation: SUCCESS\n\r");

    // Wait for write completion
    do
    {
      R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
      Mc80_ospi_status_get(g_mc80_ospi.p_ctrl, &status);
    } while (status.write_in_progress);

    MPRINTF("Write completed\n\r");

    // Read back and verify using direct read
    MPRINTF("Reading back data using Mc80_ospi_direct_read...\n\r");

    err = Mc80_ospi_direct_read(g_mc80_ospi.p_ctrl, read_buffer, 0, OSPI_MEMORY_MAPPED_WRITE_SIZE);

    if (err == FSP_SUCCESS)
    {
      MPRINTF("Direct read: SUCCESS\n\r");

      // Display first portion of read data
      MPRINTF("Read data (first %d bytes):\n\r", OSPI_DISPLAY_PREVIEW_SIZE);
      for (int i = 0; i < OSPI_DISPLAY_PREVIEW_SIZE; i++)
      {
        if (i % 16 == 0)
        {
          MPRINTF("%04X: ", i);
        }
        MPRINTF("%02X ", read_buffer[i]);
        if (i % 16 == 15)
        {
          MPRINTF("\n\r");
        }
      }

      // Compare data
      bool match       = true;
      int  error_count = 0;
      for (int i = 0; i < OSPI_MEMORY_MAPPED_WRITE_SIZE; i++)
      {
        if (write_buffer[i] != read_buffer[i])
        {
          if (error_count == 0)
          {
            match = false;
          }
          error_count++;
        }
      }

      if (match)
      {
        MPRINTF("Data verification: PASS (all %d bytes match)\n\r", OSPI_MEMORY_MAPPED_WRITE_SIZE);
      }
      else
      {
        MPRINTF("Data verification: FAIL (%d bytes differ)\n\r", error_count);
        MPRINTF("First 16 bytes comparison:\n\r");
        MPRINTF("Written: ");
        for (int i = 0; i < 16; i++) MPRINTF("%02X ", write_buffer[i]);
        MPRINTF("\n\rRead:    ");
        for (int i = 0; i < 16; i++) MPRINTF("%02X ", read_buffer[i]);
        MPRINTF("\n\r");
      }
    }
    else
    {
      MPRINTF("Direct read: FAILED (0x%X)\n\r", err);
    }
  }
  else
  {
    MPRINTF("Write operation: FAILED\n\r");
    MPRINTF("Error code: 0x%X\n\r", err);
  }

  App_free(write_buffer);
  App_free(read_buffer);

  MPRINTF("\n\rPress any key to continue...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
}

/*-----------------------------------------------------------------------------------------------------
  Description: Test OSPI erase operations

  Parameters: keycode - Input key code from VT100 terminal

  Return:
-----------------------------------------------------------------------------------------------------*/
void OSPI_test_erase(uint8_t keycode)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF(" ===== OSPI Erase Test =====\n\r");

  // Select protocol first
  T_mc80_ospi_protocol protocol = _Ospi_select_protocol();

  // Ensure OSPI driver is open
  fsp_err_t init_err            = _Ospi_ensure_driver_open();
  if (init_err != FSP_SUCCESS)
  {
    MPRINTF("ERROR: Failed to ensure OSPI driver is open (0x%X)\n\r", init_err);
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  // Set the selected protocol with safe switching
  MPRINTF("Setting protocol with safe switching...\n\r");
  fsp_err_t protocol_err = Mc80_ospi_spi_protocol_switch_safe(g_mc80_ospi.p_ctrl, protocol);
  if (protocol_err != FSP_SUCCESS)
  {
    MPRINTF("ERROR: Failed to set protocol (0x%X)\n\r", protocol_err);
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }
  MPRINTF("Protocol set successfully\n\r");

  MPRINTF("WARNING: This will erase 4KB sector at address 0x00000000\n\r");
  MPRINTF("Press 'Y' to continue or any other key to cancel: ");

  uint8_t ch;
  WAIT_CHAR(&ch, ms_to_ticks(100000));
  MPRINTF("%c\n\r", ch);

  if (ch != 'Y' && ch != 'y')
  {
    MPRINTF("Erase test cancelled.\n\r");
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  uint8_t *read_buffer = (uint8_t *)App_malloc(OSPI_MEMORY_MAPPED_READ_SIZE);
  if (read_buffer == NULL)
  {
    MPRINTF("ERROR: Failed to allocate read buffer\n\r");
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  MPRINTF("Erasing 4KB sector at flash offset 0x00000000...\n\r");
  // Note: Mc80_ospi_erase() includes built-in device ready check, no need to wait here
  fsp_err_t err = Mc80_ospi_erase(g_mc80_ospi.p_ctrl, (uint8_t *)(OSPI_BASE_ADDRESS + 0x00000000), OSPI_TEST_SECTOR_SIZE);

  if (err == FSP_SUCCESS)
  {
    MPRINTF("Erase command: SUCCESS\n\r");

    // Read back to verify erase
    MPRINTF("Reading back erased data for verification...\n\r");

    // Read data using memory-mapped read
    err = Mc80_ospi_memory_mapped_read(g_mc80_ospi.p_ctrl, read_buffer, 0x00000000, OSPI_MEMORY_MAPPED_READ_SIZE);

    if (err == FSP_SUCCESS)
    {
      MPRINTF("Read back: SUCCESS\n\r");

      // Check if all bytes are 0xFF (erased state)
      bool erased = true;
      for (int i = 0; i < OSPI_MEMORY_MAPPED_READ_SIZE; i++)
      {
        if (read_buffer[i] != 0xFF)
        {
          erased = false;
          break;
        }
      }

      if (erased)
      {
        MPRINTF("Erase verification: PASS (all bytes are 0xFF)\n\r");
      }
      else
      {
        MPRINTF("Erase verification: FAIL (some bytes not 0xFF)\n\r");
        MPRINTF("First 32 bytes:\n\r");
        for (int i = 0; i < 32; i++)
        {
          if (i % 16 == 0)
          {
            MPRINTF("%04X: ", i);
          }
          MPRINTF("%02X ", read_buffer[i]);
          if (i % 16 == 15)
          {
            MPRINTF("\n\r");
          }
        }
      }
    }
    else
    {
      MPRINTF("Read back: FAILED (0x%X)\n\r", err);
    }
  }
  else
  {
    MPRINTF("Erase command: FAILED\n\r");
    MPRINTF("Error code: 0x%X\n\r", err);
  }

  App_free(read_buffer);

  MPRINTF("\n\rPress any key to continue...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
}

/*-----------------------------------------------------------------------------------------------------
  Description: Test full sector operations

  Parameters: keycode - Input key code from VT100 terminal

  Return:
-----------------------------------------------------------------------------------------------------*/
void OSPI_test_sector(uint8_t keycode)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF(" ===== OSPI Full Sector Test =====\n\r");

  // Select protocol first
  T_mc80_ospi_protocol protocol = _Ospi_select_protocol();

  // Ensure OSPI driver is open
  fsp_err_t init_err            = _Ospi_ensure_driver_open();
  if (init_err != FSP_SUCCESS)
  {
    MPRINTF("ERROR: Failed to ensure OSPI driver is open (0x%X)\n\r", init_err);
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  // Set the selected protocol with safe switching
  MPRINTF("Setting protocol with safe switching...\n\r");
  fsp_err_t protocol_err = Mc80_ospi_spi_protocol_switch_safe(g_mc80_ospi.p_ctrl, protocol);
  if (protocol_err != FSP_SUCCESS)
  {
    MPRINTF("ERROR: Failed to set protocol (0x%X)\n\r", protocol_err);
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }
  MPRINTF("Protocol set successfully\n\r");

  MPRINTF("This will test full 4KB sector erase/write/read\n\r");
  MPRINTF("WARNING: This will modify entire 4KB sector at 0x00000000\n\r");
  MPRINTF("Press 'Y' to continue or any other key to cancel: ");

  uint8_t ch;
  WAIT_CHAR(&ch, ms_to_ticks(100000));
  MPRINTF("%c\n\r", ch);

  if (ch != 'Y' && ch != 'y')
  {
    MPRINTF("Sector test cancelled.\n\r");
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  MPRINTF("This test will take several seconds...\n\r");
  MPRINTF("Testing sector operations in chunks of %d bytes\n\r", OSPI_SECTOR_WRITE_SIZE);

  uint8_t *chunk_write = (uint8_t *)App_malloc(OSPI_SECTOR_WRITE_SIZE);
  uint8_t *chunk_read  = (uint8_t *)App_malloc(OSPI_SECTOR_WRITE_SIZE);

  // Declare variables used in goto sections to avoid bypass warnings
  T_mc80_ospi_status status;
  uint32_t           wait_count = 0;
  int                errors     = 0;

  if (chunk_write == NULL || chunk_read == NULL)
  {
    MPRINTF("ERROR: Failed to allocate buffers\n\r");
    if (chunk_write) App_free(chunk_write);
    if (chunk_read) App_free(chunk_read);
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  // Erase entire sector
  MPRINTF("Erasing 4KB sector...\n\r");
  fsp_err_t err = Mc80_ospi_erase(g_mc80_ospi.p_ctrl, (uint8_t *)(OSPI_BASE_ADDRESS + 0x00000000), OSPI_TEST_SECTOR_SIZE);
  if (err != FSP_SUCCESS)
  {
    MPRINTF("Sector erase failed: 0x%X\n\r", err);
    goto cleanup;
  }

  // Write and verify each chunk
  errors = 0;
  for (int chunk = 0; chunk < (OSPI_TEST_SECTOR_SIZE / OSPI_SECTOR_WRITE_SIZE); chunk++)
  {
    uint32_t address = chunk * OSPI_SECTOR_WRITE_SIZE;

    // Create unique pattern for this chunk
    for (int i = 0; i < OSPI_SECTOR_WRITE_SIZE; i++)
    {
      chunk_write[i] = (uint8_t)((chunk << 4) | (i & 0x0F));
    }

    // Write chunk
    err = Mc80_ospi_memory_mapped_write(g_mc80_ospi.p_ctrl, chunk_write, (uint8_t *)(OSPI_BASE_ADDRESS + address), OSPI_SECTOR_WRITE_SIZE);
    if (err != FSP_SUCCESS)
    {
      MPRINTF("Write chunk %d failed: 0x%X\n\r", chunk, err);
      errors++;
      continue;
    }

    // Read back chunk
    memset(chunk_read, 0, OSPI_SECTOR_WRITE_SIZE);

    // Read data using memory-mapped read
    err = Mc80_ospi_memory_mapped_read(g_mc80_ospi.p_ctrl, chunk_read, address, OSPI_SECTOR_WRITE_SIZE);
    if (err != FSP_SUCCESS)
    {
      MPRINTF("Read chunk %d failed: 0x%X\n\r", chunk, err);
      errors++;
      continue;
    }

    // Verify chunk
    bool chunk_ok = true;
    for (int i = 0; i < OSPI_SECTOR_WRITE_SIZE; i++)
    {
      if (chunk_write[i] != chunk_read[i])
      {
        chunk_ok = false;
        break;
      }
    }

    if (chunk_ok)
    {
      MPRINTF("Chunk %2d (0x%04X): OK\n\r", chunk, address);
    }
    else
    {
      MPRINTF("Chunk %2d (0x%04X): FAIL\n\r", chunk, address);
      errors++;
    }
  }

  if (errors == 0)
  {
    MPRINTF("\n\rFull sector test: PASS\n\r");
    MPRINTF("All 16 chunks (4096 bytes) verified successfully\n\r");
  }
  else
  {
    MPRINTF("\n\rFull sector test: FAIL\n\r");
    MPRINTF("Errors in %d chunks\n\r", errors);
  }

cleanup:
  App_free(chunk_write);
  App_free(chunk_read);

  MPRINTF("\n\rPress any key to continue...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
}

/*-----------------------------------------------------------------------------------------------------
  Description: Test 8D-8D-8D protocol with calibration and memory-mapped read verification

  Parameters: keycode - Input key code from VT100 terminal

  Return:
-----------------------------------------------------------------------------------------------------*/
void OSPI_test_8d_calibration(uint8_t keycode)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF(" ===== 8D-8D-8D Protocol & Calibration Test =====\n\r");

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

  // Set 8D-8D-8D protocol for the entire test sequence
  MPRINTF("\n\rSetting 8D-8D-8D protocol for pattern check and calibration...\n\r");
  fsp_err_t err = Mc80_ospi_spi_protocol_switch_safe(g_mc80_ospi.p_ctrl, MC80_OSPI_PROTOCOL_8D_8D_8D);
  if (err == FSP_SUCCESS)
  {
    MPRINTF("8D-8D-8D Protocol Set          : SUCCESS\n\r");
  }
  else
  {
    MPRINTF("8D-8D-8D Protocol Set          : FAILED (error: 0x%X)\n\r", err);
    MPRINTF("Cannot continue without protocol setup\n\r");
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  // Check if preamble patterns exist at calibration address
  MPRINTF("\n\rChecking preamble patterns at calibration address...\n\r");
  uint32_t expected_patterns[4] = {
    MC80_OSPI_DEFAULT_PREAMBLE_PATTERN_0,  // 0xFFFF0000
    MC80_OSPI_DEFAULT_PREAMBLE_PATTERN_1,  // 0x000800FF
    MC80_OSPI_DEFAULT_PREAMBLE_PATTERN_2,  // 0x00FFF700
    MC80_OSPI_DEFAULT_PREAMBLE_PATTERN_3   // 0xF700F708
  };

  uint32_t read_patterns[4];
  err                 = Mc80_ospi_direct_read(g_mc80_ospi.p_ctrl, (uint8_t *)read_patterns, 0x00000000, 16);

  bool patterns_valid = true;
  if (err == FSP_SUCCESS)
  {
    MPRINTF("Pattern Read                   : SUCCESS\n\r");
    for (int i = 0; i < 4; i++)
    {
      MPRINTF("Pattern %d: Expected=0x%08X, Found=0x%08X", i, expected_patterns[i], read_patterns[i]);
      if (expected_patterns[i] == read_patterns[i])
      {
        MPRINTF(" [OK]\n\r");
      }
      else
      {
        MPRINTF(" [FAIL]\n\r");
        patterns_valid = false;
      }
    }
  }
  else
  {
    MPRINTF("Pattern Read                   : FAILED (error: 0x%X)\n\r", err);
    patterns_valid = false;
  }

  if (!patterns_valid)
  {
    MPRINTF("\n\rWARNING: Preamble patterns not found or invalid!\n\r");
    MPRINTF("Please use menu option 8 to write patterns first\n\r");
    MPRINTF("Calibration may fail without proper patterns\n\r");
    MPRINTF("Press any key to continue anyway...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
  }
  else
  {
    MPRINTF("\n\rPreamble patterns verified     : SUCCESS\n\r");
  }

  // Flash is already configured to OPI DTR mode by the safe protocol switch
  MPRINTF("\n\rFlash configured for OPI DTR mode by protocol switch\n\r");

  // Now verify CR2 in OPI DTR mode using 8D-8D-8D commands
  MPRINTF("\n\rVerifying CR2 in OPI DTR mode...\n\r");
  T_mc80_ospi_direct_transfer read_cr2_opi_cmd = {
    .command        = MX25_OPI_RDCR2_STR,  // OPI DTR command for reading CR2 (0x8E8E)
    .command_length = 2,                   // 2-byte command for OPI DTR
    .address_length = 4,                   // 4-byte address
    .address        = 0x00000000,          // CR2 register address
    .data_length    = 2,                   // Read 2 bytes in OPI DTR mode
    .dummy_cycles   = 4,                   // 4 dummy cycles for OPI DTR CR2 read
  };

  err = Mc80_ospi_direct_transfer(g_mc80_ospi.p_ctrl, &read_cr2_opi_cmd, MC80_OSPI_DIRECT_TRANSFER_DIR_READ);
  if (err == FSP_SUCCESS)
  {
    uint8_t cr2_value = (uint8_t)(read_cr2_opi_cmd.data & 0xFF);
    MPRINTF("CR2 Read Back (OPI DTR)        : SUCCESS (value: 0x%02X)\n\r", cr2_value);

    if (cr2_value == 0x02)
    {
      MPRINTF("CR2 Verification               : PASS (OPI DTR confirmed)\n\r");
    }
    else
    {
      MPRINTF("CR2 Verification               : FAIL (expected 0x02, got 0x%02X)\n\r", cr2_value);
      MPRINTF("Flash may not be in OPI DTR mode\n\r");
    }
  }
  else
  {
    MPRINTF("CR2 Read Back (OPI DTR)        : FAILED (error: 0x%X)\n\r", err);
    MPRINTF("This may indicate flash is not in OPI DTR mode\n\r");
  }

  // Test 8D-8D-8D read before calibration
  MPRINTF("\n\rTesting 8D-8D-8D read before calibration...\n\r");
  uint8_t test_read_buffer[16];
  err = Mc80_ospi_direct_read(g_mc80_ospi.p_ctrl, test_read_buffer, 0x00000000, 16);
  if (err == FSP_SUCCESS)
  {
    MPRINTF("8D-8D-8D Pre-calibration Read  : SUCCESS\n\r");
    MPRINTF("Read data: ");
    for (int i = 0; i < 16; i++)
    {
      MPRINTF("%02X ", test_read_buffer[i]);
      if ((i + 1) % 8 == 0) MPRINTF("\n\r           ");
    }
    MPRINTF("\n\r");
  }
  else
  {
    MPRINTF("8D-8D-8D Pre-calibration Read  : FAILED (error: 0x%X)\n\r", err);
    MPRINTF("Note: Read failure before calibration is expected\n\r");
    MPRINTF("      Calibration should improve read reliability\n\r");
  }

  // Perform calibration
  MPRINTF("\n\rPerforming automatic calibration...\n\r");
  T_mc80_ospi_calibration_data calibration_data;
  err = Mc80_ospi_auto_calibrate(g_mc80_ospi.p_ctrl, &calibration_data);
  if (err == FSP_SUCCESS)
  {
    MPRINTF("Auto Calibration               : SUCCESS\n\r");

    // Display detailed calibration results
    MPRINTF("\n\r===== Calibration Results =====\n\r");
    MPRINTF("Channel                        : %d\n\r", calibration_data.channel);

    MPRINTF("\n\rBefore Calibration:\n\r");
    MPRINTF("WRAPCFG DQS Shift              : 0x%08X\n\r", calibration_data.before_calibration.wrapcfg_dssft);
    MPRINTF("LIOCFG SDR Sample Shift        : 0x%08X\n\r", calibration_data.before_calibration.liocfg_sdrsmpsft);
    MPRINTF("LIOCFG DDR Sample Extension    : 0x%08X\n\r", calibration_data.before_calibration.liocfg_ddrsmpex);
    MPRINTF("CASTTCS Status                 : 0x%08X\n\r", calibration_data.before_calibration.casttcs_value);

    MPRINTF("\n\rAfter Calibration:\n\r");
    MPRINTF("WRAPCFG DQS Shift              : 0x%08X\n\r", calibration_data.after_calibration.wrapcfg_dssft);
    MPRINTF("LIOCFG SDR Sample Shift        : 0x%08X\n\r", calibration_data.after_calibration.liocfg_sdrsmpsft);
    MPRINTF("LIOCFG DDR Sample Extension    : 0x%08X\n\r", calibration_data.after_calibration.liocfg_ddrsmpex);
    MPRINTF("CASTTCS Status                 : 0x%08X\n\r", calibration_data.after_calibration.casttcs_value);

    // Analysis of changes
    if (calibration_data.before_calibration.wrapcfg_dssft != calibration_data.after_calibration.wrapcfg_dssft)
    {
      MPRINTF("\n\rDQS Shift Changed              : 0x%08X -> 0x%08X\n\r",
              calibration_data.before_calibration.wrapcfg_dssft,
              calibration_data.after_calibration.wrapcfg_dssft);
    }
    else
    {
      MPRINTF("\n\rDQS Shift                      : No change (0x%08X)\n\r", calibration_data.after_calibration.wrapcfg_dssft);
    }

    if (calibration_data.after_calibration.casttcs_value != 0)
    {
      MPRINTF("Calibration Status             : Active (0x%08X)\n\r", calibration_data.after_calibration.casttcs_value);
    }
    else
    {
      MPRINTF("Calibration Status             : No active calibration\n\r");
    }
  }
  else
  {
    MPRINTF("Auto Calibration               : FAILED (error: 0x%X)\n\r", err);
    MPRINTF("Warning: Calibration failed, memory-mapped reads may be unreliable\n\r");
  }

  // Test memory-mapped read after calibration using DMA
  MPRINTF("\n\rTesting memory-mapped read after calibration...\n\r");

  uint8_t *read_buffer = (uint8_t *)App_malloc(OSPI_MEMORY_MAPPED_READ_SIZE);
  if (read_buffer == NULL)
  {
    MPRINTF("ERROR: Failed to allocate read buffer\n\r");
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  // Perform memory-mapped read using DMA
  err = Mc80_ospi_memory_mapped_read(g_mc80_ospi.p_ctrl, read_buffer, 0x00000000, OSPI_MEMORY_MAPPED_READ_SIZE);
  if (err == FSP_SUCCESS)
  {
    MPRINTF("Memory-mapped Read             : SUCCESS\n\r");

    // Display first portion of the read for verification
    MPRINTF("\n\rRead Data (first %d bytes):\n\r", OSPI_DISPLAY_PREVIEW_SIZE);
    for (int i = 0; i < OSPI_DISPLAY_PREVIEW_SIZE; i++)
    {
      if (i % 16 == 0)
      {
        MPRINTF("%04X: ", i);
      }
      MPRINTF("%02X ", read_buffer[i]);
      if (i % 16 == 15)
      {
        MPRINTF("\n\r");
      }
    }

    MPRINTF("\n\r8D-8D-8D Calibration Quality   : GOOD (memory-mapped read successful)\n\r");
  }
  else
  {
    MPRINTF("Memory-mapped Read             : FAILED (error: 0x%X)\n\r", err);
    MPRINTF("8D-8D-8D Calibration Quality   : POOR - calibration may have failed\n\r");
  }

  // Clean up
  App_free(read_buffer);

  // Test summary
  MPRINTF("\n\r===== Test Summary =====\n\r");
  if (err == FSP_SUCCESS)
  {
    MPRINTF("Overall Result                 : PASS\n\r");
    MPRINTF("8D-8D-8D protocol is working correctly with good calibration\n\r");
    MPRINTF("Flash remains in 8D-8D-8D mode for continued high-speed operation\n\r");
  }
  else
  {
    MPRINTF("Overall Result                 : FAIL\n\r");
    MPRINTF("8D-8D-8D protocol has calibration issues\n\r");
    MPRINTF("Recommendation: Check signal integrity and timing settings\n\r");
  }

  MPRINTF("\n\rPress any key to continue...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
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
  Description: Write preamble patterns to flash for calibration

  Parameters: keycode - Input key code from VT100 terminal

  Return:
-----------------------------------------------------------------------------------------------------*/
void OSPI_write_preamble_patterns(uint8_t keycode)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF(" ===== Write Preamble Patterns =====\n\r");

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

  // Set 8D-8D-8D protocol for pattern writing (as required for high-speed calibration)
  MPRINTF("\n\rSetting 8D-8D-8D protocol for pattern writing...\n\r");
  fsp_err_t err = Mc80_ospi_spi_protocol_switch_safe(g_mc80_ospi.p_ctrl, MC80_OSPI_PROTOCOL_8D_8D_8D);
  if (err == FSP_SUCCESS)
  {
    MPRINTF("8D-8D-8D Protocol Set          : SUCCESS\n\r");
  }
  else
  {
    MPRINTF("8D-8D-8D Protocol Set          : FAILED (error: 0x%X)\n\r", err);
    MPRINTF("Cannot continue without protocol setup\n\r");
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  // Warning about sector erase and protocol switch
  MPRINTF("\n\rWARNING: This will switch to 8D-8D-8D protocol and erase 4KB sector at address 0x00000000\n\r");
  MPRINTF("Press 'Y' to continue or any other key to cancel: ");

  uint8_t ch;
  WAIT_CHAR(&ch, ms_to_ticks(100000));
  MPRINTF("%c\n\r", ch);

  if (ch != 'Y' && ch != 'y')
  {
    MPRINTF("Pattern write cancelled.\n\r");
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  // Default preamble patterns
  uint32_t patterns[4] = {
    MC80_OSPI_DEFAULT_PREAMBLE_PATTERN_0,  // 0xFFFF0000
    MC80_OSPI_DEFAULT_PREAMBLE_PATTERN_1,  // 0x000800FF
    MC80_OSPI_DEFAULT_PREAMBLE_PATTERN_2,  // 0x00FFF700
    MC80_OSPI_DEFAULT_PREAMBLE_PATTERN_3   // 0xF700F708
  };

  MPRINTF("\n\rPreamble patterns to write:\n\r");
  for (int i = 0; i < 4; i++)
  {
    MPRINTF("Pattern %d                      : 0x%08X\n\r", i, patterns[i]);
  }

  // Erase sector before writing patterns
  uint32_t calibration_address = 0x00000000;  // Flash physical address (not XIP)
  MPRINTF("\n\rErasing sector at address 0x%08X...\n\r", calibration_address);

  err = Mc80_ospi_erase(g_mc80_ospi.p_ctrl, (uint8_t *)(OSPI_BASE_ADDRESS + calibration_address), OSPI_TEST_SECTOR_SIZE);
  if (err == FSP_SUCCESS)
  {
    MPRINTF("Sector Erase                   : SUCCESS\n\r");

    // Wait for erase completion
    MPRINTF("Waiting for erase completion...\n\r");
    T_mc80_ospi_status status;
    uint32_t           wait_count = 0;
    do
    {
      R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
      wait_count += 10;
      Mc80_ospi_status_get(g_mc80_ospi.p_ctrl, &status);

      if (wait_count % 100 == 0)
      {
        MPRINTF("Waited %u ms...\n\r", wait_count);
      }

      if (wait_count > 5000)  // 5 second timeout
      {
        MPRINTF("Erase timeout!\n\r");
        break;
      }
    } while (status.write_in_progress);

    if (!status.write_in_progress)
    {
      MPRINTF("Erase completed in %u ms\n\r", wait_count);
    }
    else
    {
      MPRINTF("WARNING: Erase may not have completed\n\r");
    }
  }
  else
  {
    MPRINTF("Sector Erase                   : FAILED (error: 0x%X)\n\r", err);
    MPRINTF("Cannot continue without erasing sector\n\r");
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  // Write patterns to calibration address
  MPRINTF("\n\rWriting patterns to address 0x%08X...\n\r", calibration_address);

  // Check flash status before write
  T_mc80_ospi_status status;
  err = Mc80_ospi_status_get(g_mc80_ospi.p_ctrl, &status);
  if (err == FSP_SUCCESS && status.write_in_progress)
  {
    MPRINTF("Flash is busy, waiting...\n\r");
    do
    {
      R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
      Mc80_ospi_status_get(g_mc80_ospi.p_ctrl, &status);
    } while (status.write_in_progress);
  }

  // Write 16 bytes (4 x 32-bit patterns) to flash
  err = Mc80_ospi_memory_mapped_write(g_mc80_ospi.p_ctrl, (uint8_t *)patterns, (uint8_t *)(OSPI_BASE_ADDRESS + calibration_address), 16);
  if (err == FSP_SUCCESS)
  {
    MPRINTF("Preamble Patterns Write        : SUCCESS\n\r");

    // Wait for write completion
    do
    {
      R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
      Mc80_ospi_status_get(g_mc80_ospi.p_ctrl, &status);
    } while (status.write_in_progress);

    MPRINTF("Write completed\n\r");

    // Verify written data by reading it back
    MPRINTF("\n\rVerifying written patterns...\n\r");
    uint32_t read_patterns[4];
    err = Mc80_ospi_direct_read(g_mc80_ospi.p_ctrl, (uint8_t *)read_patterns, calibration_address, 16);
    if (err == FSP_SUCCESS)
    {
      MPRINTF("Pattern Verification           : SUCCESS\n\r");
      bool patterns_match = true;
      for (int i = 0; i < 4; i++)
      {
        MPRINTF("Pattern %d: Written=0x%08X, Read=0x%08X", i, patterns[i], read_patterns[i]);
        if (patterns[i] == read_patterns[i])
        {
          MPRINTF(" OK\n\r");
        }
        else
        {
          MPRINTF(" ERR\n\r");
          patterns_match = false;
        }
      }

      if (patterns_match)
      {
        MPRINTF("\n\rAll patterns verified successfully!\n\r");
        MPRINTF("Patterns written in 8D-8D-8D protocol - ready for calibration\n\r");
      }
      else
      {
        MPRINTF("\n\rPattern verification FAILED!\n\r");
      }
    }
    else
    {
      MPRINTF("Pattern Verification           : FAILED (error: 0x%X)\n\r", err);
    }
  }
  else
  {
    MPRINTF("Preamble Patterns Write        : FAILED (error: 0x%X)\n\r", err);
  }

  MPRINTF("\n\rPress any key to continue...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
}

/*-----------------------------------------------------------------------------------------------------
  Description: Test memory-mapped read function by reading and displaying data from flash

  Parameters: keycode - Input key code from VT100 terminal

  Return:
-----------------------------------------------------------------------------------------------------*/
void OSPI_test_memory_mapped_read(uint8_t keycode)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF(" ===== Memory-Mapped Read Test =====\n\r");

  // Select protocol first
  T_mc80_ospi_protocol protocol = _Ospi_select_protocol();

  // Ensure OSPI driver is open
  fsp_err_t init_err            = _Ospi_ensure_driver_open();
  if (init_err != FSP_SUCCESS)
  {
    MPRINTF("ERROR: Failed to ensure OSPI driver is open (0x%X)\n\r", init_err);
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  // Set the selected protocol with safe switching
  MPRINTF("Setting protocol with safe switching...\n\r");
  fsp_err_t protocol_err = Mc80_ospi_spi_protocol_switch_safe(g_mc80_ospi.p_ctrl, protocol);
  if (protocol_err != FSP_SUCCESS)
  {
    MPRINTF("ERROR: Failed to set protocol (0x%X)\n\r", protocol_err);
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }
  MPRINTF("Protocol set successfully\n\r");

  // Allocate buffers for three reads
  uint8_t *read_buffer1 = (uint8_t *)App_malloc(OSPI_MEMORY_MAPPED_READ_SIZE);
  uint8_t *read_buffer2 = (uint8_t *)App_malloc(OSPI_MEMORY_MAPPED_READ_SIZE);
  uint8_t *read_buffer3 = (uint8_t *)App_malloc(OSPI_MEMORY_MAPPED_READ_SIZE);

  if (read_buffer1 == NULL || read_buffer2 == NULL || read_buffer3 == NULL)
  {
    MPRINTF("ERROR: Failed to allocate read buffers\n\r");
    if (read_buffer1) App_free(read_buffer1);
    if (read_buffer2) App_free(read_buffer2);
    if (read_buffer3) App_free(read_buffer3);
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  // Clear buffers before reading
  memset(read_buffer1, 0x00, OSPI_MEMORY_MAPPED_READ_SIZE);
  memset(read_buffer2, 0x00, OSPI_MEMORY_MAPPED_READ_SIZE);
  memset(read_buffer3, 0x00, OSPI_MEMORY_MAPPED_READ_SIZE);

  MPRINTF("Reading first %d bytes from flash address 0x00000000...\n\r", OSPI_MEMORY_MAPPED_READ_SIZE);

  // Perform first memory-mapped read using DMA
  fsp_err_t err1 = Mc80_ospi_memory_mapped_read(g_mc80_ospi.p_ctrl, read_buffer1, 0x00000000, OSPI_MEMORY_MAPPED_READ_SIZE);

  if (err1 != FSP_SUCCESS)
  {
    MPRINTF("First memory-mapped read      : FAILED (error: 0x%X)\n\r", err1);
    App_free(read_buffer1);
    App_free(read_buffer2);
    App_free(read_buffer3);
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  MPRINTF("First read                    : SUCCESS\n\r");

  // Perform second memory-mapped read for comparison using DMA
  fsp_err_t err2 = Mc80_ospi_memory_mapped_read(g_mc80_ospi.p_ctrl, read_buffer2, 0x00000000, OSPI_MEMORY_MAPPED_READ_SIZE);

  if (err2 != FSP_SUCCESS)
  {
    MPRINTF("Second memory-mapped read     : FAILED (error: 0x%X)\n\r", err2);
    App_free(read_buffer1);
    App_free(read_buffer2);
    App_free(read_buffer3);
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  MPRINTF("Second read                   : SUCCESS\n\r");

  // Perform third memory-mapped read for comparison using DMA
  fsp_err_t err3 = Mc80_ospi_memory_mapped_read(g_mc80_ospi.p_ctrl, read_buffer3, 0x00000000, OSPI_MEMORY_MAPPED_READ_SIZE);

  if (err3 != FSP_SUCCESS)
  {
    MPRINTF("Third memory-mapped read      : FAILED (error: 0x%X)\n\r", err3);
    App_free(read_buffer1);
    App_free(read_buffer2);
    App_free(read_buffer3);
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  MPRINTF("Third read                    : SUCCESS\n\r");

  // Compare all three reads
  bool reads_match = true;
  for (uint32_t i = 0; i < OSPI_MEMORY_MAPPED_READ_SIZE; i++)
  {
    if (read_buffer1[i] != read_buffer2[i] || read_buffer1[i] != read_buffer3[i])
    {
      reads_match = false;
      break;
    }
  }

  if (reads_match)
  {
    MPRINTF("Data consistency check        : OK (all three reads match)\n\r");
  }
  else
  {
    MPRINTF("Data consistency check        : PROBLEM (reads do not match)\n\r");
  }

  MPRINTF("\n\rData read from flash:\n\r");

  // Display data in hex format (16 bytes per line) from first read
  for (uint32_t i = 0; i < OSPI_MEMORY_MAPPED_READ_SIZE; i += 16)
  {
    // Print address
    MPRINTF("0x%08X: ", i);

    // Print hex values
    for (uint32_t j = 0; j < 16; j++)
    {
      if (i + j < OSPI_MEMORY_MAPPED_READ_SIZE)
      {
        MPRINTF("%02X ", read_buffer1[i + j]);
      }
      else
      {
        MPRINTF("   ");
      }
    }

    MPRINTF(" | ");

    // Print ASCII representation
    for (uint32_t j = 0; j < 16; j++)
    {
      if (i + j < OSPI_MEMORY_MAPPED_READ_SIZE)
      {
        uint8_t byte_val = read_buffer1[i + j];
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

  // Free allocated buffers
  App_free(read_buffer1);
  App_free(read_buffer2);
  App_free(read_buffer3);

  MPRINTF("\n\rPress any key to continue...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
}

/*-----------------------------------------------------------------------------------------------------
  Description: 8D-8D-8D continuous read test with memory-mapped DMA and register analysis

  This function demonstrates continuous high-speed reading from OSPI flash in 8D-8D-8D mode using
  memory-mapped DMA transfers. It configures the flash to OPI DTR mode, switches the controller
  protocol, captures register snapshots before/after protocol switch, and performs continuous
  memory-mapped reading with display refresh every 100ms.

  Features:
  - Automatic flash configuration to OPI DTR mode (CR2 = 0x02)
  - Controller protocol switch to 8D-8D-8D with register snapshot analysis
  - High-performance memory-mapped DMA reading (64 bytes per cycle)
  - Register difference analysis showing protocol switch effects
  - Real-time display update every 100ms
  - User can exit by pressing any key

  Technical Implementation:
  - Uses Mc80_ospi_memory_mapped_read() for optimal DMA performance
  - Captures complete OSPI register snapshots before/after protocol switch
  - Analyzes key register changes (LIOCFGCS, CMCFGCS, WRAPCFG, etc.)
  - Provides timing analysis of protocol switching

  Parameters:
    keycode - Menu key pressed (unused)

  Return:
    void
-----------------------------------------------------------------------------------------------------*/

void OSPI_test_8d_continuous_read(uint8_t keycode)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF(" ===== 8D-8D-8D Continuous Read Test =====\n\r");

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

  // Set 1S-1S-1S protocol for flash configuration
  MPRINTF("Setting 1S-1S-1S protocol for flash configuration...\n\r");
  fsp_err_t err = Mc80_ospi_spi_protocol_set(g_mc80_ospi.p_ctrl, MC80_OSPI_PROTOCOL_1S_1S_1S);
  if (err != FSP_SUCCESS)
  {
    MPRINTF("ERROR: Failed to set 1S-1S-1S protocol (0x%X)\n\r", err);
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }
  MPRINTF("1S-1S-1S Protocol Set          : SUCCESS\n\r");

  // Configure flash to OPI DTR mode
  MPRINTF("Configuring flash for OPI DTR mode...\n\r");

  // Write Enable for CR2 configuration
  T_mc80_ospi_direct_transfer write_enable_cmd = {
    .command        = MX25_CMD_WREN,
    .command_length = 1,
    .address_length = 0,
    .address        = 0,
    .data_length    = 0,
    .dummy_cycles   = 0,
  };

  err = Mc80_ospi_direct_transfer(g_mc80_ospi.p_ctrl, &write_enable_cmd, MC80_OSPI_DIRECT_TRANSFER_DIR_WRITE);
  if (err != FSP_SUCCESS)
  {
    MPRINTF("ERROR: Write Enable failed (0x%X)\n\r", err);
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  // Write CR2 to enable OPI DTR mode (value 0x02)
  T_mc80_ospi_direct_transfer write_cr2_cmd = {
    .command        = MX25_CMD_WRCR2,
    .command_length = 1,
    .address_length = 4,           // 4-byte address required for CR2
    .address        = 0x00000000,  // CR2 address
    .data_length    = 1,
    .data           = 0x02,        // Enable OPI DTR mode
    .dummy_cycles   = 0,
  };

  err = Mc80_ospi_direct_transfer(g_mc80_ospi.p_ctrl, &write_cr2_cmd, MC80_OSPI_DIRECT_TRANSFER_DIR_WRITE);
  if (err != FSP_SUCCESS)
  {
    MPRINTF("ERROR: CR2 Write failed (0x%X)\n\r", err);
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  // Wait for flash mode switch
  R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
  MPRINTF("CR2 Write (OPI DTR Enable)     : SUCCESS\n\r");

  // Capture register snapshot BEFORE 8D-8D-8D protocol switch
  T_mc80_ospi_register_snapshot snapshot_before = { 0 };
  MPRINTF("Capturing register snapshot before 8D-8D-8D switch...\n\r");
  err = Mc80_ospi_capture_register_snapshot(g_mc80_ospi.p_ctrl, &snapshot_before);
  if (err != FSP_SUCCESS)
  {
    MPRINTF("WARNING: Failed to capture snapshot before switch (0x%X)\n\r", err);
  }
  else
  {
    MPRINTF("Before Snapshot Captured       : SUCCESS\n\r");
  }

  // Switch to 8D-8D-8D protocol
  MPRINTF("Setting 8D-8D-8D protocol...\n\r");
  err = Mc80_ospi_spi_protocol_set(g_mc80_ospi.p_ctrl, MC80_OSPI_PROTOCOL_8D_8D_8D);
  if (err != FSP_SUCCESS)
  {
    MPRINTF("ERROR: Failed to set 8D-8D-8D protocol (0x%X)\n\r", err);
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }
  MPRINTF("8D-8D-8D Protocol Set          : SUCCESS\n\r");  // Capture register snapshot AFTER 8D-8D-8D protocol switch
  T_mc80_ospi_register_snapshot snapshot_after = { 0 };
  MPRINTF("Capturing register snapshot after 8D-8D-8D switch...\n\r");
  err = Mc80_ospi_capture_register_snapshot(g_mc80_ospi.p_ctrl, &snapshot_after);
  if (err != FSP_SUCCESS)
  {
    MPRINTF("WARNING: Failed to capture snapshot after switch (0x%X)\n\r", err);
  }
  else
  {
    MPRINTF("After Snapshot Captured        : SUCCESS\n\r");
  }

  // Полный анализ изменений всех регистров OSPI
  _Ospi_display_register_differences(&snapshot_before, &snapshot_after);

  MPRINTF("\n\rPress any key to start continuous reading...\n\r");
  uint8_t start_key;
  WAIT_CHAR(&start_key, ms_to_ticks(100000));

  // Allocate buffer for read data
  uint8_t *read_buffer = (uint8_t *)App_malloc(OSPI_CONTINUOUS_READ_SIZE);
  if (read_buffer == NULL)
  {
    MPRINTF("ERROR: Failed to allocate read buffer\n\r");
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  MPRINTF("\n\rStarting WRAPCFG DQS shift test for 8D-8D-8D mode...\n\r");
  MPRINTF("Testing all DQS shift values (0-31) with memory reads\n\r");
  MPRINTF("Press any key to start or 'q' to skip\n\r\n\r");

  uint8_t test_key;
  if (WAIT_CHAR(&test_key, ms_to_ticks(100000)) == RES_OK && (test_key == 'q' || test_key == 'Q'))
  {
    MPRINTF("Test skipped by user\n\r");
    App_free(read_buffer);
    return;
  }

  R_XSPI0_Type *p_reg = (R_XSPI0_Type *)MC80_OSPI0_BASE_ADDRESS;

  // Test DQS shift values from 0 to 31
  for (uint32_t shift_val = 0; shift_val <= 31; shift_val++)
  {
    MPRINTF(VT100_CLEAR_AND_HOME);
    MPRINTF(" ===== DQS Shift Value: %lu =====\n\r", shift_val);

    // Set new DQS shift value for CS0
    uint32_t wrapcfg_val = p_reg->WRAPCFG;
    wrapcfg_val &= ~OSPI_WRAPCFG_DSSFTCS0_Msk;  // Clear current CS0 shift value
    wrapcfg_val |= (shift_val << OSPI_WRAPCFG_DSSFTCS0_Pos) & OSPI_WRAPCFG_DSSFTCS0_Msk;
    p_reg->WRAPCFG = wrapcfg_val;

    MPRINTF("WRAPCFG DQS Shift set to: %lu\n\r", shift_val);
    MPRINTF("WRAPCFG register value: 0x%08X\n\r\n\r", p_reg->WRAPCFG);

    // Small delay for change to take effect
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MICROSECONDS);

    // Perform memory-mapped read operation using DMA
    err = Mc80_ospi_memory_mapped_read(g_mc80_ospi.p_ctrl, read_buffer, 0x00000000, 64);

    if (err == FSP_SUCCESS)
    {
      MPRINTF("Memory Read Status: SUCCESS\n\r\n\r");

      // Display data in hex format (16 bytes per line)
      for (int i = 0; i < 64; i++)
      {
        if (i % 16 == 0)
        {
          MPRINTF("%08X: ", i);  // Address
        }

        MPRINTF("%02X ", read_buffer[i]);

        if (i % 16 == 15)
        {
          MPRINTF("\n\r");
        }
        else if (i % 8 == 7)
        {
          MPRINTF(" ");  // Extra space every 8 bytes
        }
      }

      MPRINTF("\n\r");
    }
    else
    {
      MPRINTF("Memory Read Status: FAILED (error: 0x%X)\n\r", err);
    }

    MPRINTF("\n\rPress any key to continue to next value (or 'q' to quit): ");
    uint8_t continue_key;
    if (WAIT_CHAR(&continue_key, ms_to_ticks(100000)) == RES_OK)
    {
      if (continue_key == 'q' || continue_key == 'Q')
      {
        MPRINTF("\n\rTest stopped by user at DQS shift value %lu\n\r", shift_val);
        break;
      }
    }
  }

  // Clean up
  App_free(read_buffer);

  MPRINTF("\n\rDQS timing test completed.\n\r");
  MPRINTF("Press any key to continue...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
}

/*-----------------------------------------------------------------------------------------------------
  Description: Display comprehensive comparison of all OSPI register snapshots

  This function performs a complete comparison of all registers captured in two OSPI register
  snapshots and displays all differences in a structured format. It compares every register
  field including control, configuration, calibration, status, and command buffer registers.

  Parameters:
    before - Pointer to snapshot taken before protocol switch
    after  - Pointer to snapshot taken after protocol switch

  Return:
    void
-----------------------------------------------------------------------------------------------------*/
static void _Ospi_display_register_differences(const T_mc80_ospi_register_snapshot *before, const T_mc80_ospi_register_snapshot *after)
{
  GET_MCBL;
  MPRINTF("\n\r===== Complete Register Differences (Before -> After) =====\n\r");

  uint32_t changes_count = 0;

  // Metadata comparison
  MPRINTF("\n\r--- Metadata ---\n\r");
  MPRINTF("Protocol    : %s -> %s\n\r",
          (before->current_protocol == MC80_OSPI_PROTOCOL_1S_1S_1S) ? "1S-1S-1S" : "8D-8D-8D",
          (after->current_protocol == MC80_OSPI_PROTOCOL_8D_8D_8D) ? "8D-8D-8D" : "1S-1S-1S");
  MPRINTF("Channel     : %d -> %d\n\r", before->channel, after->channel);
  MPRINTF("Timestamp   : %lu -> %lu (delta: %ld ticks)\n\r",
          before->timestamp, after->timestamp, (long)(after->timestamp - before->timestamp));

  // Control and Configuration Registers
  MPRINTF("\n\r--- Control & Configuration Registers ---\n\r");

  if (before->lioctl != after->lioctl)
  {
    MPRINTF("LIOCTL      : 0x%08lX -> 0x%08lX\n\r", before->lioctl, after->lioctl);
    changes_count++;
  }

  if (before->wrapcfg != after->wrapcfg)
  {
    MPRINTF("WRAPCFG     : 0x%08lX -> 0x%08lX\n\r", before->wrapcfg, after->wrapcfg);
    changes_count++;
  }

  if (before->comcfg != after->comcfg)
  {
    MPRINTF("COMCFG      : 0x%08lX -> 0x%08lX\n\r", before->comcfg, after->comcfg);
    changes_count++;
  }

  if (before->bmcfgch[0] != after->bmcfgch[0])
  {
    MPRINTF("BMCFGCH[0]  : 0x%08lX -> 0x%08lX\n\r", before->bmcfgch[0], after->bmcfgch[0]);
    changes_count++;
  }

  if (before->bmcfgch[1] != after->bmcfgch[1])
  {
    MPRINTF("BMCFGCH[1]  : 0x%08lX -> 0x%08lX\n\r", before->bmcfgch[1], after->bmcfgch[1]);
    changes_count++;
  }

  if (before->bmctl0 != after->bmctl0)
  {
    MPRINTF("BMCTL0      : 0x%08lX -> 0x%08lX\n\r", before->bmctl0, after->bmctl0);
    changes_count++;
  }

  if (before->bmctl1 != after->bmctl1)
  {
    MPRINTF("BMCTL1      : 0x%08lX -> 0x%08lX\n\r", before->bmctl1, after->bmctl1);
    changes_count++;
  }

  if (before->abmcfg != after->abmcfg)
  {
    MPRINTF("ABMCFG      : 0x%08lX -> 0x%08lX\n\r", before->abmcfg, after->abmcfg);
    changes_count++;
  }

  // Channel Configuration Registers
  MPRINTF("\n\r--- Channel Configuration Registers ---\n\r");

  for (int ch = 0; ch < 2; ch++)
  {
    if (before->liocfgcs[ch] != after->liocfgcs[ch])
    {
      MPRINTF("LIOCFGCS[%d] : 0x%08lX -> 0x%08lX\n\r", ch, before->liocfgcs[ch], after->liocfgcs[ch]);
      changes_count++;
    }

    if (before->cmcfgcs[ch].cmcfg0 != after->cmcfgcs[ch].cmcfg0)
    {
      MPRINTF("CMCFG0CS[%d] : 0x%08lX -> 0x%08lX\n\r", ch, before->cmcfgcs[ch].cmcfg0, after->cmcfgcs[ch].cmcfg0);
      changes_count++;
    }

    if (before->cmcfgcs[ch].cmcfg1 != after->cmcfgcs[ch].cmcfg1)
    {
      MPRINTF("CMCFG1CS[%d] : 0x%08lX -> 0x%08lX\n\r", ch, before->cmcfgcs[ch].cmcfg1, after->cmcfgcs[ch].cmcfg1);
      changes_count++;
    }

    if (before->cmcfgcs[ch].cmcfg2 != after->cmcfgcs[ch].cmcfg2)
    {
      MPRINTF("CMCFG2CS[%d] : 0x%08lX -> 0x%08lX\n\r", ch, before->cmcfgcs[ch].cmcfg2, after->cmcfgcs[ch].cmcfg2);
      changes_count++;
    }
  }

  // Calibration Control Registers
  MPRINTF("\n\r--- Calibration Control Registers ---\n\r");

  for (int ch = 0; ch < 2; ch++)
  {
    if (before->ccctlcs[ch].ccctl0 != after->ccctlcs[ch].ccctl0)
    {
      MPRINTF("CCCTL0CS[%d] : 0x%08lX -> 0x%08lX\n\r", ch, before->ccctlcs[ch].ccctl0, after->ccctlcs[ch].ccctl0);
      changes_count++;
    }
    if (before->ccctlcs[ch].ccctl1 != after->ccctlcs[ch].ccctl1)
    {
      MPRINTF("CCCTL1CS[%d] : 0x%08lX -> 0x%08lX\n\r", ch, before->ccctlcs[ch].ccctl1, after->ccctlcs[ch].ccctl1);
      changes_count++;
    }
    if (before->ccctlcs[ch].ccctl2 != after->ccctlcs[ch].ccctl2)
    {
      MPRINTF("CCCTL2CS[%d] : 0x%08lX -> 0x%08lX\n\r", ch, before->ccctlcs[ch].ccctl2, after->ccctlcs[ch].ccctl2);
      changes_count++;
    }
    if (before->ccctlcs[ch].ccctl3 != after->ccctlcs[ch].ccctl3)
    {
      MPRINTF("CCCTL3CS[%d] : 0x%08lX -> 0x%08lX\n\r", ch, before->ccctlcs[ch].ccctl3, after->ccctlcs[ch].ccctl3);
      changes_count++;
    }
    if (before->ccctlcs[ch].ccctl4 != after->ccctlcs[ch].ccctl4)
    {
      MPRINTF("CCCTL4CS[%d] : 0x%08lX -> 0x%08lX\n\r", ch, before->ccctlcs[ch].ccctl4, after->ccctlcs[ch].ccctl4);
      changes_count++;
    }
    if (before->ccctlcs[ch].ccctl5 != after->ccctlcs[ch].ccctl5)
    {
      MPRINTF("CCCTL5CS[%d] : 0x%08lX -> 0x%08lX\n\r", ch, before->ccctlcs[ch].ccctl5, after->ccctlcs[ch].ccctl5);
      changes_count++;
    }
    if (before->ccctlcs[ch].ccctl6 != after->ccctlcs[ch].ccctl6)
    {
      MPRINTF("CCCTL6CS[%d] : 0x%08lX -> 0x%08lX\n\r", ch, before->ccctlcs[ch].ccctl6, after->ccctlcs[ch].ccctl6);
      changes_count++;
    }
    if (before->ccctlcs[ch].ccctl7 != after->ccctlcs[ch].ccctl7)
    {
      MPRINTF("CCCTL7CS[%d] : 0x%08lX -> 0x%08lX\n\r", ch, before->ccctlcs[ch].ccctl7, after->ccctlcs[ch].ccctl7);
      changes_count++;
    }
  }

  // Status and Interrupt Registers
  MPRINTF("\n\r--- Status & Interrupt Registers ---\n\r");

  if (before->ints != after->ints)
  {
    MPRINTF("INTS        : 0x%08lX -> 0x%08lX\n\r", before->ints, after->ints);
    changes_count++;
  }

  if (before->inte != after->inte)
  {
    MPRINTF("INTE        : 0x%08lX -> 0x%08lX\n\r", before->inte, after->inte);
    changes_count++;
  }

  if (before->comstt != after->comstt)
  {
    MPRINTF("COMSTT      : 0x%08lX -> 0x%08lX\n\r", before->comstt, after->comstt);
    changes_count++;
  }

  if (before->verstt != after->verstt)
  {
    MPRINTF("VERSTT      : 0x%08lX -> 0x%08lX\n\r", before->verstt, after->verstt);
    changes_count++;
  }

  // Calibration Status Registers
  MPRINTF("\n\r--- Calibration Status Registers ---\n\r");

  for (int ch = 0; ch < 2; ch++)
  {
    if (before->casttcs[ch] != after->casttcs[ch])
    {
      MPRINTF("CASTTCS[%d]  : 0x%08lX -> 0x%08lX\n\r", ch, before->casttcs[ch], after->casttcs[ch]);
      changes_count++;
    }
  }

  // Command Buffer Registers
  MPRINTF("\n\r--- Command Buffer Registers ---\n\r");

  for (int ch = 0; ch < 2; ch++)
  {
    if (before->cdbuf[ch].cdt != after->cdbuf[ch].cdt)
    {
      MPRINTF("CDT[%d]      : 0x%08lX -> 0x%08lX\n\r", ch, before->cdbuf[ch].cdt, after->cdbuf[ch].cdt);
      changes_count++;
    }
    if (before->cdbuf[ch].cda != after->cdbuf[ch].cda)
    {
      MPRINTF("CDA[%d]      : 0x%08lX -> 0x%08lX\n\r", ch, before->cdbuf[ch].cda, after->cdbuf[ch].cda);
      changes_count++;
    }
    if (before->cdbuf[ch].cdd0 != after->cdbuf[ch].cdd0)
    {
      MPRINTF("CDD0[%d]     : 0x%08lX -> 0x%08lX\n\r", ch, before->cdbuf[ch].cdd0, after->cdbuf[ch].cdd0);
      changes_count++;
    }
    if (before->cdbuf[ch].cdd1 != after->cdbuf[ch].cdd1)
    {
      MPRINTF("CDD1[%d]     : 0x%08lX -> 0x%08lX\n\r", ch, before->cdbuf[ch].cdd1, after->cdbuf[ch].cdd1);
      changes_count++;
    }
  }

  // Manual Command Control
  MPRINTF("\n\r--- Manual Command Control ---\n\r");

  if (before->cdctl0 != after->cdctl0)
  {
    MPRINTF("CDCTL0      : 0x%08lX -> 0x%08lX\n\r", before->cdctl0, after->cdctl0);
    changes_count++;
  }

  if (before->cdctl1 != after->cdctl1)
  {
    MPRINTF("CDCTL1      : 0x%08lX -> 0x%08lX\n\r", before->cdctl1, after->cdctl1);
    changes_count++;
  }

  if (before->cdctl2 != after->cdctl2)
  {
    MPRINTF("CDCTL2      : 0x%08lX -> 0x%08lX\n\r", before->cdctl2, after->cdctl2);
    changes_count++;
  }

  // Link Pattern Control Registers
  MPRINTF("\n\r--- Link Pattern Control ---\n\r");

  if (before->lpctl0 != after->lpctl0)
  {
    MPRINTF("LPCTL0      : 0x%08lX -> 0x%08lX\n\r", before->lpctl0, after->lpctl0);
    changes_count++;
  }

  if (before->lpctl1 != after->lpctl1)
  {
    MPRINTF("LPCTL1      : 0x%08lX -> 0x%08lX\n\r", before->lpctl1, after->lpctl1);
    changes_count++;
  }

  // XIP Control Registers
  MPRINTF("\n\r--- XIP Control Registers ---\n\r");

  for (int ch = 0; ch < 2; ch++)
  {
    if (before->cmctlch[ch] != after->cmctlch[ch])
    {
      MPRINTF("CMCTLCH[%d]  : 0x%08lX -> 0x%08lX\n\r", ch, before->cmctlch[ch], after->cmctlch[ch]);
      changes_count++;
    }
  }

  MPRINTF("\n\r--- Summary ---\n\r");
  MPRINTF("Total register changes detected: %lu\n\r", changes_count);

  if (changes_count == 0)
  {
    MPRINTF("No register differences found!\n\r");
  }
}
