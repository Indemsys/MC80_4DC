// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2019.04.21
// 12:42:33
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "App.h"

#define PIN_D(x, y) (uint32_t *)&(R_PFS->PORT[x].PIN[y]), x, y

// clang-format off

const T_IO_pins_configuration MC80_pins_cfg[] =
{
  { "P000", PIN_D(0,  0), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL__EN ,PORT__IO ,GP_INP ,0 , "AN100       "  },   //  MD1_I_PWR, MD2_I_PWR
  { "P001", PIN_D(0,  1), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL__EN ,PORT__IO ,GP_INP ,0 , "AN101       "  },   //  MD1_SPEED, MD2_SPEED
  { "P002", PIN_D(0,  2), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL__EN ,PORT__IO ,GP_INP ,0 , "AN102       "  },   //  MD1_POS, MD2_POS
  { "P003", PIN_D(0,  3), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL__EN ,PORT__IO ,GP_INP ,0 , "AN104       "  },   //  +5V measuring
  { "P004", PIN_D(0,  4), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL__EN ,PORT__IO ,GP_INP ,0 , "AN000       "  },   //  MD1_I_U, MD2_I_U
  { "P005", PIN_D(0,  5), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL__EN ,PORT__IO ,GP_INP ,0 , "AN001       "  },   //  MD1_I_V, MD2_I_V
  { "P006", PIN_D(0,  6), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL__EN ,PORT__IO ,GP_INP ,0 , "AN002       "  },   //  MD1_I_W, MD2_I_W
  { "P007", PIN_D(0,  7), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL__EN ,PORT__IO ,GP_INP ,0 , "AN004       "  },   //  +24V measuring
  { "P009", PIN_D(0,  9), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL__EN ,PORT__IO ,GP_INP ,0 , "AN006       "  },   //  MD2_V_U, MD2_V_V, MD2_V_W
  { "P010", PIN_D(0, 10), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL__EN ,PORT__IO ,GP_INP ,0 , "AN005       "  },   //  MD1_V_U, MD1_V_V, MD1_V_W
  { "P014", PIN_D(0, 14), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL__EN ,PORT__IO ,GP_INP ,0 , "DA0         "  },   //  Sound
  { "P015", PIN_D(0, 15), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL__EN ,PORT__IO ,GP_INP ,0 , "AN105       "  },   //  VREF measuring
  { "P100", PIN_D(1,  0), PSEL_28 ,PULLUP_DIS ,OD_DIS ,NO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "OSPI_SIO0   "  },   //  SPI NOR FLASH
  { "P101", PIN_D(1,  1), PSEL_28 ,PULLUP_DIS ,OD_DIS ,NO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "OSPI_SIO3   "  },   //  SPI NOR FLASH
  { "P102", PIN_D(1,  2), PSEL_28 ,PULLUP_DIS ,OD_DIS ,NO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "OSPI_SIO4   "  },   //  SPI NOR FLASH
  { "P103", PIN_D(1,  3), PSEL_28 ,PULLUP_DIS ,OD_DIS ,NO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "OSPI_SIO2   "  },   //  SPI NOR FLASH
  { "P105", PIN_D(1,  5), PSEL_03 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "GTIOC1A     "  },   //  MD1 Phase VH
  { "P106", PIN_D(1,  6), PSEL_28 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "OSPI_RESET  "  },   //  SPI NOR FLASH
  { "P107", PIN_D(1,  7), PSEL_28 ,PULLUP_DIS ,OD_DIS ,HI_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "OSPI_CS0    "  },   //  SPI NOR FLASH
  { "P112", PIN_D(1, 12), PSEL_03 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "GTIOC3B     "  },   //  MD2 Phase UL
  { "P113", PIN_D(1, 13), PSEL_03 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "GTIOC2A     "  },   //  MD1 Phase WH
  { "P114", PIN_D(1, 14), PSEL_03 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "GTIOC2B     "  },   //  MD1 Phase WL
  { "P115", PIN_D(1, 15), PSEL_06 ,PULLUP_DIS ,OD_DIS ,MI_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "SPI0_MOSI   "  },   //
  { "P201", PIN_D(2,  1), PSEL_00 ,PULLUP__EN ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT__IO ,GP_INP ,0 , "MD          "  },   //
  { "P202", PIN_D(2,  2), PSEL_16 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "CAN_RX      "  },   //
  { "P203", PIN_D(2,  3), PSEL_16 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "CAN_TX      "  },   //
  { "P204", PIN_D(2,  4), PSEL_03 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "GTIOC4B     "  },   //  MD2 Phase VL
  { "P205", PIN_D(2,  5), PSEL_03 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "GTIOC4A     "  },   //  MD2 Phase VH
  { "P206", PIN_D(2,  6), PSEL_00 ,PULLUP_DIS ,OD_DIS ,MI_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT__IO ,GP_OUT ,1 , "SPI0_SSLA2  "  },   //  Display CS
  { "P208", PIN_D(2,  8), PSEL_03 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "GTIOC1B     "  },   //  MD1 Phase VL
  { "P209", PIN_D(2,  9), PSEL_26 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "SWO         "  },   //
  { "P210", PIN_D(2, 10), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "SWDIO       "  },   //
  { "P211", PIN_D(2, 11), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "SWCLK       "  },   //
  { "P300", PIN_D(3,  0), PSEL_03 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "GTIOC3A     "  },   //  MD2 Phase UH
  { "P301", PIN_D(3,  1), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT__IO ,GP_OUT ,0 , "MD2_EN      "  },   //  MD2_EN
  { "P302", PIN_D(3,  2), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ__EN ,ANAL_DIS ,PORT__IO ,GP_INP ,0 , "IRQ5        "  },   //  IO extender IRQ
  { "P303", PIN_D(3,  3), PSEL_03 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "GTIOC7B     "  },   //  MD2_QUAD_B
  { "P304", PIN_D(3,  4), PSEL_26 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "TRACE_DATA3 "  },   //
  { "P305", PIN_D(3,  5), PSEL_26 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "TRACE_DATA2 "  },   //
  { "P306", PIN_D(3,  6), PSEL_26 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "TRACE_DATA1 "  },   //
  { "P307", PIN_D(3,  7), PSEL_26 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "TRACE_DATA0 "  },   //
  { "P308", PIN_D(3,  8), PSEL_26 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "TRACE_TCLK  "  },   //
  { "P309", PIN_D(3,  9), PSEL_05 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "SCI3_RXD    "  },   //  RS485/ESP RX
  { "P310", PIN_D(3, 10), PSEL_05 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "SCI3_TXD    "  },   //  RS485/ESP TX
  { "P311", PIN_D(3, 11), PSEL_15 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "SCI3_DE     "  },   //  RS485_DE
  { "P312", PIN_D(3, 12), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT__IO ,GP_OUT ,1 , "P312        "  },   //  ICM-42605 CS
  { "P313", PIN_D(3, 13), PSEL_06 ,PULLUP_DIS ,OD_DIS ,MI_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "SPI0_MISO   "  },   //
  { "P400", PIN_D(4,  0), PSEL_21 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "SDHI1_CLK   "  },   //
  { "P401", PIN_D(4,  1), PSEL_21 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "SDHI1_CMD   "  },   //
  { "P402", PIN_D(4,  2), PSEL_21 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "SDHI1_DAT0  "  },   //
  { "P403", PIN_D(4,  3), PSEL_21 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "SDHI1_DAT1  "  },   //
  { "P404", PIN_D(4,  4), PSEL_21 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "SDHI1_DAT2  "  },   //
  { "P405", PIN_D(4,  5), PSEL_21 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "SDHI1_DAT3  "  },   //
  { "P406", PIN_D(4,  6), PSEL_00 ,PULLUP_DIS ,OD_DIS ,HI_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT__IO ,GP_OUT ,1 , "SPI0_SSLA3  "  },   //  IO Extender CS
  { "P407", PIN_D(4,  7), PSEL_19 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "USBFS_VBUS  "  },   //
  { "P408", PIN_D(4,  8), PSEL_03 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "GTIOC10A    "  },   //  MD1_ENC_W
  { "P410", PIN_D(4, 10), PSEL_03 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "GTIOC9B     "  },   //  MD1_ENC_V
  { "P411", PIN_D(4, 11), PSEL_03 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "GTIOC9A     "  },   //  MD1_ENC_U
  { "P412", PIN_D(4, 12), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT__IO ,GP_OUT ,1 , "IO_EXP_RESET"  },   //  IO Expander reset
  { "P413", PIN_D(4, 13), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT__IO ,GP_OUT ,0 , "P413        "  },   //  MD1/MD2 Analog channels sel
  { "P414", PIN_D(4, 14), PSEL_03 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "GTIOC0B     "  },   //  MD1 Phase UL
  { "P415", PIN_D(4, 15), PSEL_03 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "GTIOC0A     "  },   //  MD1 Phase UH
  { "P511", PIN_D(5, 11), PSEL_07 ,PULLUP_DIS ,OD_DIS ,MI_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "I2C1_SDA    "  },   //  I2C1_SDA
  { "P512", PIN_D(5, 12), PSEL_07 ,PULLUP_DIS ,OD_DIS ,MI_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "I2C1_SCL    "  },   //  I2C1_SCL
  { "P513", PIN_D(5, 13), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL__EN ,PORT__IO ,GP_INP ,0 , "AN016       "  },   //  AN016
  { "P600", PIN_D(6,  0), PSEL_03 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "GTIOC6B     "  },   //  MD1_QUAD_B
  { "P602", PIN_D(6,  2), PSEL_04 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "SCI0_RX     "  },   //  ICM-42605 SDO
  { "P603", PIN_D(6,  3), PSEL_03 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "GTIOC7A     "  },   //  MD2_QUAD_A
  { "P604", PIN_D(6,  4), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT__IO ,GP_INP ,0 , "GTIOC8B     "  },   //  Manual Encoder A
  { "P605", PIN_D(6,  5), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT__IO ,GP_INP ,0 , "GTIOC8A     "  },   //  Manual encoder B
  { "P609", PIN_D(6,  9), PSEL_04 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "SCI0_TX     "  },   //  ICM-42605 SDI
  { "P610", PIN_D(6, 10), PSEL_06 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "SPI0_RSPCK  "  },   //
  { "P611", PIN_D(6, 11), PSEL_04 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "SCI0_SCK    "  },   //  ICM-42605 CLK
  { "P612", PIN_D(6, 12), PSEL_00 ,PULLUP_DIS ,OD_DIS ,MI_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT__IO ,GP_OUT ,1 , "SPI0_SSLA0  "  },   //  MD1 CS
  { "P613", PIN_D(6, 13), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT__IO ,GP_OUT ,0 , "P613        "  },   //  MD1/MD2 mux A0
  { "P614", PIN_D(6, 14), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT__IO ,GP_OUT ,0 , "P614        "  },   //  MD1/MD2 mux A1
  { "P700", PIN_D(7,  0), PSEL_03 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "GTIOC5A     "  },   //  MD2 Phase WH
  { "P701", PIN_D(7,  1), PSEL_03 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "GTIOC5B     "  },   //  MD2 Phase WL
  { "P702", PIN_D(7,  2), PSEL_03 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "GTIOC6A     "  },   //  MD1_QUAD_A
  { "P703", PIN_D(7,  3), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT__IO ,GP_OUT ,0 , "P703        "  },   //  Display reset
  { "P704", PIN_D(7,  4), PSEL_00 ,PULLUP_DIS ,OD_DIS ,MI_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT__IO ,GP_OUT ,1 , "SPI0_SSLA1  "  },   //  MD2 CS
  { "P705", PIN_D(7,  5), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT__IO ,GP_OUT ,0 , "P705        "  },   //  Display D/C
  { "P708", PIN_D(7,  8), PSEL_00 ,PULLUP__EN ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT__IO ,GP_INP ,0 , "MD2_FAULT   "  },   //  MD2_FAULT
  { "P709", PIN_D(7,  9), PSEL_00 ,PULLUP__EN ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT__IO ,GP_INP ,0 , "MD1_FAULT   "  },   //  MD1_FAULT
  { "P710", PIN_D(7, 10), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT__IO ,GP_OUT ,0 , "P710        "  },   //  Display BLK
  { "P711", PIN_D(7, 11), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT__IO ,GP_INP ,0 , "P711        "  },   //  MD2_ENC_U
  { "P712", PIN_D(7, 12), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT__IO ,GP_INP ,0 , "P712        "  },   //  MD2_ENC_V
  { "P713", PIN_D(7, 13), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT__IO ,GP_INP ,0 , "P713        "  },   //  MD2_ENC_W
  { "P800", PIN_D(8,  0), PSEL_28 ,PULLUP_DIS ,OD_DIS ,NO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "OSPI_SIO5   "  },   //
  { "P801", PIN_D(8,  1), PSEL_28 ,PULLUP_DIS ,OD_DIS ,NO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "OSPI_DQS    "  },   //
  { "P802", PIN_D(8,  2), PSEL_28 ,PULLUP_DIS ,OD_DIS ,NO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "OSPI_SIO6   "  },   //
  { "P803", PIN_D(8,  3), PSEL_28 ,PULLUP_DIS ,OD_DIS ,NO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "OSPI_SIO1   "  },   //
  { "P804", PIN_D(8,  4), PSEL_28 ,PULLUP_DIS ,OD_DIS ,NO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "OSPI_SIO7   "  },   //
  { "P805", PIN_D(8,  5), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL__EN ,PORT__IO ,GP_INP ,0 , "AN117       "  },   //
  { "P806", PIN_D(8,  6), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ__EN ,ANAL_DIS ,PORT__IO ,GP_INP ,0 , "IRQ0        "  },   //  ICM-42605 IRQ0
  { "P808", PIN_D(8,  8), PSEL_28 ,PULLUP_DIS ,OD_DIS ,NO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT_PER ,GP_INP ,0 , "OSPI_SCLK   "  },   //
  { "P905", PIN_D(9,  5), PSEL_00 ,PULLUP_DIS ,OD_DIS ,LO_DRV ,EV_DNC ,IRQ_DIS ,ANAL_DIS ,PORT__IO ,GP_OUT ,0 , "MD1_EN      "  },   //  MD1_EN
};
// clang-format on
/*-----------------------------------------------------------------------------------------------------


  \param cgf
  \param sz
-----------------------------------------------------------------------------------------------------*/
static void Config_pins(const T_IO_pins_configuration *cfg, uint32_t sz)
{
  T_reg_PFS pfs;
  for (uint32_t i = 0; i < sz; i++)
  {
    memset(&pfs, 0, sizeof(pfs));
    pfs.bf.PODR  = cfg[i].init;
    pfs.bf.PDR   = cfg[i].dir;
    pfs.bf.PCR   = cfg[i].PCR;
    pfs.bf.NCODR = cfg[i].ODR;
    pfs.bf.DSCR  = cfg[i].DSCR;
    pfs.bf.ISEL  = cfg[i].ISEL;
    pfs.bf.ASEL  = cfg[i].ASEL;
    pfs.bf.PSEL  = cfg[i].PSEL;

    if (cfg[i].PORTM != 0)
    {
      pfs.bf.PMR  = 0;
      *cfg[i].PFS = pfs.word;
    }
    pfs.bf.PMR  = cfg[i].PORTM;
    *cfg[i].PFS = pfs.word;
  }
}

