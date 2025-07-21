#include "App.h"


// === LevelX NOR SPI Configuration ===

// LevelX NOR SPI instance control structure
rm_levelx_nor_spi_instance_ctrl_t g_rm_levelx_nor_OSPI_ctrl;

// LevelX NOR SPI configuration
#define RA_NOT_DEFINED 0xFFFFFFFF
rm_levelx_nor_spi_cfg_t g_rm_levelx_nor_OSPI_cfg = {
#if (RA_NOT_DEFINED != RA_NOT_DEFINED)
  .p_lower_lvl  = &RA_NOT_DEFINED,
  .base_address = BSP_FEATURE_QSPI_DEVICE_START_ADDRESS,
#elif (RA_NOT_DEFINED != RA_NOT_DEFINED)
  .p_lower_lvl  = &RA_NOT_DEFINED,
  .base_address = BSP_FEATURE_OSPI_DEVICE_RA_NOT_DEFINED_START_ADDRESS,
#else
  .p_lower_lvl  = NULL,                                    // Reference to OSPI driver instance
  .base_address = BSP_FEATURE_OSPI_B_DEVICE_0_START_ADDRESS,  // OSPI memory-mapped base address
#endif
  .address_offset    = 0,                                // Address offset within flash memory
  .size              = 33554432,                         // Flash size: 32MB (MX25UM25645G)
  .poll_status_count = 0xFFFFFFFF,                       // Maximum polling count for status check
  .p_context         = &g_rm_filex_levelx_NOR_ctrl,      // FileX context
  .p_callback        = rm_filex_levelx_nor_spi_callback  // Callback function
};
#undef RA_NOT_DEFINED

// LevelX read buffer (used only when LX_DIRECT_READ is disabled)
#ifndef LX_DIRECT_READ
  #define FSP_LX_READ_BUFFER_SIZE_WORDS (128U)
ULONG g_rm_levelx_nor_OSPI_read_buffer[FSP_LX_READ_BUFFER_SIZE_WORDS] = { 0 };
#endif

// === LevelX NOR System Error Handling ===

/*-----------------------------------------------------------------------------------------------------
  Description: Weak system error callback for LevelX NOR operations
               This function is called when a critical error occurs in LevelX operations
               Override this function to implement custom error handling

  Parameters: error_code - error code indicating the type of error that occurred

  Return: LX_ERROR - indicates error condition to LevelX
-----------------------------------------------------------------------------------------------------*/
#if defined(__ICCARM__)
  #define g_rm_levelx_nor_OSPI_system_error_WEAK_ATTRIBUTE
  #pragma weak g_rm_levelx_nor_OSPI_system_error = g_rm_levelx_nor_OSPI_system_error_internal
#elif defined(__GNUC__)
  #define g_rm_levelx_nor_OSPI_system_error_WEAK_ATTRIBUTE \
    __attribute__((weak, alias("g_rm_levelx_nor_OSPI_system_error_internal")))
#endif

UINT g_rm_levelx_nor_OSPI_system_error(UINT error_code) g_rm_levelx_nor_OSPI_system_error_WEAK_ATTRIBUTE;

/*-----------------------------------------------------------------------------------------------------
  Description: Internal system error handler for LevelX NOR operations
               Default implementation triggers unrecoverable error handler

  Parameters: error_code - error code indicating the type of error that occurred

  Return: LX_ERROR - indicates error condition to LevelX
-----------------------------------------------------------------------------------------------------*/
static UINT g_rm_levelx_nor_OSPI_system_error_internal(UINT error_code);
static UINT g_rm_levelx_nor_OSPI_system_error_internal(UINT error_code)
{
  FSP_PARAMETER_NOT_USED(error_code);

  // Trigger unrecoverable error handler - system will halt
  BSP_CFG_HANDLE_UNRECOVERABLE_ERROR(0);

  return LX_ERROR;
}

// === LevelX NOR Driver Service Functions ===

