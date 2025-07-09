#include "App.h"

#define FLAG_FLASH_OP_DONE             BIT(0)  // Flash operation completion flag
#define FLASH_DRIVER_SWITCH_TIMEOUT_MS 100     // Timeout for flash driver switching operations

static TX_EVENT_FLAGS_GROUP flash_bgo_flags;
static uint32_t             flash_bgo_status;
static T_flash_driver_state g_flash_driver_state = FLASH_DRIVER_NONE;

flash_hp_instance_ctrl_t g_flash_ctrl;

// Flash driver configuration for non-BGO mode
const flash_cfg_t g_flash_cfg =
{
 .data_flash_bgo = false,               // BGO mode disabled
 .p_callback     = NULL,                // No callback for non-BGO
 .p_context      = NULL,                // No user context
 .irq            = FSP_INVALID_VECTOR,  // No IRQ
 .err_irq        = FSP_INVALID_VECTOR,  // No error IRQ
 .err_ipl        = (0xFF),              // Invalid error priority
 .ipl            = (0xFF),              // Invalid priority
};

// Flash driver instance for non-BGO mode
const flash_instance_t g_flash_cbl =
{
 .p_ctrl = &g_flash_ctrl,                   // Pointer to control structure
 .p_cfg  = &g_flash_cfg,                    // Pointer to configuration structure
 .p_api  = &g_flash_on_flash_hp             // Pointer to API structure
};

flash_hp_instance_ctrl_t g_flash_bgo_ctrl;  // Control structure for BGO mode

// Flash driver configuration for BGO mode
const flash_cfg_t g_flash_bgo_cfg =
{
 .data_flash_bgo = true,                      // BGO mode enabled
 .p_callback     = Flash_bgo_callback,        // Callback for BGO events
 .p_context      = NULL,                      // No user context
 .irq            = VECTOR_NUMBER_FCU_FRDYI,   // Ready interrupt vector
 .err_irq        = VECTOR_NUMBER_FCU_FIFERR,  // Error interrupt vector
 .err_ipl        = (5),                       // Error interrupt priority
 .ipl            = (5),                       // Interrupt priority
};

// Flash driver instance for BGO mode
const flash_instance_t g_flash_bgo_cbl =
{
 .p_ctrl = &g_flash_bgo_ctrl,    // Pointer to control structure
 .p_cfg  = &g_flash_bgo_cfg,     // Pointer to configuration structure
 .p_api  = &g_flash_on_flash_hp  // Pointer to API structure
};

