/*-----------------------------------------------------------------------------------------------------
  MC80 OSPI driver implementation - refactored from Renesas FSP r_ospi_b driver
-----------------------------------------------------------------------------------------------------*/

#include "App.h"
#include "MC80_OSPI_drv.h"
#include "RA8M1_OSPI.h"  // For register definitions only

/*-----------------------------------------------------------------------------------------------------
  Static variables for DMA callback support
-----------------------------------------------------------------------------------------------------*/
// RTOS event flags for DMA notifications
static TX_EVENT_FLAGS_GROUP g_ospi_dma_event_flags;
static bool g_ospi_dma_event_flags_initialized = false;

// DMA event flag definitions
#define OSPI_DMA_EVENT_TRANSFER_COMPLETE  (0x00000001UL)
#define OSPI_DMA_EVENT_TRANSFER_ERROR     (0x00000002UL)
#define OSPI_DMA_EVENT_ALL_EVENTS         (OSPI_DMA_EVENT_TRANSFER_COMPLETE | OSPI_DMA_EVENT_TRANSFER_ERROR)

/*-----------------------------------------------------------------------------------------------------
  Macro definitions
-----------------------------------------------------------------------------------------------------*/

// "MC80" in ASCII. Used to determine if the control block is open
#define MC80_OSPI_PRV_OPEN                               (0x4D433830U)  // "MC80"

#define MC80_OSPI_PRV_CHANNELS_PER_UNIT                  (2U)
#define MC80_OSPI_PRV_UNIT_CHANNELS_SHIFT                (MC80_OSPI_PRV_CHANNELS_PER_UNIT)
#define MC80_OSPI_PRV_UNIT_CHANNELS_MASK                 ((1U << MC80_OSPI_PRV_UNIT_CHANNELS_SHIFT) - 1U)

// Mask of all channels for a given OSPI unit
#define MC80_OSPI_PRV_UNIT_MASK(p_ext_cfg)               (MC80_OSPI_PRV_UNIT_CHANNELS_MASK << (((p_ext_cfg)->ospi_unit) * MC80_OSPI_PRV_UNIT_CHANNELS_SHIFT))

// Individual bit mask for a single channel on a given OSPI unit
#define MC80_OSPI_PRV_CH_MASK(p_ext_cfg)                 ((1U << ((p_ext_cfg)->channel)) << (((p_ext_cfg)->ospi_unit) * MC80_OSPI_PRV_UNIT_CHANNELS_SHIFT))

// Indicates the provided protocol mode requires the Data-Strobe signal
#define MC80_OSPI_PRV_PROTOCOL_USES_DS_SIGNAL(protocol)  ((bool)(((uint32_t)(protocol)) & 0x200UL))

// Number of bytes combined into a single transaction for memory-mapped writes
#define MC80_OSPI_PRV_COMBINATION_WRITE_LENGTH           (2U * ((uint8_t)MC80_OSPI_CFG_COMBINATION_FUNCTION + 1U))

#define MC80_OSPI_PRV_BMCTL_DEFAULT_VALUE                (0x0C)

#define MC80_OSPI_PRV_CMCFG_1BYTE_VALUE_MASK             (0xFF00U)
#define MC80_OSPI_PRV_CMCFG_2BYTE_VALUE_MASK             (0xFFFFU)

#define MC80_OSPI_PRV_AUTOCALIBRATION_DATA_SIZE          (0xFU)
#define MC80_OSPI_PRV_AUTOCALIBRATION_LATENCY_CYCLES     (0U)

#define MC80_OSPI_PRV_ADDRESS_REPLACE_VALUE              (0xF0U)
#define MC80_OSPI_PRV_ADDRESS_REPLACE_ENABLE_BITS        (MC80_OSPI_PRV_ADDRESS_REPLACE_VALUE << OSPI_CMCFG0CSN_ADDRPEN_Pos)
#define MC80_OSPI_PRV_ADDRESS_REPLACE_MASK               (~(MC80_OSPI_PRV_ADDRESS_REPLACE_VALUE << 24))

#define MC80_OSPI_PRV_WORD_ACCESS_SIZE                   (4U)
#define MC80_OSPI_PRV_HALF_WORD_ACCESS_SIZE              (2U)

#define MC80_OSPI_PRV_DIRECT_ADDR_AND_DATA_MASK          (7U)
#define MC80_OSPI_PRV_PAGE_SIZE_BYTES                    (256U)

#define MC80_OSPI_PRV_DIRECT_CMD_SIZE_MASK               (0x3U)

#define MC80_OSPI_PRV_CDTBUF_CMD_OFFSET                  (16U)
#define MC80_OSPI_PRV_CDTBUF_CMD_UPPER_OFFSET            (24U)
#define MC80_OSPI_PRV_CDTBUF_CMD_1B_VALUE_MASK           (0xFFU)
#define MC80_OSPI_PRV_CDTBUF_CMD_1B_VALUE_SHIFT          (8U)
#define MC80_OSPI_PRV_CDTBUF_CMD_2B_VALUE_MASK           (0xFFFFU)

// BMCTL0 register values - bits 4-7 must always be set to 1 per documentation
#define MC80_OSPI_PRV_BMCTL0_RESERVED_BITS               (0xF0)  // Bits 4-7 must be 1
#define MC80_OSPI_PRV_BMCTL0_DISABLED_VALUE              (0xF0)  // 0b1111'0000 - Reserved bits + disabled
#define MC80_OSPI_PRV_BMCTL0_READ_ONLY_VALUE             (0xF5)  // 0b1111'0101 - Reserved bits + read-only
#define MC80_OSPI_PRV_BMCTL0_WRITE_ONLY_VALUE            (0xFA)  // 0b1111'1010 - Reserved bits + write-only
#define MC80_OSPI_PRV_BMCTL0_READ_WRITE_VALUE            (0xFF)  // 0b1111'1111 - Reserved bits + read/write

#define MC80_OSPI_PRV_BMCTL1_CLEAR_PREFETCH_MASK         (0x03 << OSPI_BMCTL1_PBUFCLRCH0_Pos)
#define MC80_OSPI_PRV_BMCTL1_PUSH_COMBINATION_WRITE_MASK (0x03 << OSPI_BMCTL1_MWRPUSHCH0_Pos)

#define MC80_OSPI_PRV_COMSTT_MEMACCCH_MASK               (0x03 << OSPI_COMSTT_MEMACCCH0_Pos)

#define MC80_OSPI_SOFTWARE_DELAY                         (50U)

// These are used as modulus checking, make sure they are powers of 2
#define MC80_OSPI_PRV_CPU_ACCESS_LENGTH                  (8U)
#define MC80_OSPI_PRV_CPU_ACCESS_ALIGNMENT               (8U)

#define MC80_OSPI_PRV_PROTOCOL_USES_DS_MASK              (0x200U)

#define MC80_OSPI_PRV_UINT32_BITS                        (32)

#define MC80_OSPI_MAX_WRITE_ENABLE_LOOPS                 (5)

// Number of address bytes in 4 byte address mode
#define MC80_OSPI_4_BYTE_ADDRESS                         (4U)

/*-----------------------------------------------------------------------------------------------------
  Static function prototypes
-----------------------------------------------------------------------------------------------------*/
static bool                                _Mc80_ospi_status_sub(T_mc80_ospi_instance_ctrl *p_ctrl, uint8_t bit_pos);
static fsp_err_t                           _Mc80_ospi_protocol_specific_settings(T_mc80_ospi_instance_ctrl *p_ctrl);
static fsp_err_t                           _Mc80_ospi_write_enable(T_mc80_ospi_instance_ctrl *p_ctrl);
static void                                _Mc80_ospi_direct_transfer(T_mc80_ospi_instance_ctrl *p_ctrl, T_mc80_ospi_direct_transfer *const p_transfer, T_mc80_ospi_direct_transfer_dir direction);
static T_mc80_ospi_xspi_command_set const *_Mc80_ospi_command_set_get(T_mc80_ospi_instance_ctrl *p_ctrl);
static void _Mc80_ospi_xip(T_mc80_ospi_instance_ctrl *p_ctrl, bool is_entering);
static void _Ospi_dma_callback(dmac_callback_args_t *p_args);
static fsp_err_t _Ospi_dma_event_flags_initialize(void);
static void _Ospi_dma_event_flags_cleanup(void);

/*-----------------------------------------------------------------------------------------------------
  Private global variables
-----------------------------------------------------------------------------------------------------*/

// Bit-flags specifying which channels are open so the module can be stopped when all are closed
static uint32_t g_mc80_ospi_channels_open_flags = 0;

