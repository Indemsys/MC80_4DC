//----------------------------------------------------------------------
// File created on 2025-01-27
//----------------------------------------------------------------------

#include "App.h"

// Structure for storing SPI dividers
typedef struct
{
  uint8_t spbr;  // Divider SPBR
  uint8_t brdv;  // Divider BRDV
} T_spi0_speed_dividers;

// Pre-calculated dividers for PCLKA = FRQ_PCLKA_MHZ (usually 120 MHz)
static const T_spi0_speed_dividers g_spi0_speed_table[] =
{
 {2, 0},   // 20 MHz: 120 / (2*(2+1)*1) = 20 MHz
 {5, 0},   // 10 MHz: 120 / (2*(5+1)*1) = 10 MHz
 {9, 0},   // 6  MHz: 120 / (2*(9+1)*1) = 6 MHz
 {11, 0},  // 5  MHz: 120 / (2*(11+1)*1) = 5 MHz
 {14, 0},  // 4  MHz: 120 / (2*(14+1)*1) = 4 MHz
 {19, 0},  // 3  MHz: 120 / (2*(19+1)*1) = 3 MHz
 {59, 0},  // 1  MHz: 120 / (2*(59+1)*1) = 1 MHz
 {119, 0}  // 500 kHz: 120 / (2*(119+1)*1) = 0.5 MHz
};

// Global variable for storing current SPI0 configuration
static uint8_t g_spi0_current_speed_idx = 0xFF;  // 0xFF — not initialized

dtc_instance_ctrl_t g_transfer_SPI0_rx_ctrl;

transfer_info_t g_transfer_SPI0_rx_info DTC_TRANSFER_INFO_ALIGNMENT =
{
 .transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED,
 .transfer_settings_word_b.repeat_area    = TRANSFER_REPEAT_AREA_DESTINATION,
 .transfer_settings_word_b.irq            = TRANSFER_IRQ_END,
 .transfer_settings_word_b.chain_mode     = TRANSFER_CHAIN_MODE_DISABLED,
 .transfer_settings_word_b.src_addr_mode  = TRANSFER_ADDR_MODE_FIXED,
 .transfer_settings_word_b.size           = TRANSFER_SIZE_2_BYTE,
 .transfer_settings_word_b.mode           = TRANSFER_MODE_NORMAL,
 .p_dest                                  = (void *)NULL,
 .p_src                                   = (void const *)NULL,
 .num_blocks                              = (uint16_t)0,
 .length                                  = (uint16_t)0,
};

const dtc_extended_cfg_t g_transfer_SPI0_rx_cfg_extend =
{
 .activation_source = VECTOR_NUMBER_SPI0_RXI,
};

const transfer_cfg_t g_transfer_SPI0_rx_cfg =
{
 .p_info   = &g_transfer_SPI0_rx_info,
 .p_extend = &g_transfer_SPI0_rx_cfg_extend,
};

/* Instance structure to use this module. */
const transfer_instance_t g_transfer_SPI0_rx =
{
 .p_ctrl = &g_transfer_SPI0_rx_ctrl,
 .p_cfg  = &g_transfer_SPI0_rx_cfg,
 .p_api  = &g_transfer_on_dtc,
};

dtc_instance_ctrl_t g_transfer_SPI0_tx_ctrl;

transfer_info_t g_transfer_SPI0_tx_info DTC_TRANSFER_INFO_ALIGNMENT =
{
 .transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_FIXED,
 .transfer_settings_word_b.repeat_area    = TRANSFER_REPEAT_AREA_SOURCE,
 .transfer_settings_word_b.irq            = TRANSFER_IRQ_END,
 .transfer_settings_word_b.chain_mode     = TRANSFER_CHAIN_MODE_DISABLED,
 .transfer_settings_word_b.src_addr_mode  = TRANSFER_ADDR_MODE_INCREMENTED,
 .transfer_settings_word_b.size           = TRANSFER_SIZE_2_BYTE,
 .transfer_settings_word_b.mode           = TRANSFER_MODE_NORMAL,
 .p_dest                                  = (void *)NULL,
 .p_src                                   = (void const *)NULL,
 .num_blocks                              = (uint16_t)0,
 .length                                  = (uint16_t)0,
};

const dtc_extended_cfg_t g_transfer_SPI0_tx_cfg_extend =
{
 .activation_source = VECTOR_NUMBER_SPI0_TXI,
};

