#ifndef MX25UM25645G_H
#define MX25UM25645G_H

/**
 * @file    mx25um25645g.h
 * @brief   Macronix MX25UM25645G 256Mbit 1.8V Octa/SPI NOR Flash Memory commands.
 *
 * All SPI and OPI commands from datasheet, with address/dummy/data byte info.
 */

/* ==== SPI COMMAND SET (1-byte opcodes) ==== */

/* --- Array Access --- */
#define MX25_CMD_READ3B             0x03 /**< Normal Read; 3-byte addr, 0 dummy, N data */
#define MX25_CMD_FAST_READ3B        0x0B /**< Fast Read; 3-byte addr, 8 dummy, N data */
#define MX25_CMD_PP3B               0x02 /**< Page Program; 3-byte addr, 0 dummy, 1–256 data */
#define MX25_CMD_SE3B               0x20 /**< Sector Erase 4KB; 3-byte addr, 0 dummy, 0 data */
#define MX25_CMD_BE3B               0xD8 /**< Block Erase 64KB; 3-byte addr, 0 dummy, 0 data */
#define MX25_CMD_READ4B             0x13 /**< Normal Read; 4-byte addr, 0 dummy, N data */
#define MX25_CMD_FAST_READ4B        0x0C /**< Fast Read; 4-byte addr, 8 dummy, N data */
#define MX25_CMD_PP4B               0x12 /**< Page Program; 4-byte addr, 0 dummy, 1–256 data */
#define MX25_CMD_SE4B               0x21 /**< Sector Erase 4KB; 4-byte addr, 0 dummy, 0 data */
#define MX25_CMD_BE4B               0xDC /**< Block Erase 64KB; 4-byte addr, 0 dummy, 0 data */
#define MX25_CMD_CE                 0x60 /**< Chip Erase; no address, 0 dummy, 0 data */
#define MX25_CMD_CE_ALT             0xC7 /**< Chip Erase (alt code) */

/* --- Device Operation --- */
#define MX25_CMD_WREN               0x06 /**< Write Enable */
#define MX25_CMD_WRDI               0x04 /**< Write Disable */
#define MX25_CMD_WPSEL              0x68 /**< Write Protect Selection */
#define MX25_CMD_PGMERS_SUSPEND     0xB0 /**< Program/Erase Suspend */
#define MX25_CMD_PGMERS_RESUME      0x30 /**< Program/Erase Resume */
#define MX25_CMD_DP                 0xB9 /**< Deep Power Down */
#define MX25_CMD_RDP                0xAB /**< Release from Deep Power Down */
#define MX25_CMD_NOP                0x00 /**< No Operation */
#define MX25_CMD_RSTEN              0x66 /**< Reset Enable */
#define MX25_CMD_RST                0x99 /**< Reset Memory */
#define MX25_CMD_GBLK               0x7E /**< Gang Block Lock */
#define MX25_CMD_GBULK              0x98 /**< Gang Block Unlock */

