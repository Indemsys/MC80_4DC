/*-----------------------------------------------------------------------------------------------------
  Description: OSPI Flash memory testing terminal interface
               Provides direct access to OSPI flash operations for debugging

  Parameters:

  Return:
-----------------------------------------------------------------------------------------------------*/

#include "App.h"
#include "RTT_utils.h"

// External OSPI driver reference
extern const spi_flash_instance_t g_OSPI;

#define OSPI_TEST_PATTERN_SIZE 256
#define OSPI_TEST_SECTOR_SIZE  4096

// Function declarations
void OSPI_test_info(uint8_t keycode);
void OSPI_test_read(uint8_t keycode);
void OSPI_test_write(uint8_t keycode);
void OSPI_test_erase(uint8_t keycode);
void OSPI_test_pattern(uint8_t keycode);
void OSPI_test_sector(uint8_t keycode);
void OSPI_test_status(uint8_t keycode);
void OSPI_test_xip_mode(uint8_t keycode);

// Internal helper functions
static fsp_err_t _Ospi_ensure_driver_open(void);

const T_VT100_Menu_item MENU_OSPI_ITEMS[] = {
  { '1', OSPI_test_info, 0 },
  { '2', OSPI_test_status, 0 },
  { '3', OSPI_test_read, 0 },
  { '4', OSPI_test_write, 0 },
  { '5', OSPI_test_erase, 0 },
  { '6', OSPI_test_pattern, 0 },
  { '7', OSPI_test_sector, 0 },
  { '8', OSPI_test_xip_mode, 0 },
  { 'R', 0, 0 },
  { 0 }
};

const T_VT100_Menu MENU_OSPI = {
  "OSPI Flash Testing",
  "\033[5C OSPI Flash memory testing menu\r\n"
  "\033[5C <1> - OSPI Flash information\r\n"
  "\033[5C <2> - Check OSPI status\r\n"
  "\033[5C <3> - Read data test\r\n"
  "\033[5C <4> - Write data test\r\n"
  "\033[5C <5> - Erase sector test\r\n"
  "\033[5C <6> - Pattern test (write/read/verify)\r\n"
  "\033[5C <7> - Full sector test\r\n"
  "\033[5C <8> - XIP mode test\r\n"
  "\033[5C <R> - Return to previous menu\r\n",
  MENU_OSPI_ITEMS,
};

/*-----------------------------------------------------------------------------------------------------
  Description: Display OSPI flash information

  Parameters: keycode - Input key code from VT100 terminal

  Return:
-----------------------------------------------------------------------------------------------------*/
void OSPI_test_info(uint8_t keycode)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF(" ===== OSPI Flash Information =====\n\r");

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
  spi_flash_status_t status;
  fsp_err_t          err = g_OSPI.p_api->statusGet(g_OSPI.p_ctrl, &status);

  if (err == FSP_SUCCESS)
  {
    MPRINTF("OSPI Flash Communication: OK\n\r");
    MPRINTF("Write in progress: %s\n\r", status.write_in_progress ? "YES" : "NO");
  }
  else
  {
    MPRINTF("OSPI Flash Communication: FAILED (error: 0x%X)\n\r", err);
  }

  MPRINTF("\n\rFlash Memory Specifications (MX25UM25645G):\n\r");
  MPRINTF("- Total capacity: 256 Mbit (32 MB)\n\r");
  MPRINTF("- Page size: 256 bytes\n\r");
  MPRINTF("- Sector size: 4 KB\n\r");
  MPRINTF("- Block size: 64 KB\n\r");
  MPRINTF("- Operating voltage: 1.8V\n\r");
  MPRINTF("- Max read frequency: 200 MHz (SDR)\n\r");
  MPRINTF("- Max program time: 700 µs (page)\n\r");
  MPRINTF("- Max erase time: 300 ms (sector)\n\r");

  MPRINTF("\n\rOSPI Address Mapping:\n\r");
  MPRINTF("- XIP base address: 0x80000000\n\r");
  MPRINTF("- XIP address range: 0x80000000 - 0x81FFFFFF\n\r");
  MPRINTF("- Total addressable: 32 MB\n\r");

  MPRINTF("\n\rPress any key to continue...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
}

