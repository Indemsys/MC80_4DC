/*-----------------------------------------------------------------------------------------------------
  MC80 OSPI driver implementation - refactored from Renesas FSP r_ospi_b driver
-----------------------------------------------------------------------------------------------------*/

#include "App.h"

/*-----------------------------------------------------------------------------------------------------
  Macro definitions
-----------------------------------------------------------------------------------------------------*/

// "MC80" in ASCII. Used to determine if the control block is open
#define MC80_OSPI_PRV_OPEN                                  (0x4D433830U)  // "MC80"

#define MC80_OSPI_PRV_CHANNELS_PER_UNIT                     (2U)
#define MC80_OSPI_PRV_UNIT_CHANNELS_SHIFT                   (MC80_OSPI_PRV_CHANNELS_PER_UNIT)
#define MC80_OSPI_PRV_UNIT_CHANNELS_MASK                    ((1U << MC80_OSPI_PRV_UNIT_CHANNELS_SHIFT) - 1U)

// Mask of all channels for a given OSPI unit
#define MC80_OSPI_PRV_UNIT_MASK(p_ext_cfg)                  (MC80_OSPI_PRV_UNIT_CHANNELS_MASK << (((p_ext_cfg)->ospi_unit) * MC80_OSPI_PRV_UNIT_CHANNELS_SHIFT))

// Individual bit mask for a single channel on a given OSPI unit
#define MC80_OSPI_PRV_CH_MASK(p_ext_cfg)                    ((1U << ((p_ext_cfg)->channel)) << (((p_ext_cfg)->ospi_unit) * MC80_OSPI_PRV_UNIT_CHANNELS_SHIFT))

// Gets the extended configuration struct for this instance
#define MC80_OSPI_PRV_EXTENDED_CFG(p_ctrl)                  ((T_mc80_ospi_extended_cfg *)((T_mc80_ospi_instance_ctrl *)(p_ctrl))->p_cfg->p_extend)

// Indicates the provided protocol mode requires the Data-Strobe signal
#define MC80_OSPI_PRV_PROTOCOL_USES_DS_SIGNAL(protocol)     ((bool)(((uint32_t)(protocol)) & 0x200UL))

// Number of bytes combined into a single transaction for memory-mapped writes
#define MC80_OSPI_PRV_COMBINATION_WRITE_LENGTH              (2U * ((uint8_t)MC80_OSPI_CFG_COMBINATION_FUNCTION + 1U))

// Converts spi_flash_address_bytes_t to a register compatible length value
#define MC80_OSPI_PRV_ADDR_BYTES_TO_LENGTH(spi_flash_bytes) ((uint8_t)((spi_flash_bytes) + 1U))

#define MC80_OSPI_PRV_BMCTL_DEFAULT_VALUE                   (0x0C)

#define MC80_OSPI_PRV_CMCFG_1BYTE_VALUE_MASK                (0xFF00U)
#define MC80_OSPI_PRV_CMCFG_2BYTE_VALUE_MASK                (0xFFFFU)

#define MC80_OSPI_PRV_AUTOCALIBRATION_DATA_SIZE             (0xFU)
#define MC80_OSPI_PRV_AUTOCALIBRATION_LATENCY_CYCLES        (0U)

#define MC80_OSPI_PRV_ADDRESS_REPLACE_VALUE                 (0xF0U)
#define MC80_OSPI_PRV_ADDRESS_REPLACE_ENABLE_BITS           (MC80_OSPI_PRV_ADDRESS_REPLACE_VALUE << R_XSPI0_CMCFGCS_CMCFG0_ADDRPEN_Pos)
#define MC80_OSPI_PRV_ADDRESS_REPLACE_MASK                  (~(MC80_OSPI_PRV_ADDRESS_REPLACE_VALUE << 24))

#define MC80_OSPI_PRV_WORD_ACCESS_SIZE                      (4U)
#define MC80_OSPI_PRV_HALF_WORD_ACCESS_SIZE                 (2U)

#define MC80_OSPI_PRV_DIRECT_ADDR_AND_DATA_MASK             (7U)
#define MC80_OSPI_PRV_PAGE_SIZE_BYTES                       (256U)

#define MC80_OSPI_PRV_DIRECT_CMD_SIZE_MASK                  (0x3U)

#define MC80_OSPI_PRV_CDTBUF_CMD_OFFSET                     (16U)
#define MC80_OSPI_PRV_CDTBUF_CMD_UPPER_OFFSET               (24U)
#define MC80_OSPI_PRV_CDTBUF_CMD_1B_VALUE_MASK              (0xFFU)
#define MC80_OSPI_PRV_CDTBUF_CMD_1B_VALUE_SHIFT             (8U)
#define MC80_OSPI_PRV_CDTBUF_CMD_2B_VALUE_MASK              (0xFFFFU)

#define MC80_OSPI_PRV_BMCTL0_DISABLED_VALUE                 (0x00)  // 0b0000'0000
#define MC80_OSPI_PRV_BMCTL0_READ_ONLY_VALUE                (0x55)  // 0b0101'0101
#define MC80_OSPI_PRV_BMCTL0_WRITE_ONLY_VALUE               (0xAA)  // 0b1010'1010
#define MC80_OSPI_PRV_BMCTL0_READ_WRITE_VALUE               (0xFF)  // 0b1111'1111

#define MC80_OSPI_PRV_BMCTL1_CLEAR_PREFETCH_MASK            (0x03 << R_XSPI0_BMCTL1_PBUFCLRCH_Pos)
#define MC80_OSPI_PRV_BMCTL1_PUSH_COMBINATION_WRITE_MASK    (0x03 << R_XSPI0_BMCTL1_MWRPUSHCH_Pos)

#define MC80_OSPI_PRV_COMSTT_MEMACCCH_MASK                  (0x03 << R_XSPI0_COMSTT_MEMACCCH_Pos)

#define MC80_OSPI_SOFTWARE_DELAY                            (50U)

// These are used as modulus checking, make sure they are powers of 2
#define MC80_OSPI_PRV_CPU_ACCESS_LENGTH                     (8U)
#define MC80_OSPI_PRV_CPU_ACCESS_ALIGNMENT                  (8U)

#define MC80_OSPI_PRV_PROTOCOL_USES_DS_MASK                 (0x200U)

#define MC80_OSPI_PRV_UINT32_BITS                           (32)

#define MC80_OSPI_MAX_WRITE_ENABLE_LOOPS                    (5)