/* --- Register/Identification --- */
#define MX25_CMD_RDID               0x9F /**< Read Identification; 0 addr, 0 dummy, 3 data */
#define MX25_CMD_RDSFDP             0x5A /**< Read SFDP Table; 3-byte addr, 8 dummy, 0 data */
#define MX25_CMD_RDSR               0x05 /**< Read Status Register; 0 addr, 0 dummy, 1 data */
#define MX25_CMD_RDCR               0x15 /**< Read Config Register; 0 addr, 0 dummy, 1 data */
#define MX25_CMD_WRSR               0x01 /**< Write Status/Config Register; 0 addr, 0 dummy, 1–2 data */
#define MX25_CMD_RDCR2              0x71 /**< Read Config Register 2; 4-byte addr, 0 dummy, 1 data */
#define MX25_CMD_WRCR2              0x72 /**< Write Config Register 2; 4-byte addr, 0 dummy, 1 data */
#define MX25_CMD_RDSCUR             0x2B /**< Read Security Register; 0 addr, 0 dummy, 1 data */
#define MX25_CMD_WRSCUR             0x2F /**< Write Security Register; 0 addr, 0 dummy, 1 data */
#define MX25_CMD_RDFBR              0x16 /**< Read Fast Boot Register; 0 addr, 0 dummy, 1–4 data */
#define MX25_CMD_WRFBR              0x17 /**< Write Fast Boot Register; 0 addr, 0 dummy, 4 data */
#define MX25_CMD_ESFBR              0x18 /**< Erase Fast Boot Register; 0 addr, 0 dummy, 0 data */
#define MX25_CMD_SBL                0xC0 /**< Set Burst Length; 0 addr, 0 dummy, 1 data */
#define MX25_CMD_ENSO               0xB1 /**< Enter Secured OTP; 0 addr, 0 dummy, 0 data */
#define MX25_CMD_EXSO               0xC1 /**< Exit Secured OTP; 0 addr, 0 dummy, 0 data */
#define MX25_CMD_WRLR               0x2C /**< Write Lock Register; 0 addr, 0 dummy, 1 data */
#define MX25_CMD_RDLR               0x2D /**< Read Lock Register; 0 addr, 0 dummy, 1 data */
#define MX25_CMD_WRSPB              0xE3 /**< SPB Bit Program; 4-byte addr, 0 dummy, 0 data */
#define MX25_CMD_WRDPB              0xE1 /**< Write DPB Register; 4-byte addr, 0 dummy, 1 data */
#define MX25_CMD_RDDPB              0xE0 /**< Read DPB Register; 4-byte addr, 0 dummy, 1 data */
#define MX25_CMD_ESSPB              0xE4 /**< All SPB Bit Erase; 0 addr, 0 dummy, 0 data */
#define MX25_CMD_RDSPB              0xE2 /**< Read SPB Status; 4-byte addr, 0 dummy, 1 data */
#define MX25_CMD_RDPASS             0x27 /**< Read Password Register; 4-byte addr=0, 8 dummy, 8 data */
#define MX25_CMD_WRPASS             0x28 /**< Write Password Register; 4-byte addr=0, 0 dummy, 8 data */
#define MX25_CMD_PASSULK            0x29 /**< Password Unlock; 4-byte addr=0, 0 dummy, 8 data */

/* ==== OPI COMMAND SET (Octal, STR/DTR, usually 2-byte opcodes) ==== */
/* Format: <STR opcode> / <DTR opcode> */

/* --- Array Access --- */
#define MX25_OPI_8READ_STR          0xEC13 /**< Octal Read, STR (4-byte addr, 6–20 dummy, N data) */
#define MX25_OPI_8READ_DTR          0xEE11 /**< Octal Read, DTR (4-byte addr, 6–20 dummy, N data) */
#define MX25_OPI_PP4B_STR           0x12ED /**< Page Program, STR (4-byte addr, 0 dummy, 1–256 data) */
#define MX25_OPI_PP4B_DTR           0xEDED /**< Page Program, DTR (4-byte addr, 0 dummy, 1–256 data) */
#define MX25_OPI_SE4B_STR           0x21DE /**< Sector Erase 4KB, STR (4-byte addr, 0 dummy, 0 data) */
#define MX25_OPI_SE4B_DTR           0xDEDE /**< Sector Erase 4KB, DTR (4-byte addr, 0 dummy, 0 data) */
#define MX25_OPI_BE4B_STR           0xDC23 /**< Block Erase 64KB, STR (4-byte addr, 0 dummy, 0 data) */
#define MX25_OPI_BE4B_DTR           0x2323 /**< Block Erase 64KB, DTR (4-byte addr, 0 dummy, 0 data) */
#define MX25_OPI_CE_STR             0x609F /**< Chip Erase, STR (no address, no dummy, 0 data) */
#define MX25_OPI_CE_DTR             0xC738 /**< Chip Erase, DTR (no address, no dummy, 0 data) */

