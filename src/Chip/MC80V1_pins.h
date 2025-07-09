#ifndef MC80_PINS_H
#define MC80_PINS_H

typedef struct
{
  const char        *name;
  volatile uint32_t *PFS;
  uint32_t           port;
  uint32_t           pin;

  uint8_t PSEL;       // The PSEL bits assign the peripheral function

  uint8_t PCR;        // 0: Disable input pull-up
                      // 1: Enable input pull-up.

  uint8_t ODR;        // 0 Open Drain output is disabled on the corresponding pin.
                      // 1 Open Drain output is enabled on the corresponding pin

  uint8_t DSCR;       // Port Drive Capability
                      // 00: Low drive
                      // 01: Middle drive
                      // 10: Setting prohibited
                      // 11: High drive.

  uint8_t EOFR;       // Event on Falling/Event on Rising
                      // 00: Don’t-care
                      // 01: Detect rising edge
                      // 10: Detect falling edge
                      // 11: Detect both edges

  uint8_t ISEL;       // IRQ Input Enable
                      // 0: Do not use as IRQn input pin
                      // 1: Use as IRQn input pin.

  uint8_t ASEL;       // Analog Input Enable
                      // 0: Do not use as analog pin
                      // 1: Use as analog pin.

  uint8_t PORTM;      // Port Mode Control
                      // 0: Use as general I/O pin
                      // 1: Use as I/O port for peripheral functions.

  uint8_t dir;        // 0 Pin is configured as general purpose input, if configured for the GPIO function
                      // 1 Pin is configured for general purpose output, if configured for the GPIO function
  uint8_t     init;   // Init state
  const char *net_name;  // Description

} T_IO_pins_configuration;

#define PULLUP_DIS 0  // 0: Disable input pull-up
#define PULLUP__EN 1  // 1: Enable input pull-up.

#define OD_DIS     0  // 0 Open Drain output is disabled on the corresponding pin.
#define OD__EN     1  // 1 Open Drain output is enabled on the corresponding pin.

#define LO_DRV     0  // 00: Low drive
#define MI_DRV     1  // 01: Middle drive
#define NO_DRV     2  // 10: Setting prohibited
#define HI_DRV     3  // 11: High drive.

#define EV_DNC     0  // 00: Don’t-care
#define EV_DRE     1  // 01: Detect rising edge
#define EV_DFE     2  // 10: Detect falling edge
#define EV_DBE     3  // 11: Detect both edges

#define IRQ_DIS    0  // 0: Do not use as IRQn input pin
#define IRQ__EN    1  // 1: Use as IRQn input pin.

#define ANAL_DIS   0  // 0: Do not use as IRQn input pin
#define ANAL__EN   1  // 1: Use as IRQn input pin.

#define PORT__IO   0  // 0: Use as general I/O pin
#define PORT_PER   1  // 1: Use as I/O port for peripheral functions.

                      // These bits select the peripheral function
#define PSEL_00    0   // IO  DEBUG
#define PSEL_01    1   // AGT  AGTW
#define PSEL_02    2   // GPT0
#define PSEL_03    3   // GPT1
#define PSEL_04    4   // SCI0_2_4_6_8
#define PSEL_05    5   // SCI1_3_5_7_9
#define PSEL_06    6   // SPI
#define PSEL_07    7   // IIC
#define PSEL_08    8   // KEY
#define PSEL_09    9   // CLKOUT_COMP_RTC
#define PSEL_10    10  // CAC_AD
#define PSEL_11    11  // BUS
#define PSEL_12    12  // CTSU  ACMPHS
#define PSEL_13    13  // LCDC   DE_SCI0_2_4_6_8
#define PSEL_14    14  // DALI   DE_SCI1_3_5_7_9
#define PSEL_15    15  // CEU
#define PSEL_16    16  // CAN
#define PSEL_17    17  // QSPI
#define PSEL_18    18  // SSI
#define PSEL_19    19  // USB_FS
#define PSEL_20    20  // USB_HS  GPT2
#define PSEL_21    21  // SDHI_MMC  GPT3
#define PSEL_22    22  // ETHER_MII GPT4 UARTA
#define PSEL_23    23  // ETHER_RMII
#define PSEL_24    24  // AGT1  PDC
#define PSEL_25    25  // LCD_GRAPHICS CAC
#define PSEL_26    26  // TRACE
#define PSEL_28    28  // OSPI
#define PSEL_29    29  // CEC  PGAOUT0
#define PSEL_30    30  // PGAOUT1  ULPT
#define PSEL_31    31  // MIPI

#define GP_INP     0   // 0 Pin is configured as general purpose input, if configured for the GPIO function
#define GP_OUT     1   // 1 Pin is configured for general purpose output, if configured for the GPIO function

// clang-format off
typedef union
{
  struct
  {
    __IOM uint32_t PODR   : 1;    // Port Output Data Register
                                  // 0: Output low level
                                  // 1: Output high level
    __IM uint32_t  PIDR   : 1;    // Port Input Data Register (Read-only)
                                  // 0: Pin input level is low
                                  // 1: Pin input level is high
    __IOM uint32_t PDR    : 1;    // Port Direction Register
                                  // 0: Input (high-impedance state)
                                  // 1: Output
    uint32_t              : 1;    //
    __IOM uint32_t PCR    : 1;    // Pull-up Control Register
                                  // 0: Disable input pull-up
                                  // 1: Enable input pull-up
    uint32_t              : 1;    //
    __IOM uint32_t NCODR  : 1;    // N-Channel Open Drain Control Register
                                  // 0: CMOS output
                                  // 1: N-channel open-drain output
    uint32_t              : 3;    //
    __IOM uint32_t DSCR   : 2;    // Drive Strength Control Register
                                  // 00: Low drive
                                  // 01: Middle drive
                                  // 10: Setting prohibited
                                  // 11: High drive
    __IOM uint32_t EOFR   : 2;    // Event on Falling/Event on Rising
                                  // 00: Don't-care
                                  // 01: Detect rising edge
                                  // 10: Detect falling edge
                                  // 11: Detect both edges
    __IOM uint32_t ISEL   : 1;    // IRQ Input Enable
                                  // 0: Do not use as IRQn input pin
                                  // 1: Use as IRQn input pin
    __IOM uint32_t ASEL   : 1;    // Analog Input Enable
                                  // 0: Do not use as analog pin
                                  // 1: Use as analog pin
    __IOM uint32_t PMR    : 1;    // Port Mode Control
                                  // 0: Use as general I/O pin (GPIO mode)
                                  // 1: Use as I/O port for peripheral functions
    uint32_t              : 7;    //
    __IOM uint32_t PSEL   : 5;    // Port Function Select (0-31)
                                  // These bits select the peripheral function
                                  // See PSEL_xx defines above for specific values
                                  // 0: GPIO/DEBUG, 1: AGT/AGTW, 2: GPT0, 3: GPT1, etc.
    uint32_t              : 3;
  } bf;                           // bit fields access
  uint32_t word;                  // word access
} T_reg_PFS;
// clang-format on

uint32_t Get_board_pin_count(void);
void     Get_board_pin_conf_str(uint32_t pin_num, char *dstr);
void     Config_pins_MC80_pins(void);
void     Config_pin_P705_GTADSM1_mode(void);

#endif