/*-----------------------------------------------------------------------------------------------------
  Initializes Flash driver for synchronous write/erase operations without interrupts.

  Parameters:

  Return:
    fsp_err_t - FSP error code
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Flash_driver_init(void)
{
  fsp_err_t result = g_flash_cbl.p_api->open(&g_flash_ctrl, &g_flash_cfg);
  if (result == FSP_SUCCESS)
  {
    g_flash_driver_state = FLASH_DRIVER_NO_BGO;
  }
  return result;
}

/*-----------------------------------------------------------------------------------------------------
  Deinitializes the Flash driver.

  Parameters:

  Return:
    fsp_err_t - FSP error code
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Flash_driver_deinit(void)
{
  fsp_err_t result = g_flash_cbl.p_api->close(&g_flash_ctrl);
  if (result == FSP_SUCCESS)
  {
    g_flash_driver_state = FLASH_DRIVER_NONE;
  }
  return result;
}

/*-----------------------------------------------------------------------------------------------------
  Initializes Flash driver for asynchronous write/erase operations with Background Operation
  using interrupts.

  Parameters:

  Return:
    uint32_t - Operation result code
-----------------------------------------------------------------------------------------------------*/
uint32_t Flash_driver_bgo_init(void)
{
  UINT res;

  // Create synchronization object
  res = tx_event_flags_create(&flash_bgo_flags, "Flash");
  if (res != TX_SUCCESS)
  {
    return RES_ERROR;
  }

  fsp_err_t flash_res = g_flash_bgo_cbl.p_api->open(&g_flash_bgo_ctrl, &g_flash_bgo_cfg);
  if (flash_res == FSP_SUCCESS)
  {
    g_flash_driver_state = FLASH_DRIVER_BGO;
    return RES_OK;
  }
  else
  {
    // Clean up the event flags if flash init failed
    tx_event_flags_delete(&flash_bgo_flags);
    return RES_ERROR;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Deinitializes Flash driver for Background Operation mode.

  Parameters:

  Return:
    uint32_t - Operation result code
-----------------------------------------------------------------------------------------------------*/
uint32_t Flash_driver_bgo_deinit(void)
{
  g_flash_bgo_cbl.p_api->close(&g_flash_bgo_ctrl);
  uint32_t result = tx_event_flags_delete(&flash_bgo_flags);
  if (result == TX_SUCCESS)
  {
    g_flash_driver_state = FLASH_DRIVER_NONE;
  }
  return result;
}

/*-----------------------------------------------------------------------------------------------------
  Flash Background Operation callback function called after DataFlash operation completion.

  Parameters:
    p_args - Pointer to flash callback arguments structure

  Return:
-----------------------------------------------------------------------------------------------------*/
void Flash_bgo_callback(flash_callback_args_t *p_args)
{
  switch (p_args->event)
  {
    case FLASH_EVENT_ERASE_COMPLETE:
      flash_bgo_status |= BIT_FLASH_EVENT_ERASE_COMPLETE;
      break;
    case FLASH_EVENT_WRITE_COMPLETE:
      flash_bgo_status |= BIT_FLASH_EVENT_WRITE_COMPLETE;
      break;
    case FLASH_EVENT_BLANK:
      flash_bgo_status |= BIT_FLASH_EVENT_BLANK;
      break;
    case FLASH_EVENT_NOT_BLANK:
      flash_bgo_status |= BIT_FLASH_EVENT_NOT_BLANK;
      break;
    case FLASH_EVENT_ERR_DF_ACCESS:
      flash_bgo_status |= BIT_FLASH_EVENT_ERR_DF_ACCESS;
      break;
    case FLASH_EVENT_ERR_CF_ACCESS:
      flash_bgo_status |= BIT_FLASH_EVENT_ERR_CF_ACCESS;
      break;
    case FLASH_EVENT_ERR_CMD_LOCKED:
      flash_bgo_status |= BIT_FLASH_EVENT_ERR_CMD_LOCKED;
      break;
    case FLASH_EVENT_ERR_FAILURE:
      flash_bgo_status |= BIT_FLASH_EVENT_ERR_FAILURE;
      break;
    case FLASH_EVENT_ERR_ONE_BIT:
      flash_bgo_status |= BIT_FLASH_EVENT_ERR_ONE_BIT;
      break;
  }
  tx_event_flags_set(&flash_bgo_flags, FLAG_FLASH_OP_DONE, TX_OR);
}

/*-----------------------------------------------------------------------------------------------------
  Waits for Background Operation to complete with specified timeout.

  Parameters:
    wait_option - Wait timeout option

  Return:
    uint32_t - TX_SUCCESS if flag was received successfully
-----------------------------------------------------------------------------------------------------*/
uint32_t Wait_bgo_end(ULONG wait_option)
{
  ULONG actual_flags = 0;
  return tx_event_flags_get(&flash_bgo_flags, FLAG_FLASH_OP_DONE, TX_OR_CLEAR, &actual_flags, wait_option);
}

/*-----------------------------------------------------------------------------------------------------
  Gets current Background Operation status.

  Parameters:

  Return:
    uint32_t - Current BGO status value
-----------------------------------------------------------------------------------------------------*/
uint32_t Get_bgo_status(void)
{
  return flash_bgo_status;
}

/*-----------------------------------------------------------------------------------------------------
  Sets Background Operation status value.

  Parameters:
    stat - Status value to set

  Return:
-----------------------------------------------------------------------------------------------------*/
void Set_bgo_status(uint32_t stat)
{
  flash_bgo_status = stat;
}

/*-----------------------------------------------------------------------------------------------------
  Gets current Flash driver state.

  Parameters:

  Return:
    T_flash_driver_state - Current driver state (NONE, NO_BGO, or BGO)
-----------------------------------------------------------------------------------------------------*/
T_flash_driver_state Get_flash_driver_state(void)
{
  return g_flash_driver_state;
}

/*-----------------------------------------------------------------------------------------------------
  Erases DataFlash area using asynchronous Background Operation.

  Parameters:
    start_addr - Start address for erase operation
    area_size - Size of area to erase

  Return:
    int - Returns RES_OK if erase operation completed successfully
-----------------------------------------------------------------------------------------------------*/
uint32_t DataFlash_bgo_EraseArea(uint32_t start_addr, uint32_t area_size)
{
  fsp_err_t res;
  UINT      os_res;
  uint32_t  addr;
  uint32_t  num_blocks;

  // Automatically switch to BGO driver if needed
  if (g_flash_driver_state != FLASH_DRIVER_BGO)
  {
    uint32_t switch_res = Switch_Flash_driver_to_bgo();
    if (switch_res != RES_OK)
    {
      return RES_ERROR;  // Failed to switch to BGO driver
    }
  }

  addr       = start_addr & 0xFFFFFFC0;  // Align address to 64-byte boundary
  num_blocks = (((start_addr + area_size - 1) & 0xFFFFFFC0) + 0x40 - addr) / 0x40;

  Set_bgo_status(0);
  res = g_flash_bgo_cbl.p_api->erase(g_flash_bgo_cbl.p_ctrl, addr, num_blocks);
  if (res != FSP_SUCCESS) return RES_ERROR;
  os_res = Wait_bgo_end(ms_to_ticks(FLASH_DRIVER_SWITCH_TIMEOUT_MS));
  if (os_res != TX_SUCCESS) return RES_ERROR;
  if ((Get_bgo_status() & BIT_FLASH_EVENT_ERASE_COMPLETE) == 0) return RES_ERROR;
  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Writes data to DataFlash using asynchronous Background Operation.

  Parameters:
    start_addr - Address must be aligned to DATA_FLASH_WR_SZ (4 bytes) boundary
    buf - Data buffer to write
    buf_size - Buffer size must be multiple of DATA_FLASH_WR_SZ (4 bytes)

  Return:
    uint32_t - Operation result code
-----------------------------------------------------------------------------------------------------*/
uint32_t DataFlash_bgo_WriteArea(uint32_t start_addr, uint8_t *buf, uint32_t buf_size)
{
  fsp_err_t res;
  UINT      os_res;

  // Automatically switch to BGO driver if needed
  if (g_flash_driver_state != FLASH_DRIVER_BGO)
  {
    uint32_t switch_res = Switch_Flash_driver_to_bgo();
    if (switch_res != RES_OK)
    {
      return RES_ERROR;  // Failed to switch to BGO driver
    }
  }

  // Check address alignment and size divisibility
  if (((start_addr % DATA_FLASH_WR_SZ) != 0) || ((buf_size % DATA_FLASH_WR_SZ) != 0) || (buf_size == 0))
  {
    return RES_ERROR;  // Or more specific error code FSP_ERR_INVALID_ALIGNMENT / FSP_ERR_INVALID_SIZE
  }

  Set_bgo_status(0);
  res    = g_flash_bgo_cbl.p_api->write(g_flash_bgo_cbl.p_ctrl, (uint32_t)buf, start_addr, buf_size);  // Do not check res immediately as operation is asynchronous
  os_res = Wait_bgo_end(ms_to_ticks(FLASH_DRIVER_SWITCH_TIMEOUT_MS));                                  // Use timeout defined by macro
  if (os_res != TX_SUCCESS) return RES_ERROR;                                                          // Wait error (timeout)
  // Check FSP status after BGO completion
  if (res != FSP_SUCCESS) return RES_ERROR;                                        // FSP operation start error
  if ((Get_bgo_status() & BIT_FLASH_EVENT_WRITE_COMPLETE) == 0) return RES_ERROR;  // Operation completed with error
  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Reads data from DataFlash area.

  Parameters:
    start_addr - Start address to read from
    buf - Buffer to store read data
    buf_size - Size of data to read

  Return:
    uint32_t - Operation result code
-----------------------------------------------------------------------------------------------------*/
uint32_t DataFlash_bgo_ReadArea(uint32_t start_addr, uint8_t *buf, uint32_t buf_size)
{
  for (uint32_t i = 0; i < buf_size; i++)
  {
    buf[i] = *((volatile uint8_t *)(start_addr + i));
  }
  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Performs blank check on DataFlash area. Cannot simply compare with 0xFF in Data Flash
  to determine blank state, as blank cells return random values. Must use API.

  Parameters:
    start_addr - Start address for blank check
    num_bytes - Number of bytes to check

  Return:
    uint32_t - Operation result code
-----------------------------------------------------------------------------------------------------*/
uint32_t DataFlash_bgo_BlankCheck(uint32_t start_addr, uint32_t num_bytes)
{
  fsp_err_t      res;
  UINT           os_res;
  flash_result_t blank_check_result;

  // Automatically switch to BGO driver if needed
  if (g_flash_driver_state != FLASH_DRIVER_BGO)
  {
    uint32_t switch_res = Switch_Flash_driver_to_bgo();
    if (switch_res != RES_OK)
    {
      return RES_ERROR;  // Failed to switch to BGO driver
    }
  }

  Set_bgo_status(0);
  res    = g_flash_bgo_cbl.p_api->blankCheck(g_flash_bgo_cbl.p_ctrl, start_addr, num_bytes, &blank_check_result);
  os_res = Wait_bgo_end(ms_to_ticks(FLASH_DRIVER_SWITCH_TIMEOUT_MS));
  if (os_res != TX_SUCCESS) return RES_ERROR;
  if (res != FSP_SUCCESS) return RES_ERROR;
  if ((Get_bgo_status() & BIT_FLASH_EVENT_BLANK) != 0) return RES_OK;
  return RES_ERROR;
}

/*-----------------------------------------------------------------------------------------------------
  Sets Flash protection using ID code.

  Parameters:
    p_id_bytes - Pointer to ID bytes array
    mode - Flash ID code mode

  Return:
    fsp_err_t - FSP error code
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Set_Flash_protection(uint8_t const *const p_id_bytes, flash_id_code_mode_t mode)
{
  return g_flash_bgo_cbl.p_api->idCodeSet(g_flash_bgo_cbl.p_ctrl, p_id_bytes, mode);
}

/*-----------------------------------------------------------------------------------------------------
  Switches Flash driver from BGO mode to non-BGO (synchronous) mode.

  Parameters:

  Return:
    uint32_t - RES_OK if switch is successful, RES_ERROR otherwise
-----------------------------------------------------------------------------------------------------*/
uint32_t Switch_Flash_driver_to_no_bgo(void)
{
  fsp_err_t      res;
  flash_status_t status;

  // If already in non-BGO mode, return success
  if (g_flash_driver_state == FLASH_DRIVER_NO_BGO)
  {
    return RES_OK;
  }

  // If no driver is initialized, initialize non-BGO driver
  if (g_flash_driver_state == FLASH_DRIVER_NONE)
  {
    fsp_err_t init_res = Flash_driver_init();
    if (init_res == FSP_SUCCESS)
    {
      return RES_OK;
    }
    else
    {
      return RES_ERROR;
    }
  }

  // Wait for BGO driver to become idle before switching
  res = g_flash_bgo_cbl.p_api->statusGet(g_flash_bgo_cbl.p_ctrl, &status);
  if (res == FSP_SUCCESS && status == FLASH_STATUS_BUSY)
  {
    if (Wait_bgo_end(ms_to_ticks(FLASH_DRIVER_SWITCH_TIMEOUT_MS)) != TX_SUCCESS)
    {
      return RES_ERROR;  // Failed to wait for BGO operation completion
    }
  }

  // Deinitialize BGO driver
  uint32_t deinit_res = Flash_driver_bgo_deinit();
  if (deinit_res != RES_OK)
  {
    return RES_ERROR;
  }

  // Initialize non-BGO driver
  fsp_err_t init_res = Flash_driver_init();
  if (init_res != FSP_SUCCESS)
  {
    g_flash_driver_state = FLASH_DRIVER_NONE;
    return RES_ERROR;
  }
  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Switches the Flash driver from non-BGO mode to BGO (Background Operation) mode.
  This involves deinitializing the current non-BGO driver and initializing the BGO driver.

  Parameters:

  Return:
    uint32_t - RES_OK if switch is successful, RES_ERROR otherwise
-----------------------------------------------------------------------------------------------------*/
uint32_t Switch_Flash_driver_to_bgo(void)
{
  fsp_err_t      res;
  flash_status_t status;
  uint32_t       wait_time    = 0;
  const uint32_t wait_step_ms = 10;

  // If already in BGO mode, return success
  if (g_flash_driver_state == FLASH_DRIVER_BGO)
  {
    return RES_OK;
  }

  // If no driver is initialized, initialize BGO driver
  if (g_flash_driver_state == FLASH_DRIVER_NONE)
  {
    uint32_t init_res = Flash_driver_bgo_init();
    return init_res;
  }

  // Wait for non-BGO driver to become idle before switching (loop with pause)
  while (1)
  {
    res = g_flash_cbl.p_api->statusGet(g_flash_cbl.p_ctrl, &status);
    if (res != FSP_SUCCESS || status != FLASH_STATUS_BUSY)
    {
      break;  // Not busy or status error
    }
    if (wait_time >= FLASH_DRIVER_SWITCH_TIMEOUT_MS)
    {
      return RES_ERROR;  // Timeout
    }
    tx_thread_sleep(ms_to_ticks(wait_step_ms));
    wait_time += wait_step_ms;
  }

  // Deinitialize non-BGO driver
  fsp_err_t deinit_res = Flash_driver_deinit();
  if (deinit_res != FSP_SUCCESS)
  {
    return RES_ERROR;
  }

  // Initialize BGO driver
  uint32_t init_res = Flash_driver_bgo_init();
  if (init_res != RES_OK)
  {
    g_flash_driver_state = FLASH_DRIVER_NONE;
    return RES_ERROR;
  }
  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Performs blank check on Flash memory area.

  Parameters:
    address - Start address for blank check
    num_bytes - Number of bytes to check
    p_blank_check_result - Pointer to store blank check result

  Return:
    fsp_err_t - FSP error code
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Flash_blank_check(uint32_t const address, uint32_t const num_bytes, flash_result_t *const p_blank_check_result)
{
  // Automatically switch to non-BGO driver if needed
  if (g_flash_driver_state != FLASH_DRIVER_NO_BGO)
  {
    uint32_t switch_res = Switch_Flash_driver_to_no_bgo();
    if (switch_res != RES_OK)
    {
      return FSP_ERR_NOT_OPEN;  // Failed to switch to non-BGO driver
    }
  }

  return g_flash_cbl.p_api->blankCheck(g_flash_cbl.p_ctrl, address, num_bytes, p_blank_check_result);
}

/*-----------------------------------------------------------------------------------------------------
  Erases Flash memory blocks.

  Parameters:
    address - Start address for erase operation
    num_blocks - Number of blocks to erase

  Return:
    fsp_err_t - FSP error code
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Flash_erase_block(uint32_t const address, uint32_t const num_blocks)
{
  // Automatically switch to non-BGO driver if needed
  if (g_flash_driver_state != FLASH_DRIVER_NO_BGO)
  {
    uint32_t switch_res = Switch_Flash_driver_to_no_bgo();
    if (switch_res != RES_OK)
    {
      return FSP_ERR_NOT_OPEN;  // Failed to switch to non-BGO driver
    }
  }

  return g_flash_cbl.p_api->erase(g_flash_cbl.p_ctrl, address, num_blocks);
}

/*-----------------------------------------------------------------------------------------------------
  Writes data to Flash memory block with address and size validation.

  Parameters:
    src_address - Source address containing data to write
    flash_address - Destination address in Flash memory
    num_bytes - Number of bytes to write

  Return:
    fsp_err_t - FSP error code
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Flash_write_block(uint32_t const src_address, uint32_t const flash_address, uint32_t const num_bytes)
{
  uint32_t write_granularity;
  uint32_t end_address = flash_address + num_bytes;  // Calculate write end address

  // Automatically switch to non-BGO driver if needed
  if (g_flash_driver_state != FLASH_DRIVER_NO_BGO)
  {
    uint32_t switch_res = Switch_Flash_driver_to_no_bgo();
    if (switch_res != RES_OK)
    {
      return FSP_ERR_NOT_OPEN;  // Failed to switch to non-BGO driver
    }
  }

  // Determine write granularity and check boundaries depending on memory region
  if ((flash_address >= DATA_FLASH_START) && (flash_address < (DATA_FLASH_START + DATA_FLASH_SIZE)))
  {
    write_granularity = DATA_FLASH_WR_SZ;
    // Check Data Flash boundary overflow
    if (end_address > (DATA_FLASH_START + DATA_FLASH_SIZE))
    {
      return FSP_ERR_INVALID_ADDRESS;                                                 // Write goes beyond Data Flash bounds
    }
  }
  else if ((flash_address >= CODE_FLASH_START) && (flash_address <= CODE_FLASH_END))  // Use <= for CODE_FLASH_END
  {
    write_granularity = CODE_FLASH_WR_SZ;
    // Check Code Flash boundary overflow
    // Consider that CODE_FLASH_END is the last valid address
    if (end_address > (CODE_FLASH_END + 1))
    {
      return FSP_ERR_INVALID_ADDRESS;  // Write goes beyond Code Flash bounds
    }
  }
  else
  {
    return FSP_ERR_INVALID_ADDRESS;  // Address outside valid ranges
  }

  // Check address alignment and size divisibility
  if (((flash_address % write_granularity) != 0) || ((num_bytes % write_granularity) != 0) || (num_bytes == 0))
  {
    return FSP_ERR_INVALID_ALIGNMENT;  // Or FSP_ERR_INVALID_SIZE
  }

  return g_flash_cbl.p_api->write(g_flash_cbl.p_ctrl, (uint32_t)src_address, flash_address, num_bytes);
}
