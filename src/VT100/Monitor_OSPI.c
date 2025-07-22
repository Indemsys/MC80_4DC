/*-----------------------------------------------------------------------------------------------------
  Description: OSPI Flash memory testing terminal interface
               Provides direct access to OSPI flash operations for debugging

  Parameters:

  Return:
-----------------------------------------------------------------------------------------------------*/

#include "App.h"

#define OSPI_TEST_PATTERN_SIZE 256
#define OSPI_TEST_SECTOR_SIZE  4096

// Function declarations
void OSPI_test_info(uint8_t keycode);
void OSPI_test_read(uint8_t keycode);
void OSPI_test_write(uint8_t keycode);
void OSPI_test_erase(uint8_t keycode);
void OSPI_test_sector(uint8_t keycode);
void OSPI_test_8d_calibration(uint8_t keycode);
void OSPI_write_preamble_patterns(uint8_t keycode);

// Internal helper functions
static fsp_err_t _Ospi_ensure_driver_open(void);

const T_VT100_Menu_item MENU_OSPI_ITEMS[] = {
  { '1', OSPI_test_info, 0 },
  { '2', OSPI_test_read, 0 },
  { '3', OSPI_test_write, 0 },
  { '4', OSPI_test_erase, 0 },
  { '5', OSPI_test_sector, 0 },
  { '6', OSPI_test_8d_calibration, 0 },
  { '7', OSPI_write_preamble_patterns, 0 },
  { 'R', 0, 0 },
  { 0 }
};