/*-----------------------------------------------------------------------------------------------------
  Description: Check OSPI flash status

  Parameters: keycode - Input key code from VT100 terminal

  Return:
-----------------------------------------------------------------------------------------------------*/
void OSPI_test_status(uint8_t keycode)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF(" ===== OSPI Flash Status =====\n\r");

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

  spi_flash_status_t status;
  fsp_err_t          err = g_OSPI.p_api->statusGet(g_OSPI.p_ctrl, &status);

  if (err == FSP_SUCCESS)
  {
    MPRINTF("Status read: SUCCESS\n\r");
    MPRINTF("Write in progress: %s\n\r", status.write_in_progress ? "YES" : "NO");
  }
  else
  {
    MPRINTF("Status read: FAILED\n\r");
    MPRINTF("Error code: 0x%X\n\r", err);
  }

  // Test XIP status
  MPRINTF("\n\rTesting XIP mode operations...\n\r");

  // Try to enter XIP mode
  err = R_OSPI_B_XipEnter(g_OSPI.p_ctrl);
  if (err == FSP_SUCCESS)
  {
    MPRINTF("XIP Enter: SUCCESS\n\r");

    // Try to exit XIP mode
    err = R_OSPI_B_XipExit(g_OSPI.p_ctrl);
    if (err == FSP_SUCCESS)
    {
      MPRINTF("XIP Exit: SUCCESS\n\r");
    }
    else
    {
      MPRINTF("XIP Exit: FAILED (error: 0x%X)\n\r", err);
    }
  }
  else
  {
    MPRINTF("XIP Enter: FAILED (error: 0x%X)\n\r", err);
  }

  // Test SPI protocol setting
  MPRINTF("\n\rTesting SPI protocol setting...\n\r");
  err = g_OSPI.p_api->spiProtocolSet(g_OSPI.p_ctrl, SPI_FLASH_PROTOCOL_EXTENDED_SPI);
  if (err == FSP_SUCCESS)
  {
    MPRINTF("SPI Protocol Set: SUCCESS\n\r");
  }
  else
  {
    MPRINTF("SPI Protocol Set: FAILED (error: 0x%X)\n\r", err);
  }

  MPRINTF("\n\rPress any key to continue...\n\r");
  uint8_t dummy_key;
  WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
}