/* --- Device Operation --- */
#define MX25_OPI_WREN_STR           0x06F9 /**< Write Enable */
#define MX25_OPI_WREN_DTR           0xF9F9 /**< Write Enable */
#define MX25_OPI_WRDI_STR           0x04FB /**< Write Disable */
#define MX25_OPI_WRDI_DTR           0xFBFB /**< Write Disable */
#define MX25_OPI_WPSEL_STR          0x6897 /**< Write Protect Selection */
#define MX25_OPI_WPSEL_DTR          0x9797 /**< Write Protect Selection */
#define MX25_OPI_PGMERS_SUSPEND_STR 0xB04F /**< Suspend Program/Erase */
#define MX25_OPI_PGMERS_SUSPEND_DTR 0x4F4F /**< Suspend Program/Erase */
#define MX25_OPI_PGMERS_RESUME_STR  0x30CF /**< Resume Program/Erase */
#define MX25_OPI_PGMERS_RESUME_DTR  0xCFCF /**< Resume Program/Erase */
#define MX25_OPI_DP_STR             0xB946 /**< Deep Power-down */
#define MX25_OPI_DP_DTR             0x4646 /**< Deep Power-down */
#define MX25_OPI_RDP_STR            0xAB54 /**< Release from Deep Power-down */
#define MX25_OPI_RDP_DTR            0x5454 /**< Release from Deep Power-down */
#define MX25_OPI_NOP_STR            0x00FF /**< No Operation */
#define MX25_OPI_NOP_DTR            0xFFFF /**< No Operation */
#define MX25_OPI_RSTEN_STR          0x6699 /**< Reset Enable */
#define MX25_OPI_RSTEN_DTR          0x9999 /**< Reset Enable */
#define MX25_OPI_RST_STR            0x9966 /**< Reset */
#define MX25_OPI_RST_DTR            0x6666 /**< Reset */
#define MX25_OPI_GBLK_STR           0x7E81 /**< Gang Block Lock */
#define MX25_OPI_GBLK_DTR           0x8181 /**< Gang Block Lock */
#define MX25_OPI_GBULK_STR          0x9867 /**< Gang Block Unlock */
#define MX25_OPI_GBULK_DTR          0x6767 /**< Gang Block Unlock */

/* --- Register/Identification --- */
#define MX25_OPI_RDID_STR           0x9F60 /**< Read Identification; 4-byte addr=0, 4 dummy, 3 data */
#define MX25_OPI_RDID_DTR           0x6060 /**< Read Identification */
#define MX25_OPI_RDSFDP_STR         0x5AA5 /**< Read SFDP Table; 4-byte addr, 20 dummy, 0 data */
#define MX25_OPI_RDSFDP_DTR         0xA5A5 /**< Read SFDP Table */
#define MX25_OPI_RDSR_STR           0x05FA /**< Read Status Register; 4-byte addr=0, 4 dummy, 1 data */
#define MX25_OPI_RDSR_DTR           0xFAFA /**< Read Status Register */
#define MX25_OPI_RDCR_STR           0x15EA /**< Read Config Register; 4-byte addr=0, 4 dummy, 1 data */
#define MX25_OPI_RDCR_DTR           0xEAEA /**< Read Config Register */
#define MX25_OPI_WRSR_STR           0x01FE /**< Write Status Register; 4-byte addr=0, 0 dummy, 1 data */
#define MX25_OPI_WRSR_DTR           0xFEFE /**< Write Status Register */
#define MX25_OPI_WRCR_STR           0x01FE /**< Write Config Register; 4-byte addr=0, 0 dummy, 1 data */
#define MX25_OPI_WRCR_DTR           0xFEFE /**< Write Config Register */
#define MX25_OPI_RDCR2_STR          0x718E /**< Read Config Register 2; 4-byte addr, 4 dummy, 1 data */
#define MX25_OPI_RDCR2_DTR          0x8E8E /**< Read Config Register 2 */
#define MX25_OPI_WRCR2_STR          0x728D /**< Write Config Register 2; 4-byte addr, 0 dummy, 1 data */
#define MX25_OPI_WRCR2_DTR          0x8D8D /**< Write Config Register 2 */
#define MX25_OPI_RDSCUR_STR         0x2BD4 /**< Read Security Register; 4-byte addr=0, 4 dummy, 1 data */
#define MX25_OPI_RDSCUR_DTR         0xD4D4 /**< Read Security Register */
#define MX25_OPI_WRSCUR_STR         0x2FD0 /**< Write Security Register; 0 addr, 0 dummy, 0 data */
#define MX25_OPI_WRSCUR_DTR         0xD0D0 /**< Write Security Register */
#define MX25_OPI_SBL_STR            0xC03F /**< Set Burst Length; 4-byte addr=0, 0 dummy, 1 data */
#define MX25_OPI_SBL_DTR            0x3F3F /**< Set Burst Length */
#define MX25_OPI_ENSO_STR           0xB14E /**< Enter Secured OTP; 0 addr, 0 dummy, 0 data */
#define MX25_OPI_ENSO_DTR           0x4E4E /**< Enter Secured OTP */
#define MX25_OPI_EXSO_STR           0xC13E /**< Exit Secured OTP; 0 addr, 0 dummy, 0 data */
#define MX25_OPI_EXSO_DTR           0x3E3E /**< Exit Secured OTP */
#define MX25_OPI_WRLR_STR           0x2CD3 /**< Write Lock Register; 4-byte addr=0, 0 dummy, 1 data */
#define MX25_OPI_WRLR_DTR           0xD3D3 /**< Write Lock Register */
#define MX25_OPI_RDLR_STR           0x2DD2 /**< Read Lock Register; 4-byte addr=0, 4 dummy, 1 data */
#define MX25_OPI_RDLR_DTR           0xD2D2 /**< Read Lock Register */
#define MX25_OPI_WRSPB_STR          0xE31C /**< SPB Bit Program; 4-byte addr, 0 dummy, 0 data */
#define MX25_OPI_WRSPB_DTR          0x1C1C /**< SPB Bit Program */
#define MX25_OPI_ESSPB_STR          0xE41B /**< All SPB Bit Erase; 0 addr, 0 dummy, 0 data */
#define MX25_OPI_ESSPB_DTR          0x1B1B /**< All SPB Bit Erase */
#define MX25_OPI_RDSPB_STR          0xE21D /**< Read SPB Status; 4-byte addr, 6–20 dummy, 1 data */
#define MX25_OPI_RDSPB_DTR          0x1D1D /**< Read SPB Status */
#define MX25_OPI_WRDPB_STR          0xE11E /**< Write DPB Register; 4-byte addr, 0 dummy, 1 data */
#define MX25_OPI_WRDPB_DTR          0x1E1E /**< Write DPB Register */
#define MX25_OPI_RDDPB_STR          0xE01F /**< Read DPB Register; 4-byte addr, 6–20 dummy, 1 data */
#define MX25_OPI_RDDPB_DTR          0x1F1F /**< Read DPB Register */
#define MX25_OPI_RDPASS_STR         0x27D8 /**< Read Password Register; 4-byte addr=0, 20 dummy, 8 data */
#define MX25_OPI_RDPASS_DTR         0xD8D8 /**< Read Password Register */
#define MX25_OPI_WRPASS_STR         0x28D7 /**< Write Password Register; 4-byte addr=0, 0 dummy, 8 data */
#define MX25_OPI_WRPASS_DTR         0xD7D7 /**< Write Password Register */
#define MX25_OPI_PASSULK_STR        0x29D6 /**< Password Unlock; 4-byte addr=0, 0 dummy, 8 data */
#define MX25_OPI_PASSULK_DTR        0xD6D6 /**< Password Unlock */