/*-----------------------------------------------------------------------------------------------------
  Open the xSPI device. After the driver is open, the xSPI device can be accessed like internal flash memory.

  Parameters:
    p_ctrl - Pointer to the control structure
    p_cfg  - Pointer to the configuration structure

  Return:
    FSP_SUCCESS              - Configuration was successful
    FSP_ERR_ASSERTION        - The parameter p_ctrl or p_cfg is NULL
    FSP_ERR_ALREADY_OPEN     - Driver has already been opened with the same p_ctrl
    FSP_ERR_CALIBRATE_FAILED - Failed to perform auto-calibrate
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_open(T_mc80_ospi_instance_ctrl *const p_ctrl, T_mc80_ospi_cfg const *const p_cfg)
{
  fsp_err_t                             ret          = FSP_SUCCESS;
  const T_mc80_ospi_extended_cfg *const p_cfg_extend = p_cfg->p_extend;

  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl || NULL == p_cfg || NULL == p_cfg->p_extend)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN == p_ctrl->open)
    {
      return FSP_ERR_ALREADY_OPEN;
    }
  }

  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (!(MC80_OSPI_PERIPHERAL_CHANNEL_MASK & (1U << p_cfg_extend->ospi_unit)) ||
        (0 != (g_mc80_ospi_channels_open_flags & MC80_OSPI_PRV_CH_MASK(p_cfg_extend))))
    {
      if (!(MC80_OSPI_PERIPHERAL_CHANNEL_MASK & (1U << p_cfg_extend->ospi_unit)))
      {
        return FSP_ERR_ASSERTION;
      }
      else
      {
        return FSP_ERR_ALREADY_OPEN;
      }
    }
  }

  R_XSPI0_Type *p_reg = (R_XSPI0_Type *)(MC80_OSPI0_BASE_ADDRESS + p_cfg_extend->ospi_unit * MC80_OSPI_UNIT_ADDRESS_OFFSET);

  // Enable clock to the xSPI block
  R_BSP_MODULE_START(FSP_IP_OSPI, p_cfg_extend->ospi_unit);

  // Initialize control block
  p_ctrl->p_cfg                         = p_cfg;
  p_ctrl->p_reg                         = p_reg;
  p_ctrl->spi_protocol                  = p_cfg->spi_protocol;
  p_ctrl->channel                       = p_cfg_extend->channel;
  p_ctrl->ospi_unit                     = p_cfg_extend->ospi_unit;

  // Initialize transfer instance
  transfer_instance_t const *p_transfer = p_cfg_extend->p_lower_lvl_transfer;

  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE && NULL == p_transfer)
  {
    return FSP_ERR_ASSERTION;
  }

  p_transfer->p_api->open(p_transfer->p_ctrl, p_transfer->p_cfg);

  // Disable memory-mapping for this slave. It will be enabled later on after initialization
  // Note: Preserve bits 4-7 which must always be 1
  if (MC80_OSPI_DEVICE_NUMBER_0 == p_ctrl->channel)
  {
    p_reg->BMCTL0 = (p_reg->BMCTL0 & ~OSPI_BMCTL0_CH0CS0ACC_Msk) | MC80_OSPI_PRV_BMCTL0_RESERVED_BITS;
  }
  else
  {
    p_reg->BMCTL0 = (p_reg->BMCTL0 & ~OSPI_BMCTL0_CH0CS1ACC_Msk) | MC80_OSPI_PRV_BMCTL0_RESERVED_BITS;
  }

  // Perform xSPI Initial configuration as described in hardware manual
  // Set xSPI protocol mode
  uint32_t liocfg                        = ((uint32_t)p_cfg->spi_protocol) << OSPI_LIOCFGCSN_PRTMD_Pos;
  p_reg->LIOCFGCS[p_cfg_extend->channel] = liocfg;

  // Set xSPI drive/sampling timing
  if (MC80_OSPI_DEVICE_NUMBER_0 == p_ctrl->channel)
  {
    p_reg->WRAPCFG = ((uint32_t)p_cfg_extend->data_latch_delay_clocks << OSPI_WRAPCFG_DSSFTCS0_Pos) & OSPI_WRAPCFG_DSSFTCS0_Msk;
  }
  else
  {
    p_reg->WRAPCFG = ((uint32_t)p_cfg_extend->data_latch_delay_clocks << OSPI_WRAPCFG_DSSFTCS1_Pos) & OSPI_WRAPCFG_DSSFTCS1_Msk;
  }

  // Set minimum cycles between xSPI frames
  liocfg |= ((uint32_t)p_cfg_extend->p_timing_settings->command_to_command_interval << OSPI_LIOCFGCSN_CSMIN_Pos) & OSPI_LIOCFGCSN_CSMIN_Msk;

  // Set CS asserting extension in cycles
  liocfg |= ((uint32_t)p_cfg_extend->p_timing_settings->cs_pulldown_lead << OSPI_LIOCFGCSN_CSASTEX_Pos) & OSPI_LIOCFGCSN_CSASTEX_Msk;

  // Set CS releasing extension in cycles
  liocfg |= ((uint32_t)p_cfg_extend->p_timing_settings->cs_pullup_lag << OSPI_LIOCFGCSN_CSNEGEX_Pos) & OSPI_LIOCFGCSN_CSNEGEX_Msk;

  // Set SDR and DDR timing
  liocfg |= ((uint32_t)p_cfg_extend->p_timing_settings->sdr_drive_timing << OSPI_LIOCFGCSN_SDRDRV_Pos) & OSPI_LIOCFGCSN_SDRDRV_Msk;
  liocfg |= ((uint32_t)p_cfg_extend->p_timing_settings->sdr_sampling_edge << OSPI_LIOCFGCSN_SDRSMPMD_Pos) & OSPI_LIOCFGCSN_SDRSMPMD_Msk;
  liocfg |= ((uint32_t)p_cfg_extend->p_timing_settings->sdr_sampling_delay << OSPI_LIOCFGCSN_SDRSMPSFT_Pos) & OSPI_LIOCFGCSN_SDRSMPSFT_Msk;
  liocfg |= ((uint32_t)p_cfg_extend->p_timing_settings->ddr_sampling_extension << OSPI_LIOCFGCSN_DDRSMPEX_Pos) & OSPI_LIOCFGCSN_DDRSMPEX_Msk;

  // Set xSPI CSn signal timings
  p_reg->LIOCFGCS[p_cfg_extend->channel] = liocfg;

  // Set xSPI memory-mapping operation
  ret                                    = _Mc80_ospi_protocol_specific_settings(p_ctrl);

  // Return response after issuing write transaction to xSPI bus, Enable prefetch function and combination if desired
  const uint32_t bmcfgch                 = (0 << OSPI_BMCFGCH_WRMD_Pos) |
                           ((MC80_OSPI_CFG_COMBINATION_FUNCTION << OSPI_BMCFGCH_MWRCOMB_Pos) &
                            (OSPI_BMCFGCH_MWRCOMB_Msk | OSPI_BMCFGCH_MWRSIZE_Msk)) |
                           ((MC80_OSPI_CFG_PREFETCH_FUNCTION << OSPI_BMCFGCH_PREEN_Pos) &
                            OSPI_BMCFGCH_PREEN_Msk);

  // Both of these should have the same configuration and it affects all OSPI slave channels
  p_reg->BMCFGCH[0] = bmcfgch;
  p_reg->BMCFGCH[1] = bmcfgch;

  // Re-activate memory-mapped mode in Read/Write
  // Note: Preserve bits 4-7 which must always be 1
  if (0 == p_ctrl->channel)
  {
    p_reg->BMCTL0 = (p_reg->BMCTL0 & ~OSPI_BMCTL0_CH0CS0ACC_Msk) | OSPI_BMCTL0_CH0CS0ACC_Msk | MC80_OSPI_PRV_BMCTL0_RESERVED_BITS;
  }
  else
  {
    p_reg->BMCTL0 = (p_reg->BMCTL0 & ~OSPI_BMCTL0_CH0CS1ACC_Msk) | OSPI_BMCTL0_CH0CS1ACC_Msk | MC80_OSPI_PRV_BMCTL0_RESERVED_BITS;
  }

  if (FSP_SUCCESS == ret)
  {
    // Initialize RTOS event flags for DMA synchronization
    _Ospi_dma_event_flags_initialize();

    p_ctrl->open = MC80_OSPI_PRV_OPEN;
    g_mc80_ospi_channels_open_flags |= MC80_OSPI_PRV_CH_MASK(p_cfg_extend);
  }
  else if (0 == (g_mc80_ospi_channels_open_flags & MC80_OSPI_PRV_UNIT_MASK(p_cfg_extend)))
  {
    // If the open fails and no other channels are open, stop the module
    R_BSP_MODULE_STOP(FSP_IP_OSPI, p_cfg_extend->ospi_unit);
  }

  return ret;
}

/*-----------------------------------------------------------------------------------------------------
  Writes raw data directly to the OctaFlash using direct transfer interface.
  This function uses the current protocol's write command to send data directly to the flash device.
  Large transfers are automatically split into multiple 8-byte chunks with proper address increment.

  Parameters:
    p_ctrl           - Pointer to the control structure
    p_src            - Source data buffer
    address          - Starting address in flash memory
    bytes            - Number of bytes to write (any size)
    read_after_write - Read after write flag (not used in this implementation)

  Return:
    FSP_SUCCESS       - Data was written successfully
    FSP_ERR_ASSERTION - A required pointer is NULL or invalid parameters
    FSP_ERR_NOT_OPEN  - Driver is not opened
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_direct_write(T_mc80_ospi_instance_ctrl *p_ctrl,
                                 uint8_t const *const       p_src,
                                 uint32_t const             address,
                                 uint32_t const             bytes,
                                 bool const                 read_after_write)
{
  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl || NULL == p_src || 0 == bytes)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }
  }

  // Get current command set for the active protocol
  T_mc80_ospi_xspi_command_set const *p_cmd_set = p_ctrl->p_cmd_set;
  if (NULL == p_cmd_set)
  {
    return FSP_ERR_ASSERTION;
  }

  uint32_t bytes_remaining = bytes;
  uint32_t current_address = address;
  uint32_t offset          = 0;

  // Process data in chunks of up to 8 bytes
  while (bytes_remaining > 0)
  {
    uint32_t chunk_size;
    if (bytes_remaining > 8)
    {
      chunk_size = 8;
    }
    else
    {
      chunk_size = bytes_remaining;
    }

    // Prepare direct transfer structure for this chunk
    T_mc80_ospi_direct_transfer direct_transfer = {
      .command        = p_cmd_set->program_command,
      .command_length = (uint8_t)p_cmd_set->command_bytes,
      .address        = current_address,                           // Use current address
      .address_length = (uint8_t)(p_cmd_set->address_bytes + 1U),  // Include address
      .data_length    = (uint8_t)chunk_size,
      .dummy_cycles   = p_cmd_set->program_dummy_cycles,
      .data_u64       = 0,
    };

    // Copy data from source buffer to transfer structure
    for (uint32_t i = 0; i < chunk_size; i++)
    {
      direct_transfer.data_bytes[i] = p_src[offset + i];
    }

    // Execute the direct transfer
    _Mc80_ospi_direct_transfer(p_ctrl, &direct_transfer, MC80_OSPI_DIRECT_TRANSFER_DIR_WRITE);

    // Move to next chunk with incremented address
    offset += chunk_size;
    current_address += chunk_size;
    bytes_remaining -= chunk_size;
  }

  return FSP_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Reads raw data directly from the OctaFlash using direct transfer interface.
  This function uses the current protocol's read command to receive data directly from the flash device.
  Large transfers are automatically split into multiple 8-byte chunks with proper address increment.

  Parameters:
    p_ctrl  - Pointer to the control structure
    p_dest  - Destination data buffer
    address - Starting address in flash memory
    bytes   - Number of bytes to read (any size)

  Return:
    FSP_SUCCESS       - Data was read successfully
    FSP_ERR_ASSERTION - A required pointer is NULL or invalid parameters
    FSP_ERR_NOT_OPEN  - Driver is not opened
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_direct_read(T_mc80_ospi_instance_ctrl *p_ctrl, uint8_t *const p_dest, uint32_t const address, uint32_t const bytes)
{
  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl || NULL == p_dest || 0 == bytes)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }
  }

  // Get current command set for the active protocol
  T_mc80_ospi_xspi_command_set const *p_cmd_set = p_ctrl->p_cmd_set;
  if (NULL == p_cmd_set)
  {
    return FSP_ERR_ASSERTION;
  }

  uint32_t bytes_remaining = bytes;
  uint32_t current_address = address;
  uint32_t offset          = 0;

  // Process data in chunks of up to 8 bytes
  while (bytes_remaining > 0)
  {
    uint32_t chunk_size;
    if (bytes_remaining > 8)
    {
      chunk_size = 8;
    }
    else
    {
      chunk_size = bytes_remaining;
    }

    // Prepare direct transfer structure for this chunk
    T_mc80_ospi_direct_transfer direct_transfer = {
      .command        = p_cmd_set->read_command,
      .command_length = (uint8_t)p_cmd_set->command_bytes,
      .address        = current_address,                           // Use current address
      .address_length = (uint8_t)(p_cmd_set->address_bytes + 1U),  // Include address
      .data_length    = (uint8_t)chunk_size,
      .dummy_cycles   = p_cmd_set->read_dummy_cycles,
      .data_u64       = 0,
    };

    // Execute the direct transfer
    _Mc80_ospi_direct_transfer(p_ctrl, &direct_transfer, MC80_OSPI_DIRECT_TRANSFER_DIR_READ);

    // Copy data from transfer structure to destination buffer
    for (uint32_t i = 0; i < chunk_size; i++)
    {
      p_dest[offset + i] = direct_transfer.data_bytes[i];
    }

    // Move to next chunk with incremented address
    offset += chunk_size;
    current_address += chunk_size;
    bytes_remaining -= chunk_size;
  }

  return FSP_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Read/Write raw data directly with the OctaFlash.

  Parameters:
    p_ctrl     - Pointer to the control structure
    p_transfer - Pointer to transfer structure
    direction  - Transfer direction

  Return:
    FSP_SUCCESS       - The flash was programmed successfully
    FSP_ERR_ASSERTION - A required pointer is NULL
    FSP_ERR_NOT_OPEN  - Driver is not opened
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_direct_transfer(T_mc80_ospi_instance_ctrl *p_ctrl, T_mc80_ospi_direct_transfer *const p_transfer, T_mc80_ospi_direct_transfer_dir direction)
{
  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl || NULL == p_transfer || 0 == p_transfer->command_length)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }
  }

  _Mc80_ospi_direct_transfer(p_ctrl, p_transfer, direction);

  return FSP_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Enters XIP (execute in place) mode.

  NOTE: For the MX25UM25645G flash memory chip used in this project, XIP mode manipulation
  is not required. The chip can operate in continuous memory-mapped mode without explicit
  XIP enter/exit commands. This function is provided for compatibility but may not be
  necessary for normal operation.

  Parameters:
    p_ctrl - Pointer to the control structure

  Return:
    FSP_SUCCESS        - XiP mode was entered successfully
    FSP_ERR_ASSERTION  - A required pointer is NULL
    FSP_ERR_NOT_OPEN   - Driver is not opened
    FSP_ERR_UNSUPPORTED- XiP support is not enabled
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_xip_enter(T_mc80_ospi_instance_ctrl *p_ctrl)
{
  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl || NULL == p_ctrl->p_cfg)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }
  }

  // Enter XIP mode
  _Mc80_ospi_xip(p_ctrl, true);

  return FSP_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Exits XIP (execute in place) mode.

  NOTE: For the MX25UM25645G flash memory chip used in this project, XIP mode manipulation
  is not required. The chip can operate in continuous memory-mapped mode without explicit
  XIP enter/exit commands. This function is provided for compatibility but may not be
  necessary for normal operation.

  Parameters:
    p_ctrl - Pointer to the control structure

  Return:
    FSP_SUCCESS        - XiP mode was exited successfully
    FSP_ERR_ASSERTION  - A required pointer is NULL
    FSP_ERR_NOT_OPEN   - Driver is not opened
    FSP_ERR_UNSUPPORTED- XiP support is not enabled
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_xip_exit(T_mc80_ospi_instance_ctrl *p_ctrl)
{
  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl || NULL == p_ctrl->p_cfg)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }
  }

  // Exit XIP mode
  _Mc80_ospi_xip(p_ctrl, false);

  return FSP_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Program a page of data to the flash using memory-mapped DMA transfer.

  This function performs high-performance flash programming using the OSPI memory-mapped mode
  combined with DMA (Direct Memory Access) for efficient data transfer. It operates fundamentally
  differently from the direct transfer functions by using the hardware's automatic memory-mapped
  interface rather than manual command sequences.

  Protocol and Operation Method:
  - Uses memory-mapped write mode where flash appears as regular system memory
  - Flash device is accessed via memory-mapped addresses (0x80000000 for Device 0, 0x90000000 for Device 1)
  - Automatically uses the current active protocol's write commands configured in hardware registers
  - Protocol commands are pre-configured in CMCFG2 register during driver initialization
  - Supports both Standard SPI (1S-1S-1S) and Octal DDR (8D-8D-8D) protocols transparently

  DMA Transfer Mechanism:
  1. Configures DMAC (Direct Memory Access Controller) for bufferable write operations
  2. Sets up block-mode transfer from source buffer to memory-mapped flash address
  3. Enables OSPI DMA Bufferable Write (DMBWR) for optimal performance
  4. Starts DMA transfer in repeat mode for continuous data streaming
  5. Waits for DMA completion to ensure deterministic operation
  6. Handles combination write function for small transfers (< combination bytes)

  Write Enable and Status Management:
  - Automatically sends Write Enable command before programming
  - Verifies write enable status through status register polling
  - Checks device busy status to prevent conflicts with ongoing operations
  - Uses the current protocol's write enable and status commands

  Page Boundary and Alignment Requirements:
  - Enforces page boundary constraints (typically 256 bytes)
  - Requires 8-byte alignment for CPU access compatibility
  - Validates transfer size and alignment for hardware requirements

  Hardware Integration:
  - Leverages OSPI peripheral's automatic command generation
  - Uses pre-configured timing and protocol settings from driver initialization
  - Integrates with combination write and prefetch functions when enabled
  - Maintains compatibility with both OSPI units and channels

  Parameters:
    p_ctrl     - Pointer to the control structure
    p_src      - Source data buffer
    p_dest     - Destination address in flash (memory-mapped address)
    byte_count - Number of bytes to write

  Return:
    FSP_SUCCESS            - The flash was programmed successfully
    FSP_ERR_ASSERTION      - p_ctrl, p_dest or p_src is NULL, or byte_count crosses a page boundary
    FSP_ERR_NOT_OPEN       - Driver is not opened
    FSP_ERR_INVALID_SIZE   - Insufficient space remaining in page or write length is not a multiple of CPU access size when not using the DMAC
    FSP_ERR_DEVICE_BUSY    - Another Write/Erase transaction is in progress
    FSP_ERR_WRITE_FAILED   - Write operation failed
    FSP_ERR_INVALID_ADDRESS- Destination or source is not aligned to CPU access alignment when not using the DMAC
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_write(T_mc80_ospi_instance_ctrl *p_ctrl, uint8_t const *const p_src, uint8_t *const p_dest, uint32_t byte_count)
{
  fsp_err_t err = FSP_SUCCESS;

  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl || NULL == p_src || NULL == p_dest || 0 == byte_count)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }

    // Check that space remaining in page is sufficient for requested write size
    uint32_t page_size   = p_ctrl->p_cfg->page_size_bytes;
    uint32_t page_offset = (uint32_t)p_dest & (page_size - 1);
    if ((page_size - page_offset) < byte_count)
    {
      return FSP_ERR_INVALID_SIZE;
    }

    if ((byte_count & (MC80_OSPI_PRV_CPU_ACCESS_LENGTH - 1)) != 0)
    {
      return FSP_ERR_INVALID_SIZE;
    }
    if (((uint32_t)p_dest & (MC80_OSPI_PRV_CPU_ACCESS_ALIGNMENT - 1)) != 0)
    {
      return FSP_ERR_INVALID_ADDRESS;
    }
#if defined(__llvm__) && !defined(__ARMCC_VERSION)
    // LLVM needs 32-bit aligned data
    if (((uint32_t)p_src & (0x3)) != 0)
    {
      return FSP_ERR_INVALID_ADDRESS;
    }
#endif
  }

  R_XSPI0_Type *const p_reg = p_ctrl->p_reg;

  if (true == _Mc80_ospi_status_sub(p_ctrl, p_ctrl->p_cfg->write_status_bit))
  {
    return FSP_ERR_DEVICE_BUSY;
  }

  // Setup and start DMAC transfer
  T_mc80_ospi_extended_cfg const *p_cfg_extend             = p_ctrl->p_cfg->p_extend;
  transfer_instance_t const      *p_transfer               = p_cfg_extend->p_lower_lvl_transfer;

  // Enable Octa-SPI DMA Bufferable Write
  dmac_extended_cfg_t const *p_dmac_extend                 = p_transfer->p_cfg->p_extend;
  R_DMAC0_Type              *p_dma_reg                     = R_DMAC0 + (sizeof(R_DMAC0_Type) * p_dmac_extend->channel);
  p_dma_reg->DMBWR                                         = R_DMAC0_DMBWR_BWE_Msk;

  // Update the block-mode transfer settings
  p_transfer->p_cfg->p_info->p_src                         = p_src;
  p_transfer->p_cfg->p_info->p_dest                        = p_dest;
  p_transfer->p_cfg->p_info->transfer_settings_word_b.size = TRANSFER_SIZE_1_BYTE;
  p_transfer->p_cfg->p_info->transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL;
  p_transfer->p_cfg->p_info->length                        = (uint16_t)byte_count;
  err                                                      = p_transfer->p_api->reconfigure(p_transfer->p_ctrl, p_transfer->p_cfg->p_info);
  if (FSP_SUCCESS != err)
  {
    return err;
  }

  _Mc80_ospi_write_enable(p_ctrl);

  // Start DMA
  err = p_transfer->p_api->softwareStart(p_transfer->p_ctrl, TRANSFER_START_MODE_REPEAT);
  if (FSP_SUCCESS != err)
  {
    return err;
  }

  // Wait for DMAC to complete to maintain deterministic processing and backward compatibility
  volatile transfer_properties_t transfer_properties = { 0U };
  err                                                = p_transfer->p_api->infoGet(p_transfer->p_ctrl, (transfer_properties_t *)&transfer_properties);
  if (FSP_SUCCESS != err)
  {
    return err;
  }
  while (FSP_SUCCESS == err && transfer_properties.transfer_length_remaining > 0)
  {
    err = p_transfer->p_api->infoGet(p_transfer->p_ctrl, (transfer_properties_t *)&transfer_properties);
    if (FSP_SUCCESS != err)
    {
      return err;
    }
  }

  // Disable Octa-SPI DMA Bufferable Write
  p_dma_reg->DMBWR = 0U;

  // If this number of bytes is less than the combination count, push the data to force a transaction
  if (MC80_OSPI_COMBINATION_FUNCTION_DISABLE != MC80_OSPI_CFG_COMBINATION_FUNCTION)
  {
    uint8_t combo_bytes = (uint8_t)(2U * ((uint8_t)MC80_OSPI_CFG_COMBINATION_FUNCTION + 1U));
    if (byte_count < combo_bytes)
    {
      p_reg->BMCTL1 = MC80_OSPI_PRV_BMCTL1_PUSH_COMBINATION_WRITE_MASK;
    }
  }

  return FSP_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Erase a block or sector of flash. The byte_count must exactly match one of the erase sizes defined in T_mc80_ospi_cfg.
  For chip erase, byte_count must be MC80_OSPI_ERASE_SIZE_CHIP_ERASE.

  Parameters:
    p_ctrl           - Pointer to the control structure
    p_device_address - Address in flash to erase
    byte_count       - Number of bytes to erase

  Return:
    FSP_SUCCESS         - The command to erase the flash was executed successfully
    FSP_ERR_ASSERTION   - p_ctrl or p_device_address is NULL, byte_count doesn't match an erase size defined in T_mc80_ospi_cfg, or byte_count is set to 0
    FSP_ERR_NOT_OPEN    - Driver is not opened
    FSP_ERR_DEVICE_BUSY - The device is busy
    FSP_ERR_WRITE_FAILED- Write operation failed
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_erase(T_mc80_ospi_instance_ctrl *p_ctrl, uint8_t *const p_device_address, uint32_t byte_count)
{
  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl || NULL == p_device_address || 0 == byte_count)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }
  }

  T_mc80_ospi_cfg const *p_cfg         = p_ctrl->p_cfg;
  uint16_t               erase_command = 0;
  uint32_t               chip_address_base;
  uint32_t               chip_address;
  bool                   send_address = true;

  if (p_ctrl->channel)
  {
    chip_address_base = MC80_OSPI_DEVICE_1_START_ADDRESS;
  }
  else
  {
    chip_address_base = MC80_OSPI_DEVICE_0_START_ADDRESS;
  }
  chip_address                                  = (uint32_t)p_device_address - chip_address_base;

  T_mc80_ospi_xspi_command_set const *p_cmd_set = p_ctrl->p_cmd_set;

  if (true == _Mc80_ospi_status_sub(p_ctrl, p_cfg->write_status_bit))
  {
    return FSP_ERR_DEVICE_BUSY;
  }

  // Select the appropriate erase command from the command set
  T_mc80_ospi_erase_command const *p_erase_list      = p_cmd_set->p_erase_commands->p_table;
  const uint8_t                    erase_list_length = p_cmd_set->p_erase_commands->length;

  for (uint32_t index = 0; index < erase_list_length; index++)
  {
    // If requested byte_count is supported by underlying flash, store the command
    if (byte_count == p_erase_list[index].size)
    {
      if (MC80_OSPI_ERASE_SIZE_CHIP_ERASE == byte_count)
      {
        // Don't send address for chip erase
        send_address = false;
      }

      erase_command = p_erase_list[index].command;
      break;
    }
  }

  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE && 0U == erase_command)
  {
    return FSP_ERR_ASSERTION;
  }

  fsp_err_t err = _Mc80_ospi_write_enable(p_ctrl);
  if (FSP_SUCCESS != err)
  {
    return err;
  }

  T_mc80_ospi_direct_transfer direct_command = {
    .command        = erase_command,
    .command_length = (uint8_t)p_cmd_set->command_bytes,
    .address        = chip_address,
    .address_length = 0,
    .data_length    = 0,
  };

  if (send_address)
  {
    direct_command.address_length = (uint8_t)(p_cmd_set->address_bytes + 1U);
  }

  _Mc80_ospi_direct_transfer(p_ctrl, &direct_command, MC80_OSPI_DIRECT_TRANSFER_DIR_WRITE);

  // If prefetch is enabled, make sure the banks aren't being used and flush the prefetch caches after an erase
  if (MC80_OSPI_CFG_PREFETCH_FUNCTION)
  {
    R_XSPI0_Type *const p_reg = p_ctrl->p_reg;
    while ((p_reg->COMSTT & MC80_OSPI_PRV_COMSTT_MEMACCCH_MASK) != 0)
    {
      __NOP();  // Breakpoint for memory access wait
    }
    p_reg->BMCTL1 = MC80_OSPI_PRV_BMCTL1_CLEAR_PREFETCH_MASK;
  }

  return FSP_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Gets the write or erase status of the flash.

  Parameters:
    p_ctrl   - Pointer to the control structure
    p_status - Pointer to status structure

  Return:
    FSP_SUCCESS       - The write status is in p_status
    FSP_ERR_ASSERTION - p_ctrl or p_status is NULL
    FSP_ERR_NOT_OPEN  - Driver is not opened
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_status_get(T_mc80_ospi_instance_ctrl *p_ctrl, T_mc80_ospi_status *const p_status)
{
  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl || NULL == p_status)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }
  }

  // Read device status
  p_status->write_in_progress = _Mc80_ospi_status_sub(p_ctrl, p_ctrl->p_cfg->write_status_bit);

  return FSP_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Selects the bank to access. Bank value should be 0 or 1.

  Parameters:
    p_ctrl - Pointer to the control structure
    bank   - Bank value (0 or 1)

  Return:
    FSP_ERR_UNSUPPORTED - This function is unsupported
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_bank_set(T_mc80_ospi_instance_ctrl *p_ctrl, uint32_t bank)
{
  return FSP_ERR_UNSUPPORTED;
}

/*-----------------------------------------------------------------------------------------------------
  Sets the SPI protocol. Dynamically switches between Standard SPI (1S-1S-1S) and Octal DDR (8D-8D-8D)
  modes by updating OSPI hardware registers and selecting appropriate command set from MC80_OSPI_config.c.

  === Mc80_ospi_spi_protocol_set() Function Operation ===

  The Mc80_ospi_spi_protocol_set() function allows dynamic switching between different OSPI protocols
  during runtime. It uses the command set table from this configuration file to reconfigure the
  peripheral for the new protocol.

  How it works:
  1. Accepts a new protocol type (e.g., MC80_OSPI_PROTOCOL_1S_1S_1S or MC80_OSPI_PROTOCOL_8D_8D_8D)
  2. Searches through g_OSPI_command_set_table[] to find matching protocol configuration
  3. Updates the peripheral registers with new timing, commands, and protocol settings
  4. Switches between Standard SPI mode and high-speed Octal DDR mode seamlessly

  Configuration Elements Used:

  * g_OSPI_command_set_table[] - Main protocol configuration array containing:
    - Protocol definitions (1S-1S-1S for Standard SPI, 8D-8D-8D for Octal DDR)
    - Command formats and dummy cycle counts
    - Address and data phase configurations
    - Frame formats and latency modes

  * g_OSPI_command_set - Table descriptor pointing to the command set array
    - Used by _Mc80_ospi_command_set_get() to locate protocol configurations
    - Length field ensures safe iteration through available protocols

  * Protocol-specific configurations:
    - Standard SPI (1S-1S-1S): Uses single-line commands with 1-byte opcodes
    - Octal DDR (8D-8D-8D): Uses 8-line double data rate with 2-byte opcodes

  * Command mappings for each protocol:
    - Read commands (FAST_READ4B vs 8READ_DTR)
    - Write commands (PP4B vs PP4B_STR)
    - Status commands (RDSR vs RDSR_STR)
    - Dummy cycle requirements (8 cycles for SPI, 20 cycles for DDR)

  * Erase command tables:
    - g_OSPI_command_set_initial_erase_table for Standard SPI mode
    - g_OSPI_command_set_high_speed_erase_table for Octal DDR mode (empty - erase only in SPI)

  The function enables seamless protocol switching for performance optimization:
  - Start with reliable Standard SPI for initialization
  - Switch to high-speed Octal DDR for bulk data operations
  - Return to Standard SPI for erase operations (required by flash device)

  Parameters:
    p_ctrl       - Pointer to the control structure
    spi_protocol - SPI protocol to set

  Return:
    FSP_SUCCESS              - SPI protocol updated on MPU peripheral
    FSP_ERR_ASSERTION        - A required pointer is NULL
    FSP_ERR_NOT_OPEN         - Driver is not opened
    FSP_ERR_CALIBRATE_FAILED - Failed to perform auto-calibrate
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_spi_protocol_set(T_mc80_ospi_instance_ctrl *p_ctrl, T_mc80_ospi_protocol spi_protocol)
{
  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }
  }

  // Save the old protocol in case of an undefined command set
  T_mc80_ospi_protocol old_protocol = p_ctrl->spi_protocol;
  p_ctrl->spi_protocol              = spi_protocol;

  // Update the SPI protocol and its associated registers
  fsp_err_t err                     = _Mc80_ospi_protocol_specific_settings(p_ctrl);

  if (FSP_ERR_INVALID_MODE == err)
  {
    // Restore the original spi protocol. Nothing else has been changed in this case
    p_ctrl->spi_protocol = old_protocol;
  }

  return err;
}

/*-----------------------------------------------------------------------------------------------------
  Close the OSPI driver module.

  Parameters:
    p_ctrl - Pointer to the control structure

  Return:
    FSP_SUCCESS       - Configuration was successful
    FSP_ERR_ASSERTION - p_ctrl is NULL
    FSP_ERR_NOT_OPEN  - Driver is not opened
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_close(T_mc80_ospi_instance_ctrl *p_ctrl)
{
  fsp_err_t err = FSP_SUCCESS;

  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl || NULL == p_ctrl->p_cfg || NULL == p_ctrl->p_cfg->p_extend)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }
  }

  T_mc80_ospi_extended_cfg const *p_cfg_extend = p_ctrl->p_cfg->p_extend;

  // Close transfer instance
  transfer_instance_t const *p_transfer        = p_cfg_extend->p_lower_lvl_transfer;
  p_transfer->p_api->close(p_transfer->p_ctrl);

  // Cleanup RTOS resources if all channels are being closed
  if (0 == ((g_mc80_ospi_channels_open_flags & ~MC80_OSPI_PRV_CH_MASK(p_cfg_extend)) & MC80_OSPI_PRV_UNIT_MASK(p_cfg_extend)))
  {
    _Ospi_dma_event_flags_cleanup();
  }

  p_ctrl->open = 0U;
  g_mc80_ospi_channels_open_flags &= ~MC80_OSPI_PRV_CH_MASK(p_cfg_extend);

  // Disable clock to the OSPI block if all channels are closed
  if (0 == (g_mc80_ospi_channels_open_flags & MC80_OSPI_PRV_UNIT_MASK(p_cfg_extend)))
  {
    R_BSP_MODULE_STOP(FSP_IP_OSPI, p_cfg_extend->ospi_unit);
  }

  return err;
}

/*-----------------------------------------------------------------------------------------------------
  Perform initialization based on SPI/OPI protocol

  Parameters:
    p_ctrl - Pointer to OSPI specific control structure

  Return:
    FSP_SUCCESS              - Protocol based settings completed successfully
    FSP_ERR_CALIBRATE_FAILED - Auto-Calibration failed
-----------------------------------------------------------------------------------------------------*/
static fsp_err_t _Mc80_ospi_protocol_specific_settings(T_mc80_ospi_instance_ctrl *p_ctrl)
{
  R_XSPI0_Type *const p_reg                     = p_ctrl->p_reg;
  fsp_err_t           ret                       = FSP_SUCCESS;

  // Get the command set for the configured protocol and save it to the control struct
  T_mc80_ospi_xspi_command_set const *p_cmd_set = _Mc80_ospi_command_set_get(p_ctrl);
  if (NULL == p_cmd_set)
  {
    return FSP_ERR_INVALID_MODE;
  }

  p_ctrl->p_cmd_set = p_cmd_set;

  // Update the SPI protocol and latency mode
  uint32_t liocfg   = p_reg->LIOCFGCS[p_ctrl->channel] & ~(OSPI_LIOCFGCSN_LATEMD_Msk | OSPI_LIOCFGCSN_PRTMD_Msk);
  liocfg |= (((uint32_t)p_ctrl->spi_protocol << OSPI_LIOCFGCSN_PRTMD_Pos) & OSPI_LIOCFGCSN_PRTMD_Msk);
  liocfg |= (((uint32_t)p_cmd_set->latency_mode << OSPI_LIOCFGCSN_LATEMD_Pos) & OSPI_LIOCFGCSN_LATEMD_Msk);
  p_reg->LIOCFGCS[p_ctrl->channel] = liocfg;

  // Specifies the read/write commands and Read dummy clocks for Device
  uint32_t cmcfg0                  = ((uint32_t)(p_cmd_set->address_msb_mask << OSPI_CMCFG0CSN_ADDRPEN_Pos)) |
                    ((uint32_t)(p_cmd_set->frame_format << OSPI_CMCFG0CSN_FFMT_Pos)) |
                    (((uint32_t)p_cmd_set->address_bytes << OSPI_CMCFG0CSN_ADDSIZE_Pos) &
                     OSPI_CMCFG0CSN_ADDSIZE_Msk);

  // When using 4-byte addressing, always mask off the most-significant nybble to remove the system bus offset from
  // the transmitted addresses. Ex. CS1 starts at 0x9000_0000 so it needs to mask off bits [31:28]
  if (p_cmd_set->address_bytes == MC80_OSPI_ADDRESS_BYTES_4)
  {
    cmcfg0 |= MC80_OSPI_PRV_ADDRESS_REPLACE_ENABLE_BITS;
  }

  // Apply the frame format setting and update the register
  cmcfg0 |= (uint32_t)(p_cmd_set->frame_format << OSPI_CMCFG0CSN_FFMT_Pos);
  p_reg->CMCFGCS[p_ctrl->channel].CMCFG0 = cmcfg0;

  // Cache the appropriate command values for later use
  uint16_t read_command                  = p_cmd_set->read_command;
  uint16_t write_command                 = p_cmd_set->program_command;

  // If no length is specified or if the command byte length is 1, move the command to the upper byte
  if (MC80_OSPI_COMMAND_BYTES_1 == p_cmd_set->command_bytes)
  {
    read_command  = (uint16_t)((read_command & MC80_OSPI_PRV_CDTBUF_CMD_1B_VALUE_MASK) << MC80_OSPI_PRV_CDTBUF_CMD_1B_VALUE_SHIFT);
    write_command = (uint16_t)((write_command & MC80_OSPI_PRV_CDTBUF_CMD_1B_VALUE_MASK) << MC80_OSPI_PRV_CDTBUF_CMD_1B_VALUE_SHIFT);
  }

  const uint8_t read_dummy_cycles        = p_cmd_set->read_dummy_cycles;
  const uint8_t write_dummy_cycles       = p_cmd_set->program_dummy_cycles;

  p_reg->CMCFGCS[p_ctrl->channel].CMCFG1 = (uint32_t)(((uint32_t)(read_command) << OSPI_CMCFG1CSN_RDCMD_Pos) |
                                                      ((uint32_t)(read_dummy_cycles << OSPI_CMCFG1CSN_RDLATE_Pos) &
                                                       OSPI_CMCFG1CSN_RDLATE_Msk));

  p_reg->CMCFGCS[p_ctrl->channel].CMCFG2 = (uint32_t)(((uint32_t)(write_command) << OSPI_CMCFG2CSN_WRCMD_Pos) |
                                                      ((uint32_t)(write_dummy_cycles << OSPI_CMCFG2CSN_WRLATE_Pos) &
                                                       OSPI_CMCFG2CSN_WRLATE_Msk));

  return ret;
}