const transfer_cfg_t g_transfer_SPI0_tx_cfg =
{
 .p_info   = &g_transfer_SPI0_tx_info,
 .p_extend = &g_transfer_SPI0_tx_cfg_extend,
};

/* Instance structure to use this module. */
const transfer_instance_t g_transfer_SPI0_tx =
{
 .p_ctrl = &g_transfer_SPI0_tx_ctrl,
 .p_cfg  = &g_transfer_SPI0_tx_cfg,
 .p_api  = &g_transfer_on_dtc,
};

spi_b_instance_ctrl_t g_SPI0_ctrl;

/** SPI extended configuration for SPI HAL driver */
spi_b_extended_cfg_t g_SPI0_ext_cfg =
{
 .spi_clksyn   = SPI_B_SSL_MODE_CLK_SYN,
 .spi_comm     = SPI_B_COMMUNICATION_FULL_DUPLEX,
 .ssl_polarity = SPI_B_SSLP_LOW,
 .ssl_select   = SPI_B_SSL_SELECT_SSL3,
 .mosi_idle    = SPI_B_MOSI_IDLE_VALUE_FIXING_DISABLE,
 .parity       = SPI_B_PARITY_MODE_DISABLE,
 .byte_swap    = SPI_B_BYTE_SWAP_DISABLE,
 .clock_source = SPI_B_CLOCK_SOURCE_PCLK,
 .spck_div     = {
  /* Actual calculated bitrate: 20000000. */ .spbr = 2, .brdv = 0},
 .spck_delay         = SPI_B_DELAY_COUNT_1,
 .ssl_negation_delay = SPI_B_DELAY_COUNT_1,
 .next_access_delay  = SPI_B_DELAY_COUNT_1,
};

/** SPI configuration for SPI HAL driver */
spi_cfg_t g_SPI0_cfg =
{
 .channel        = 0,
 .rxi_irq        = VECTOR_NUMBER_SPI0_RXI,
 .txi_irq        = VECTOR_NUMBER_SPI0_TXI,
 .tei_irq        = VECTOR_NUMBER_SPI0_TEI,
 .eri_irq        = VECTOR_NUMBER_SPI0_ERI,
 .rxi_ipl        = (12),
 .txi_ipl        = (12),
 .tei_ipl        = (12),
 .eri_ipl        = (12),
 .operating_mode = SPI_MODE_MASTER,
 .clk_phase      = SPI_CLK_PHASE_EDGE_ODD,
 .clk_polarity   = SPI_CLK_POLARITY_LOW,
 .mode_fault     = SPI_MODE_FAULT_ERROR_DISABLE,
 .bit_order      = SPI_BIT_ORDER_MSB_FIRST,
 .p_transfer_tx  = &g_transfer_SPI0_tx,
 .p_transfer_rx  = &g_transfer_SPI0_rx,
 .p_callback     = SPI0_callback,
 .p_context      = NULL,
 .p_extend       = (void *)&g_SPI0_ext_cfg,
};

/* Instance structure to use this module. */
const spi_instance_t g_SPI0 =
{
 .p_ctrl = &g_SPI0_ctrl,
 .p_cfg  = &g_SPI0_cfg,
 .p_api  = &g_spi_on_spi_b,
};

TX_MUTEX             g_spi0_bus_mutex;
TX_EVENT_FLAGS_GROUP spi0_events;

/*-----------------------------------------------------------------------------------------------------
  Initialize SPI0.

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void SPI0_open(void)
{
  fsp_err_t err = FSP_SUCCESS;

  tx_mutex_create(&g_spi0_bus_mutex, "SPI0 Bus Mutex", TX_NO_INHERIT);
  tx_event_flags_create(&spi0_events, "SPI0 Events");

  MOTOR_DRV1_CS  = 1;
  MOTOR_DRV2_CS  = 1;
  LCD_CS         = 1;
  IO_EXTENDER_CS = 1;

  /* Initialize SPI module. */
  err            = R_SPI_B_Open(&g_SPI0_ctrl, &g_SPI0_cfg);
  /* Handle any errors. This function should be defined by the user. */
  assert(FSP_SUCCESS == err);
}

