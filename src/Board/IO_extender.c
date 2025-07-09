//----------------------------------------------------------------------
// File created on 2025-01-27
//----------------------------------------------------------------------

#include "App.h"

// Array describing MCP23S17 IO extender bits configured as outputs
uint8_t IO_extender_outputs[] =
{
  AUD_STBY,
  LED_RS485,
  LED_CAN,
  LED_GR,
  LED_RD,
  SW5V_IN,
  SW5V_DIAG,
  CAN_EN,
  ESP32_RSY,
  SCI3_MUX
};

// Variables for storing output register states
static uint8_t gpioa_state = 0;
static uint8_t gpiob_state = 0;

/*-----------------------------------------------------------------------------------------------------
  Configures MCP23S17 IO extender pins as outputs based on IO_extender_outputs array

  Return:
    TX_SUCCESS - operation completed successfully
    Other value - error code
-----------------------------------------------------------------------------------------------------*/
uint32_t Configure_IO_extender(void)
{
  uint8_t  iodira = 0xFF;  // All inputs by default
  uint8_t  iodirb = 0xFF;  // All inputs by default
  uint32_t status;

  // Initialize output states
  gpioa_state = 0;
  gpiob_state = 0;

  // Buffer for sending all data in one transaction
  uint8_t tx_buffer[4];

  // Configure IODIRA and IODIRB
  for (uint8_t i = 0; i < sizeof(IO_extender_outputs) / sizeof(IO_extender_outputs[0]); i++)
  {
    if (IO_extender_outputs[i] < 8)
    {
      iodira &= ~(1 << IO_extender_outputs[i]);
    }
    else
    {
      iodirb &= ~(1 << (IO_extender_outputs[i] - 8));
    }
  }

  // Prepare data for transmission
  tx_buffer[0] = MCP23S17_WRITE_OPCODE;
  tx_buffer[1] = MCP23S17_IODIRA;  // Start from IODIRA register
  tx_buffer[2] = iodira;           // Data for IODIRA
  tx_buffer[3] = iodirb;           // Data for IODIRB

  // Acquire mutex for SPI bus access
  status = tx_mutex_get(&g_spi0_bus_mutex, TX_WAIT_FOREVER);
  if (status != TX_SUCCESS)
  {
    return status;
  }
  SPI0_set_speed(SPI0_SPEED_20MHZ, SPI_CLK_POLARITY_LOW, SPI_CLK_PHASE_EDGE_ODD);

  // Select MCP23S17 chip
  IO_EXTENDER_CS = 0;

  // Send all data in one operation
  status = R_SPI_B_Write(&g_SPI0_ctrl, tx_buffer, sizeof(tx_buffer), SPI_BIT_WIDTH_8_BITS);
  if (status != FSP_SUCCESS)
  {
    IO_EXTENDER_CS = 1;
    tx_mutex_put(&g_spi0_bus_mutex);
    return status;
  }

  // Wait for transfer completion
  status = SPI0_wait_transfer_complete(100);

  // Deselect MCP23S17 chip
  IO_EXTENDER_CS = 1;

  // Release mutex
  tx_mutex_put(&g_spi0_bus_mutex);

  return status;
}