/*-----------------------------------------------------------------------------------------------------


  \param void

  \return uint32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t Get_board_pin_count(void)
{
  return sizeof(MC80_pins_cfg) / sizeof(MC80_pins_cfg[0]);
}

/*-----------------------------------------------------------------------------------------------------


    \param void
-----------------------------------------------------------------------------------------------------*/
void Config_pins_MC80_pins(void)
{
  Config_pins(MC80_pins_cfg, sizeof(MC80_pins_cfg) / sizeof(MC80_pins_cfg[0]));
}

/*-----------------------------------------------------------------------------------------------------


  \param pin_num
  \param dstr
  \param str_sz
-----------------------------------------------------------------------------------------------------*/
void Get_board_pin_conf_str(uint32_t pin_num, char *dstr)
{
  char      str[64];
  uint32_t  pin = pin_num;
  uint32_t  tsz = Get_board_pin_count();
  T_reg_PFS pfs;

  dstr[0] = 0;

  if (pin_num >= tsz)
  {
    return;
  }

  pfs.word = *MC80_pins_cfg[pin].PFS;

  sprintf(str, "  { \"P%01d%02d\", PIN_D(%d, %2d), ", MC80_pins_cfg[pin].port, MC80_pins_cfg[pin].pin, MC80_pins_cfg[pin].port, MC80_pins_cfg[pin].pin);
  strcat(dstr, str);
  sprintf(str, "PSEL_%02d ,", pfs.bf.PSEL);
  strcat(dstr, str);

  if (pfs.bf.PCR == 0)
  {
    strcat(dstr, "PULLUP_DIS ,");
  }
  else
  {
    strcat(dstr, "PULLUP__EN ,");
  }

  if (pfs.bf.NCODR == 0)
  {
    strcat(dstr, "OD_DIS ,");
  }
  else
  {
    strcat(dstr, "OD__EN ,");
  }

  switch (pfs.bf.DSCR)
  {
    case 0:
      strcat(dstr, "LO_DRV ,");
      break;
    case 1:
      strcat(dstr, "MI_DRV ,");
      break;
    case 2:
      strcat(dstr, "NO_DRV ,");
      break;
    case 3:
      strcat(dstr, "HI_DRV ,");
      break;
  }

  switch (pfs.bf.EOFR)
  {
    case 0:
      strcat(dstr, "EV_DNC ,");
      break;
    case 1:
      strcat(dstr, "EV_DRE ,");
      break;
    case 2:
      strcat(dstr, "EV_DFE ,");
      break;
    case 3:
      strcat(dstr, "EV_DBE ,");
      break;
  }

  if (pfs.bf.ISEL == 0)
  {
    strcat(dstr, "IRQ_DIS ,");
  }
  else
  {
    strcat(dstr, "IRQ__EN ,");
  }

  if (pfs.bf.ASEL == 0)
  {
    strcat(dstr, "ANAL_DIS ,");
  }
  else
  {
    strcat(dstr, "ANAL__EN ,");
  }

  if (pfs.bf.PMR == 0)
  {
    strcat(dstr, "PORT__IO ,");
  }
  else
  {
    strcat(dstr, "PORT_PER ,");
  }

  if (pfs.bf.PDR == 0)
  {
    strcat(dstr, "GP_INP ,");
  }
  else
  {
    strcat(dstr, "GP_OUT ,");
  }

  if (pfs.bf.PODR == 0)
  {
    strcat(dstr, "0 , ");
  }
  else
  {
    strcat(dstr, "1 , ");
  }

  strcat(dstr, "\"");
  strcat(dstr, MC80_pins_cfg[pin].net_name);
  strcat(dstr, "\"  },");
}