const T_VT100_Menu MENU_OSPI = {
  "OSPI Flash Testing",
  "\033[5C OSPI Flash memory testing menu\r\n"
  "\033[5C <1> - OSPI Flash information & status\r\n"
  "\033[5C <2> - Read data test\r\n"
  "\033[5C <3> - Write data test\r\n"
  "\033[5C <4> - Erase sector test\r\n"
  "\033[5C <5> - Full sector test\r\n"
  "\033[5C <6> - 8D-8D-8D protocol & calibration test\r\n"
  "\033[5C <7> - Write preamble patterns for calibration\r\n"
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
  uint8_t jedec_id[3] = {0};
  err = Mc80_ospi_read_id(g_mc80_ospi.p_ctrl, jedec_id, 3);
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
  MPRINTF("XIP base address               : 0x80000000\n\r");
  MPRINTF("XIP address range              : 0x80000000 - 0x81FFFFFF\n\r");
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

  // Test XIP mode operations
  MPRINTF("\n\rTesting XIP mode operations...\n\r");

  // Try to enter XIP mode
  err = Mc80_ospi_xip_enter(g_mc80_ospi.p_ctrl);
  if (err == FSP_SUCCESS)
  {
    MPRINTF("XIP Enter                      : SUCCESS\n\r");

    // Try to exit XIP mode
    err = Mc80_ospi_xip_exit(g_mc80_ospi.p_ctrl);
    if (err == FSP_SUCCESS)
    {
      MPRINTF("XIP Exit                       : SUCCESS\n\r");
    }
    else
    {
      MPRINTF("XIP Exit                       : FAILED (error: 0x%X)\n\r", err);
    }
  }
  else
  {
    MPRINTF("XIP Enter                      : FAILED (error: 0x%X)\n\r", err);
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
  Description: Test OSPI read operations using direct read interface

  Parameters: keycode - Input key code from VT100 terminal

  Return:
-----------------------------------------------------------------------------------------------------*/
void OSPI_test_read(uint8_t keycode)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF(" ===== OSPI Direct Read Test =====\n\r");

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

  // Allocate buffer for reading 256 bytes
  uint8_t *read_buffer = App_malloc(256);
  if (NULL == read_buffer)
  {
    MPRINTF("ERROR: Failed to allocate memory for read buffer\n\r");
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  MPRINTF("Reading 256 bytes from address 0x00000000 using Mc80_ospi_direct_read...\n\r");

  // Read 256 bytes from offset 0 using direct read
  fsp_err_t err = Mc80_ospi_direct_read(g_mc80_ospi.p_ctrl, read_buffer, 0, 256);
  if (err != FSP_SUCCESS)
  {
    MPRINTF("Direct read failed: 0x%X\n\r", err);
    App_free(read_buffer);
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  MPRINTF("Direct read successful! Data from address 0x00000000 (256 bytes):\n\r");

  // Display the read data in hexadecimal format (16 bytes per line)
  for (int i = 0; i < 256; i++)
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
void OSPI_test_write(uint8_t keycode)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF(" ===== OSPI Write Test =====\n\r");

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

  uint8_t *write_buffer = (uint8_t *)App_malloc(256);
  uint8_t *read_buffer  = (uint8_t *)App_malloc(256);

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
  uint32_t seed = tx_time_get();
  seed = seed * 1103515245 + 12345;  // Simple LCG algorithm
  uint8_t base_value = (uint8_t)(seed >> 16);

  MPRINTF("Creating test pattern with random base: 0x%02X\n\r", base_value);

  for (int i = 0; i < 256; i++)
  {
    write_buffer[i] = base_value + i;
  }

  MPRINTF("Test pattern created (256 bytes: 0x%02X + increment)\n\r", base_value);

  // Exit XIP mode
  fsp_err_t err = Mc80_ospi_xip_exit(g_mc80_ospi.p_ctrl);
  if (err != FSP_SUCCESS)
  {
    MPRINTF("Warning: XIP exit failed (0x%X)\n\r", err);
  }

  // Check flash status
  T_mc80_ospi_status status;
  err = Mc80_ospi_status_get(g_mc80_ospi.p_ctrl, &status);
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
  MPRINTF("Writing 256 bytes to flash offset 0x00000000...\n\r");
  err = Mc80_ospi_write(g_mc80_ospi.p_ctrl, write_buffer, (uint8_t *)(OSPI_BASE_ADDRESS + 0x00000000), 256);

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

    err = Mc80_ospi_direct_read(g_mc80_ospi.p_ctrl, read_buffer, 0, 256);

    if (err == FSP_SUCCESS)
    {
      MPRINTF("Direct read: SUCCESS\n\r");

      // Display first 64 bytes of read data
      MPRINTF("Read data (first 64 bytes):\n\r");
      for (int i = 0; i < 64; i++)
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
      bool match = true;
      int  error_count = 0;
      for (int i = 0; i < 256; i++)
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
        MPRINTF("Data verification: PASS (all 256 bytes match)\n\r");
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

  uint8_t *read_buffer = (uint8_t *)App_malloc(256);
  if (read_buffer == NULL)
  {
    MPRINTF("ERROR: Failed to allocate read buffer\n\r");
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  // Exit XIP mode
  fsp_err_t err = Mc80_ospi_xip_exit(g_mc80_ospi.p_ctrl);
  if (err != FSP_SUCCESS)
  {
    MPRINTF("Warning: XIP exit failed (0x%X)\n\r", err);
  }

  // Check initial flash status
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

  MPRINTF("Erasing 4KB sector at flash offset 0x00000000...\n\r");
  err = Mc80_ospi_erase(g_mc80_ospi.p_ctrl, (uint8_t *)(OSPI_BASE_ADDRESS + 0x00000000), OSPI_TEST_SECTOR_SIZE);

  if (err == FSP_SUCCESS)
  {
    MPRINTF("Erase command: SUCCESS\n\r");

    MPRINTF("Waiting for erase completion...\n\r");
    uint32_t wait_count = 0;
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

      // Read back to verify erase
      MPRINTF("Reading back erased data for verification...\n\r");

      // Enter XIP mode for read
      err = Mc80_ospi_xip_enter(g_mc80_ospi.p_ctrl);
      if (err != FSP_SUCCESS)
      {
        RTT_printf(0, "  XIP enter failed: 0x%x\r\n", err);
        App_free(read_buffer);
        return;
      }

      // Read data directly from XIP memory
      memcpy(read_buffer, (void *)OSPI_BASE_ADDRESS, 256);

      // Exit XIP mode
      err = Mc80_ospi_xip_exit(g_mc80_ospi.p_ctrl);
      if (err != FSP_SUCCESS)
      {
        RTT_printf(0, "  XIP exit failed: 0x%x\r\n", err);
      }

      err = FSP_SUCCESS;  // Set success for the following check

      if (err == FSP_SUCCESS)
      {
        MPRINTF("Read back: SUCCESS\n\r");

        // Check if all bytes are 0xFF (erased state)
        bool erased = true;
        for (int i = 0; i < 256; i++)
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
  MPRINTF("Testing sector operations in chunks of 256 bytes\n\r");

  uint8_t *chunk_write = (uint8_t *)App_malloc(256);
  uint8_t *chunk_read  = (uint8_t *)App_malloc(256);

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

  // Exit XIP mode
  Mc80_ospi_xip_exit(g_mc80_ospi.p_ctrl);

  // Erase entire sector
  MPRINTF("Erasing 4KB sector...\n\r");
  fsp_err_t err = Mc80_ospi_erase(g_mc80_ospi.p_ctrl, (uint8_t *)(OSPI_BASE_ADDRESS + 0x00000000), OSPI_TEST_SECTOR_SIZE);
  if (err != FSP_SUCCESS)
  {
    MPRINTF("Sector erase failed: 0x%X\n\r", err);
    goto cleanup;
  }

  // Wait for erase
  wait_count = 0;
  do
  {
    R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
    wait_count += 10;
    Mc80_ospi_status_get(g_mc80_ospi.p_ctrl, &status);
  } while (status.write_in_progress && wait_count < 10000);

  MPRINTF("Sector erased in %u ms\n\r", wait_count);

  // Write and verify each 256-byte chunk
  errors = 0;
  for (int chunk = 0; chunk < 16; chunk++)  // 4096 / 256 = 16 chunks
  {
    uint32_t address = chunk * 256;

    // Create unique pattern for this chunk
    for (int i = 0; i < 256; i++)
    {
      chunk_write[i] = (uint8_t)((chunk << 4) | (i & 0x0F));
    }

    // Write chunk
    err = Mc80_ospi_write(g_mc80_ospi.p_ctrl, chunk_write, (uint8_t *)(OSPI_BASE_ADDRESS + address), 256);
    if (err != FSP_SUCCESS)
    {
      MPRINTF("Write chunk %d failed: 0x%X\n\r", chunk, err);
      errors++;
      continue;
    }

    // Wait for write completion
    do
    {
      R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
      Mc80_ospi_status_get(g_mc80_ospi.p_ctrl, &status);
    } while (status.write_in_progress);

    // Read back chunk
    memset(chunk_read, 0, 256);

    // Enter XIP mode for read
    err = Mc80_ospi_xip_enter(g_mc80_ospi.p_ctrl);
    if (err != FSP_SUCCESS)
    {
      MPRINTF("XIP enter for chunk %d failed: 0x%X\n\r", chunk, err);
      errors++;
      continue;
    }

    // Read data directly from XIP memory
    memcpy(chunk_read, (void *)(OSPI_BASE_ADDRESS + address), 256);

    // Exit XIP mode
    err = Mc80_ospi_xip_exit(g_mc80_ospi.p_ctrl);
    if (err != FSP_SUCCESS)
    {
      MPRINTF("XIP exit for chunk %d failed: 0x%X\n\r", chunk, err);
    }

    err = FSP_SUCCESS;  // Set success for the following check
    if (err != FSP_SUCCESS)
    {
      MPRINTF("Read chunk %d failed: 0x%X\n\r", chunk, err);
      errors++;
      continue;
    }

    // Verify chunk
    bool chunk_ok = true;
    for (int i = 0; i < 256; i++)
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
  Description: Test 8D-8D-8D protocol with calibration and XIP read verification

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

  // Exit XIP mode first to ensure we're in command mode
  MPRINTF("Exiting XIP mode...\n\r");
  fsp_err_t err = Mc80_ospi_xip_exit(g_mc80_ospi.p_ctrl);
  if (err == FSP_SUCCESS)
  {
    MPRINTF("XIP Exit                       : SUCCESS\n\r");
  }
  else
  {
    MPRINTF("XIP Exit                       : FAILED (error: 0x%X)\n\r", err);
  }

  // Set 1S-1S-1S protocol for pattern verification
  MPRINTF("\n\rSetting 1S-1S-1S protocol for pattern check...\n\r");
  err = Mc80_ospi_spi_protocol_set(g_mc80_ospi.p_ctrl, MC80_OSPI_PROTOCOL_1S_1S_1S);
  if (err == FSP_SUCCESS)
  {
    MPRINTF("1S-1S-1S Protocol Set          : SUCCESS\n\r");
  }
  else
  {
    MPRINTF("1S-1S-1S Protocol Set          : FAILED (error: 0x%X)\n\r", err);
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
  err = Mc80_ospi_direct_read(g_mc80_ospi.p_ctrl, (uint8_t*)read_patterns, 0x00000000, 16);

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
    MPRINTF("Please use menu option 7 to write patterns first\n\r");
    MPRINTF("Calibration may fail without proper patterns\n\r");
    MPRINTF("Press any key to continue anyway...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
  }
  else
  {
    MPRINTF("\n\rPreamble patterns verified     : SUCCESS\n\r");
  }

  // Now set 8D-8D-8D protocol for calibration
  MPRINTF("\n\rSetting 8D-8D-8D protocol for calibration...\n\r");
  err = Mc80_ospi_spi_protocol_set(g_mc80_ospi.p_ctrl, MC80_OSPI_PROTOCOL_8D_8D_8D);
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

  // Configure flash to OPI DTR mode before testing 8D-8D-8D
  MPRINTF("\n\rConfiguring flash for OPI DTR mode...\n\r");

  // First, switch back to 1S-1S-1S to send the configuration command
  err = Mc80_ospi_spi_protocol_set(g_mc80_ospi.p_ctrl, MC80_OSPI_PROTOCOL_1S_1S_1S);
  if (err == FSP_SUCCESS)
  {
    MPRINTF("1S-1S-1S Protocol Restored     : SUCCESS\n\r");
  }
  else
  {
    MPRINTF("1S-1S-1S Protocol Restored     : FAILED (error: 0x%X)\n\r", err);
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  // Read CR2 before making changes to see current state
  MPRINTF("\n\rReading current CR2 value...\n\r");
  T_mc80_ospi_direct_transfer read_cr2_initial_cmd = {
    .command        = MX25_CMD_RDCR2,
    .command_length = 1,
    .address_length = 4,
    .address        = 0x00000000,
    .data_length    = 1,
    .dummy_cycles   = 0,
  };

  err = Mc80_ospi_direct_transfer(g_mc80_ospi.p_ctrl, &read_cr2_initial_cmd, MC80_OSPI_DIRECT_TRANSFER_DIR_READ);
  if (err == FSP_SUCCESS)
  {
    uint8_t cr2_initial = (uint8_t)(read_cr2_initial_cmd.data & 0xFF);
    MPRINTF("CR2 Initial Value              : 0x%02X\n\r", cr2_initial);
    if (cr2_initial == 0x00)
    {
      MPRINTF("Flash is currently in SPI mode\n\r");
    }
    else if (cr2_initial == 0x02)
    {
      MPRINTF("Flash is already in OPI DTR mode\n\r");
    }
    else
    {
      MPRINTF("Flash is in unknown mode\n\r");
    }
  }
  else
  {
    MPRINTF("CR2 Initial Read               : FAILED (error: 0x%X)\n\r", err);
  }

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
  if (err == FSP_SUCCESS)
  {
    MPRINTF("Write Enable for CR2           : SUCCESS\n\r");
  }
  else
  {
    MPRINTF("Write Enable for CR2           : FAILED (error: 0x%X)\n\r", err);
  }

  // Write CR2 to enable OPI DTR mode (value 0x02)
  T_mc80_ospi_direct_transfer write_cr2_cmd = {
    .command        = MX25_CMD_WRCR2,
    .command_length = 1,
    .address_length = 4,  // 4-byte address required for CR2
    .address        = 0x00000000,  // CR2 address
    .data_length    = 1,
    .data           = 0x02,  // Enable OPI DTR mode
    .dummy_cycles   = 0,
  };

  err = Mc80_ospi_direct_transfer(g_mc80_ospi.p_ctrl, &write_cr2_cmd, MC80_OSPI_DIRECT_TRANSFER_DIR_WRITE);
  if (err == FSP_SUCCESS)
  {
    MPRINTF("CR2 Write (OPI DTR Enable)     : SUCCESS\n\r");

    // Wait for write completion
    R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);

    // Try to read back CR2 with SPI command (may fail if chip switched to OPI)
    T_mc80_ospi_direct_transfer read_cr2_cmd = {
      .command        = MX25_CMD_RDCR2,
      .command_length = 1,
      .address_length = 4,  // 4-byte address required for RDCR2
      .address        = 0x00000000,  // CR2 register address
      .data_length    = 1,
      .dummy_cycles   = 0,  // 0 dummy cycles for SPI mode RDCR2
    };

    err = Mc80_ospi_direct_transfer(g_mc80_ospi.p_ctrl, &read_cr2_cmd, MC80_OSPI_DIRECT_TRANSFER_DIR_READ);
    if (err == FSP_SUCCESS)
    {
      uint8_t cr2_value = (uint8_t)(read_cr2_cmd.data & 0xFF);
      MPRINTF("CR2 Read Back (SPI)            : SUCCESS (value: 0x%02X)\n\r", cr2_value);

      if (cr2_value == 0x02)
      {
        MPRINTF("CR2 Verification               : PASS (OPI DTR enabled)\n\r");
      }
      else
      {
        MPRINTF("CR2 Verification               : FAIL (expected 0x02, got 0x%02X)\n\r", cr2_value);
      }
    }
    else
    {
      MPRINTF("CR2 Read Back (SPI)            : FAILED (error: 0x%X)\n\r", err);
      MPRINTF("This likely means flash switched to OPI mode immediately\n\r");
    }

    // Now switch back to 8D-8D-8D protocol (flash should be in OPI DTR mode)
    err = Mc80_ospi_spi_protocol_set(g_mc80_ospi.p_ctrl, MC80_OSPI_PROTOCOL_8D_8D_8D);
    if (err == FSP_SUCCESS)
    {
      MPRINTF("8D-8D-8D Protocol Re-enabled   : SUCCESS\n\r");
    }
    else
    {
      MPRINTF("8D-8D-8D Protocol Re-enabled   : FAILED (error: 0x%X)\n\r", err);
    }
  }
  else
  {
    MPRINTF("CR2 Write (OPI DTR Enable)     : FAILED (error: 0x%X)\n\r", err);
    MPRINTF("Flash may not support OPI DTR mode switch\n\r");
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
    MPRINTF("Warning: Calibration failed, XIP reads may be unreliable\n\r");
  }

  // Enter XIP mode for high-speed reads
  MPRINTF("\n\rEntering XIP mode...\n\r");
  err = Mc80_ospi_xip_enter(g_mc80_ospi.p_ctrl);
  if (err == FSP_SUCCESS)
  {
    MPRINTF("XIP Enter                      : SUCCESS\n\r");
  }
  else
  {
    MPRINTF("XIP Enter                      : FAILED (error: 0x%X)\n\r", err);
    MPRINTF("Cannot perform XIP read tests\n\r");
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  // Allocate buffers for read tests
  uint8_t *read_buffer1 = (uint8_t *)App_malloc(256);
  uint8_t *read_buffer2 = (uint8_t *)App_malloc(256);
  uint8_t *read_buffer3 = (uint8_t *)App_malloc(256);

  if (read_buffer1 == NULL || read_buffer2 == NULL || read_buffer3 == NULL)
  {
    MPRINTF("ERROR: Failed to allocate read buffers\n\r");
    if (read_buffer1) App_free(read_buffer1);
    if (read_buffer2) App_free(read_buffer2);
    if (read_buffer3) App_free(read_buffer3);

    // Exit XIP mode before returning
    Mc80_ospi_xip_exit(g_mc80_ospi.p_ctrl);
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  // Perform multiple XIP reads from offset 0
  MPRINTF("\n\rPerforming XIP read tests (256 bytes from offset 0x00000000)...\n\r");

  // First read
  MPRINTF("XIP Read 1                     : ");
  memcpy(read_buffer1, (void *)(OSPI_BASE_ADDRESS + 0x00000000), 256);
  MPRINTF("SUCCESS\n\r");

  // Second read
  MPRINTF("XIP Read 2                     : ");
  memcpy(read_buffer2, (void *)(OSPI_BASE_ADDRESS + 0x00000000), 256);
  MPRINTF("SUCCESS\n\r");

  // Third read
  MPRINTF("XIP Read 3                     : ");
  memcpy(read_buffer3, (void *)(OSPI_BASE_ADDRESS + 0x00000000), 256);
  MPRINTF("SUCCESS\n\r");

  // Compare all three reads for consistency
  MPRINTF("\n\rData Consistency Check:\n\r");

  bool reads_match = true;
  int mismatch_count_1_2 = 0;
  int mismatch_count_1_3 = 0;
  int mismatch_count_2_3 = 0;

  for (int i = 0; i < 256; i++)
  {
    if (read_buffer1[i] != read_buffer2[i])
    {
      mismatch_count_1_2++;
      reads_match = false;
    }
    if (read_buffer1[i] != read_buffer3[i])
    {
      mismatch_count_1_3++;
      reads_match = false;
    }
    if (read_buffer2[i] != read_buffer3[i])
    {
      mismatch_count_2_3++;
      reads_match = false;
    }
  }

  if (reads_match)
  {
    MPRINTF("Read Consistency               : PASS (all reads identical)\n\r");
    MPRINTF("8D-8D-8D Calibration Quality   : EXCELLENT\n\r");
  }
  else
  {
    MPRINTF("Read Consistency               : FAIL\n\r");
    MPRINTF("Mismatches Read1 vs Read2      : %d bytes\n\r", mismatch_count_1_2);
    MPRINTF("Mismatches Read1 vs Read3      : %d bytes\n\r", mismatch_count_1_3);
    MPRINTF("Mismatches Read2 vs Read3      : %d bytes\n\r", mismatch_count_2_3);
    MPRINTF("8D-8D-8D Calibration Quality   : POOR - needs recalibration\n\r");
    reads_match = false;
  }

  // Display first 64 bytes of the first read for reference
  MPRINTF("\n\rFirst Read Data (first 64 bytes):\n\r");
  for (int i = 0; i < 64; i++)
  {
    if (i % 16 == 0)
    {
      MPRINTF("%04X: ", i);
    }
    MPRINTF("%02X ", read_buffer1[i]);
    if (i % 16 == 15)
    {
      MPRINTF("\n\r");
    }
  }

  // If reads don't match, show first few mismatched bytes
  if (!reads_match)
  {
    MPRINTF("\n\rFirst 16 bytes comparison:\n\r");
    MPRINTF("Read 1: ");
    for (int i = 0; i < 16; i++) MPRINTF("%02X ", read_buffer1[i]);
    MPRINTF("\n\rRead 2: ");
    for (int i = 0; i < 16; i++) MPRINTF("%02X ", read_buffer2[i]);
    MPRINTF("\n\rRead 3: ");
    for (int i = 0; i < 16; i++) MPRINTF("%02X ", read_buffer3[i]);
    MPRINTF("\n\r");
  }

  // Clean up
  App_free(read_buffer1);
  App_free(read_buffer2);
  App_free(read_buffer3);

  // Exit XIP mode
  MPRINTF("\n\rExiting XIP mode...\n\r");
  err = Mc80_ospi_xip_exit(g_mc80_ospi.p_ctrl);
  if (err == FSP_SUCCESS)
  {
    MPRINTF("XIP Exit                       : SUCCESS\n\r");
  }
  else
  {
    MPRINTF("XIP Exit                       : FAILED (error: 0x%X)\n\r", err);
  }

  // Restore flash to SPI mode (CR2 = 0x00)
  MPRINTF("\n\rRestoring flash to SPI mode...\n\r");

  // Switch to 1S-1S-1S protocol to send configuration commands
  err = Mc80_ospi_spi_protocol_set(g_mc80_ospi.p_ctrl, MC80_OSPI_PROTOCOL_1S_1S_1S);
  if (err == FSP_SUCCESS)
  {
    MPRINTF("1S-1S-1S Protocol Set          : SUCCESS\n\r");

    // Write Enable for CR2 restoration
    T_mc80_ospi_direct_transfer write_enable_cmd = {
      .command        = MX25_CMD_WREN,
      .command_length = 1,
      .address_length = 0,
      .address        = 0,
      .data_length    = 0,
      .dummy_cycles   = 0,
    };

    err = Mc80_ospi_direct_transfer(g_mc80_ospi.p_ctrl, &write_enable_cmd, MC80_OSPI_DIRECT_TRANSFER_DIR_WRITE);
    if (err == FSP_SUCCESS)
    {
      // Write CR2 to restore SPI mode (value 0x00)
      T_mc80_ospi_direct_transfer restore_cr2_cmd = {
        .command        = MX25_CMD_WRCR2,
        .command_length = 1,
        .address_length = 4,
        .address        = 0x00000000,
        .data_length    = 1,
        .data           = 0x00,  // Restore SPI mode
        .dummy_cycles   = 0,
      };

      err = Mc80_ospi_direct_transfer(g_mc80_ospi.p_ctrl, &restore_cr2_cmd, MC80_OSPI_DIRECT_TRANSFER_DIR_WRITE);
      if (err == FSP_SUCCESS)
      {
        MPRINTF("CR2 Restore (SPI Mode)         : SUCCESS\n\r");
        R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);

        // Read back CR2 to verify the restore
        T_mc80_ospi_direct_transfer read_cr2_restore_cmd = {
          .command        = MX25_CMD_RDCR2,
          .command_length = 1,
          .address_length = 4,  // 4-byte address required for RDCR2
          .address        = 0x00000000,  // CR2 register address
          .data_length    = 1,
          .dummy_cycles   = 0,  // 0 dummy cycles for SPI mode RDCR2
        };

        err = Mc80_ospi_direct_transfer(g_mc80_ospi.p_ctrl, &read_cr2_restore_cmd, MC80_OSPI_DIRECT_TRANSFER_DIR_READ);
        if (err == FSP_SUCCESS)
        {
          uint8_t cr2_restore_value = (uint8_t)(read_cr2_restore_cmd.data & 0xFF);
          MPRINTF("CR2 Restore Read Back          : SUCCESS (value: 0x%02X)\n\r", cr2_restore_value);

          if (cr2_restore_value == 0x00)
          {
            MPRINTF("CR2 Restore Verification       : PASS (SPI mode restored)\n\r");
          }
          else
          {
            MPRINTF("CR2 Restore Verification       : FAIL (expected 0x00, got 0x%02X)\n\r", cr2_restore_value);
          }
        }
        else
        {
          MPRINTF("CR2 Restore Read Back          : FAILED (error: 0x%X)\n\r", err);
        }
      }
      else
      {
        MPRINTF("CR2 Restore (SPI Mode)         : FAILED (error: 0x%X)\n\r", err);
      }
    }
    else
    {
      MPRINTF("Write Enable for CR2 Restore   : FAILED (error: 0x%X)\n\r", err);
    }
  }
  else
  {
    MPRINTF("1S-1S-1S Protocol Set          : FAILED (error: 0x%X)\n\r", err);
  }

  // Test summary
  MPRINTF("\n\r===== Test Summary =====\n\r");
  if (reads_match)
  {
    MPRINTF("Overall Result                 : PASS\n\r");
    MPRINTF("8D-8D-8D protocol is working correctly with good calibration\n\r");
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
    RTT_printf(0, "OSPI driver opened successfully\r\n");
    return FSP_SUCCESS;
  }
  else if (err == FSP_ERR_ALREADY_OPEN)
  {
    // Driver was already open - this is good
    return FSP_SUCCESS;
  }
  else
  {
    RTT_printf(0, "OSPI driver open failed: 0x%x\r\n", err);
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

  // Exit XIP mode first to ensure we're in command mode
  MPRINTF("Exiting XIP mode...\n\r");
  fsp_err_t err = Mc80_ospi_xip_exit(g_mc80_ospi.p_ctrl);
  if (err == FSP_SUCCESS)
  {
    MPRINTF("XIP Exit                       : SUCCESS\n\r");
  }
  else
  {
    MPRINTF("XIP Exit                       : FAILED (error: 0x%X)\n\r", err);
  }

  // Set 1S-1S-1S protocol for reliable pattern writing
  MPRINTF("\n\rSetting 1S-1S-1S protocol for pattern writing...\n\r");
  err = Mc80_ospi_spi_protocol_set(g_mc80_ospi.p_ctrl, MC80_OSPI_PROTOCOL_1S_1S_1S);
  if (err == FSP_SUCCESS)
  {
    MPRINTF("1S-1S-1S Protocol Set          : SUCCESS\n\r");
  }
  else
  {
    MPRINTF("1S-1S-1S Protocol Set          : FAILED (error: 0x%X)\n\r", err);
    MPRINTF("Cannot continue without protocol setup\n\r");
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  // Warning about sector erase
  MPRINTF("\n\rWARNING: This will erase 4KB sector at address 0x00000000\n\r");
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
    uint32_t wait_count = 0;
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
  err = Mc80_ospi_write(g_mc80_ospi.p_ctrl, (uint8_t*)patterns, (uint8_t *)(OSPI_BASE_ADDRESS + calibration_address), 16);
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
    err = Mc80_ospi_direct_read(g_mc80_ospi.p_ctrl, (uint8_t*)read_patterns, calibration_address, 16);
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
        MPRINTF("Ready for 8D-8D-8D calibration\n\r");
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
