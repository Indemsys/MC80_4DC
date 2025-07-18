/*-----------------------------------------------------------------------------------------------------
  Description: LittleFS adapter for OSPI flash memory (MX25UM25645GMI00)
               Integrates LittleFS with Renesas FSP OSPI driver

  Parameters:

  Return:
-----------------------------------------------------------------------------------------------------*/

#include "App.h"
#include "littlefs_adapter.h"
#include "RTT_utils.h"

// Global LittleFS context
T_littlefs_context g_littlefs_context;

// Global flag to track XIP mode status
static bool g_xip_mode_active = false;

// External references to OSPI driver
extern const spi_flash_instance_t g_OSPI;

/*-----------------------------------------------------------------------------------------------------
  Description: Enter XIP mode for direct memory access

  Parameters:

  Return: 0 on success, error code on failure
-----------------------------------------------------------------------------------------------------*/
static int _enter_xip_mode(void)
{
  if (g_xip_mode_active)
  {
    return 0; // Already in XIP mode
  }

  fsp_err_t err = g_OSPI.p_api->xipEnter(g_OSPI.p_ctrl);
  if (FSP_SUCCESS == err)
  {
    g_xip_mode_active = true;
    RTT_printf(0, "XIP mode entered successfully\n");
    return 0;
  }
  else
  {
    RTT_err_printf(0, "Failed to enter XIP mode, error: %d\n", err);
    return -1;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Exit XIP mode for write operations

  Parameters:

  Return: 0 on success, error code on failure
-----------------------------------------------------------------------------------------------------*/
static int _exit_xip_mode(void)
{
  if (!g_xip_mode_active)
  {
    return 0; // Already not in XIP mode
  }

  fsp_err_t err = g_OSPI.p_api->xipExit(g_OSPI.p_ctrl);
  if (FSP_SUCCESS == err)
  {
    g_xip_mode_active = false;
    RTT_printf(0, "XIP mode exited for write operation\n");
    return 0;
  }
  else
  {
    RTT_err_printf(0, "Failed to exit XIP mode, error: %d\n", err);
    return -1;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Wait for flash operation to complete

  Parameters: p_spi_flash - SPI flash instance
              timeout_ms - timeout in milliseconds

  Return: 0 on success, error code on failure
-----------------------------------------------------------------------------------------------------*/
static int _wait_flash_ready(const spi_flash_instance_t *p_spi_flash, uint32_t timeout_ms)
{
  spi_flash_status_t status;
  fsp_err_t err;
  uint32_t wait_count = 0;
  const uint32_t max_wait_count = timeout_ms; // 1ms per iteration

  do
  {
    err = p_spi_flash->p_api->statusGet(p_spi_flash->p_ctrl, &status);
    if (err != FSP_SUCCESS)
    {
      RTT_err_printf(0, "Failed to get flash status during wait: %u\n", (unsigned int)err);
      return -1;
    }

    if (!status.write_in_progress)
    {
      return 0; // Flash is ready
    }

    // Wait 1ms
    R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
    wait_count++;

  } while (wait_count < max_wait_count);

  RTT_err_printf(0, "Timeout waiting for flash ready (waited %u ms)\n", wait_count);
  return -1; // Timeout
}

/*-----------------------------------------------------------------------------------------------------
  Description: Initialize LittleFS configuration

  Parameters:

  Return: 0 on success, error code on failure
-----------------------------------------------------------------------------------------------------*/
int Littlefs_initialize(void)
{
  fsp_err_t err;

  // Check if already initialized
  if (g_littlefs_context.driver_initialized)
  {
    APP_PRINT("OSPI driver already initialized\n\r");
    return 0;
  }

  // Initialize OSPI driver first
  err = g_OSPI.p_api->open(g_OSPI.p_ctrl, g_OSPI.p_cfg);
  if (err != FSP_SUCCESS)
  {
    // Check if driver is already opened
    if (err == FSP_ERR_ALREADY_OPEN)
    {
      APP_PRINT("OSPI driver already opened\n\r");
    }
    else
    {
      RTT_err_printf(0, "OSPI driver open failed: %u\n\r", (unsigned int)err);
      return -1;
    }
  }
  else
  {
    APP_PRINT("OSPI driver initialized successfully\n\r");
  }

  // Check if flash is ready
  spi_flash_status_t flash_status;
  err = g_OSPI.p_api->statusGet(g_OSPI.p_ctrl, &flash_status);
  if (err != FSP_SUCCESS)
  {
    RTT_err_printf(0, "Failed to get OSPI flash status: %u\n\r", (unsigned int)err);
    return -1;
  }

  if (flash_status.write_in_progress)
  {
    APP_ERR_PRINT("OSPI flash is busy (write in progress)\n\r");
    return -1;
  }

  APP_PRINT("OSPI flash is ready for operations\n\r");

  // Zero out the context (except state flags)
  bool driver_was_initialized = g_littlefs_context.driver_initialized;
  bool filesystem_was_mounted = g_littlefs_context.filesystem_mounted;
  memset(&g_littlefs_context, 0, sizeof(g_littlefs_context));

  // Restore state flags if this is a re-initialization
  if (driver_was_initialized)
  {
    g_littlefs_context.driver_initialized = true;
  }
  if (filesystem_was_mounted)
  {
    g_littlefs_context.filesystem_mounted = true;
  }  // Configure LittleFS
  g_littlefs_context.cfg.context = (void *)&g_OSPI;

  // Block device operations
  g_littlefs_context.cfg.read = _lfs_read;
  g_littlefs_context.cfg.prog = _lfs_prog;
  g_littlefs_context.cfg.erase = _lfs_erase;
  g_littlefs_context.cfg.sync = _lfs_sync;

  // Block device configuration
  g_littlefs_context.cfg.read_size = 1;                       // Minimum read size
  g_littlefs_context.cfg.prog_size = 1;                       // Minimum program size (start with 1 for testing)
  g_littlefs_context.cfg.block_size = LITTLEFS_BLOCK_SIZE;    // Block size (4KB)
  g_littlefs_context.cfg.block_count = LITTLEFS_BLOCK_COUNT;  // Number of blocks
  g_littlefs_context.cfg.cache_size = LITTLEFS_CACHE_SIZE;    // Cache size
  g_littlefs_context.cfg.lookahead_size = LITTLEFS_LOOKAHEAD_SIZE; // Lookahead buffer size
  g_littlefs_context.cfg.block_cycles = LITTLEFS_BLOCK_CYCLES; // Block wear leveling threshold

  // Buffers for caching - important for performance
  g_littlefs_context.cfg.read_buffer = g_littlefs_context.read_buffer;
  g_littlefs_context.cfg.prog_buffer = g_littlefs_context.prog_buffer;
  g_littlefs_context.cfg.lookahead_buffer = g_littlefs_context.lookahead_buffer;

  // Mark driver as initialized
  g_littlefs_context.driver_initialized = true;

  // Print configuration for debugging
  RTT_printf(0, "LittleFS config: block_size=%u, block_count=%u, total_size=%u MB\n",
            g_littlefs_context.cfg.block_size,
            g_littlefs_context.cfg.block_count,
            (g_littlefs_context.cfg.block_size * g_littlefs_context.cfg.block_count) / (1024*1024));

  return 0;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Mount the LittleFS filesystem

  Parameters:

  Return: 0 on success, error code on failure
-----------------------------------------------------------------------------------------------------*/
int Littlefs_mount(void)
{
  int err;

  // Check if already mounted
  if (g_littlefs_context.filesystem_mounted)
  {
    APP_PRINT("LittleFS already mounted\n\r");
    return 0;
  }

  err = lfs_mount(&g_littlefs_context.lfs, &g_littlefs_context.cfg);

  if (err != 0)
  {
    RTT_err_printf(0, "LittleFS mount failed with error: %d\n\r", err);
    return err;
  }

  // Mark filesystem as mounted
  g_littlefs_context.filesystem_mounted = true;
  APP_PRINT("LittleFS mounted successfully\n\r");
  return 0;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Format the LittleFS filesystem

  Parameters:

  Return: 0 on success, error code on failure
-----------------------------------------------------------------------------------------------------*/
int Littlefs_format(void)
{
  int err = lfs_format(&g_littlefs_context.lfs, &g_littlefs_context.cfg);

  if (err != 0)
  {
    RTT_err_printf(0, "LittleFS format failed with error: %d\n\r", err);
    return err;
  }

  APP_PRINT("LittleFS formatted successfully\n\r");
  return 0;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Unmount the LittleFS filesystem

  Parameters:

  Return: 0 on success, error code on failure
-----------------------------------------------------------------------------------------------------*/
int Littlefs_unmount(void)
{
  fsp_err_t fsp_err;
  int err;

  // Check if filesystem is mounted
  if (g_littlefs_context.filesystem_mounted)
  {
    // Unmount LittleFS first
    err = lfs_unmount(&g_littlefs_context.lfs);
    if (err != 0)
    {
      APP_ERR_PRINT("LittleFS unmount failed with error: %d\n\r", err);
      return err;
    }
    g_littlefs_context.filesystem_mounted = false;
    APP_PRINT("LittleFS unmounted successfully\n\r");
  }
  else
  {
    APP_PRINT("LittleFS was not mounted\n\r");
  }

  // Check if driver is initialized and close it
  if (g_littlefs_context.driver_initialized)
  {
    fsp_err = g_OSPI.p_api->close(g_OSPI.p_ctrl);
    if (fsp_err != FSP_SUCCESS)
    {
      APP_ERR_PRINT("OSPI driver close failed: %u\n\r", (unsigned int)fsp_err);
      return -1;
    }
    g_littlefs_context.driver_initialized = false;
    APP_PRINT("OSPI driver closed successfully\n\r");
  }
  else
  {
    APP_PRINT("OSPI driver was not initialized\n\r");
  }

  return 0;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Read data from flash using XIP mode (direct memory access)

  Parameters: c - LFS configuration
              block - block number to read from
              off - offset within the block
              buffer - buffer to store read data
              size - number of bytes to read

  Return: 0 on success, error code on failure
-----------------------------------------------------------------------------------------------------*/
int _lfs_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
  // Calculate absolute address
  uint32_t address = (block * c->block_size) + off;

  // Debug info for first few reads
  static int debug_count = 0;
  if (debug_count < 5)
  {
    RTT_printf(0, "LFS XIP read: blk=%u off=%u sz=%u addr=0x%08X\n",
              (unsigned int)block, (unsigned int)off, (unsigned int)size, (unsigned int)address);
    debug_count++;
  }

  // Check buffer validity
  if (buffer == NULL || size == 0)
  {
    RTT_err_printf(0, "Invalid read parameters: buffer=%p size=%u\n", buffer, size);
    return -1;
  }

  // Ensure XIP mode is active
  if (_enter_xip_mode() != 0)
  {
    RTT_err_printf(0, "Failed to enter XIP mode for read operation\n");
    return -1;
  }

  // Calculate XIP address by adding flash offset to XIP base
  uint8_t *xip_address = (uint8_t *)(OSPI_BASE_ADDRESS + address);

  // Perform direct memory copy from XIP space
  memcpy(buffer, xip_address, size);

  // Debug for first read
  if (debug_count <= 1)
  {
    RTT_printf(0, "XIP read success: copied %u bytes from 0x%08X\n",
              (unsigned int)size, (unsigned int)xip_address);
  }

  return 0; // Success
}

/*-----------------------------------------------------------------------------------------------------
  Description: Program (write) data to flash memory through OSPI driver

  Parameters: c - LFS configuration
              block - block number to write to
              off - offset within the block
              buffer - buffer containing data to write
              size - number of bytes to write

  Return: 0 on success, error code on failure
-----------------------------------------------------------------------------------------------------*/
int _lfs_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
  fsp_err_t err;
  const spi_flash_instance_t *p_spi_flash = (const spi_flash_instance_t *)c->context;
  spi_flash_status_t status;

  // Calculate absolute address
  uint32_t address = (block * c->block_size) + off;

  RTT_printf(0, "LFS write: blk=%u off=%u sz=%u addr=0x%08X\n",
            (unsigned int)block, (unsigned int)off, (unsigned int)size, (unsigned int)address);

  // Show first few bytes of data for debugging
  if (size > 0 && buffer != NULL)
  {
    const uint8_t *data = (const uint8_t *)buffer;
    RTT_printf(0, "Write data: %02X %02X %02X %02X...\n",
              data[0],
              size > 1 ? data[1] : 0,
              size > 2 ? data[2] : 0,
              size > 3 ? data[3] : 0);
  }

  // Exit XIP mode before write operation
  if (_exit_xip_mode() != 0)
  {
    RTT_err_printf(0, "Failed to exit XIP mode for write operation\n");
    return -1;
  }

  // Set SPI protocol for write operations
  err = p_spi_flash->p_api->spiProtocolSet(p_spi_flash->p_ctrl, SPI_FLASH_PROTOCOL_EXTENDED_SPI);
  if (err != FSP_SUCCESS)
  {
    RTT_printf(0, "Warning: Failed to set SPI protocol: %u\n", (unsigned int)err);
    // Continue anyway, might not be critical
  }

  // Check flash status before write
  err = p_spi_flash->p_api->statusGet(p_spi_flash->p_ctrl, &status);
  if (err != FSP_SUCCESS)
  {
    RTT_err_printf(0, "Failed to get flash status before write: %u\n", (unsigned int)err);
    return -1;
  }

  if (status.write_in_progress)
  {
    RTT_err_printf(0, "Flash is busy, write in progress\n");
    return -1;
  }

  // Write data using OSPI driver with correct base address
  err = p_spi_flash->p_api->write(p_spi_flash->p_ctrl, (uint8_t *)buffer, (uint8_t *)(OSPI_BASE_ADDRESS + address), size);

  if (err != FSP_SUCCESS)
  {
    RTT_err_printf(0, "OSPI write fail addr=0x%08X size=%u err=%u (0x%X)\n",
                  (unsigned int)address, size, (unsigned int)err, (unsigned int)err);
    return -1; // Return LFS error
  }

  // Wait for write operation to complete
  if (_wait_flash_ready(p_spi_flash, 1000) != 0) // 1 second timeout for write
  {
    RTT_err_printf(0, "Timeout waiting for write completion\n");
    return -1;
  }

  RTT_printf(0, "OSPI write success addr=0x%08X size=%u\n", (unsigned int)address, size);
  return 0; // Success
}

/*-----------------------------------------------------------------------------------------------------
  Description: Erase a block of flash memory through OSPI driver

  Parameters: c - LFS configuration
              block - block number to erase

  Return: 0 on success, error code on failure
-----------------------------------------------------------------------------------------------------*/
int _lfs_erase(const struct lfs_config *c, lfs_block_t block)
{
  fsp_err_t err;
  const spi_flash_instance_t *p_spi_flash = (const spi_flash_instance_t *)c->context;
  spi_flash_status_t status;

  // Calculate absolute address
  uint32_t address = block * c->block_size;

  RTT_printf(0, "LFS erase: blk=%u addr=0x%08X size=%u\n",
            (unsigned int)block, (unsigned int)address, c->block_size);

  // Exit XIP mode before erase operation
  if (_exit_xip_mode() != 0)
  {
    RTT_err_printf(0, "Failed to exit XIP mode for erase operation\n");
    return -1;
  }

  // Set SPI protocol for erase operations
  err = p_spi_flash->p_api->spiProtocolSet(p_spi_flash->p_ctrl, SPI_FLASH_PROTOCOL_EXTENDED_SPI);
  if (err != FSP_SUCCESS)
  {
    RTT_printf(0, "Warning: Failed to set SPI protocol for erase: %u\n", (unsigned int)err);
    // Continue anyway, might not be critical
  }

  // Check flash status before erase
  err = p_spi_flash->p_api->statusGet(p_spi_flash->p_ctrl, &status);
  if (err != FSP_SUCCESS)
  {
    RTT_err_printf(0, "Failed to get flash status before erase: %u\n", (unsigned int)err);
    return -1;
  }

  if (status.write_in_progress)
  {
    RTT_err_printf(0, "Flash is busy, cannot erase\n");
    return -1;
  }

  // Erase block using OSPI driver with correct base address
  err = p_spi_flash->p_api->erase(p_spi_flash->p_ctrl, (uint8_t *)(OSPI_BASE_ADDRESS + address), c->block_size);

  if (err != FSP_SUCCESS)
  {
    RTT_err_printf(0, "OSPI erase fail addr=0x%08X size=%u err=%u\n\r", (unsigned int)address, c->block_size, (unsigned int)err);
    return -1; // Return LFS error
  }

  // Wait for erase operation to complete (erase can take several milliseconds)
  if (_wait_flash_ready(p_spi_flash, 5000) != 0) // 5 second timeout
  {
    RTT_err_printf(0, "Timeout waiting for erase completion\n");
    return -1;
  }

  RTT_printf(0, "OSPI erase success addr=0x%08X size=%u\n", (unsigned int)address, c->block_size);
  return 0; // Success
}

/*-----------------------------------------------------------------------------------------------------
  Description: Synchronize flash memory operations

  Parameters: c - LFS configuration

  Return: 0 on success, error code on failure
-----------------------------------------------------------------------------------------------------*/
int _lfs_sync(const struct lfs_config *c)
{
  // For this implementation, sync is not needed as writes are synchronous
  // But we can add status checking if needed
  (void)c;
  return 0; // Success
}

/*-----------------------------------------------------------------------------------------------------
  Description: Check if LittleFS driver is initialized

  Parameters:

  Return: true if initialized, false otherwise
-----------------------------------------------------------------------------------------------------*/
bool Littlefs_is_initialized(void)
{
  return g_littlefs_context.driver_initialized;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Check if LittleFS filesystem is mounted

  Parameters:

  Return: true if mounted, false otherwise
-----------------------------------------------------------------------------------------------------*/
bool Littlefs_is_mounted(void)
{
  return g_littlefs_context.filesystem_mounted;
}
