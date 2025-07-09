/* generated configuration header file - do not edit */
#ifndef BSP_PIN_CFG_H_
#define BSP_PIN_CFG_H_
#include "r_ioport.h"

/* Common macro for FSP header files. There is also a corresponding FSP_FOOTER macro at the end of this file. */
FSP_HEADER

#define AN100 (BSP_IO_PORT_00_PIN_00) /* MD1_I_PWR, MD2_I_PWR */
#define AN101 (BSP_IO_PORT_00_PIN_01) /* MD1_SPEED, MD2_SPEED */
#define AN102 (BSP_IO_PORT_00_PIN_02) /* MD1_POS, MD2_POS */
#define AN104 (BSP_IO_PORT_00_PIN_03) /* +5V measuring */
#define AN000 (BSP_IO_PORT_00_PIN_04) /* MD1_I_U, MD2_I_U */
#define AN001 (BSP_IO_PORT_00_PIN_05) /* MD1_I_V, MD2_I_V */
#define AN002 (BSP_IO_PORT_00_PIN_06) /* MD1_I_W, MD2_I_W */
#define AN004 (BSP_IO_PORT_00_PIN_07) /* +24V measuring */
#define NONE (BSP_IO_PORT_00_PIN_08)
#define AN006 (BSP_IO_PORT_00_PIN_09) /* MD2_V_U, MD2_V_V, MD2_V_W */
#define AN005 (BSP_IO_PORT_00_PIN_10) /* MD1_V_U, MD1_V_V, MD1_V_W */
#define DA0 (BSP_IO_PORT_00_PIN_14) /* Sound */
#define AN105 (BSP_IO_PORT_00_PIN_15) /* VREF measuring */
#define OSPI_SIO0 (BSP_IO_PORT_01_PIN_00) /* SPI NOR FLASH */
#define OSPI_SIO3 (BSP_IO_PORT_01_PIN_01) /* SPI NOR FLASH */
#define OSPI_SIO4 (BSP_IO_PORT_01_PIN_02) /* SPI NOR FLASH */
#define OSPI_SIO2 (BSP_IO_PORT_01_PIN_03) /* SPI NOR FLASH */
#define GTIOC1A (BSP_IO_PORT_01_PIN_05) /* MD1 Phase VH */
#define OSPI_RESET (BSP_IO_PORT_01_PIN_06) /* SPI NOR FLASH */
#define OSPI_CS0 (BSP_IO_PORT_01_PIN_07) /* SPI NOR FLASH */
#define GTIOC3B (BSP_IO_PORT_01_PIN_12) /* MD2 Phase UL */
#define GTIOC2A (BSP_IO_PORT_01_PIN_13) /* MD1 Phase WH */
#define GTIOC2B (BSP_IO_PORT_01_PIN_14) /* MD1 Phase WL */
#define SPI0_MOSI (BSP_IO_PORT_01_PIN_15)
#define NMI (BSP_IO_PORT_02_PIN_00)
#define MD_PIN (BSP_IO_PORT_02_PIN_01)
#define CAN_RX (BSP_IO_PORT_02_PIN_02)
#define CAN_TX (BSP_IO_PORT_02_PIN_03)
#define GTIOC4B (BSP_IO_PORT_02_PIN_04) /* MD2 Phase VL */
#define GTIOC4A (BSP_IO_PORT_02_PIN_05) /* MD2 Phase VH */
#define SPI0_SSLA2 (BSP_IO_PORT_02_PIN_06) /* Display CS */
#define GTIOC1B (BSP_IO_PORT_02_PIN_08) /* MD1 Phase VL */
#define SWO (BSP_IO_PORT_02_PIN_09)
#define SWDIO (BSP_IO_PORT_02_PIN_10)
#define SWCLK (BSP_IO_PORT_02_PIN_11)
#define EXTAL (BSP_IO_PORT_02_PIN_12)
#define XTAL (BSP_IO_PORT_02_PIN_13)
#define GTIOC3A (BSP_IO_PORT_03_PIN_00) /* MD2 Phase UH */
#define P301 (BSP_IO_PORT_03_PIN_01) /* MD2_EN */
#define IRQ5 (BSP_IO_PORT_03_PIN_02) /* IO extender IRQ */
#define GTIOC7B (BSP_IO_PORT_03_PIN_03) /* MD2_QUAD_B */
#define TRACE_DATA3 (BSP_IO_PORT_03_PIN_04)
#define TRACE_DATA2 (BSP_IO_PORT_03_PIN_05)
#define TRACE_DATA1 (BSP_IO_PORT_03_PIN_06)
#define TRACE_DATA0 (BSP_IO_PORT_03_PIN_07)
#define TRACE_TCLK (BSP_IO_PORT_03_PIN_08)
#define SCI3_RXD (BSP_IO_PORT_03_PIN_09) /* RS485/ESP RX */
#define SCI3_TXD (BSP_IO_PORT_03_PIN_10) /* RS485/ESP TX */
#define SCI3_DE (BSP_IO_PORT_03_PIN_11) /* RS485_DE */
#define P312 (BSP_IO_PORT_03_PIN_12) /* ICM-42605 CS */
#define SPI0_MISO (BSP_IO_PORT_03_PIN_13)
#define SDHI1_CLK (BSP_IO_PORT_04_PIN_00)
#define SDHI1_CMD (BSP_IO_PORT_04_PIN_01)
#define SDHI1_DAT0 (BSP_IO_PORT_04_PIN_02)
#define SDHI1_DAT1 (BSP_IO_PORT_04_PIN_03)
#define SDHI1_DAT2 (BSP_IO_PORT_04_PIN_04)
#define SDHI1_DAT3 (BSP_IO_PORT_04_PIN_05)
#define SPI0_SSLA3 (BSP_IO_PORT_04_PIN_06) /* IO Extender CS */
#define USBFS_VBUS (BSP_IO_PORT_04_PIN_07)
#define GTIOC10A (BSP_IO_PORT_04_PIN_08) /* MD1_ENC_W */
#define GTIOC9B (BSP_IO_PORT_04_PIN_10) /* MD1_ENC_V */
#define GTIOC9A (BSP_IO_PORT_04_PIN_11) /* MD1_ENC_U */
#define IO_EXP_RESET (BSP_IO_PORT_04_PIN_12) /* IO Expander reset */
#define P413 (BSP_IO_PORT_04_PIN_13) /* MD1/MD2 Analog channels selection */
#define GTIOC0B (BSP_IO_PORT_04_PIN_14) /* MD1 Phase UL */
#define GTIOC0A (BSP_IO_PORT_04_PIN_15) /* MD1 Phase UH */
#define GTIOC6B (BSP_IO_PORT_06_PIN_00) /* MD1_QUAD_B */
#define SCI0_RX (BSP_IO_PORT_06_PIN_02) /* ICM-42605 SDO */
#define GTIOC7A (BSP_IO_PORT_06_PIN_03) /* MD2_QUAD_A */
#define GTIOC8B (BSP_IO_PORT_06_PIN_04) /* Manual Encoder A */
#define GTIOC8A (BSP_IO_PORT_06_PIN_05) /* Manual encoder B */
#define SCI0_TX (BSP_IO_PORT_06_PIN_09) /* ICM-42605 SDI */
#define SPI0_RSPCK (BSP_IO_PORT_06_PIN_10)
#define SCI0_SCK (BSP_IO_PORT_06_PIN_11) /* ICM-42605 CLK */
#define SPI0_SSLA0 (BSP_IO_PORT_06_PIN_12) /* MD1 CS */
#define P613 (BSP_IO_PORT_06_PIN_13) /* MD1/MD2 mux A0 */
#define P614 (BSP_IO_PORT_06_PIN_14) /* MD1/MD2 mux A1 */
#define GTIOC5A (BSP_IO_PORT_07_PIN_00) /* MD2 Phase WH */
#define GTIOC5B (BSP_IO_PORT_07_PIN_01) /* MD2 Phase WL */
#define GTIOC6A (BSP_IO_PORT_07_PIN_02) /* MD1_QUAD_A */
#define P703 (BSP_IO_PORT_07_PIN_03) /* Display reset */
#define SPI0_SSLA1 (BSP_IO_PORT_07_PIN_04) /* MD2 CS */
#define P705 (BSP_IO_PORT_07_PIN_05) /* Display D/C */
#define MD2_FAULT (BSP_IO_PORT_07_PIN_08)
#define MD1_FAULT (BSP_IO_PORT_07_PIN_09)
#define P710 (BSP_IO_PORT_07_PIN_10) /* Display BLK */
#define P711 (BSP_IO_PORT_07_PIN_11) /* MD2_ENC_U */
#define P712 (BSP_IO_PORT_07_PIN_12) /* MD2_ENC_V */
#define P713 (BSP_IO_PORT_07_PIN_13) /* MD2_ENC_W */
#define OSPI_SIO5 (BSP_IO_PORT_08_PIN_00)
#define OSPI_DQS (BSP_IO_PORT_08_PIN_01)
#define OSPI_SIO6 (BSP_IO_PORT_08_PIN_02)
#define OSPI_SIO1 (BSP_IO_PORT_08_PIN_03)
#define OSPI_SIO7 (BSP_IO_PORT_08_PIN_04)
#define AN117 (BSP_IO_PORT_08_PIN_05)
#define IRQ0 (BSP_IO_PORT_08_PIN_06) /* ICM-42605 IRQ0 */
#define OSPI_SCLK (BSP_IO_PORT_08_PIN_08)
#define USBFS_P (BSP_IO_PORT_08_PIN_14)
#define USBFS_N (BSP_IO_PORT_08_PIN_15)
#define P905 (BSP_IO_PORT_09_PIN_05) /* MD1_EN */

extern const ioport_cfg_t g_bsp_pin_cfg; /* R7FA8M1AHECFB.pincfg */

void BSP_PinConfigSecurityInit();

/* Common macro for FSP header files. There is also a corresponding FSP_HEADER macro at the top of this file. */
FSP_FOOTER
#endif /* BSP_PIN_CFG_H_ */