/* Add other commands from the datasheet as needed for your firmware */

/* ==== Memory Map/Constants ==== */
#define MX25UM25645G_TOTAL_SIZE     (32 * 1024 * 1024)
#define MX25UM25645G_PAGE_SIZE      256
#define MX25UM25645G_SECTOR_SIZE    (4 * 1024)
#define MX25UM25645G_BLOCK_SIZE     (64 * 1024)
#define MX25UM25645G_NUM_BLOCKS     512
#define MX25UM25645G_NUM_SECTORS    8192
#define MX25UM25645G_JEDEC_ID       0xC2, 0x80, 0x39

// =============================================================================
// Read Identification (RDID) Instruction for MX25UM25645G
// =============================================================================
//
// The RDID command (opcode 0x9F) allows the host to read the Manufacturer ID,
// Memory Type, and Memory Density of the flash device.
//
// Protocol (Standard SPI):
//   1. Pull CS# (chip select) low to select the device.
//   2. Send command byte: 0x9F
//   3. Read at least 3 bytes in response:
//        Byte 1: Manufacturer ID     (should be 0xC2 for Macronix)
//        Byte 2: Memory Type         (e.g., 0x38)
//        Byte 3: Memory Density      (e.g., 0x39 for 256Mbit device)
//   4. Pull CS# high to deselect the device.
//
// Example sequence (Standard SPI):
//   CS# = LOW
//   Send: 0x9F
//   Receive: [0xC2] [0x38] [0x39]
//   CS# = HIGH
//
// Parsing the result:
//   - Manufacturer ID (0xC2): Macronix
//   - Memory Type     (0x38): MX25UM series
//   - Memory Density  (0x39): 256Mbit (see datasheet Table 10-2)
//
// Code Example:
//
//   uint8_t tx[1] = {0x9F};
//   uint8_t rx[3] = {0};
//   CS_LOW();                // Pull CS# low
//   SPI_Transmit(tx, 1);     // Send RDID command
//   SPI_Receive(rx, 3);      // Read 3 bytes of ID
//   CS_HIGH();               // Pull CS# high
//
//   // rx[0] = Manufacturer ID, rx[1] = Memory Type, rx[2] = Memory Density
//
// =============================================================================



#endif /* MX25UM25645G_H */