/*-----------------------------------------------------------------------------------------------------
  Description: LevelX NOR "Read Sector" service function
               Reads data from NOR flash memory through RM_LEVELX_NOR_SPI driver

  Parameters: flash_address - pointer to flash memory address to read from
              destination   - pointer to destination buffer for read data
              words         - number of 32-bit words to read

  Return: LX_SUCCESS on success, LX_ERROR on failure
-----------------------------------------------------------------------------------------------------*/
static UINT g_rm_levelx_nor_OSPI_read(ULONG *flash_address, ULONG *destination, ULONG words);
static UINT g_rm_levelx_nor_OSPI_read(ULONG *flash_address, ULONG *destination, ULONG words)
{
  fsp_err_t err;

  err = RM_LEVELX_NOR_SPI_Read(&g_rm_levelx_nor_OSPI_ctrl, flash_address, destination, words);
  if (FSP_SUCCESS != err)
  {
    return LX_ERROR;
  }

  return LX_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Description: LevelX NOR "Write Sector" service function
               Writes data to NOR flash memory through RM_LEVELX_NOR_SPI driver

  Parameters: flash_address - pointer to flash memory address to write to
              source        - pointer to source buffer containing data to write
              words         - number of 32-bit words to write

  Return: LX_SUCCESS on success, LX_ERROR on failure
-----------------------------------------------------------------------------------------------------*/
static UINT g_rm_levelx_nor_OSPI_write(ULONG *flash_address, ULONG *source, ULONG words);
static UINT g_rm_levelx_nor_OSPI_write(ULONG *flash_address, ULONG *source, ULONG words)
{
  fsp_err_t err;

  err = RM_LEVELX_NOR_SPI_Write(&g_rm_levelx_nor_OSPI_ctrl, flash_address, source, words);
  if (FSP_SUCCESS != err)
  {
    return LX_ERROR;
  }

  return LX_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Description: LevelX NOR "Block Erase" service function
               Erases a block of NOR flash memory through RM_LEVELX_NOR_SPI driver

  Parameters: block             - block number to erase
              block_erase_count - erase count for wear leveling

  Return: LX_SUCCESS on success, LX_ERROR on failure
-----------------------------------------------------------------------------------------------------*/
static UINT g_rm_levelx_nor_OSPI_block_erase(ULONG block, ULONG block_erase_count);
static UINT g_rm_levelx_nor_OSPI_block_erase(ULONG block, ULONG block_erase_count)
{
  fsp_err_t err;

  err = RM_LEVELX_NOR_SPI_BlockErase(&g_rm_levelx_nor_OSPI_ctrl, block, block_erase_count);
  if (FSP_SUCCESS != err)
  {
    return LX_ERROR;
  }

  return LX_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Description: LevelX NOR "Block Erased Verify" service function
               Verifies that a block has been properly erased (all bits set to 1)

  Parameters: block - block number to verify

  Return: LX_SUCCESS if block is erased, LX_ERROR if not erased or on failure
-----------------------------------------------------------------------------------------------------*/
static UINT g_rm_levelx_nor_OSPI_block_erased_verify(ULONG block);
static UINT g_rm_levelx_nor_OSPI_block_erased_verify(ULONG block)
{
  fsp_err_t err;

  err = RM_LEVELX_NOR_SPI_BlockErasedVerify(&g_rm_levelx_nor_OSPI_ctrl, block);
  if (FSP_SUCCESS != err)
  {
    return LX_ERROR;
  }

  return LX_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Description: LevelX NOR "Driver Initialization" service function
               Initializes the LevelX NOR driver and sets up function pointers

  Parameters: p_nor_flash - pointer to LX_NOR_FLASH structure to initialize

  Return: LX_SUCCESS on successful initialization, LX_ERROR on failure
-----------------------------------------------------------------------------------------------------*/
UINT g_rm_levelx_nor_OSPI_initialize(LX_NOR_FLASH *p_nor_flash)
{
  fsp_err_t err;

  // Set LevelX NOR flash structure pointer in configuration
  g_rm_levelx_nor_OSPI_cfg.p_lx_nor_flash = p_nor_flash;

  // Open the RM_LEVELX_NOR_SPI driver
  err                                     = RM_LEVELX_NOR_SPI_Open(&g_rm_levelx_nor_OSPI_ctrl, &g_rm_levelx_nor_OSPI_cfg);
  if (FSP_SUCCESS != err)
  {
    return LX_ERROR;
  }

#ifndef LX_DIRECT_READ
  // Set sector buffer for LevelX (used only when LX_DIRECT_READ is disabled)
  p_nor_flash->lx_nor_flash_sector_buffer = g_rm_levelx_nor_OSPI_read_buffer;
#endif

  // Set LevelX driver function pointers
  p_nor_flash->lx_nor_flash_driver_read                = g_rm_levelx_nor_OSPI_read;
  p_nor_flash->lx_nor_flash_driver_write               = g_rm_levelx_nor_OSPI_write;
  p_nor_flash->lx_nor_flash_driver_block_erase         = g_rm_levelx_nor_OSPI_block_erase;
  p_nor_flash->lx_nor_flash_driver_block_erased_verify = g_rm_levelx_nor_OSPI_block_erased_verify;
  p_nor_flash->lx_nor_flash_driver_system_error        = g_rm_levelx_nor_OSPI_system_error;

  return LX_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Description: LevelX NOR "Driver Close" service function
               Closes the LevelX NOR driver and releases resources

  Parameters: None

  Return: FSP_SUCCESS on successful close, error code on failure
-----------------------------------------------------------------------------------------------------*/
fsp_err_t g_rm_levelx_nor_OSPI_close(void)
{
  return RM_LEVELX_NOR_SPI_Close(&g_rm_levelx_nor_OSPI_ctrl);
}

// === LevelX NOR Flash Instance ===

// LevelX NOR flash structure
LX_NOR_FLASH g_lx_NOR;

// FileX LevelX NOR instance control structure
rm_filex_levelx_nor_instance_ctrl_t g_rm_filex_levelx_NOR_ctrl;

// FileX LevelX NOR configuration
const rm_filex_levelx_nor_cfg_t g_rm_filex_levelx_NOR_cfg = {
  .close                 = g_rm_levelx_nor_OSPI_close,       // Driver close function
  .nor_driver_initialize = g_rm_levelx_nor_OSPI_initialize,  // Driver initialization function
  .p_nor_flash           = &g_lx_NOR,                        // LevelX NOR flash instance
  .p_nor_flash_name      = "g_rm_filex_levelx_NOR",          // FileX media name
  .p_callback            = g_rm_filex_levelx_NOR_callback,   // FileX callback function
  .p_context             = NULL                              // User context (not used)
};

// FileX LevelX NOR instance
const rm_filex_levelx_nor_instance_t g_rm_filex_levelx_NOR_instance = {
  .p_ctrl = &g_rm_filex_levelx_NOR_ctrl,  // Control structure
  .p_cfg  = &g_rm_filex_levelx_NOR_cfg    // Configuration structure
};
