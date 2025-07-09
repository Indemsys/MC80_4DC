//----------------------------------------------------------------------
// File created on 2025-03-23
//----------------------------------------------------------------------
#include "App.h"

extern uint16_t video_buffer[];

static uint32_t X_OFFSET;
static uint32_t Y_OFFSET;

/*-----------------------------------------------------------------------------------------------------
  Sends one byte to the display via SPI bus.

  Parameters:
    b - byte to send

  Return:
    uint32_t Operation result:
      - RES_OK: Success
      - RES_ERROR: Error during transfer
      - Other values: Error code from tx_mutex_get or R_SPI_B_Write
-----------------------------------------------------------------------------------------------------*/
uint32_t SPI0_send_byte_to_display(uint8_t b)
{
  uint32_t res = RES_ERROR;
  uint32_t status;
  uint8_t  tx_buffer[1];

  // Acquire mutex for SPI bus access
  status = tx_mutex_get(&g_spi0_bus_mutex, TX_WAIT_FOREVER);
  if (status != TX_SUCCESS)
  {
    return status;
  }
  SPI0_set_speed(SPI0_SPEED_20MHZ, SPI_CLK_POLARITY_LOW, SPI_CLK_PHASE_EDGE_ODD);
  // Select display chip
  LCD_CS       = 0;
  tx_buffer[0] = b;
  // Send data
  status       = R_SPI_B_Write(&g_SPI0_ctrl, tx_buffer, sizeof(tx_buffer), SPI_BIT_WIDTH_8_BITS);
  if (status == FSP_SUCCESS)
  {
    res = SPI0_wait_transfer_complete(100);
  }
  // Deselect display chip
  LCD_CS = 1;
  // Release mutex
  tx_mutex_put(&g_spi0_bus_mutex);
  return res;
}

/*-----------------------------------------------------------------------------------------------------
  Sends a buffer to the display via SPI bus.

  Parameters:
    buf - pointer to data buffer
    sz  - buffer size in bytes

  Return:
    uint32_t Operation result:
      - RES_OK: Success
      - RES_ERROR: Error during transfer
      - Other values: Error code from tx_mutex_get or R_SPI_B_Write
-----------------------------------------------------------------------------------------------------*/
uint32_t SPI0_send_buff_to_display(uint16_t *buf, uint32_t sz)
{
  uint32_t res = RES_OK;
  uint32_t status;

  // Acquire mutex for SPI bus access
  status = tx_mutex_get(&g_spi0_bus_mutex, TX_WAIT_FOREVER);
  if (status != TX_SUCCESS)
  {
    return status;
  }
  SPI0_set_speed(SPI0_SPEED_20MHZ, SPI_CLK_POLARITY_LOW, SPI_CLK_PHASE_EDGE_ODD);
  // Select display chip
  LCD_CS = 0;

  status = R_SPI_B_Write(&g_SPI0_ctrl, (uint8_t *)(buf), sz / 2, SPI_BIT_WIDTH_16_BITS);
  if (status != FSP_SUCCESS)
  {
    res = RES_ERROR;
  }

  status = SPI0_wait_transfer_complete(400);
  if (status != TX_SUCCESS)
  {
    res = RES_ERROR;
  }

  // Deselect display chip
  LCD_CS = 1;

  // Release mutex
  tx_mutex_put(&g_spi0_bus_mutex);

  return res;
}

/*-----------------------------------------------------------------------------------------------------
  Sends a command to the display.

  Parameters:
    data - command byte

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void TFT_wr__cmd(uint8_t data)
{
  LCD_DC = 0;
  SPI0_send_byte_to_display(data);
}

/*-----------------------------------------------------------------------------------------------------
  Sends data to the display.

  Parameters:
    data - data byte

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void TFT_wr_data(uint8_t data)
{
  LCD_DC = 1;
  SPI0_send_byte_to_display(data);
}

/*-----------------------------------------------------------------------------------------------------
  Sends a buffer of data to the display.

  Parameters:
    buf    - pointer to data buffer
    buf_sz - buffer size in bytes

  Return:
    uint32_t Operation result
-----------------------------------------------------------------------------------------------------*/
uint32_t TFT_wr_data_buf(uint16_t *buf, uint32_t buf_sz)
{
  uint32_t res;

  LCD_DC = 1;
  res    = SPI0_send_buff_to_display(buf, buf_sz);
  return res;
}