// Number of address bytes in 4 byte address mode
#define MC80_OSPI_4_BYTE_ADDRESS                            (4U)

/*-----------------------------------------------------------------------------------------------------
  Static function prototypes
-----------------------------------------------------------------------------------------------------*/
static bool                                _Mc80_ospi_status_sub(T_mc80_ospi_instance_ctrl *p_instance_ctrl, uint8_t bit_pos);
static fsp_err_t                           _Mc80_ospi_protocol_specific_settings(T_mc80_ospi_instance_ctrl *p_instance_ctrl);
static fsp_err_t                           _Mc80_ospi_write_enable(T_mc80_ospi_instance_ctrl *p_instance_ctrl);
static void                                _Mc80_ospi_direct_transfer(T_mc80_ospi_instance_ctrl *p_instance_ctrl, spi_flash_direct_transfer_t *const p_transfer, spi_flash_direct_transfer_dir_t direction);
static T_mc80_ospi_xspi_command_set const *_Mc80_ospi_command_set_get(T_mc80_ospi_instance_ctrl *p_instance_ctrl);

#if MC80_OSPI_CFG_AUTOCALIBRATION_SUPPORT_ENABLE
static fsp_err_t _Mc80_ospi_automatic_calibration_seq(T_mc80_ospi_instance_ctrl *p_instance_ctrl);
#endif