/*-----------------------------------------------------------------------------------------------------
  Gets device status.

  Parameters:
    p_ctrl - Pointer to a driver handle
    bit_pos         - Write-in-progress bit position

  Return:
    True if busy, false if not
-----------------------------------------------------------------------------------------------------*/
static bool _Mc80_ospi_status_sub(T_mc80_ospi_instance_ctrl *p_ctrl, uint8_t bit_pos)
{
  T_mc80_ospi_xspi_command_set const *p_cmd_set = p_ctrl->p_cmd_set;

  // Skip status check if no command was specified
  if (0 == p_cmd_set->status_command)
  {
    return false;
  }

  T_mc80_ospi_direct_transfer direct_command = {
    .command        = p_cmd_set->status_command,
    .command_length = (uint8_t)p_cmd_set->command_bytes,
    .address_length = 0U,
    .address        = 0U,
    .data_length    = 1U,
    .dummy_cycles   = p_cmd_set->status_dummy_cycles,
  };

  if (p_cmd_set->status_needs_address)
  {
    direct_command.address_length = (uint8_t)(p_cmd_set->status_address_bytes + 1U);
    direct_command.address        = p_cmd_set->status_address;
  }

  // 8D-8D-8D mode requires an address for any kind of read. If the address wasn't set by the configuration
  // set it to the general address length
  if ((direct_command.address_length != 0) && (MC80_OSPI_PROTOCOL_8D_8D_8D == p_ctrl->spi_protocol))
  {
    direct_command.address_length = (uint8_t)(p_cmd_set->address_bytes + 1U);
  }

  _Mc80_ospi_direct_transfer(p_ctrl, &direct_command, MC80_OSPI_DIRECT_TRANSFER_DIR_READ);

  return (direct_command.data >> bit_pos) & 1U;
}