/*-----------------------------------------------------------------------------------------------------
  Description: Test OSPI read operations

  Parameters: keycode - Input key code from VT100 terminal

  Return:
-----------------------------------------------------------------------------------------------------*/
void OSPI_test_read(uint8_t keycode)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF(" ===== OSPI Read Test =====\n\r");

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

  uint8_t *buffer1 = (uint8_t *)App_malloc(256);
  uint8_t *buffer2 = (uint8_t *)App_malloc(256);
  uint8_t *buffer3 = (uint8_t *)App_malloc(256);

  if (!buffer1 || !buffer2 || !buffer3)
  {
    MPRINTF("ERROR: Failed to allocate buffers\n\r");
    if (buffer1) App_free(buffer1);
    if (buffer2) App_free(buffer2);
    if (buffer3) App_free(buffer3);
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  // Exit XIP mode to start fresh
  fsp_err_t err = R_OSPI_B_XipExit(g_OSPI.p_ctrl);
  if (err == FSP_SUCCESS)
  {
    MPRINTF("Initial XIP exit: SUCCESS\n\r");
  }
  else
  {
    MPRINTF("Initial XIP exit: 0x%X (probably wasn't in XIP mode)\n\r", err);
  }

  R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);

  MPRINTF("\n\r=== Test 1: Multiple reads in same XIP session ===\n\r");

  // Enter XIP mode
  err = R_OSPI_B_XipEnter(g_OSPI.p_ctrl);
  if (err != FSP_SUCCESS)
  {
    MPRINTF("XIP enter failed: 0x%X\n\r", err);
    goto cleanup;
  }
  MPRINTF("XIP mode entered\n\r");

  // Add cache invalidation after XIP enter
  SCB_InvalidateDCache();
  __DSB();

  R_BSP_SoftwareDelay(5, BSP_DELAY_UNITS_MILLISECONDS);

  // First read using volatile pointer
  MPRINTF("\n\rRead 1 (volatile pointer, address 0x%08X): ", OSPI_BASE_ADDRESS);
  volatile uint8_t *vol_ptr = (volatile uint8_t *)OSPI_BASE_ADDRESS;
  for (int i = 0; i < 16; i++)
  {
    buffer1[i] = vol_ptr[i];
    MPRINTF("%02X ", buffer1[i]);
  }
  MPRINTF("\n\r");

  // Check if all 0xFF
  bool all_ff = true;
  for (int i = 0; i < 16; i++)
  {
    if (buffer1[i] != 0xFF)
    {
      all_ff = false;
      break;
    }
  }
  if (all_ff)
  {
    MPRINTF("WARNING: First read returned all 0xFF!\n\r");
  }

  // Delay between reads
  R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);

  // Second read from same address without exit/enter
  MPRINTF("\n\rRead 2 (same address, no exit/enter): ");
  for (int i = 0; i < 16; i++)
  {
    buffer2[i] = vol_ptr[i];
    MPRINTF("%02X ", buffer2[i]);
  }
  MPRINTF("\n\r");

  // Third read from different offset
  MPRINTF("\n\rRead 3 (offset +256 bytes): ");
  volatile uint8_t *vol_ptr2 = (volatile uint8_t *)(OSPI_BASE_ADDRESS + 256);
  for (int i = 0; i < 16; i++)
  {
    buffer3[i] = vol_ptr2[i];
    MPRINTF("%02X ", buffer3[i]);
  }
  MPRINTF("\n\r");

  // Compare reads
  bool same = true;
  for (int i = 0; i < 16; i++)
  {
    if (buffer1[i] != buffer2[i])
    {
      same = false;
      break;
    }
  }
  MPRINTF("\n\rRead 1 vs Read 2: %s\n\r", same ? "IDENTICAL" : "DIFFERENT");

  // Exit and re-enter XIP
  MPRINTF("\n\r=== Test 2: Read after XIP exit/enter cycle ===\n\r");

  err = R_OSPI_B_XipExit(g_OSPI.p_ctrl);
  if (err != FSP_SUCCESS)
  {
    MPRINTF("XIP exit failed: 0x%X\n\r", err);
  }
  else
  {
    MPRINTF("XIP exit successful\n\r");
  }

  R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);

  err = R_OSPI_B_XipEnter(g_OSPI.p_ctrl);
  if (err != FSP_SUCCESS)
  {
    MPRINTF("XIP re-enter failed: 0x%X\n\r", err);
    goto cleanup;
  }
  MPRINTF("XIP re-enter successful\n\r");

  // Invalidate cache again
  SCB_InvalidateDCache();
  __DSB();

  R_BSP_SoftwareDelay(5, BSP_DELAY_UNITS_MILLISECONDS);

  // Read after re-enter
  MPRINTF("\n\rRead 4 (after exit/enter cycle): ");
  for (int i = 0; i < 16; i++)
  {
    uint8_t val = vol_ptr[i];
    MPRINTF("%02X ", val);
  }
  MPRINTF("\n\r");

  // Test hypothesis: read twice after each XIP enter
  MPRINTF("\n\r=== Test 3: Double read after each XIP enter ===\n\r");

  // Exit and enter again
  R_OSPI_B_XipExit(g_OSPI.p_ctrl);
  R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
  R_OSPI_B_XipEnter(g_OSPI.p_ctrl);
  SCB_InvalidateDCache();
  __DSB();
  R_BSP_SoftwareDelay(5, BSP_DELAY_UNITS_MILLISECONDS);

  MPRINTF("First read after new XIP enter: ");
  for (int i = 0; i < 16; i++)
  {
    MPRINTF("%02X ", vol_ptr[i]);
  }
  MPRINTF("\n\r");

  MPRINTF("Second read without exit/enter: ");
  for (int i = 0; i < 16; i++)
  {
    MPRINTF("%02X ", vol_ptr[i]);
  }
  MPRINTF("\n\r");

  // Test with memory barriers
  MPRINTF("\n\r=== Test 4: Testing with memory barriers ===\n\r");

  MPRINTF("Read with DSB/ISB barriers: ");
  __DSB();
  __ISB();
  for (int i = 0; i < 16; i++)
  {
    MPRINTF("%02X ", vol_ptr[i]);
  }
  MPRINTF("\n\r");

  // Test cache flush by address
  MPRINTF("Read with cache invalidation by address: ");
  SCB_InvalidateDCache_by_Addr((uint32_t *)vol_ptr, 16);
  __DSB();
  for (int i = 0; i < 16; i++)
  {
    MPRINTF("%02X ", vol_ptr[i]);
  }
  MPRINTF("\n\r");

  // Test reading different memory types
  MPRINTF("\n\r=== Test 5: Different access patterns ===\n\r");

  // Byte access
  MPRINTF("Byte access: ");
  for (int i = 0; i < 4; i++)
  {
    MPRINTF("%02X ", vol_ptr[i]);
  }
  MPRINTF("\n\r");

  // Word access
  MPRINTF("Word access: ");
  volatile uint32_t *word_ptr = (volatile uint32_t *)OSPI_BASE_ADDRESS;
  uint32_t           word     = word_ptr[0];
  MPRINTF("%08X (bytes: %02X %02X %02X %02X)\n\r",
          word,
          (uint8_t)(word & 0xFF),
          (uint8_t)((word >> 8) & 0xFF),
          (uint8_t)((word >> 16) & 0xFF),
          (uint8_t)((word >> 24) & 0xFF));