/*-----------------------------------------------------------------------------------------------------
  Turns on the display.

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void TFT_display_on(void)
{
  TFT_wr__cmd(0x29);  // display ON
}

/*-----------------------------------------------------------------------------------------------------
  Fills a rectangular area of the display with a fixed word.

  Parameters:
    w          - word to fill
    pixels_num - number of pixels to fill

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void TFT_fill_by_pixel(uint16_t w, uint32_t pixels_num)
{
  uint16_t *ptr;

  ptr = video_buffer;
  for (uint32_t i = 0; i < pixels_num; i++)
  {
    *ptr = w;
    ptr++;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Clears the display.

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void TFT_clear(void)
{
  uint16_t w = 0x07E0;
  TFT_fill_by_pixel(w, LCD_X_SIZE * LCD_Y_SIZE);
  TFT_set_display_stream();
  TFT_wr_data_buf(video_buffer, LCD_X_SIZE * LCD_Y_SIZE * 2);
}

/*-----------------------------------------------------------------------------------------------------
  Sets a rectangular area on the display.

  Parameters:
    x0, y0 - top-left corner coordinates
    x1, y1 - bottom-right corner coordinates

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void TFT_Set_rect(int x0, int y0, int x1, int y1)
{
  uint16_t dbuf[2];
  uint16_t xaddr0;
  uint16_t xaddr1;
  uint16_t yaddr0;
  uint16_t yaddr1;
  uint8_t *p = (uint8_t *)dbuf;

  if (x0 > LCD_MAX_X) x0 = LCD_MAX_X;
  if (y0 > LCD_MAX_Y) y0 = LCD_MAX_Y;

  if (x1 > LCD_MAX_X) x1 = LCD_MAX_X;
  if (y1 > LCD_MAX_Y) y1 = LCD_MAX_Y;

  xaddr0 = X_OFFSET + x0;
  xaddr1 = X_OFFSET + x1;

  yaddr0 = Y_OFFSET + y0;
  yaddr1 = Y_OFFSET + y1;

  TFT_wr__cmd(0x2A);
  p[0] = (xaddr0 >> 8) & 0xFF;
  p[1] = xaddr0 & 0xFF;
  p[2] = (xaddr1 >> 8) & 0xFF;
  p[3] = xaddr1 & 0xFF;
  TFT_wr_data(p[0]);
  TFT_wr_data(p[1]);
  TFT_wr_data(p[2]);
  TFT_wr_data(p[3]);

  TFT_wr__cmd(0x2B);
  p[0] = (yaddr0 >> 8) & 0xFF;
  p[1] = yaddr0 & 0xFF;
  p[2] = (yaddr1 >> 8) & 0xFF;
  p[3] = yaddr1 & 0xFF;
  TFT_wr_data(p[0]);
  TFT_wr_data(p[1]);
  TFT_wr_data(p[2]);
  TFT_wr_data(p[3]);
}

/*-----------------------------------------------------------------------------------------------------
  Sets the display to stream mode.

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void TFT_set_display_stream(void)
{
  TFT_Set_rect(0, 0, LCD_X_SIZE - 1, LCD_Y_SIZE - 1);
  TFT_wr__cmd(0x2C);
}

/*-----------------------------------------------------------------------------------------------------
  Gets the screen orientation word.

  Parameters:
    None

  Return:
    uint8_t Screen orientation word
-----------------------------------------------------------------------------------------------------*/
uint8_t Get_screen_orientation_word(void)
{
  uint8_t w;

  switch (wvar.display_orientation)
  {
    case 0:
      w = 0 + LSHIFT(0, 7)  // MY  - Page Address Order       | 0 = Top to Bottom, 1 = Bottom to Top
          + LSHIFT(0, 6)    // MX  - Column Address Order     | 0 = Left to Right, 1 = Right to Left
          + LSHIFT(0, 5)    // MV  - Page/Column Order        | 0 = Normal Mode, 1 = Reverse Mode
          + LSHIFT(0, 4)    // ML  - Line Address Order       | 0 = LCD Refresh Top to Bottom, 1 = LCD Refresh Bottom to Top
          + LSHIFT(0, 3)    // RGB - RGB/BGR Order            | 0 = RGB, 1 = BGR
          + LSHIFT(0, 2)    // MH  - Display Data Latch Order | 0 = LCD Refresh Left to Right, 1 = LCD Refresh Right to Left
      ;
      X_OFFSET = 0;
      Y_OFFSET = 0;
      break;
    case 1:
      w = 0 + LSHIFT(0, 7)  // MY  - Page Address Order       | 0 = Top to Bottom, 1 = Bottom to Top
          + LSHIFT(1, 6)    // MX  - Column Address Order     | 0 = Left to Right, 1 = Right to Left
          + LSHIFT(1, 5)    // MV  - Page/Column Order        | 0 = Normal Mode, 1 = Reverse Mode
          + LSHIFT(0, 4)    // ML  - Line Address Order       | 0 = LCD Refresh Top to Bottom, 1 = LCD Refresh Bottom to Top
          + LSHIFT(0, 3)    // RGB - RGB/BGR Order            | 0 = RGB, 1 = BGR
          + LSHIFT(0, 2)    // MH  - Display Data Latch Order | 0 = LCD Refresh Left to Right, 1 = LCD Refresh Right to Left
      ;
      X_OFFSET = 0;
      Y_OFFSET = 0;
      break;
    case 2:
      w = 0 + LSHIFT(1, 7)  // MY  - Page Address Order       | 0 = Top to Bottom, 1 = Bottom to Top   // 1 не работает 0 не работает
          + LSHIFT(1, 6)    // MX  - Column Address Order     | 0 = Left to Right, 1 = Right to Left   // 1             1
          + LSHIFT(0, 5)    // MV  - Page/Column Order        | 0 = Normal Mode, 1 = Reverse Mode
          + LSHIFT(0, 4)    // ML  - Line Address Order       | 0 = LCD Refresh Top to Bottom, 1 = LCD Refresh Bottom to Top
          + LSHIFT(0, 3)    // RGB - RGB/BGR Order            | 0 = RGB, 1 = BGR
          + LSHIFT(0, 2)    // MH  - Display Data Latch Order | 0 = LCD Refresh Left to Right, 1 = LCD Refresh Right to Left
      ;
      X_OFFSET = 0;
      Y_OFFSET = 0x50;
      break;
    case 3:
      w = 0 + LSHIFT(1, 7)  // MY  - Page Address Order       | 0 = Top to Bottom, 1 = Bottom to Top
          + LSHIFT(0, 6)    // MX  - Column Address Order     | 0 = Left to Right, 1 = Right to Left
          + LSHIFT(1, 5)    // MV  - Page/Column Order        | 0 = Normal Mode, 1 = Reverse Mode
          + LSHIFT(0, 4)    // ML  - Line Address Order       | 0 = LCD Refresh Top to Bottom, 1 = LCD Refresh Bottom to Top
          + LSHIFT(0, 3)    // RGB - RGB/BGR Order            | 0 = RGB, 1 = BGR
          + LSHIFT(0, 2)    // MH  - Display Data Latch Order | 0 = LCD Refresh Left to Right, 1 = LCD Refresh Right to Left
      ;
      X_OFFSET = 0x50;
      Y_OFFSET = 0;
      break;
  }
  return w;
}

/*-----------------------------------------------------------------------------------------------------
  Initializes the TFT display.

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void TFT_init(void)
{
  uint8_t tmp;

  LCD_BLK = 1;
  LCD_RST = 0;
  Wait_ms(10);
  LCD_RST = 1;

  Wait_ms(100);
  TFT_wr__cmd(0x11);  // exit SLEEP mode
  Wait_ms(100);
  TFT_wr__cmd(0x36);

  tmp = Get_screen_orientation_word();

  TFT_wr_data(tmp);   // MADCTL: memory data access control
  TFT_wr__cmd(0x3A);
  TFT_wr_data(0x55);  // COLMOD: 16 bit/pixel
  TFT_wr__cmd(0xC6);
  TFT_wr_data(0x0F);  // FRCTRL2: Frame Rate control in normal mode
  TFT_wr__cmd(0x21);  // Inverse

  Wait_ms(1);
}