/*-----------------------------------------------------------------------------------------------------
  Send Write enable command to the OctaFlash

  Parameters:
    p_ctrl - Pointer to OSPI specific control structure

  Return:
    FSP_SUCCESS         - Write enable operation completed
    FSP_ERR_NOT_ENABLED - Write enable failed
-----------------------------------------------------------------------------------------------------*/
static fsp_err_t _Mc80_ospi_write_enable(T_mc80_ospi_instance_ctrl *p_ctrl)
{
  T_mc80_ospi_xspi_command_set const *const p_cmd_set = p_ctrl->p_cmd_set;

  // If the command is 0x00, then skip sending the write enable
  if (0 == p_cmd_set->write_enable_command)
  {
    return FSP_SUCCESS;
  }

  T_mc80_ospi_direct_transfer direct_command = {
    .command        = p_cmd_set->write_enable_command,
    .command_length = (uint8_t)p_cmd_set->command_bytes,
    .address_length = 0,
    .address        = 0,
    .data_length    = 0,
    .dummy_cycles   = 0,
  };

  _Mc80_ospi_direct_transfer(p_ctrl, &direct_command, MC80_OSPI_DIRECT_TRANSFER_DIR_WRITE);

  // In case write enable is not checked, assume write is enabled
  bool write_enabled = true;

  // Verify write is enabled
  for (uint32_t i = 0U; i < MC80_OSPI_MAX_WRITE_ENABLE_LOOPS; i++)
  {
    write_enabled = _Mc80_ospi_status_sub(p_ctrl, p_ctrl->p_cfg->write_enable_bit);
    if (write_enabled)
    {
      break;
    }
  }

  if (!write_enabled)
  {
    return FSP_ERR_NOT_ENABLED;
  }

  return FSP_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Direct transfer implementation - executes manual OSPI commands outside of memory-mapped mode.

  This function performs direct communication with the OSPI flash device using the manual command
  interface. It bypasses the automatic memory-mapped mode and allows sending custom commands
  with specific parameters for read/write operations, status checks, and device configuration.

  Operation sequence:
  1. Configures the Command Data Buffer (CDTBUF) with command parameters:
     - Command code and length (1 or 2 bytes)
     - Address and address length
     - Data length and dummy cycles
     - Transfer direction (read/write)
  2. Sets up manual command control (CDCTL0) to select the target chip
  3. Waits for any ongoing transactions to complete
  4. Loads command data into the command buffer registers
  5. For write operations: loads data into CDD0/CDD1 registers
  6. Initiates the transaction by setting TRREQ bit
  7. Waits for transaction completion
  8. For read operations: retrieves data from CDD0/CDD1 registers
  9. Clears interrupt flags

  The function handles both 1-byte and 2-byte command formats, adjusting the command positioning
  in the register based on the command length. For data transfers larger than 4 bytes, it uses
  both CDD0 and CDD1 registers to handle up to 8 bytes of data.

  Parameters:
    p_ctrl - Pointer to OSPI specific control structure
    p_transfer      - Pointer to transfer structure
    direction       - Transfer direction

  Return:
    void
-----------------------------------------------------------------------------------------------------*/
static void _Mc80_ospi_direct_transfer(T_mc80_ospi_instance_ctrl         *p_ctrl,
                                       T_mc80_ospi_direct_transfer *const p_transfer,
                                       T_mc80_ospi_direct_transfer_dir    direction)
{
  R_XSPI0_Type *const             p_reg   = p_ctrl->p_reg;
  const T_mc80_ospi_device_number channel = p_ctrl->channel;

  // Build the Command Data Buffer configuration register value
  // This register defines the complete transaction format including command, address, data sizes and dummy cycles
  uint32_t cdtbuf0 =
  (((uint32_t)p_transfer->command_length << OSPI_CDTBUFn_CMDSIZE_Pos) & OSPI_CDTBUFn_CMDSIZE_Msk) |
  (((uint32_t)p_transfer->address_length << OSPI_CDTBUFn_ADDSIZE_Pos) & OSPI_CDTBUFn_ADDSIZE_Msk) |
  (((uint32_t)p_transfer->data_length << OSPI_CDTBUFn_DATASIZE_Pos) & OSPI_CDTBUFn_DATASIZE_Msk) |
  (((uint32_t)p_transfer->dummy_cycles << OSPI_CDTBUFn_LATE_Pos) & OSPI_CDTBUFn_LATE_Msk) |
  (((uint32_t)direction << OSPI_CDTBUFn_TRTYPE_Pos) & OSPI_CDTBUFn_TRTYPE_Msk);

  // Handle command code positioning based on command length
  // 1-byte commands go in upper byte, 2-byte commands use both bytes
  if (1 == p_transfer->command_length)
  {
    cdtbuf0 |= (p_transfer->command & MC80_OSPI_PRV_CDTBUF_CMD_1B_VALUE_MASK) << MC80_OSPI_PRV_CDTBUF_CMD_UPPER_OFFSET;
  }
  else
  {
    cdtbuf0 |= (p_transfer->command & MC80_OSPI_PRV_CDTBUF_CMD_2B_VALUE_MASK) << MC80_OSPI_PRV_CDTBUF_CMD_OFFSET;
  }

  // Configure manual command control: cancel ongoing transactions, select target channel
  p_reg->CDCTL0 = ((((uint32_t)channel) << OSPI_CDCTL0_CSSEL_Pos) & OSPI_CDCTL0_CSSEL_Msk);

  // Wait for any ongoing transaction to complete before starting new one
  while (p_reg->CDCTL0_b.TRREQ != 0)
  {
    __NOP();  // Breakpoint for transaction ready wait
  }

  // Load the transaction configuration and address into command buffer
  p_reg->CDBUF[0].CDT = cdtbuf0;              // Command Data Transaction register
  p_reg->CDBUF[0].CDA = p_transfer->address;  // Command Data Address register

  // For write operations: load data into the data registers before starting transaction
  if (MC80_OSPI_DIRECT_TRANSFER_DIR_WRITE == direction)
  {
    p_reg->CDBUF[0].CDD0 = (uint32_t)(p_transfer->data_u64 & UINT32_MAX);                    // Lower 32 bits
    if (p_transfer->data_length > sizeof(uint32_t))
    {
      p_reg->CDBUF[0].CDD1 = (uint32_t)(p_transfer->data_u64 >> MC80_OSPI_PRV_UINT32_BITS);  // Upper 32 bits
    }
  }

  // Start the transaction and wait for completion
  p_reg->CDCTL0_b.TRREQ = 1;  // Initiate transaction
  while (p_reg->CDCTL0_b.TRREQ != 0)
  {
    __NOP();                  // Breakpoint for transaction completion wait
  }

  // For read operations: retrieve data from the data registers after transaction completes
  if (MC80_OSPI_DIRECT_TRANSFER_DIR_READ == direction)
  {
    p_transfer->data_u64 = p_reg->CDBUF[0].CDD0;                                                // Read lower 32 bits
    if (p_transfer->data_length > sizeof(uint32_t))
    {
      p_transfer->data_u64 |= (((uint64_t)p_reg->CDBUF[0].CDD1) << MC80_OSPI_PRV_UINT32_BITS);  // Combine with upper 32 bits
    }
  }

  // Clear interrupt flags to prepare for next transaction
  p_reg->INTC = p_reg->INTS;
}

/*-----------------------------------------------------------------------------------------------------
  Get command set for current protocol

  Parameters:
    p_ctrl - Pointer to OSPI specific control structure

  Return:
    Pointer to command set structure, or NULL if not found
-----------------------------------------------------------------------------------------------------*/
static T_mc80_ospi_xspi_command_set const *_Mc80_ospi_command_set_get(T_mc80_ospi_instance_ctrl *p_ctrl)
{
  T_mc80_ospi_extended_cfg const *p_cfg_extend = p_ctrl->p_cfg->p_extend;

  if (NULL == p_cfg_extend->p_xspi_command_set)
  {
    return NULL;
  }

  T_mc80_ospi_xspi_command_set *p_cmd_set;
  for (uint32_t i = 0; i < p_cfg_extend->p_xspi_command_set->length; i++)
  {
    p_cmd_set = &((T_mc80_ospi_xspi_command_set *)p_cfg_extend->p_xspi_command_set->p_table)[i];
    if (p_cmd_set->protocol == p_ctrl->spi_protocol)
    {
      return p_cmd_set;
    }
  }

  // If the protocol isn't found, return NULL
  return NULL;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Automatic calibration sequence for OSPI timing parameters in high-speed protocols.

  This function performs automatic calibration of read data strobe timing for reliable high-speed
  OSPI communication, particularly critical for 8D-8D-8D (Octal DDR) protocol operation.

  Calibration Process:
  1. Validates that no calibration is currently in progress (CAEN bit check)
  2. Saves current timing parameters before calibration (if p_calibration_data provided)
  3. Extracts current protocol parameters (command format, address size, dummy cycles)
  4. Configures calibration control registers (CCCTL1-3) with:
     - Command size and format (1 or 2 bytes, positioned correctly)
     - Address size from current protocol settings
     - Read command from active command set
     - Dummy cycles matching protocol requirements
     - Calibration data size (0xF = 15 bytes)
     - Preamble pattern address for known data reading
  5. Sets preamble patterns in CCCTL4-7 registers for data comparison:
     - Uses patterns from configuration (p_autocalibration_preamble_patterns)
     - Falls back to default patterns if configuration pointer is NULL
     - Patterns must match data at the specified calibration address in flash
     - Default patterns: 0xFFFF0000, 0x000800FF, 0x00FFF700, 0xF700F708
     - Write patterns to flash at calibration address before running calibration
  6. Sets calibration parameters in CCCTL0:
     - CAITV: Calibration interval (0x1F cycles)
     - CANOWR: No overwrite mode enabled
     - CASFTEND: Shift end value (0x1F)
  7. Starts automatic calibration by setting CAEN bit
  8. Waits for hardware to complete calibration process by monitoring INTS register:
     - CASUCCS flag indicates successful calibration
     - CAFAIL flag indicates calibration failure
     - Timeout protection prevents infinite waiting
  9. Disables calibration by clearing CAEN bit
  10. Checks calibration result and clears appropriate interrupt flags
  11. Saves timing parameters after calibration (if p_calibration_data provided)

  Hardware Operation:
  The OSPI peripheral automatically:
  - Tests multiple data strobe timing delays
  - Reads known pattern data from the specified address
  - Compares received data against expected pattern
  - Selects optimal timing that provides reliable data reception
  - Updates internal timing registers with calibrated values

  Calibrated Parameters Storage:
  Results are automatically written to hardware registers by the OSPI peripheral:
  - CASTTCSn (0x188 + 0x004*n): Calibration Status Register - contains success flags for each
    tested OM_DQS shift value, indicating which timing delays passed validation
  - Internal OM_DQS timing registers: Hardware automatically selects and applies the optimal
    data strobe timing delay from the successful calibration range
  - LIOCFGCS register timing fields: May be updated with optimized sampling delays

  The calibration process tests all possible data strobe (OM_DQS) timing delays and identifies
  which ones provide reliable data reception. The hardware then automatically configures the
  optimal timing for subsequent high-speed read operations.

  Protocol Compatibility:
  - Standard SPI (1S-1S-1S): Basic calibration for timing optimization
  - Octal DDR (8D-8D-8D): Critical for high-speed data integrity
  - Automatically uses current protocol's command format and timing

  Prerequisites:
  - Flash device must be in appropriate protocol mode
  - Preamble pattern address must contain valid, known data
  - Device must not be busy with other operations

  Parameters: p_ctrl - Pointer to OSPI instance control structure
              p_calibration_data - Pointer to structure for storing calibration data (can be NULL)

  Return: FSP_SUCCESS - Auto-calibration completed successfully
          FSP_ERR_DEVICE_BUSY - Auto-calibration already in progress
          FSP_ERR_CALIBRATE_FAILED - Auto-calibration failed
          FSP_ERR_ASSERTION - Invalid parameter
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_auto_calibrate(T_mc80_ospi_instance_ctrl *p_ctrl, T_mc80_ospi_calibration_data *p_calibration_data)
{
  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }
  }

  R_XSPI0_Type *const             p_OSPI        = p_ctrl->p_reg;
  fsp_err_t                       ret           = FSP_SUCCESS;
  T_mc80_ospi_extended_cfg const *p_cfg_extend  = p_ctrl->p_cfg->p_extend;

  T_mc80_ospi_xspi_command_set const *p_cmd_set = p_ctrl->p_cmd_set;

  T_mc80_ospi_device_number channel             = p_ctrl->channel;

  // Check that calibration is not in progress
  if (0 != p_OSPI->CCCTLCS[channel].CCCTL0_b.CAEN)
  {
    return FSP_ERR_DEVICE_BUSY;
  }

  // Save timing parameters before calibration if structure provided
  if (NULL != p_calibration_data)
  {
    // Clear the structure
    memset(p_calibration_data, 0, sizeof(T_mc80_ospi_calibration_data));
    p_calibration_data->channel = (uint8_t)channel;

    // Save DQS shift value from WRAPCFG register
    if (MC80_OSPI_DEVICE_NUMBER_0 == channel)
    {
      p_calibration_data->before_calibration.wrapcfg_dssft = (p_OSPI->WRAPCFG & OSPI_WRAPCFG_DSSFTCS0_Msk) >> OSPI_WRAPCFG_DSSFTCS0_Pos;
    }
    else
    {
      p_calibration_data->before_calibration.wrapcfg_dssft = (p_OSPI->WRAPCFG & OSPI_WRAPCFG_DSSFTCS1_Msk) >> OSPI_WRAPCFG_DSSFTCS1_Pos;
    }

    // Save SDR and DDR timing parameters from LIOCFGCS register
    uint32_t liocfg_value                                   = p_OSPI->LIOCFGCS[channel];
    p_calibration_data->before_calibration.liocfg_sdrsmpsft = (liocfg_value & OSPI_LIOCFGCSN_SDRSMPSFT_Msk) >> OSPI_LIOCFGCSN_SDRSMPSFT_Pos;
    p_calibration_data->before_calibration.liocfg_ddrsmpex  = (liocfg_value & OSPI_LIOCFGCSN_DDRSMPEX_Msk) >> OSPI_LIOCFGCSN_DDRSMPEX_Pos;

    // Save current calibration status
    p_calibration_data->before_calibration.casttcs_value    = p_OSPI->CASTTCS[channel];
  }

  const uint8_t command_bytes     = (uint8_t)p_cmd_set->command_bytes;
  uint16_t      read_command      = p_cmd_set->read_command;
  const uint8_t read_dummy_cycles = p_cmd_set->read_dummy_cycles;
  const uint8_t address_bytes     = (uint8_t)(p_cmd_set->address_bytes + 1U);

  // If using 1 command byte, shift the read command over as the peripheral expects
  if (1U == command_bytes)
  {
    read_command = (uint16_t)((read_command & MC80_OSPI_PRV_CDTBUF_CMD_1B_VALUE_MASK) << MC80_OSPI_PRV_CDTBUF_CMD_1B_VALUE_SHIFT);
  }

  p_OSPI->CCCTLCS[channel].CCCTL1 = (((uint32_t)command_bytes << OSPI_CCCTL1CSn_CACMDSIZE_Pos) & OSPI_CCCTL1CSn_CACMDSIZE_Msk) | (((uint32_t)address_bytes << OSPI_CCCTL1CSn_CAADDSIZE_Pos) & OSPI_CCCTL1CSn_CAADDSIZE_Msk) | (0xFU << OSPI_CCCTL1CSn_CADATASIZE_Pos) | (0U << OSPI_CCCTL1CSn_CAWRLATE_Pos) | (((uint32_t)read_dummy_cycles << OSPI_CCCTL1CSn_CARDLATE_Pos) & OSPI_CCCTL1CSn_CARDLATE_Msk);
  p_OSPI->CCCTLCS[channel].CCCTL2 = (uint32_t)read_command << OSPI_CCCTL2CSn_CARDCMD_Pos;

  // Set calibration address - use flash start address if preamble address is invalid
  uint32_t calibration_address    = (uint32_t)p_cfg_extend->p_autocalibration_preamble_pattern_addr;
  if (calibration_address == 0)
  {
    // Use start of flash memory if no specific pattern address is configured
    if (channel == MC80_OSPI_DEVICE_NUMBER_0)
    {
      calibration_address = 0x00000000;
    }
    else
    {
      calibration_address = 0x10000000;
    }
  }
  else
  {
    // Convert XIP address to physical flash address for calibration
    if (channel == MC80_OSPI_DEVICE_NUMBER_0)
    {
      if (calibration_address >= MC80_OSPI_DEVICE_0_START_ADDRESS)
      {
        calibration_address = calibration_address - MC80_OSPI_DEVICE_0_START_ADDRESS;
      }
    }
    else
    {
      if (calibration_address >= MC80_OSPI_DEVICE_1_START_ADDRESS)
      {
        calibration_address = calibration_address - MC80_OSPI_DEVICE_1_START_ADDRESS;
      }
    }
  }
  p_OSPI->CCCTLCS[channel].CCCTL3 = calibration_address;

  // Set preamble patterns from configuration or use defaults
  if (NULL != p_cfg_extend->p_autocalibration_preamble_patterns)
  {
    p_OSPI->CCCTLCS[channel].CCCTL4 = p_cfg_extend->p_autocalibration_preamble_patterns[0];  // Pattern 0
    p_OSPI->CCCTLCS[channel].CCCTL5 = p_cfg_extend->p_autocalibration_preamble_patterns[1];  // Pattern 1
    p_OSPI->CCCTLCS[channel].CCCTL6 = p_cfg_extend->p_autocalibration_preamble_patterns[2];  // Pattern 2
    p_OSPI->CCCTLCS[channel].CCCTL7 = p_cfg_extend->p_autocalibration_preamble_patterns[3];  // Pattern 3
  }
  else
  {
    // Use default patterns if none provided
    p_OSPI->CCCTLCS[channel].CCCTL4 = MC80_OSPI_DEFAULT_PREAMBLE_PATTERN_0;  // Pattern 0
    p_OSPI->CCCTLCS[channel].CCCTL5 = MC80_OSPI_DEFAULT_PREAMBLE_PATTERN_1;  // Pattern 1
    p_OSPI->CCCTLCS[channel].CCCTL6 = MC80_OSPI_DEFAULT_PREAMBLE_PATTERN_2;  // Pattern 2
    p_OSPI->CCCTLCS[channel].CCCTL7 = MC80_OSPI_DEFAULT_PREAMBLE_PATTERN_3;  // Pattern 3
  }

  // Configure auto-calibration parameters using defined constants
  p_OSPI->CCCTLCS[channel].CCCTL0 = (MC80_OSPI_CALIBRATION_INTERVAL_MAX << OSPI_CCCTL0CSn_CAITV_Pos) |          // CAITV: Interval between calibration patterns (2^(4+1) = 32 cycles)
                                    (MC80_OSPI_CALIBRATION_NO_OVERWRITE_ENABLE << OSPI_CCCTL0CSn_CANOWR_Pos) |  // CANOWR: No overwrite mode enabled
                                    (MC80_OSPI_CALIBRATION_SHIFT_START_MIN << OSPI_CCCTL0CSn_CASFTSTA_Pos) |    // CASFTSTA: Start shift value (0)
                                    (MC80_OSPI_CALIBRATION_SHIFT_END_MAX << OSPI_CCCTL0CSn_CASFTEND_Pos);       // CASFTEND: Maximum OM_DQS shift value

  // Start auto-calibration
  p_OSPI->CCCTLCS[channel].CCCTL0_b.CAEN = 1;

  // Wait for calibration to complete by checking interrupt flags with timeout
  uint32_t       timeout_count           = 0;
  const uint32_t CALIBRATION_TIMEOUT     = 100000;  // Timeout counter limit

  while ((0 == ((p_OSPI->INTS >> (OSPI_INTS_CASUCCS0_Pos + channel)) & 0x01)) &&
         (0 == ((p_OSPI->INTS >> (OSPI_INTS_CAFAILCS0_Pos + channel)) & 0x01)) &&
         (timeout_count < CALIBRATION_TIMEOUT))
  {
    __NOP();  // Breakpoint for calibration wait
    timeout_count++;
  }

  // Disable automatic calibration
  p_OSPI->CCCTLCS[channel].CCCTL0_b.CAEN = 0;

  // Check calibration result
  if (timeout_count >= CALIBRATION_TIMEOUT)
  {
    // Calibration timed out
    ret = FSP_ERR_CALIBRATE_FAILED;
  }
  else if (1 == ((p_OSPI->INTS >> (OSPI_INTS_CASUCCS0_Pos + channel)) & 0x01))
  {
    // Calibration succeeded
    ret          = FSP_SUCCESS;
    // Clear automatic calibration success status
    p_OSPI->INTC = (uint32_t)1 << (OSPI_INTS_CASUCCS0_Pos + channel);
  }
  else if (1 == ((p_OSPI->INTS >> (OSPI_INTS_CAFAILCS0_Pos + channel)) & 0x01))
  {
    // Calibration failed
    ret          = FSP_ERR_CALIBRATE_FAILED;
    // Clear automatic calibration failure status
    p_OSPI->INTC = (uint32_t)1 << (OSPI_INTS_CAFAILCS0_Pos + channel);
  }
  else
  {
    // Unexpected state - neither success nor failure flag set
    ret = FSP_ERR_CALIBRATE_FAILED;
  }

  // Save timing parameters after calibration if structure provided
  if (NULL != p_calibration_data)
  {
    // Set calibration result
    p_calibration_data->calibration_success = (FSP_SUCCESS == ret);

    // Save DQS shift value from WRAPCFG register
    if (MC80_OSPI_DEVICE_NUMBER_0 == channel)
    {
      p_calibration_data->after_calibration.wrapcfg_dssft = (p_OSPI->WRAPCFG & OSPI_WRAPCFG_DSSFTCS0_Msk) >> OSPI_WRAPCFG_DSSFTCS0_Pos;
    }
    else
    {
      p_calibration_data->after_calibration.wrapcfg_dssft = (p_OSPI->WRAPCFG & OSPI_WRAPCFG_DSSFTCS1_Msk) >> OSPI_WRAPCFG_DSSFTCS1_Pos;
    }

    // Save SDR and DDR timing parameters from LIOCFGCS register
    uint32_t liocfg_value                                  = p_OSPI->LIOCFGCS[channel];
    p_calibration_data->after_calibration.liocfg_sdrsmpsft = (liocfg_value & OSPI_LIOCFGCSN_SDRSMPSFT_Msk) >> OSPI_LIOCFGCSN_SDRSMPSFT_Pos;
    p_calibration_data->after_calibration.liocfg_ddrsmpex  = (liocfg_value & OSPI_LIOCFGCSN_DDRSMPEX_Msk) >> OSPI_LIOCFGCSN_DDRSMPEX_Pos;

    // Save calibration status after calibration
    p_calibration_data->after_calibration.casttcs_value    = p_OSPI->CASTTCS[channel];
  }

  return ret;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Read JEDEC identification data from flash device using RDID command (0x9F)
               Returns Manufacturer ID, Memory Type, and Memory Density

  Parameters: p_ctrl - Pointer to instance control structure
              p_id - Pointer to buffer for ID data (minimum 3 bytes)
              id_length - Number of ID bytes to read (typically 3)

  Return: FSP_SUCCESS - ID read successfully
          FSP_ERR_ASSERTION - Invalid parameters
          Error code from direct transfer operation
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_read_id(T_mc80_ospi_instance_ctrl *const p_ctrl, uint8_t *const p_id, uint32_t id_length)
{
  // Parameter validation
  if ((NULL == p_ctrl) || (NULL == p_id) || (0 == id_length) || (id_length > 8))
  {
    return FSP_ERR_ASSERTION;
  }

  // Ensure the instance is open
  if (MC80_OSPI_PRV_OPEN != p_ctrl->open)
  {
    return FSP_ERR_NOT_OPEN;
  }

  // Clear ID buffer
  for (uint32_t i = 0; i < id_length; i++)
  {
    p_id[i] = 0;
  }

  // Setup direct transfer structure for RDID command
  T_mc80_ospi_direct_transfer transfer = { 0 };
  transfer.command                     = MX25_CMD_RDID;       // 0x9F - Read Identification command
  transfer.command_length              = 1;                   // Single byte command
  transfer.address                     = 0;                   // No address for RDID
  transfer.address_length              = 0;                   // No address bytes
  transfer.data_length                 = (uint8_t)id_length;  // Number of ID bytes to read
  transfer.dummy_cycles                = 0;                   // No dummy cycles for RDID

  // Execute read command using direct transfer
  fsp_err_t err                        = Mc80_ospi_direct_transfer(p_ctrl, &transfer, MC80_OSPI_DIRECT_TRANSFER_DIR_READ);
  if (FSP_SUCCESS != err)
  {
    return err;
  }

  // Copy received data to output buffer
  for (uint32_t i = 0; i < id_length; i++)
  {
    p_id[i] = transfer.data_bytes[i];
  }

  return FSP_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Description: High-performance memory-mapped read from flash using DMA transfer

  This function performs optimized memory-mapped reading from OSPI flash memory using DMA
  for maximum performance. The function directly accesses the memory-mapped flash region
  and uses DMA to transfer data to the destination buffer.

  Operation sequence:
  1. Validates input parameters (pointers, size limits, alignment)
  2. Calculates memory-mapped address based on channel and input address
  3. Configures DMA for memory-to-memory transfer from flash to destination buffer
  4. Starts DMA transfer and waits for completion
  5. Returns success when transfer is complete

  Performance Benefits:
  - DMA handles data transfer without CPU intervention
  - Maximum throughput for large data transfers
  - CPU can perform other tasks during transfer
  - Hardware-optimized memory access patterns

  Memory-mapped Address Calculation:
  - Device 0 (Channel 0): Flash appears at 0x80000000 + flash_address
  - Device 1 (Channel 1): Flash appears at 0x90000000 + flash_address
  - Input address parameter should be the physical flash address (0x00000000-based)
  - Function automatically calculates the correct memory-mapped address

  DMA Configuration:
  - Source: Memory-mapped flash address
  - Destination: User-provided buffer
  - Transfer mode: Memory-to-memory
  - Transfer size: 1 byte increments
  - Completion: Waits for transfer completion

  Parameters: p_ctrl - Pointer to OSPI instance control structure
              p_dest - Destination buffer for read data
              address - Physical flash address to read from (0x00000000-based)
              bytes - Number of bytes to read

  Return: FSP_SUCCESS - Data read successfully
          FSP_ERR_ASSERTION - Invalid parameters
          FSP_ERR_NOT_OPEN - Driver not opened
          FSP_ERR_UNSUPPORTED - XIP support not enabled
          Error codes from DMA operations
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_memory_mapped_read(T_mc80_ospi_instance_ctrl* const p_ctrl, uint8_t* const p_dest, uint32_t const address, uint32_t const bytes)
{
  // Parameter validation
  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if ((NULL == p_ctrl) || (NULL == p_dest) || (0 == bytes))
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }
  }

  fsp_err_t err = FSP_SUCCESS;

  // Calculate memory-mapped address based on channel
  uint32_t memory_mapped_address;
  if (p_ctrl->channel == MC80_OSPI_DEVICE_NUMBER_0)
  {
    memory_mapped_address = MC80_OSPI_DEVICE_0_START_ADDRESS + address;
  }
  else
  {
    memory_mapped_address = MC80_OSPI_DEVICE_1_START_ADDRESS + address;
  }

  // Get DMA transfer instance from OSPI configuration
  T_mc80_ospi_extended_cfg const *p_cfg_extend = p_ctrl->p_cfg->p_extend;
  transfer_instance_t const      *p_transfer   = p_cfg_extend->p_lower_lvl_transfer;

  // Configure DMA for memory-to-memory transfer
  p_transfer->p_cfg->p_info->p_src                         = (void const *)memory_mapped_address;
  p_transfer->p_cfg->p_info->p_dest                        = p_dest;
  p_transfer->p_cfg->p_info->transfer_settings_word_b.size = TRANSFER_SIZE_1_BYTE;
  p_transfer->p_cfg->p_info->transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL;
  p_transfer->p_cfg->p_info->length                        = (uint16_t)bytes;

  // Reconfigure DMA with new settings
  err = p_transfer->p_api->reconfigure(p_transfer->p_ctrl, p_transfer->p_cfg->p_info);
  if (FSP_SUCCESS != err)
  {
    return err;
  }

  // Start DMA transfer
  err = p_transfer->p_api->softwareStart(p_transfer->p_ctrl, TRANSFER_START_MODE_REPEAT);
  if (FSP_SUCCESS != err)
  {
    return err;
  }

  // Wait for DMA transfer to complete with proper error handling
  volatile transfer_properties_t transfer_properties = { 0U };
  err = p_transfer->p_api->infoGet(p_transfer->p_ctrl, (transfer_properties_t *)&transfer_properties);
  if (FSP_SUCCESS != err)
  {
    return err;
  }
  while (FSP_SUCCESS == err && transfer_properties.transfer_length_remaining > 0)
  {
    err = p_transfer->p_api->infoGet(p_transfer->p_ctrl, (transfer_properties_t *)&transfer_properties);
    if (FSP_SUCCESS != err)
    {
      return err;
    }
  }

  return FSP_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Configures the device to enter or exit XiP mode

  NOTE: For the MX25UM25645G flash memory chip used in this project, XIP mode manipulation
  is not required. The chip can operate in continuous memory-mapped mode without explicit
  XIP enter/exit commands. This function is provided for compatibility but may not be
  necessary for normal operation with our memory chip.

  Parameters:
    p_ctrl - Pointer to the instance ctrl struct
    is_entering     - true if entering XiP mode, false if exiting

  Return:
    void
-----------------------------------------------------------------------------------------------------*/
static void _Mc80_ospi_xip(T_mc80_ospi_instance_ctrl *p_ctrl, bool is_entering)
{
  R_XSPI0_Type *const    p_reg = p_ctrl->p_reg;
  const T_mc80_ospi_cfg *p_cfg = p_ctrl->p_cfg;
  volatile uint8_t      *p_dummy_read_address;
  volatile uint8_t       dummy_read = 0;

  if (MC80_OSPI_DEVICE_NUMBER_0 == p_ctrl->channel)
  {
    p_dummy_read_address = (volatile uint8_t *)MC80_OSPI_DEVICE_0_START_ADDRESS;
  }
  else
  {
    p_dummy_read_address = (volatile uint8_t *)MC80_OSPI_DEVICE_1_START_ADDRESS;
  }

  // Clear the pre-fetch buffer for this bank so the next read is guaranteed to use the XiP code
  #if MC80_OSPI_CFG_PREFETCH_FUNCTION
  p_reg->BMCTL1 |= 0x03U << OSPI_BMCTL1_PBUFCLRCH0_Pos;
  #endif

  // Wait for any on-going access to complete
  while ((p_reg->COMSTT & MC80_OSPI_PRV_COMSTT_MEMACCCH_MASK) != 0)
  {
    __NOP();  // Breakpoint for access completion wait
  }

  if (is_entering)
  {
    // Change memory-mapping to read-only mode (preserve bits 4-7)
    p_reg->BMCTL0          = MC80_OSPI_PRV_BMCTL0_READ_ONLY_VALUE;

    // Configure XiP codes and enable
    const uint32_t cmctlch = OSPI_CMCTLCHn_XIPEN_Msk |
                             ((uint32_t)(p_cfg->xip_enter_command << OSPI_CMCTLCHn_XIPENCODE_Pos)) |
                             ((uint32_t)(p_cfg->xip_exit_command << OSPI_CMCTLCHn_XIPEXCODE_Pos));

    // XiP enter/exit codes are configured only for memory mapped operations and affects both OSPI slave channels
    p_reg->CMCTLCH[0] = cmctlch;
    p_reg->CMCTLCH[1] = cmctlch;

    // Perform a read to send the enter code. All further reads will use the enter code and will not send a read command code
    dummy_read        = *p_dummy_read_address;

    // Wait for the read to complete
    while ((p_reg->COMSTT & MC80_OSPI_PRV_COMSTT_MEMACCCH_MASK) != 0)
    {
      __NOP();  // Breakpoint for XIP enter read wait
    }
  }
  else
  {
    // Disable XiP
    p_reg->CMCTLCH[0] &= ~OSPI_CMCTLCHn_XIPEN_Msk;
    p_reg->CMCTLCH[1] &= ~OSPI_CMCTLCHn_XIPEN_Msk;

    // Perform a read to send the exit code. All further reads will not send an exit code
    dummy_read = *p_dummy_read_address;

    // Wait for the read to complete
    while ((p_reg->COMSTT & MC80_OSPI_PRV_COMSTT_MEMACCCH_MASK) != 0)
    {
      __NOP();  // Breakpoint for XIP exit read wait
    }

    // Change memory-mapping back to R/W mode (preserve bits 4-7)
    p_reg->BMCTL0 = MC80_OSPI_PRV_BMCTL0_READ_WRITE_VALUE;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Performs hardware reset of OSPI flash memory using LIOCTL register RSTCS0 bit
               Asserts reset for minimum 20us, then deasserts and waits 100us for initialization

  Parameters: p_ctrl - Pointer to OSPI instance control structure

  Return: FSP_SUCCESS - Reset completed successfully
          FSP_ERR_ASSERTION - Invalid parameters
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_hardware_reset(T_mc80_ospi_instance_ctrl* const p_ctrl)
{
   R_XSPI0_Type *const    p_ospi = p_ctrl->p_reg;

  // Assert reset by clearing RSTCS0 bit (bit 16) in LIOCTL register
  p_ospi->LIOCTL &= ~(1U << 16);

  // Wait minimum 20us reset pulse width
  R_BSP_SoftwareDelay(20, BSP_DELAY_UNITS_MICROSECONDS);

  // Deassert reset by setting RSTCS0 bit (bit 16) in LIOCTL register
  p_ospi->LIOCTL |= (1U << 16);

  // Wait 100us for flash memory initialization
  R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MICROSECONDS);

  return FSP_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  DMA callback function for OSPI operations

  This callback is called when DMA transfer completes and sends RTOS event notifications to
  wake up waiting tasks. The callback is safe to call from interrupt context as it only
  sends RTOS notifications.

  Parameters:
    p_args - Callback arguments (contains only p_context)

  Return:
    void
-----------------------------------------------------------------------------------------------------*/
static void _Ospi_dma_callback(dmac_callback_args_t *p_args)
{
  // Send RTOS event notification if event flags are initialized
  if (g_ospi_dma_event_flags_initialized)
  {
    // Set completion event flag - this will wake up any waiting tasks
    tx_event_flags_set(&g_ospi_dma_event_flags, OSPI_DMA_EVENT_TRANSFER_COMPLETE, TX_OR);
  }
}

/*-----------------------------------------------------------------------------------------------------
  Reset DMA transfer status flags

  Parameters:
    None

  Return:
    void
-----------------------------------------------------------------------------------------------------*/
void Mc80_ospi_dma_transfer_reset_flags(void)
{
  // Clear RTOS event flags if initialized
  if (g_ospi_dma_event_flags_initialized)
  {
    tx_event_flags_set(&g_ospi_dma_event_flags, ~OSPI_DMA_EVENT_ALL_EVENTS, TX_AND);
  }
}

/*-----------------------------------------------------------------------------------------------------
  Initialize RTOS event flags for DMA notifications

  This function must be called after RTOS kernel initialization but before using DMA operations.
  It creates the event flags group used for task synchronization with DMA completion.

  Parameters:
    None

  Return:
    FSP_SUCCESS - Event flags initialized successfully
    FSP_ERR_INTERNAL - Failed to create event flags group
-----------------------------------------------------------------------------------------------------*/
static fsp_err_t _Ospi_dma_event_flags_initialize(void)
{
  if (!g_ospi_dma_event_flags_initialized)
  {
    UINT tx_result = tx_event_flags_create(&g_ospi_dma_event_flags, "OSPI_DMA_Events");
    if (TX_SUCCESS == tx_result)
    {
      g_ospi_dma_event_flags_initialized = true;
      return FSP_SUCCESS;
    }
    else
    {
      return FSP_ERR_INTERNAL;
    }
  }

  return FSP_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Cleanup RTOS resources for OSPI DMA operations

  This internal function cleans up the RTOS event flags group when the driver is closed.
  It should only be called when no other tasks are using the OSPI driver.

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Ospi_dma_event_flags_cleanup(void)
{
  if (g_ospi_dma_event_flags_initialized)
  {
    tx_event_flags_delete(&g_ospi_dma_event_flags);
    g_ospi_dma_event_flags_initialized = false;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Get RTOS event flags group for DMA operations

  This function provides access to the event flags group for tasks that need to wait for
  DMA completion. Tasks can use tx_event_flags_get() to wait for specific events.

  Example usage in task:
    ULONG actual_flags;
    UINT result = tx_event_flags_get(&flags_group, OSPI_DMA_EVENT_TRANSFER_COMPLETE,
                                     TX_OR_CLEAR, &actual_flags, TX_WAIT_FOREVER);

  Parameters:
    pp_event_flags - Pointer to store the event flags group pointer

  Return:
    FSP_SUCCESS - Event flags group is available
    FSP_ERR_NOT_INITIALIZED - Event flags not initialized, call Mc80_ospi_dma_event_flags_initialize() first
    FSP_ERR_ASSERTION - Invalid parameter
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_dma_get_event_flags(TX_EVENT_FLAGS_GROUP **pp_event_flags)
{
  if (NULL == pp_event_flags)
  {
    return FSP_ERR_ASSERTION;
  }

  if (!g_ospi_dma_event_flags_initialized)
  {
    return FSP_ERR_NOT_INITIALIZED;
  }

  *pp_event_flags = &g_ospi_dma_event_flags;
  return FSP_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Wait for DMA transfer completion using RTOS event flags

  This function provides a blocking wait for DMA transfer completion using RTOS synchronization.
  It's more efficient than polling and allows the task to be suspended until the transfer completes.

  Parameters:
    timeout_ticks - Maximum time to wait in RTOS ticks (TX_WAIT_FOREVER for infinite wait)

  Return:
    FSP_SUCCESS - Transfer completed successfully
    FSP_ERR_TIMEOUT - Timeout occurred before transfer completion
    FSP_ERR_NOT_INITIALIZED - RTOS support not initialized
    FSP_ERR_INTERNAL - RTOS error occurred
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_dma_wait_for_completion(ULONG timeout_ticks)
{
  if (!g_ospi_dma_event_flags_initialized)
  {
    return FSP_ERR_NOT_INITIALIZED;
  }

  ULONG actual_flags;
  UINT tx_result = tx_event_flags_get(&g_ospi_dma_event_flags,
                                      OSPI_DMA_EVENT_TRANSFER_COMPLETE,
                                      TX_OR_CLEAR,
                                      &actual_flags,
                                      timeout_ticks);

  if (TX_SUCCESS == tx_result)
  {
    return FSP_SUCCESS;
  }
  else if (TX_NO_EVENTS == tx_result)
  {
    return FSP_ERR_TIMEOUT;
  }
  else
  {
    return FSP_ERR_INTERNAL;
  }
}