cleanup:
  // Final XIP exit
  R_OSPI_B_XipExit(g_OSPI.p_ctrl);

  App_free(buffer1);
  App_free(buffer2);
  App_free(buffer3);

  MPRINTF("\n\rPress any key to continue...\n\r");
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

  uint8_t *write_buffer = (uint8_t *)App_malloc(64);
  uint8_t *read_buffer  = (uint8_t *)App_malloc(64);

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

  // Create test pattern
  for (int i = 0; i < 64; i++)
  {
    write_buffer[i] = 0x55 + i;
  }

  MPRINTF("Test pattern created (64 bytes starting with 0x55)\n\r");

  // Exit XIP mode
  fsp_err_t err = R_OSPI_B_XipExit(g_OSPI.p_ctrl);
  if (err != FSP_SUCCESS)
  {
    MPRINTF("Warning: XIP exit failed (0x%X)\n\r", err);
  }

  // Check flash status
  spi_flash_status_t status;
  err = g_OSPI.p_api->statusGet(g_OSPI.p_ctrl, &status);
  if (err == FSP_SUCCESS && status.write_in_progress)
  {
    MPRINTF("Flash is busy, waiting...\n\r");
    // Simple wait - in real code should have timeout
    do
    {
      R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
      g_OSPI.p_api->statusGet(g_OSPI.p_ctrl, &status);
    } while (status.write_in_progress);
  }

  // Perform write
  MPRINTF("Writing 64 bytes to flash offset 0x00000000...\n\r");
  err = g_OSPI.p_api->write(g_OSPI.p_ctrl, write_buffer, (uint8_t *)(OSPI_BASE_ADDRESS + 0x00000000), 64);

  if (err == FSP_SUCCESS)
  {
    MPRINTF("Write operation: SUCCESS\n\r");

    // Wait for write completion
    do
    {
      R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
      g_OSPI.p_api->statusGet(g_OSPI.p_ctrl, &status);
    } while (status.write_in_progress);

    MPRINTF("Write completed\n\r");

    // Read back and verify
    MPRINTF("Reading back data for verification...\n\r");

    // Enter XIP mode for read
    err = R_OSPI_B_XipEnter(g_OSPI.p_ctrl);
    if (err != FSP_SUCCESS)
    {
      RTT_printf(0, "  XIP enter failed: 0x%x\r\n", err);
      App_free(write_buffer);
      App_free(read_buffer);
      return;
    }

    // Read data directly from XIP memory
    memcpy(read_buffer, (void *)OSPI_BASE_ADDRESS, 64);

    // Exit XIP mode
    err = R_OSPI_B_XipExit(g_OSPI.p_ctrl);
    if (err != FSP_SUCCESS)
    {
      RTT_printf(0, "  XIP exit failed: 0x%x\r\n", err);
    }

    err = FSP_SUCCESS;  // Set success for the following check

    if (err == FSP_SUCCESS)
    {
      MPRINTF("Read back: SUCCESS\n\r");

      // Compare data
      bool match = true;
      for (int i = 0; i < 64; i++)
      {
        if (write_buffer[i] != read_buffer[i])
        {
          match = false;
          break;
        }
      }

      if (match)
      {
        MPRINTF("Data verification: PASS\n\r");
      }
      else
      {
        MPRINTF("Data verification: FAIL\n\r");
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
      MPRINTF("Read back: FAILED (0x%X)\n\r", err);
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
  fsp_err_t err = R_OSPI_B_XipExit(g_OSPI.p_ctrl);
  if (err != FSP_SUCCESS)
  {
    MPRINTF("Warning: XIP exit failed (0x%X)\n\r", err);
  }

  // Check initial flash status
  spi_flash_status_t status;
  err = g_OSPI.p_api->statusGet(g_OSPI.p_ctrl, &status);
  if (err == FSP_SUCCESS && status.write_in_progress)
  {
    MPRINTF("Flash is busy, waiting...\n\r");
    do
    {
      R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
      g_OSPI.p_api->statusGet(g_OSPI.p_ctrl, &status);
    } while (status.write_in_progress);
  }

  MPRINTF("Erasing 4KB sector at flash offset 0x00000000...\n\r");
  err = g_OSPI.p_api->erase(g_OSPI.p_ctrl, (uint8_t *)(OSPI_BASE_ADDRESS + 0x00000000), OSPI_TEST_SECTOR_SIZE);

  if (err == FSP_SUCCESS)
  {
    MPRINTF("Erase command: SUCCESS\n\r");

    MPRINTF("Waiting for erase completion...\n\r");
    uint32_t wait_count = 0;
    do
    {
      R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
      wait_count += 10;
      g_OSPI.p_api->statusGet(g_OSPI.p_ctrl, &status);

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
      err = R_OSPI_B_XipEnter(g_OSPI.p_ctrl);
      if (err != FSP_SUCCESS)
      {
        RTT_printf(0, "  XIP enter failed: 0x%x\r\n", err);
        App_free(read_buffer);
        return;
      }

      // Read data directly from XIP memory
      memcpy(read_buffer, (void *)OSPI_BASE_ADDRESS, 256);

      // Exit XIP mode
      err = R_OSPI_B_XipExit(g_OSPI.p_ctrl);
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
  Description: Test OSPI pattern write/read/verify

  Parameters: keycode - Input key code from VT100 terminal

  Return:
-----------------------------------------------------------------------------------------------------*/
void OSPI_test_pattern(uint8_t keycode)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF(" ===== OSPI Pattern Test =====\n\r");

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

  MPRINTF("This will test erase/write/read/verify sequence\n\r");
  MPRINTF("WARNING: This will modify flash at address 0x00000000\n\r");
  MPRINTF("Press 'Y' to continue or any other key to cancel: ");

  uint8_t ch;
  WAIT_CHAR(&ch, ms_to_ticks(100000));
  MPRINTF("%c\n\r", ch);

  if (ch != 'Y' && ch != 'y')
  {
    MPRINTF("Pattern test cancelled.\n\r");
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  uint8_t *write_buffer = (uint8_t *)App_malloc(OSPI_TEST_PATTERN_SIZE);
  uint8_t *read_buffer  = (uint8_t *)App_malloc(OSPI_TEST_PATTERN_SIZE);

  // Declare variables used in goto sections to avoid bypass warnings
  spi_flash_status_t status;
  uint32_t           wait_count  = 0;
  bool               match       = true;
  int                first_error = -1;

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

  // Create test patterns
  MPRINTF("Creating test patterns...\n\r");
  for (int i = 0; i < OSPI_TEST_PATTERN_SIZE; i++)
  {
    write_buffer[i] = (uint8_t)(i ^ 0xAA);  // XOR pattern
  }

  // Exit XIP mode
  fsp_err_t err = R_OSPI_B_XipExit(g_OSPI.p_ctrl);
  if (err != FSP_SUCCESS)
  {
    MPRINTF("Warning: XIP exit failed (0x%X)\n\r", err);
  }

  // Step 1: Erase sector
  MPRINTF("Step 1: Erasing sector...\n\r");
  err = g_OSPI.p_api->erase(g_OSPI.p_ctrl, (uint8_t *)(OSPI_BASE_ADDRESS + 0x00000000), OSPI_TEST_SECTOR_SIZE);
  if (err != FSP_SUCCESS)
  {
    MPRINTF("Erase failed: 0x%X\n\r", err);
    goto cleanup;
  }

  // Wait for erase completion
  wait_count = 0;
  do
  {
    R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
    wait_count += 10;
    g_OSPI.p_api->statusGet(g_OSPI.p_ctrl, &status);
  } while (status.write_in_progress && wait_count < 5000);

  if (status.write_in_progress)
  {
    MPRINTF("Erase timeout!\n\r");
    goto cleanup;
  }
  MPRINTF("Erase completed in %u ms\n\r", wait_count);

  // Step 2: Write pattern
  MPRINTF("Step 2: Writing pattern (%d bytes)...\n\r", OSPI_TEST_PATTERN_SIZE);
  err = g_OSPI.p_api->write(g_OSPI.p_ctrl, write_buffer, (uint8_t *)(OSPI_BASE_ADDRESS + 0x00000000), OSPI_TEST_PATTERN_SIZE);
  if (err != FSP_SUCCESS)
  {
    MPRINTF("Write failed: 0x%X\n\r", err);
    goto cleanup;
  }

  // Wait for write completion
  wait_count = 0;
  do
  {
    R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
    wait_count++;
    g_OSPI.p_api->statusGet(g_OSPI.p_ctrl, &status);
  } while (status.write_in_progress && wait_count < 1000);

  if (status.write_in_progress)
  {
    MPRINTF("Write timeout!\n\r");
    goto cleanup;
  }
  MPRINTF("Write completed in %u ms\n\r", wait_count);

  // Step 3: Read back
  MPRINTF("Step 3: Reading back data...\n\r");
  memset(read_buffer, 0, OSPI_TEST_PATTERN_SIZE);

  // Enter XIP mode for read
  err = R_OSPI_B_XipEnter(g_OSPI.p_ctrl);
  if (err != FSP_SUCCESS)
  {
    MPRINTF("XIP enter failed: 0x%X\n\r", err);
    goto cleanup;
  }

  // Read data directly from XIP memory
  memcpy(read_buffer, (void *)OSPI_BASE_ADDRESS, OSPI_TEST_PATTERN_SIZE);

  // Exit XIP mode
  err = R_OSPI_B_XipExit(g_OSPI.p_ctrl);
  if (err != FSP_SUCCESS)
  {
    MPRINTF("XIP exit failed: 0x%X\n\r", err);
  }

  err = FSP_SUCCESS;  // Set success for the following check
  if (err != FSP_SUCCESS)
  {
    MPRINTF("Read failed: 0x%X\n\r", err);
    goto cleanup;
  }

  // Step 4: Verify
  MPRINTF("Step 4: Verifying data...\n\r");
  match       = true;
  first_error = -1;
  for (int i = 0; i < OSPI_TEST_PATTERN_SIZE; i++)
  {
    if (write_buffer[i] != read_buffer[i])
    {
      if (first_error == -1) first_error = i;
      match = false;
    }
  }

  if (match)
  {
    MPRINTF("Pattern test: PASS\n\r");
    MPRINTF("All %d bytes verified successfully\n\r", OSPI_TEST_PATTERN_SIZE);
  }
  else
  {
    MPRINTF("Pattern test: FAIL\n\r");
    MPRINTF("First error at byte %d\n\r", first_error);
    MPRINTF("Expected: 0x%02X, Got: 0x%02X\n\r",
            write_buffer[first_error], read_buffer[first_error]);
  }

cleanup:
  App_free(write_buffer);
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
  spi_flash_status_t status;
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
  R_OSPI_B_XipExit(g_OSPI.p_ctrl);

  // Erase entire sector
  MPRINTF("Erasing 4KB sector...\n\r");
  fsp_err_t err = g_OSPI.p_api->erase(g_OSPI.p_ctrl, (uint8_t *)(OSPI_BASE_ADDRESS + 0x00000000), OSPI_TEST_SECTOR_SIZE);
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
    g_OSPI.p_api->statusGet(g_OSPI.p_ctrl, &status);
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
    err = g_OSPI.p_api->write(g_OSPI.p_ctrl, chunk_write, (uint8_t *)(OSPI_BASE_ADDRESS + address), 256);
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
      g_OSPI.p_api->statusGet(g_OSPI.p_ctrl, &status);
    } while (status.write_in_progress);

    // Read back chunk
    memset(chunk_read, 0, 256);

    // Enter XIP mode for read
    err = R_OSPI_B_XipEnter(g_OSPI.p_ctrl);
    if (err != FSP_SUCCESS)
    {
      MPRINTF("XIP enter for chunk %d failed: 0x%X\n\r", chunk, err);
      errors++;
      continue;
    }

    // Read data directly from XIP memory
    memcpy(chunk_read, (void *)(OSPI_BASE_ADDRESS + address), 256);

    // Exit XIP mode
    err = R_OSPI_B_XipExit(g_OSPI.p_ctrl);
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
  Description: Test XIP mode operations

  Parameters: keycode - Input key code from VT100 terminal

  Return:
-----------------------------------------------------------------------------------------------------*/
void OSPI_test_xip_mode(uint8_t keycode)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
  MPRINTF(" ===== XIP Mode Test =====\n\r");

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

  uint8_t *compare_buffer = (uint8_t *)App_malloc(256);
  if (compare_buffer == NULL)
  {
    MPRINTF("ERROR: Failed to allocate compare buffer\n\r");
    MPRINTF("Press any key to continue...\n\r");
    uint8_t dummy_key;
    WAIT_CHAR(&dummy_key, ms_to_ticks(100000));
    return;
  }

  // Test 1: Enter XIP mode
  MPRINTF("Test 1: Entering XIP mode...\n\r");
  fsp_err_t err = R_OSPI_B_XipEnter(g_OSPI.p_ctrl);
  if (err == FSP_SUCCESS)
  {
    MPRINTF("XIP Enter: SUCCESS\n\r");

    // Test 2: Read via XIP (direct memory access)
    MPRINTF("Test 2: Reading via XIP direct access...\n\r");
    uint8_t *xip_base = (uint8_t *)OSPI_BASE_ADDRESS;

    MPRINTF("XIP data at 0x80000000 (first 32 bytes):\n\r");
    for (int i = 0; i < 32; i++)
    {
      if (i % 16 == 0)
      {
        MPRINTF("%04X: ", i);
      }
      MPRINTF("%02X ", xip_base[i]);
      if (i % 16 == 15)
      {
        MPRINTF("\n\r");
      }
    }

    // Test 3: Compare XIP read with SPI read
    MPRINTF("\n\rTest 3: Comparing XIP vs SPI read...\n\r");

    // Exit XIP for SPI read
    err = R_OSPI_B_XipExit(g_OSPI.p_ctrl);
    if (err == FSP_SUCCESS)
    {
      MPRINTF("XIP Exit: SUCCESS\n\r");

      // Enter XIP mode for read
      err = R_OSPI_B_XipEnter(g_OSPI.p_ctrl);
      if (err != FSP_SUCCESS)
      {
        MPRINTF("XIP re-enter failed: 0x%X\n\r", err);
        App_free(compare_buffer);
        return;
      }

      // Read data directly from XIP memory
      memcpy(compare_buffer, (void *)OSPI_BASE_ADDRESS, 256);

      // Exit XIP mode
      err = R_OSPI_B_XipExit(g_OSPI.p_ctrl);
      if (err != FSP_SUCCESS)
      {
        MPRINTF("XIP exit failed: 0x%X\n\r", err);
      }

      err = FSP_SUCCESS;  // Set success for the following check

      if (err == FSP_SUCCESS)
      {
        MPRINTF("SPI read: SUCCESS\n\r");

        // Re-enter XIP
        err = R_OSPI_B_XipEnter(g_OSPI.p_ctrl);
        if (err == FSP_SUCCESS)
        {
          MPRINTF("XIP Re-enter: SUCCESS\n\r");

          // Compare first 256 bytes
          bool match      = true;
          int  diff_count = 0;
          for (int i = 0; i < 256; i++)
          {
            if (xip_base[i] != compare_buffer[i])
            {
              match = false;
              diff_count++;
              if (diff_count <= 5)  // Show first 5 differences
              {
                MPRINTF("Diff at %d: XIP=0x%02X SPI=0x%02X\n\r",
                        i, xip_base[i], compare_buffer[i]);
              }
            }
          }

          if (match)
          {
            MPRINTF("XIP vs SPI comparison: MATCH\n\r");
          }
          else
          {
            MPRINTF("XIP vs SPI comparison: DIFFER (%d bytes)\n\r", diff_count);
          }
        }
        else
        {
          MPRINTF("XIP Re-enter: FAILED (0x%X)\n\r", err);
        }
      }
      else
      {
        MPRINTF("SPI read: FAILED (0x%X)\n\r", err);
      }
    }
    else
    {
      MPRINTF("XIP Exit: FAILED (0x%X)\n\r", err);
    }

    // Test 4: Performance comparison
    MPRINTF("\n\rTest 4: Performance comparison...\n\r");

    // XIP performance
    uint32_t          start_time = tx_time_get();
    volatile uint32_t checksum   = 0;
    for (int i = 0; i < 1024; i++)
    {
      checksum += xip_base[i];
    }
    uint32_t xip_time = tx_time_get() - start_time;

    MPRINTF("XIP read 1KB: %lu ticks (checksum: 0x%08X)\n\r", xip_time, checksum);
  }
  else
  {
    MPRINTF("XIP Enter: FAILED (0x%X)\n\r", err);
  }

  App_free(compare_buffer);

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
  err = g_OSPI.p_api->open(g_OSPI.p_ctrl, g_OSPI.p_cfg);

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
