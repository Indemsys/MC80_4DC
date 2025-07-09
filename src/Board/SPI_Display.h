#ifndef SPI_DISPLAY_H
#define SPI_DISPLAY_H

#define DISPLAY_ORIENTATION 0

#define LCD_X_SIZE          240
#define LCD_Y_SIZE          240

#define LCD_MAX_X           (LCD_X_SIZE - 1)
#define LCD_MAX_Y           (LCD_Y_SIZE - 1)

#define MAX_X_SZ            319
#define MAX_Y_SZ            239

void     TFT_display_on(void);
void     TFT_clear(void);
void     TFT_set_display_stream(void);
uint32_t TFT_wr_data_buf(uint16_t* buf, uint32_t buf_sz);
void     TFT_init(void);

#endif  // SPI_DISPLAY_H