void Set_AUD_SHDN(int32_t v)
{
}

/*-----------------------------------------------------------------------------------------------------
  Configure P705 pin for GTADSM1 debug mode
  Changes P705 from GPIO LCD_DC to peripheral GTADSM1 function for ADC sampling observation

  Parameters: void

  Return: void
-----------------------------------------------------------------------------------------------------*/
void Config_pin_P705_GTADSM1_mode(void)
{
  // P705 configuration for GTADSM1 (GPT0 ADC sampling signal output)
  T_IO_pins_configuration p705_gtadsm1_cfg =
  {
    .name = "P705",
    .PFS = &R_PFS->PORT[7].PIN[5].PmnPFS,
    .port = 7,
    .pin = 5,
    .PSEL = PSEL_02,     // Peripheral function 2 for GTADSM1
    .PCR = PULLUP_DIS,   // Disable pull-up
    .ODR = OD_DIS,       // Disable open-drain
    .DSCR = LO_DRV,      // Low drive capability
    .EOFR = EV_DNC,      // Don't care for edge detection
    .ISEL = IRQ_DIS,     // Disable IRQ
    .ASEL = ANAL_DIS,    // Disable analog function
    .PORTM = PORT_PER,   // Use as peripheral function
    .dir = GP_OUT,       // Output direction
    .init = 0,           // Initial state
    .net_name = "GTADSM1"
  };

  // Configure single pin
  Config_pins(&p705_gtadsm1_cfg, 1);
}