#if MC80_OSPI_CFG_XIP_SUPPORT_ENABLE
static void _Mc80_ospi_xip(T_mc80_ospi_instance_ctrl *p_instance_ctrl, bool is_entering);
#endif

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
fsp_err_t Mc80_ospi_open(spi_flash_ctrl_t *const p_ctrl, spi_flash_cfg_t const *const p_cfg)
{
  T_mc80_ospi_instance_ctrl            *p_instance_ctrl = (T_mc80_ospi_instance_ctrl *)p_ctrl;
  fsp_err_t                             ret             = FSP_SUCCESS;
  const T_mc80_ospi_extended_cfg *const p_cfg_extend    = (T_mc80_ospi_extended_cfg *)(p_cfg->p_extend);

  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_instance_ctrl || NULL == p_cfg || NULL == p_cfg->p_extend)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN == p_instance_ctrl->open)
    {
      return FSP_ERR_ALREADY_OPEN;
    }
  }

  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (!(BSP_PERIPHERAL_OSPI_B_CHANNEL_MASK & (1U << p_cfg_extend->ospi_unit)) ||
        (0 != (g_mc80_ospi_channels_open_flags & MC80_OSPI_PRV_CH_MASK(p_cfg_extend))))
    {
      return (!(BSP_PERIPHERAL_OSPI_B_CHANNEL_MASK & (1U << p_cfg_extend->ospi_unit))) ? FSP_ERR_ASSERTION : FSP_ERR_ALREADY_OPEN;
    }
  }

  R_XSPI0_Type *p_reg = (R_XSPI0_Type *)((uint32_t)R_XSPI0 + p_cfg_extend->ospi_unit * ((uint32_t)R_XSPI1 - (uint32_t)R_XSPI0));

  // Enable clock to the xSPI block
  R_BSP_MODULE_START(FSP_IP_OSPI, p_cfg_extend->ospi_unit);

  // Initialize control block
  p_instance_ctrl->p_cfg                = p_cfg;
  p_instance_ctrl->p_reg                = p_reg;
  p_instance_ctrl->spi_protocol         = p_cfg->spi_protocol;
  p_instance_ctrl->channel              = p_cfg_extend->channel;
  p_instance_ctrl->ospi_unit            = p_cfg_extend->ospi_unit;

  // Initialize transfer instance
  transfer_instance_t const *p_transfer = p_cfg_extend->p_lower_lvl_transfer;

  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE && NULL == p_transfer)
  {
    return FSP_ERR_ASSERTION;
  }

  p_transfer->p_api->open(p_transfer->p_ctrl, p_transfer->p_cfg);

  // Disable memory-mapping for this slave. It will be enabled later on after initialization
  if (MC80_OSPI_DEVICE_NUMBER_0 == p_instance_ctrl->channel)
  {
    p_reg->BMCTL0 &= ~(R_XSPI0_BMCTL0_CH0CS0ACC_Msk | R_XSPI0_BMCTL0_CH1CS0ACC_Msk);
  }
  else
  {
    p_reg->BMCTL0 &= ~(R_XSPI0_BMCTL0_CH0CS1ACC_Msk | R_XSPI0_BMCTL0_CH1CS1ACC_Msk);
  }

  // Perform xSPI Initial configuration as described in hardware manual
  // Set xSPI protocol mode
  uint32_t liocfg                        = ((uint32_t)p_cfg->spi_protocol) << R_XSPI0_LIOCFGCS_PRTMD_Pos;
  p_reg->LIOCFGCS[p_cfg_extend->channel] = liocfg;

  // Set xSPI drive/sampling timing
  if (MC80_OSPI_DEVICE_NUMBER_0 == p_instance_ctrl->channel)
  {
    p_reg->WRAPCFG = ((uint32_t)p_cfg_extend->data_latch_delay_clocks << R_XSPI0_WRAPCFG_DSSFTCS0_Pos) & R_XSPI0_WRAPCFG_DSSFTCS0_Msk;
  }
  else
  {
    p_reg->WRAPCFG = ((uint32_t)p_cfg_extend->data_latch_delay_clocks << R_XSPI0_WRAPCFG_DSSFTCS1_Pos) & R_XSPI0_WRAPCFG_DSSFTCS1_Msk;
  }

  // Set minimum cycles between xSPI frames
  liocfg |= ((uint32_t)p_cfg_extend->p_timing_settings->command_to_command_interval << R_XSPI0_LIOCFGCS_CSMIN_Pos) & R_XSPI0_LIOCFGCS_CSMIN_Msk;

  // Set CS asserting extension in cycles
  liocfg |= ((uint32_t)p_cfg_extend->p_timing_settings->cs_pulldown_lead << R_XSPI0_LIOCFGCS_CSASTEX_Pos) & R_XSPI0_LIOCFGCS_CSASTEX_Msk;

  // Set CS releasing extension in cycles
  liocfg |= ((uint32_t)p_cfg_extend->p_timing_settings->cs_pullup_lag << R_XSPI0_LIOCFGCS_CSNEGEX_Pos) & R_XSPI0_LIOCFGCS_CSNEGEX_Msk;

  // Set SDR and DDR timing
  liocfg |= ((uint32_t)p_cfg_extend->p_timing_settings->sdr_drive_timing << R_XSPI0_LIOCFGCS_SDRDRV_Pos) & R_XSPI0_LIOCFGCS_SDRDRV_Msk;
  liocfg |= ((uint32_t)p_cfg_extend->p_timing_settings->sdr_sampling_edge << R_XSPI0_LIOCFGCS_SDRSMPMD_Pos) & R_XSPI0_LIOCFGCS_SDRSMPMD_Msk;
  liocfg |= ((uint32_t)p_cfg_extend->p_timing_settings->sdr_sampling_delay << R_XSPI0_LIOCFGCS_SDRSMPSFT_Pos) & R_XSPI0_LIOCFGCS_SDRSMPSFT_Msk;
  liocfg |= ((uint32_t)p_cfg_extend->p_timing_settings->ddr_sampling_extension << R_XSPI0_LIOCFGCS_DDRSMPEX_Pos) & R_XSPI0_LIOCFGCS_DDRSMPEX_Msk;

  // Set xSPI CSn signal timings
  p_reg->LIOCFGCS[p_cfg_extend->channel] = liocfg;

  // Set xSPI memory-mapping operation
  ret                                    = _Mc80_ospi_protocol_specific_settings(p_instance_ctrl);

  // Return response after issuing write transaction to xSPI bus, Enable prefetch function and combination if desired
  const uint32_t bmcfgch                 = (0 << R_XSPI0_BMCFGCH_WRMD_Pos) |
                           ((MC80_OSPI_CFG_COMBINATION_FUNCTION << R_XSPI0_BMCFGCH_MWRCOMB_Pos) &
                            (R_XSPI0_BMCFGCH_MWRCOMB_Msk | R_XSPI0_BMCFGCH_MWRSIZE_Msk)) |
                           ((MC80_OSPI_CFG_PREFETCH_FUNCTION << R_XSPI0_BMCFGCH_PREEN_Pos) &
                            R_XSPI0_BMCFGCH_PREEN_Msk);

  // Both of these should have the same configuration and it affects all OSPI slave channels
  p_reg->BMCFGCH[0] = bmcfgch;
  p_reg->BMCFGCH[1] = bmcfgch;

  // Re-activate memory-mapped mode in Read/Write
  if (0 == p_instance_ctrl->channel)
  {
    p_reg->BMCTL0 |= R_XSPI0_BMCTL0_CH0CS0ACC_Msk | R_XSPI0_BMCTL0_CH1CS0ACC_Msk;
  }
  else
  {
    p_reg->BMCTL0 |= R_XSPI0_BMCTL0_CH0CS1ACC_Msk | R_XSPI0_BMCTL0_CH1CS1ACC_Msk;
  }

  if (FSP_SUCCESS == ret)
  {
    p_instance_ctrl->open = MC80_OSPI_PRV_OPEN;
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
  Writes raw data directly to the OctaFlash. API not supported. Use Mc80_ospi_direct_transfer

  Parameters:
    p_ctrl           - Pointer to the control structure
    p_src            - Source data buffer
    bytes            - Number of bytes to write
    read_after_write - Read after write flag

  Return:
    FSP_ERR_UNSUPPORTED - API not supported by OSPI
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_direct_write(spi_flash_ctrl_t    *p_ctrl,
                                 uint8_t const *const p_src,
                                 uint32_t const       bytes,
                                 bool const           read_after_write)
{
  return FSP_ERR_UNSUPPORTED;
}

/*-----------------------------------------------------------------------------------------------------
  Reads raw data directly from the OctaFlash. API not supported. Use Mc80_ospi_direct_transfer.

  Parameters:
    p_ctrl - Pointer to the control structure
    p_dest - Destination data buffer
    bytes  - Number of bytes to read

  Return:
    FSP_ERR_UNSUPPORTED - API not supported by OSPI
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_direct_read(spi_flash_ctrl_t *p_ctrl, uint8_t *const p_dest, uint32_t const bytes)
{
  return FSP_ERR_UNSUPPORTED;
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
fsp_err_t Mc80_ospi_direct_transfer(spi_flash_ctrl_t *p_ctrl, spi_flash_direct_transfer_t *const p_transfer, spi_flash_direct_transfer_dir_t direction)
{
  T_mc80_ospi_instance_ctrl *p_instance_ctrl = (T_mc80_ospi_instance_ctrl *)p_ctrl;

  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_instance_ctrl || NULL == p_transfer || 0 == p_transfer->command_length)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_instance_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }
  }

  _Mc80_ospi_direct_transfer(p_instance_ctrl, p_transfer, direction);

  return FSP_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Enters XIP (execute in place) mode.

  Parameters:
    p_ctrl - Pointer to the control structure

  Return:
    FSP_SUCCESS        - XiP mode was entered successfully
    FSP_ERR_ASSERTION  - A required pointer is NULL
    FSP_ERR_NOT_OPEN   - Driver is not opened
    FSP_ERR_UNSUPPORTED- XiP support is not enabled
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_xip_enter(spi_flash_ctrl_t *p_ctrl)
{
  if (MC80_OSPI_CFG_XIP_SUPPORT_ENABLE)
  {
    T_mc80_ospi_instance_ctrl *p_instance_ctrl = (T_mc80_ospi_instance_ctrl *)p_ctrl;

    if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
    {
      if (NULL == p_instance_ctrl || NULL == p_instance_ctrl->p_cfg)
      {
        return FSP_ERR_ASSERTION;
      }
      if (MC80_OSPI_PRV_OPEN != p_instance_ctrl->open)
      {
        return FSP_ERR_NOT_OPEN;
      }
    }

    // Enter XIP mode
    _Mc80_ospi_xip(p_instance_ctrl, true);

    return FSP_SUCCESS;
  }
  else
  {
    return FSP_ERR_UNSUPPORTED;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Exits XIP (execute in place) mode.

  Parameters:
    p_ctrl - Pointer to the control structure

  Return:
    FSP_SUCCESS        - XiP mode was exited successfully
    FSP_ERR_ASSERTION  - A required pointer is NULL
    FSP_ERR_NOT_OPEN   - Driver is not opened
    FSP_ERR_UNSUPPORTED- XiP support is not enabled
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_xip_exit(spi_flash_ctrl_t *p_ctrl)
{
  if (MC80_OSPI_CFG_XIP_SUPPORT_ENABLE)
  {
    T_mc80_ospi_instance_ctrl *p_instance_ctrl = (T_mc80_ospi_instance_ctrl *)p_ctrl;

    if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
    {
      if (NULL == p_instance_ctrl || NULL == p_instance_ctrl->p_cfg)
      {
        return FSP_ERR_ASSERTION;
      }
      if (MC80_OSPI_PRV_OPEN != p_instance_ctrl->open)
      {
        return FSP_ERR_NOT_OPEN;
      }
    }

    // Exit XIP mode
    _Mc80_ospi_xip(p_instance_ctrl, false);

    return FSP_SUCCESS;
  }
  else
  {
    return FSP_ERR_UNSUPPORTED;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Program a page of data to the flash.

  Parameters:
    p_ctrl     - Pointer to the control structure
    p_src      - Source data buffer
    p_dest     - Destination address in flash
    byte_count - Number of bytes to write

  Return:
    FSP_SUCCESS            - The flash was programmed successfully
    FSP_ERR_ASSERTION      - p_instance_ctrl, p_dest or p_src is NULL, or byte_count crosses a page boundary
    FSP_ERR_NOT_OPEN       - Driver is not opened
    FSP_ERR_INVALID_SIZE   - Insufficient space remaining in page or write length is not a multiple of CPU access size when not using the DMAC
    FSP_ERR_DEVICE_BUSY    - Another Write/Erase transaction is in progress
    FSP_ERR_WRITE_FAILED   - Write operation failed
    FSP_ERR_INVALID_ADDRESS- Destination or source is not aligned to CPU access alignment when not using the DMAC
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_write(spi_flash_ctrl_t *p_ctrl, uint8_t const *const p_src, uint8_t *const p_dest, uint32_t byte_count)
{
  T_mc80_ospi_instance_ctrl *p_instance_ctrl = (T_mc80_ospi_instance_ctrl *)p_ctrl;
  fsp_err_t                  err             = FSP_SUCCESS;

  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl || NULL == p_src || NULL == p_dest || 0 == byte_count)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_instance_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }

    // Check that space remaining in page is sufficient for requested write size
    uint32_t page_size   = p_instance_ctrl->p_cfg->page_size_bytes;
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

  R_XSPI0_Type *const p_reg = p_instance_ctrl->p_reg;

  if (true == _Mc80_ospi_status_sub(p_instance_ctrl, p_instance_ctrl->p_cfg->write_status_bit))
  {
    return FSP_ERR_DEVICE_BUSY;
  }

  // Setup and start DMAC transfer
  T_mc80_ospi_extended_cfg  *p_cfg_extend                  = MC80_OSPI_PRV_EXTENDED_CFG(p_instance_ctrl);
  transfer_instance_t const *p_transfer                    = p_cfg_extend->p_lower_lvl_transfer;

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

  _Mc80_ospi_write_enable(p_instance_ctrl);

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
  Erase a block or sector of flash. The byte_count must exactly match one of the erase sizes defined in spi_flash_cfg_t.
  For chip erase, byte_count must be SPI_FLASH_ERASE_SIZE_CHIP_ERASE.

  Parameters:
    p_ctrl           - Pointer to the control structure
    p_device_address - Address in flash to erase
    byte_count       - Number of bytes to erase

  Return:
    FSP_SUCCESS         - The command to erase the flash was executed successfully
    FSP_ERR_ASSERTION   - p_instance_ctrl or p_device_address is NULL, byte_count doesn't match an erase size defined in spi_flash_cfg_t, or byte_count is set to 0
    FSP_ERR_NOT_OPEN    - Driver is not opened
    FSP_ERR_DEVICE_BUSY - The device is busy
    FSP_ERR_WRITE_FAILED- Write operation failed
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_erase(spi_flash_ctrl_t *p_ctrl, uint8_t *const p_device_address, uint32_t byte_count)
{
  T_mc80_ospi_instance_ctrl *p_instance_ctrl = (T_mc80_ospi_instance_ctrl *)p_ctrl;

  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl || NULL == p_device_address || 0 == byte_count)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_instance_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }
  }

  spi_flash_cfg_t const *p_cfg                  = p_instance_ctrl->p_cfg;
  uint16_t               erase_command          = 0;
  const uint32_t         chip_address_base      = p_instance_ctrl->channel ? BSP_FEATURE_OSPI_B_DEVICE_1_START_ADDRESS : BSP_FEATURE_OSPI_B_DEVICE_0_START_ADDRESS;
  uint32_t               chip_address           = (uint32_t)p_device_address - chip_address_base;
  bool                   send_address           = true;

  T_mc80_ospi_xspi_command_set const *p_cmd_set = p_instance_ctrl->p_cmd_set;

  if (true == _Mc80_ospi_status_sub(p_instance_ctrl, p_cfg->write_status_bit))
  {
    return FSP_ERR_DEVICE_BUSY;
  }

  // Select the appropriate erase command from the command set
  spi_flash_erase_command_t const *p_erase_list      = p_cmd_set->p_erase_commands->p_table;
  const uint8_t                    erase_list_length = p_cmd_set->p_erase_commands->length;

  for (uint32_t index = 0; index < erase_list_length; index++)
  {
    // If requested byte_count is supported by underlying flash, store the command
    if (byte_count == p_erase_list[index].size)
    {
      if (SPI_FLASH_ERASE_SIZE_CHIP_ERASE == byte_count)
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

  fsp_err_t err = _Mc80_ospi_write_enable(p_instance_ctrl);
  if (FSP_SUCCESS != err)
  {
    return err;
  }

  spi_flash_direct_transfer_t direct_command = {
    .command        = erase_command,
    .command_length = (uint8_t)p_cmd_set->command_bytes,
    .address        = chip_address,
    .address_length = (send_address) ? MC80_OSPI_PRV_ADDR_BYTES_TO_LENGTH(p_cmd_set->address_bytes) : 0U,
    .data_length    = 0,
  };

  _Mc80_ospi_direct_transfer(p_instance_ctrl, &direct_command, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);

  // If prefetch is enabled, make sure the banks aren't being used and flush the prefetch caches after an erase
  if (MC80_OSPI_CFG_PREFETCH_FUNCTION)
  {
    R_XSPI0_Type *const p_reg = p_instance_ctrl->p_reg;
    while ((p_reg->COMSTT & MC80_OSPI_PRV_COMSTT_MEMACCCH_MASK) != 0)
    {
      __NOP(); // Breakpoint for memory access wait
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
    FSP_ERR_ASSERTION - p_instance_ctrl or p_status is NULL
    FSP_ERR_NOT_OPEN  - Driver is not opened
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_status_get(spi_flash_ctrl_t *p_ctrl, spi_flash_status_t *const p_status)
{
  T_mc80_ospi_instance_ctrl *p_instance_ctrl = (T_mc80_ospi_instance_ctrl *)p_ctrl;

  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl || NULL == p_status)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_instance_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }
  }

  // Read device status
  p_status->write_in_progress = _Mc80_ospi_status_sub(p_instance_ctrl, p_instance_ctrl->p_cfg->write_status_bit);

  return FSP_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Selects the bank to access. Use ospi_b_bank_select_t as the bank value.

  Parameters:
    p_ctrl - Pointer to the control structure
    bank   - Bank value

  Return:
    FSP_ERR_UNSUPPORTED - This function is unsupported
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_bank_set(spi_flash_ctrl_t *p_ctrl, uint32_t bank)
{
  return FSP_ERR_UNSUPPORTED;
}

/*-----------------------------------------------------------------------------------------------------
  Sets the SPI protocol.

  Parameters:
    p_ctrl       - Pointer to the control structure
    spi_protocol - SPI protocol to set

  Return:
    FSP_SUCCESS              - SPI protocol updated on MPU peripheral
    FSP_ERR_ASSERTION        - A required pointer is NULL
    FSP_ERR_NOT_OPEN         - Driver is not opened
    FSP_ERR_CALIBRATE_FAILED - Failed to perform auto-calibrate
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_spi_protocol_set(spi_flash_ctrl_t *p_ctrl, spi_flash_protocol_t spi_protocol)
{
  T_mc80_ospi_instance_ctrl *p_instance_ctrl = (T_mc80_ospi_instance_ctrl *)p_ctrl;

  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_instance_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }
  }

  // Save the old protocol in case of an undefined command set
  spi_flash_protocol_t old_protocol = p_instance_ctrl->spi_protocol;
  p_instance_ctrl->spi_protocol     = spi_protocol;

  // Update the SPI protocol and its associated registers
  fsp_err_t err                     = _Mc80_ospi_protocol_specific_settings(p_instance_ctrl);

  if (FSP_ERR_INVALID_MODE == err)
  {
    // Restore the original spi protocol. Nothing else has been changed in this case
    p_instance_ctrl->spi_protocol = old_protocol;
  }

  return err;
}

/*-----------------------------------------------------------------------------------------------------
  Close the OSPI driver module.

  Parameters:
    p_ctrl - Pointer to the control structure

  Return:
    FSP_SUCCESS       - Configuration was successful
    FSP_ERR_ASSERTION - p_instance_ctrl is NULL
    FSP_ERR_NOT_OPEN  - Driver is not opened
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_close(spi_flash_ctrl_t *p_ctrl)
{
  T_mc80_ospi_instance_ctrl *p_instance_ctrl = (T_mc80_ospi_instance_ctrl *)p_ctrl;
  fsp_err_t                  err             = FSP_SUCCESS;

  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl || NULL == p_instance_ctrl->p_cfg || NULL == p_instance_ctrl->p_cfg->p_extend)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_instance_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }
  }

  T_mc80_ospi_extended_cfg *p_cfg_extend = MC80_OSPI_PRV_EXTENDED_CFG(p_instance_ctrl);

  // Close transfer instance
  transfer_instance_t const *p_transfer  = p_cfg_extend->p_lower_lvl_transfer;
  p_transfer->p_api->close(p_transfer->p_ctrl);

  p_instance_ctrl->open = 0U;
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
    p_instance_ctrl - Pointer to OSPI specific control structure

  Return:
    FSP_SUCCESS              - Protocol based settings completed successfully
    FSP_ERR_CALIBRATE_FAILED - Auto-Calibration failed
-----------------------------------------------------------------------------------------------------*/
static fsp_err_t _Mc80_ospi_protocol_specific_settings(T_mc80_ospi_instance_ctrl *p_instance_ctrl)
{
  R_XSPI0_Type *const p_reg                     = p_instance_ctrl->p_reg;
  fsp_err_t           ret                       = FSP_SUCCESS;

  // Get the command set for the configured protocol and save it to the control struct
  T_mc80_ospi_xspi_command_set const *p_cmd_set = _Mc80_ospi_command_set_get(p_instance_ctrl);
  if (NULL == p_cmd_set)
  {
    return FSP_ERR_INVALID_MODE;
  }

  p_instance_ctrl->p_cmd_set = p_cmd_set;

  // Update the SPI protocol and latency mode
  uint32_t liocfg            = p_reg->LIOCFGCS[p_instance_ctrl->channel] & ~(R_XSPI0_LIOCFGCS_LATEMD_Msk | R_XSPI0_LIOCFGCS_PRTMD_Msk);
  liocfg |= (((uint32_t)p_instance_ctrl->spi_protocol << R_XSPI0_LIOCFGCS_PRTMD_Pos) & R_XSPI0_LIOCFGCS_PRTMD_Msk);
  liocfg |= (((uint32_t)p_cmd_set->latency_mode << R_XSPI0_LIOCFGCS_LATEMD_Pos) & R_XSPI0_LIOCFGCS_LATEMD_Msk);
  p_reg->LIOCFGCS[p_instance_ctrl->channel] = liocfg;

  // Specifies the read/write commands and Read dummy clocks for Device
  uint32_t cmcfg0                           = ((uint32_t)(p_cmd_set->address_msb_mask << R_XSPI0_CMCFGCS_CMCFG0_ADDRPEN_Pos)) |
                    ((uint32_t)(p_cmd_set->frame_format << R_XSPI0_CMCFGCS_CMCFG0_FFMT_Pos)) |
                    (((uint32_t)p_cmd_set->address_bytes << R_XSPI0_CMCFGCS_CMCFG0_ADDSIZE_Pos) &
                     R_XSPI0_CMCFGCS_CMCFG0_ADDSIZE_Msk);

  // When using 4-byte addressing, always mask off the most-significant nybble to remove the system bus offset from
  // the transmitted addresses. Ex. CS1 starts at 0x9000_0000 so it needs to mask off bits [31:28]
  if (p_cmd_set->address_bytes == SPI_FLASH_ADDRESS_BYTES_4)
  {
    cmcfg0 |= MC80_OSPI_PRV_ADDRESS_REPLACE_ENABLE_BITS;
  }

  // Apply the frame format setting and update the register
  cmcfg0 |= (uint32_t)(p_cmd_set->frame_format << R_XSPI0_CMCFGCS_CMCFG0_FFMT_Pos);
  p_reg->CMCFGCS[p_instance_ctrl->channel].CMCFG0 = cmcfg0;

  // Cache the appropriate command values for later use
  uint16_t read_command                           = p_cmd_set->read_command;
  uint16_t write_command                          = p_cmd_set->program_command;

  // If no length is specified or if the command byte length is 1, move the command to the upper byte
  if (MC80_OSPI_COMMAND_BYTES_1 == p_cmd_set->command_bytes)
  {
    read_command  = (uint16_t)((read_command & MC80_OSPI_PRV_CDTBUF_CMD_1B_VALUE_MASK) << MC80_OSPI_PRV_CDTBUF_CMD_1B_VALUE_SHIFT);
    write_command = (uint16_t)((write_command & MC80_OSPI_PRV_CDTBUF_CMD_1B_VALUE_MASK) << MC80_OSPI_PRV_CDTBUF_CMD_1B_VALUE_SHIFT);
  }

  const uint8_t read_dummy_cycles                 = p_cmd_set->read_dummy_cycles;
  const uint8_t write_dummy_cycles                = p_cmd_set->program_dummy_cycles;

  p_reg->CMCFGCS[p_instance_ctrl->channel].CMCFG1 = (uint32_t)(((uint32_t)(read_command) << R_XSPI0_CMCFGCS_CMCFG1_RDCMD_Pos) |
                                                               ((uint32_t)(read_dummy_cycles << R_XSPI0_CMCFGCS_CMCFG1_RDLATE_Pos) &
                                                                R_XSPI0_CMCFGCS_CMCFG1_RDLATE_Msk));

  p_reg->CMCFGCS[p_instance_ctrl->channel].CMCFG2 = (uint32_t)(((uint32_t)(write_command) << R_XSPI0_CMCFGCS_CMCFG2_WRCMD_Pos) |
                                                               ((uint32_t)(write_dummy_cycles << R_XSPI0_CMCFGCS_CMCFG2_WRLATE_Pos) &
                                                                R_XSPI0_CMCFGCS_CMCFG2_WRLATE_Msk));

  return ret;
}

/*-----------------------------------------------------------------------------------------------------
  Gets device status.

  Parameters:
    p_instance_ctrl - Pointer to a driver handle
    bit_pos         - Write-in-progress bit position

  Return:
    True if busy, false if not
-----------------------------------------------------------------------------------------------------*/
static bool _Mc80_ospi_status_sub(T_mc80_ospi_instance_ctrl *p_instance_ctrl, uint8_t bit_pos)
{
  T_mc80_ospi_xspi_command_set const *p_cmd_set = p_instance_ctrl->p_cmd_set;

  // Skip status check if no command was specified
  if (0 == p_cmd_set->status_command)
  {
    return false;
  }

  spi_flash_direct_transfer_t direct_command = {
    .command        = p_cmd_set->status_command,
    .command_length = (uint8_t)p_cmd_set->command_bytes,
    .address_length = (uint8_t)(p_cmd_set->status_needs_address ? MC80_OSPI_PRV_ADDR_BYTES_TO_LENGTH(p_cmd_set->status_address_bytes) : 0U),
    .address        = (p_cmd_set->status_needs_address) ? p_cmd_set->status_address : 0U,
    .data_length    = 1U,
    .dummy_cycles   = p_cmd_set->status_dummy_cycles,
  };

  // 8D-8D-8D mode requires an address for any kind of read. If the address wasn't set by the configuration
  // set it to the general address length
  if ((direct_command.address_length != 0) && (SPI_FLASH_PROTOCOL_8D_8D_8D == p_instance_ctrl->spi_protocol))
  {
    direct_command.address_length = MC80_OSPI_PRV_ADDR_BYTES_TO_LENGTH(p_cmd_set->address_bytes);
  }

  _Mc80_ospi_direct_transfer(p_instance_ctrl, &direct_command, SPI_FLASH_DIRECT_TRANSFER_DIR_READ);

  return (direct_command.data >> bit_pos) & 1U;
}

/*-----------------------------------------------------------------------------------------------------
  Send Write enable command to the OctaFlash

  Parameters:
    p_instance_ctrl - Pointer to OSPI specific control structure

  Return:
    FSP_SUCCESS         - Write enable operation completed
    FSP_ERR_NOT_ENABLED - Write enable failed
-----------------------------------------------------------------------------------------------------*/
static fsp_err_t _Mc80_ospi_write_enable(T_mc80_ospi_instance_ctrl *p_instance_ctrl)
{
  T_mc80_ospi_xspi_command_set const *const p_cmd_set = p_instance_ctrl->p_cmd_set;

  // If the command is 0x00, then skip sending the write enable
  if (0 == p_cmd_set->write_enable_command)
  {
    return FSP_SUCCESS;
  }

  spi_flash_direct_transfer_t direct_command = {
    .command        = p_cmd_set->write_enable_command,
    .command_length = (uint8_t)p_cmd_set->command_bytes,
    .address_length = 0,
    .address        = 0,
    .data_length    = 0,
    .dummy_cycles   = 0,
  };

  _Mc80_ospi_direct_transfer(p_instance_ctrl, &direct_command, SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);

  // In case write enable is not checked, assume write is enabled
  bool write_enabled = true;

  // Verify write is enabled
  for (uint32_t i = 0U; i < MC80_OSPI_MAX_WRITE_ENABLE_LOOPS; i++)
  {
    write_enabled = _Mc80_ospi_status_sub(p_instance_ctrl, p_instance_ctrl->p_cfg->write_enable_bit);
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
  Direct transfer implementation

  Parameters:
    p_instance_ctrl - Pointer to OSPI specific control structure
    p_transfer      - Pointer to transfer structure
    direction       - Transfer direction

  Return:
    void
-----------------------------------------------------------------------------------------------------*/
static void _Mc80_ospi_direct_transfer(T_mc80_ospi_instance_ctrl         *p_instance_ctrl,
                                       spi_flash_direct_transfer_t *const p_transfer,
                                       spi_flash_direct_transfer_dir_t    direction)
{
  R_XSPI0_Type *const             p_reg   = p_instance_ctrl->p_reg;
  const T_mc80_ospi_device_number channel = p_instance_ctrl->channel;

  uint32_t cdtbuf0 =
  (((uint32_t)p_transfer->command_length << R_XSPI0_CDBUF_CDT_CMDSIZE_Pos) & R_XSPI0_CDBUF_CDT_CMDSIZE_Msk) |
  (((uint32_t)p_transfer->address_length << R_XSPI0_CDBUF_CDT_ADDSIZE_Pos) & R_XSPI0_CDBUF_CDT_ADDSIZE_Msk) |
  (((uint32_t)p_transfer->data_length << R_XSPI0_CDBUF_CDT_DATASIZE_Pos) & R_XSPI0_CDBUF_CDT_DATASIZE_Msk) |
  (((uint32_t)p_transfer->dummy_cycles << R_XSPI0_CDBUF_CDT_LATE_Pos) & R_XSPI0_CDBUF_CDT_LATE_Msk) |
  (((uint32_t)direction << R_XSPI0_CDBUF_CDT_TRTYPE_Pos) & R_XSPI0_CDBUF_CDT_TRTYPE_Msk);

  cdtbuf0 |= (1 == p_transfer->command_length) ? ((p_transfer->command & MC80_OSPI_PRV_CDTBUF_CMD_1B_VALUE_MASK) << MC80_OSPI_PRV_CDTBUF_CMD_UPPER_OFFSET) : ((p_transfer->command & MC80_OSPI_PRV_CDTBUF_CMD_2B_VALUE_MASK) << MC80_OSPI_PRV_CDTBUF_CMD_OFFSET);

  // Setup the manual command control. Cancel any ongoing transactions, direct mode, set channel, 1 transaction
  p_reg->CDCTL0 = ((((uint32_t)channel) << R_XSPI0_CDCTL0_CSSEL_Pos) & R_XSPI0_CDCTL0_CSSEL_Msk);

  // Direct Read/Write settings (see RA8M1 User's Manual section "Flow of Manual-command Procedure")
  while (p_reg->CDCTL0_b.TRREQ != 0)
  {
    __NOP(); // Breakpoint for transaction ready wait
  }

  p_reg->CDBUF[0].CDT = cdtbuf0;
  p_reg->CDBUF[0].CDA = p_transfer->address;

  if (SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE == direction)
  {
    p_reg->CDBUF[0].CDD0 = (uint32_t)(p_transfer->data_u64 & UINT32_MAX);
    if (p_transfer->data_length > sizeof(uint32_t))
    {
      p_reg->CDBUF[0].CDD1 = (uint32_t)(p_transfer->data_u64 >> MC80_OSPI_PRV_UINT32_BITS);
    }
  }

  // Start the transaction and wait for completion
  p_reg->CDCTL0_b.TRREQ = 1;
  while (p_reg->CDCTL0_b.TRREQ != 0)
  {
    __NOP(); // Breakpoint for transaction completion wait
  }

  if (SPI_FLASH_DIRECT_TRANSFER_DIR_READ == direction)
  {
    p_transfer->data_u64 = p_reg->CDBUF[0].CDD0;
    if (p_transfer->data_length > sizeof(uint32_t))
    {
      p_transfer->data_u64 |= (((uint64_t)p_reg->CDBUF[0].CDD1) << MC80_OSPI_PRV_UINT32_BITS);
    }
  }

  // Clear interrupt flags
  p_reg->INTC = p_reg->INTS;
}

/*-----------------------------------------------------------------------------------------------------
  Get command set for current protocol

  Parameters:
    p_instance_ctrl - Pointer to OSPI specific control structure

  Return:
    Pointer to command set structure, or NULL if not found
-----------------------------------------------------------------------------------------------------*/
static T_mc80_ospi_xspi_command_set const *_Mc80_ospi_command_set_get(T_mc80_ospi_instance_ctrl *p_instance_ctrl)
{
  T_mc80_ospi_extended_cfg *p_cfg_extend = MC80_OSPI_PRV_EXTENDED_CFG(p_instance_ctrl);

  if (NULL == p_cfg_extend->p_xspi_command_set)
  {
    return NULL;
  }

  T_mc80_ospi_xspi_command_set *p_cmd_set;
  for (uint32_t i = 0; i < p_cfg_extend->p_xspi_command_set->length; i++)
  {
    p_cmd_set = &((T_mc80_ospi_xspi_command_set *)p_cfg_extend->p_xspi_command_set->p_table)[i];
    if (p_cmd_set->protocol == p_instance_ctrl->spi_protocol)
    {
      return p_cmd_set;
    }
  }

  // If the protocol isn't found, return NULL
  return NULL;
}

#if MC80_OSPI_CFG_AUTOCALIBRATION_SUPPORT_ENABLE
/*-----------------------------------------------------------------------------------------------------
  Automatic calibration sequence for OSPI

  Parameters:
    p_instance_ctrl - Pointer to OSPI specific control structure

  Return:
    FSP_SUCCESS              - Auto-calibration completed successfully
    FSP_ERR_DEVICE_BUSY      - Auto-calibration already in progress
    FSP_ERR_CALIBRATE_FAILED - Auto-calibration failed
-----------------------------------------------------------------------------------------------------*/
static fsp_err_t _Mc80_ospi_automatic_calibration_seq(T_mc80_ospi_instance_ctrl *p_instance_ctrl)
{
  R_XSPI0_Type *const       p_reg               = p_instance_ctrl->p_reg;
  fsp_err_t                 ret                 = FSP_SUCCESS;
  T_mc80_ospi_extended_cfg *p_cfg_extend        = MC80_OSPI_PRV_EXTENDED_CFG(p_instance_ctrl);

  T_mc80_ospi_xspi_command_set const *p_cmd_set = p_instance_ctrl->p_cmd_set;

  T_mc80_ospi_device_number channel             = p_instance_ctrl->channel;

  // Check that calibration is not in progress
  if (0 != p_reg->CCCTLCS[channel].CCCTL0_b.CAEN)
  {
    return FSP_ERR_DEVICE_BUSY;
  }

  const uint8_t command_bytes     = (uint8_t)p_cmd_set->command_bytes;
  uint16_t      read_command      = p_cmd_set->read_command;
  const uint8_t read_dummy_cycles = p_cmd_set->read_dummy_cycles;
  const uint8_t address_bytes     = MC80_OSPI_PRV_ADDR_BYTES_TO_LENGTH(p_cmd_set->address_bytes);

  // If using 1 command byte, shift the read command over as the peripheral expects
  if (1U == command_bytes)
  {
    read_command = (uint16_t)((read_command & MC80_OSPI_PRV_CDTBUF_CMD_1B_VALUE_MASK) << MC80_OSPI_PRV_CDTBUF_CMD_1B_VALUE_SHIFT);
  }

  p_reg->CCCTLCS[channel].CCCTL1 =
  (((uint32_t)command_bytes << R_XSPI0_CCCTLCS_CCCTL1_CACMDSIZE_Pos) &
   R_XSPI0_CCCTLCS_CCCTL1_CACMDSIZE_Msk) |
  (((uint32_t)address_bytes << R_XSPI0_CCCTLCS_CCCTL1_CAADDSIZE_Pos) &
   R_XSPI0_CCCTLCS_CCCTL1_CAADDSIZE_Msk) |
  (0xFU << R_XSPI0_CCCTLCS_CCCTL1_CADATASIZE_Pos) |
  (0U << R_XSPI0_CCCTLCS_CCCTL1_CAWRLATE_Pos) |
  (((uint32_t)read_dummy_cycles << R_XSPI0_CCCTLCS_CCCTL1_CARDLATE_Pos) &
   R_XSPI0_CCCTLCS_CCCTL1_CARDLATE_Msk);

  p_reg->CCCTLCS[channel].CCCTL2 = (uint32_t)read_command << R_XSPI0_CCCTLCS_CCCTL2_CARDCMD_Pos;

  p_reg->CCCTLCS[channel].CCCTL3 = (uint32_t)p_cfg_extend->p_autocalibration_preamble_pattern_addr;

  // Configure auto-calibration
  p_reg->CCCTLCS[channel].CCCTL0 =
  (0x1FU << R_XSPI0_CCCTLCS_CCCTL0_CAINTE_Pos) |
  (0x1U << R_XSPI0_CCCTLCS_CCCTL0_CANOWR_Pos) |
  (0x1FU << R_XSPI0_CCCTLCS_CCCTL0_CADS_Pos);

  // Start auto-calibration
  p_reg->CCCTLCS[channel].CCCTL0_b.CAEN = 1;

  // Wait for calibration to complete
  while (p_reg->CCCTLCS[channel].CCCTL0_b.CAEN != 0)
  {
    __NOP(); // Breakpoint for calibration wait
  }

  // Check if calibration was successful
  if (0 != (p_reg->INTS & (1U << (R_XSPI0_INTS_CAFAILCS_Pos + channel))))
  {
    ret         = FSP_ERR_CALIBRATE_FAILED;

    // Clear automatic calibration failure status
    p_reg->INTC = (uint32_t)1 << (R_XSPI0_INTS_CAFAILCS_Pos + channel);
  }

  return ret;
}
#endif

#if MC80_OSPI_CFG_XIP_SUPPORT_ENABLE
/*-----------------------------------------------------------------------------------------------------
  Configures the device to enter or exit XiP mode

  Parameters:
    p_instance_ctrl - Pointer to the instance ctrl struct
    is_entering     - true if entering XiP mode, false if exiting

  Return:
    void
-----------------------------------------------------------------------------------------------------*/
static void _Mc80_ospi_xip(T_mc80_ospi_instance_ctrl *p_instance_ctrl, bool is_entering)
{
  R_XSPI0_Type *const    p_reg                = p_instance_ctrl->p_reg;
  const spi_flash_cfg_t *p_cfg                = p_instance_ctrl->p_cfg;
  volatile uint8_t      *p_dummy_read_address = (volatile uint8_t *)((MC80_OSPI_DEVICE_NUMBER_0 == p_instance_ctrl->channel) ? BSP_FEATURE_OSPI_B_DEVICE_0_START_ADDRESS : BSP_FEATURE_OSPI_B_DEVICE_1_START_ADDRESS);
  volatile uint8_t       dummy_read           = 0;

  // Clear the pre-fetch buffer for this bank so the next read is guaranteed to use the XiP code
  #if MC80_OSPI_CFG_PREFETCH_FUNCTION
  p_reg->BMCTL1 |= 0x03U << R_XSPI0_BMCTL1_PBUFCLRCH_Pos;
  #endif

  // Wait for any on-going access to complete
  while ((p_reg->COMSTT & MC80_OSPI_PRV_COMSTT_MEMACCCH_MASK) != 0)
  {
    __NOP(); // Breakpoint for access completion wait
  }

  if (is_entering)
  {
    // Change memory-mapping to read-only mode
    p_reg->BMCTL0          = MC80_OSPI_PRV_BMCTL0_READ_ONLY_VALUE;

    // Configure XiP codes and enable
    const uint32_t cmctlch = R_XSPI0_CMCTLCH_XIPEN_Msk |
                             ((uint32_t)(p_cfg->xip_enter_command << R_XSPI0_CMCTLCH_XIPENCODE_Pos)) |
                             ((uint32_t)(p_cfg->xip_exit_command << R_XSPI0_CMCTLCH_XIPEXCODE_Pos));

    // XiP enter/exit codes are configured only for memory mapped operations and affects both OSPI slave channels
    p_reg->CMCTLCH[0] = cmctlch;
    p_reg->CMCTLCH[1] = cmctlch;

    // Perform a read to send the enter code. All further reads will use the enter code and will not send a read command code
    dummy_read        = *p_dummy_read_address;

    // Wait for the read to complete
    while ((p_reg->COMSTT & MC80_OSPI_PRV_COMSTT_MEMACCCH_MASK) != 0)
    {
      __NOP(); // Breakpoint for XIP enter read wait
    }
  }
  else
  {
    // Disable XiP
    p_reg->CMCTLCH[0] &= ~R_XSPI0_CMCTLCH_XIPEN_Msk;
    p_reg->CMCTLCH[1] &= ~R_XSPI0_CMCTLCH_XIPEN_Msk;

    // Perform a read to send the exit code. All further reads will not send an exit code
    dummy_read = *p_dummy_read_address;

    // Wait for the read to complete
    while ((p_reg->COMSTT & MC80_OSPI_PRV_COMSTT_MEMACCCH_MASK) != 0)
    {
      __NOP(); // Breakpoint for XIP exit read wait
    }

    // Change memory-mapping back to R/W mode
    p_reg->BMCTL0 = MC80_OSPI_PRV_BMCTL0_READ_WRITE_VALUE;
  }
}
#endif
