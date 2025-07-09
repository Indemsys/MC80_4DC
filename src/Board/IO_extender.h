#ifndef IO_EXTENDER_H
#define IO_EXTENDER_H

#define AUD_STBY              0   // Output
#define LED_RS485             1   // Output
#define LED_CAN               2   // Output
#define LED_GR                3   // Output
#define LED_RD                4   // Output
#define SW5V_IN               5   // Output
#define SW5V_DIAG             6   // Output
#define CAN_EN                7   // Output
#define PGOOD                 8   // Input
#define SW5V_FAULT            9   // Input
#define ESP32_RSY             10  // Output
#define SCI3_MUX              11  // Output

#define MCP23S17_WRITE_OPCODE 0x40
#define MCP23S17_GPIOA        0x12
#define MCP23S17_GPIOB        0x13
#define MCP23S17_IODIRA       0x00
#define MCP23S17_IODIRB       0x01

uint32_t Configure_IO_extender(void);
uint32_t Write_to_IO_extender(uint8_t bit_id, uint8_t val);
uint32_t Write_multiple_bits_to_IO_extender(uint8_t led_gr, uint8_t led_rd, uint8_t led_can, uint8_t led_rs485);

#endif  // IO_EXTENDER_H