/*-----------------------------------------------------------------------------------------------------
  SPI0 callback function.

  Parameters:
    arg - pointer to callback arguments

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void SPI0_callback(spi_callback_args_t *arg)
{
  // Get the result of the SPI read/write operation
  switch (arg->event)
  {
    case SPI_EVENT_TRANSFER_COMPLETE:
      tx_event_flags_set(&spi0_events, SPI0_EVENT_TRANSFER_COMPLETE, TX_OR);
      break;

    case SPI_EVENT_TRANSFER_ABORTED:
      tx_event_flags_set(&spi0_events, SPI0_EVENT_TRANSFER_ABORTED, TX_OR);
      break;

    case SPI_EVENT_ERR_READ_OVERFLOW:
    case SPI_EVENT_ERR_MODE_FAULT:
    case SPI_EVENT_ERR_PARITY:
    case SPI_EVENT_ERR_OVERRUN:
    case SPI_EVENT_ERR_FRAMING:
    case SPI_EVENT_ERR_MODE_UNDERRUN:
      tx_event_flags_set(&spi0_events, SPI0_EVENT_TRANSFER_ERROR, TX_OR);
      break;

    default:
      break;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Wait for SPI transfer completion with timeout.

  Parameters:
    timeout_ticks - number of ticks to wait (TX_WAIT_FOREVER for infinite)

  Return:
    TX_SUCCESS      - operation completed successfully
    TX_WAIT_ABORTED - operation was aborted
    TX_NO_EVENTS    - timeout occurred
-----------------------------------------------------------------------------------------------------*/
uint32_t SPI0_wait_transfer_complete(uint32_t timeout_ticks)
{
  ULONG    actual_flags = 0;
  uint32_t status       = tx_event_flags_get(&spi0_events,
                                             SPI0_EVENT_TRANSFER_COMPLETE | SPI0_EVENT_TRANSFER_ABORTED | SPI0_EVENT_TRANSFER_ERROR,
                                             TX_OR_CLEAR,
                                             &actual_flags,
                                             timeout_ticks);

  if (status != TX_SUCCESS)
  {
    return status;
  }

  if (actual_flags & SPI0_EVENT_TRANSFER_COMPLETE)
  {
    return TX_SUCCESS;
  }
  else if (actual_flags & SPI0_EVENT_TRANSFER_ABORTED)
  {
    return TX_WAIT_ABORTED;
  }
  else
  {
    return TX_NO_EVENTS;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Close SPI0.

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void SPI0_close(void)
{
  R_SPI_B_Close(&g_SPI0_ctrl);
  tx_event_flags_delete(&spi0_events);
  tx_mutex_delete(&g_spi0_bus_mutex);
}

/*-----------------------------------------------------------------------------------------------------
  Set SPI0 bus speed (bitrate) and clock polarity/phase using predefined divider table.

  Parameters:
    speed_idx  - speed index (macro SPI0_SPEED_20MHZ, SPI0_SPEED_10MHZ, etc.)
    cpol       - clock polarity (SPI_CLK_POLARITY_LOW or SPI_CLK_POLARITY_HIGH)
    cpha       - clock phase    (SPI_CLK_PHASE_EDGE_ODD or SPI_CLK_PHASE_EDGE_EVEN)

  Return:
    None

  Note:
    Mutex protection is NOT used inside this function. The caller must ensure thread safety.
-----------------------------------------------------------------------------------------------------*/
void SPI0_set_speed(uint8_t speed_idx, spi_clk_polarity_t cpol, spi_clk_phase_t cpha)
{
  fsp_err_t err = FSP_SUCCESS;

  // Check range
  if (speed_idx >= (sizeof(g_spi0_speed_table) / sizeof(g_spi0_speed_table[0])))
  {
    return;  // Invalid index
  }

  // Check if all parameters are already set
  if ((g_spi0_current_speed_idx == speed_idx) &&
      (g_SPI0_cfg.clk_polarity == cpol) &&
      (g_SPI0_cfg.clk_phase == cpha))
  {
    return;
  }

  // Close SPI0 before changing configuration
  R_SPI_B_Close(&g_SPI0_ctrl);

  // Set new dividers
  ((spi_b_extended_cfg_t *)g_SPI0_cfg.p_extend)->spck_div.spbr = g_spi0_speed_table[speed_idx].spbr;
  ((spi_b_extended_cfg_t *)g_SPI0_cfg.p_extend)->spck_div.brdv = g_spi0_speed_table[speed_idx].brdv;

  // Set new CPOL and CPHA
  g_SPI0_cfg.clk_polarity = cpol;
  g_SPI0_cfg.clk_phase    = cpha;

  // Open SPI0 with new configuration
  err = R_SPI_B_Open(&g_SPI0_ctrl, &g_SPI0_cfg);
  assert(FSP_SUCCESS == err);

  // Save current index
  g_spi0_current_speed_idx = speed_idx;
}