/*-----------------------------------------------------------------------------------------------------
  Writes output state to MCP23S17 IO extender

  Parameters:
    bit_id - bit identifier (0-15)
    val    - value (0 or 1)

  Return:
    TX_SUCCESS - operation completed successfully
    Other value - error code
-----------------------------------------------------------------------------------------------------*/
uint32_t Write_to_IO_extender(uint8_t bit_id, uint8_t val)
{
  uint8_t  tx_buffer[3];
  uint32_t status;
  uint8_t  bit_mask;

  // Determine mask and register for the bit
  if (bit_id < 8)
  {
    bit_mask = (1 << bit_id);

    // Change only the specified bit
    if (val)
    {
      gpioa_state |= bit_mask;
    }
    else
    {
      gpioa_state &= ~bit_mask;
    }

    // Prepare data for transmission
    tx_buffer[0] = MCP23S17_WRITE_OPCODE;
    tx_buffer[1] = MCP23S17_GPIOA;
    tx_buffer[2] = gpioa_state;
  }
  else
  {
    bit_mask = (1 << (bit_id - 8));

    // Change only the specified bit
    if (val)
    {
      gpiob_state |= bit_mask;
    }
    else
    {
      gpiob_state &= ~bit_mask;
    }

    // Prepare data for transmission
    tx_buffer[0] = MCP23S17_WRITE_OPCODE;
    tx_buffer[1] = MCP23S17_GPIOB;
    tx_buffer[2] = gpiob_state;
  }

  // Acquire mutex for SPI bus access
  status = tx_mutex_get(&g_spi0_bus_mutex, TX_WAIT_FOREVER);
  if (status != TX_SUCCESS)
  {
    return status;
  }
  SPI0_set_speed(SPI0_SPEED_20MHZ, SPI_CLK_POLARITY_LOW, SPI_CLK_PHASE_EDGE_ODD);
  // Select MCP23S17 chip
  IO_EXTENDER_CS = 0;

  // Send all data in one operation
  status = R_SPI_B_Write(&g_SPI0_ctrl, tx_buffer, sizeof(tx_buffer), SPI_BIT_WIDTH_8_BITS);
  if (status != FSP_SUCCESS)
  {
    IO_EXTENDER_CS = 1;
    tx_mutex_put(&g_spi0_bus_mutex);
    return status;
  }

  // Wait for transfer completion
  status = SPI0_wait_transfer_complete(100);

  // Deselect MCP23S17 chip
  IO_EXTENDER_CS = 1;

  // Release mutex
  tx_mutex_put(&g_spi0_bus_mutex);

  return status;
}

/*-----------------------------------------------------------------------------------------------------
  Write multiple LED bits to IO extender in one SPI transaction for efficiency

  Parameters:
    led_gr    - Green LED value (0 or 1)
    led_rd    - Red LED value (0 or 1)
    led_can   - CAN LED value (0 or 1)
    led_rs485 - RS485 LED value (0 or 1)

  Return:
    TX_SUCCESS - operation completed successfully
    Other value - error code
-----------------------------------------------------------------------------------------------------*/
uint32_t Write_multiple_bits_to_IO_extender(uint8_t led_gr, uint8_t led_rd, uint8_t led_can, uint8_t led_rs485)
{
  uint8_t  tx_buffer_a[3];
  uint32_t status;
  uint8_t  new_gpioa_state = gpioa_state;

  // Update LED bits in local copy
  // LED_GR=3, LED_RD=4, LED_CAN=2, LED_RS485=1 (all in GPIOA)
  if (led_gr)
  {
    new_gpioa_state |= (1 << LED_GR);
  }
  else
  {
    new_gpioa_state &= ~(1 << LED_GR);
  }

  if (led_rd)
  {
    new_gpioa_state |= (1 << LED_RD);
  }
  else
  {
    new_gpioa_state &= ~(1 << LED_RD);
  }

  if (led_can)
  {
    new_gpioa_state |= (1 << LED_CAN);
  }
  else
  {
    new_gpioa_state &= ~(1 << LED_CAN);
  }

  if (led_rs485)
  {
    new_gpioa_state |= (1 << LED_RS485);
  }
  else
  {
    new_gpioa_state &= ~(1 << LED_RS485);
  }

  // If no changes needed, return success
  if (new_gpioa_state == gpioa_state)
  {
    return TX_SUCCESS;
  }

  // Update global state
  gpioa_state = new_gpioa_state;

  // Acquire mutex for SPI bus access
  status = tx_mutex_get(&g_spi0_bus_mutex, TX_WAIT_FOREVER);
  if (status != TX_SUCCESS)
  {
    return status;
  }

  SPI0_set_speed(SPI0_SPEED_20MHZ, SPI_CLK_POLARITY_LOW, SPI_CLK_PHASE_EDGE_ODD);

  // Select MCP23S17 chip
  IO_EXTENDER_CS = 0;

  // Prepare and send data
  tx_buffer_a[0] = MCP23S17_WRITE_OPCODE;
  tx_buffer_a[1] = MCP23S17_GPIOA;
  tx_buffer_a[2] = gpioa_state;

  status = R_SPI_B_Write(&g_SPI0_ctrl, tx_buffer_a, sizeof(tx_buffer_a), SPI_BIT_WIDTH_8_BITS);
  if (status != FSP_SUCCESS)
  {
    IO_EXTENDER_CS = 1;
    tx_mutex_put(&g_spi0_bus_mutex);
    return status;
  }

  // Wait for transfer completion
  status = SPI0_wait_transfer_complete(100);

  // Deselect MCP23S17 chip
  IO_EXTENDER_CS = 1;

  // Release mutex
  tx_mutex_put(&g_spi0_bus_mutex);

  return status;
}
