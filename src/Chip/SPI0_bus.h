#ifndef SPI0_BUS_H
#define SPI0_BUS_H

extern spi_b_instance_ctrl_t g_SPI0_ctrl;

#ifndef SPI0_callback
void SPI0_callback(spi_callback_args_t* p_args);
#endif

// Indices for predefined SPI0 speeds
#define SPI0_SPEED_20MHZ             0
#define SPI0_SPEED_10MHZ             1
#define SPI0_SPEED_6MHZ              2
#define SPI0_SPEED_5MHZ              3
#define SPI0_SPEED_4MHZ              4
#define SPI0_SPEED_3MHZ              5
#define SPI0_SPEED_1MHZ              6
#define SPI0_SPEED_500KHZ            7

// Event flag definitions
#define SPI0_EVENT_TRANSFER_COMPLETE 0x01
#define SPI0_EVENT_TRANSFER_ABORTED  0x02
#define SPI0_EVENT_TRANSFER_ERROR    0x04

extern TX_MUTEX g_spi0_bus_mutex;

void     SPI0_open(void);
void     SPI0_callback(spi_callback_args_t* arg);
uint32_t SPI0_wait_transfer_complete(uint32_t timeout_ticks);
void     SPI0_close(void);
void     SPI0_set_speed(uint8_t speed_idx, spi_clk_polarity_t cpol, spi_clk_phase_t cpha);

#endif  // SPI0_BUS_H
