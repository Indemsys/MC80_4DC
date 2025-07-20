#ifndef RA8M1_OSPI_H
#define RA8M1_OSPI_H

// =============================================================================
// RA8M1 OSPI (Octal Serial Peripheral Interface) Register Map
// =============================================================================

#define OSPI0_B_BASE                 0x40268000UL  // OSPI0 base address (secure)
#define OSPI0_B_NS_BASE              0x50268000UL  // OSPI0 base address (non-secure)

// =============================================================================
// Register Offsets
// =============================================================================

#define OSPI_WRAPCFG_OFS             0x000                  // Wrapper Configuration Register
#define OSPI_COMCFG_OFS              0x004                  // Common Configuration Register
#define OSPI_BMCFGCH_OFS(n)          (0x008 + 0x004 * (n))  // Bridge Map Configuration Register chn (n=0,1)
#define OSPI_CMCFG0CS_OFS(n)         (0x010 + 0x010 * (n))  // Command Map Configuration Register 0 CSn (n=0,1)
#define OSPI_CMCFG1CS_OFS(n)         (0x014 + 0x010 * (n))  // Command Map Configuration Register 1 CSn
#define OSPI_CMCFG2CS_OFS(n)         (0x018 + 0x010 * (n))  // Command Map Configuration Register 2 CSn
#define OSPI_LIOCFGCS_OFS(n)         (0x050 + 0x004 * (n))  // Link I/O Configuration Register CSn
#define OSPI_BMCTL0_OFS              0x060                  // Bridge Map Control Register 0
#define OSPI_BMCTL1_OFS              0x064                  // Bridge Map Control Register 1
#define OSPI_CMCTLCH_OFS(n)          (0x068 + 0x004 * (n))  // Command Map Control Register chn (n=0,1)
#define OSPI_CDCTL0_OFS              0x070                  // Command Manual Control Register 0
#define OSPI_CDCTL1_OFS              0x074                  // Command Manual Control Register 1
#define OSPI_CDCTL2_OFS              0x078                  // Command Manual Control Register 2
#define OSPI_CDTBUF_OFS(n)           (0x080 + 0x010 * (n))  // Command Transaction Buffer (n=0..3)
#define OSPI_CDABUF_OFS(n)           (0x084 + 0x010 * (n))  // Command Address Buffer
#define OSPI_CDD0BUF_OFS(n)          (0x088 + 0x010 * (n))  // Command Data0 Buffer
#define OSPI_CDD1BUF_OFS(n)          (0x08C + 0x010 * (n))  // Command Data1 Buffer
#define OSPI_LPCTL0_OFS              0x100                  // Link Pattern Control Register 0
#define OSPI_LPCTL1_OFS              0x104                  // Link Pattern Control Register 1
#define OSPI_LIOCTL_OFS              0x108                  // Link I/O Control Register

// Calibration/Status block
#define OSPI_CCCTL0CS_OFS(n)         (0x130 + 0x020 * (n))  // Calibration Control Register 0 CSn (n=0,1)
#define OSPI_CCCTL1CS_OFS(n)         (0x134 + 0x020 * (n))  // Calibration Control Register 1 CSn
#define OSPI_CCCTL2CS_OFS(n)         (0x138 + 0x020 * (n))  // Calibration Control Register 2 CSn
#define OSPI_CCCTL3CS_OFS(n)         (0x13C + 0x020 * (n))  // Calibration Control Register 3 CSn
#define OSPI_CCCTL4CS_OFS(n)         (0x140 + 0x020 * (n))  // Calibration Control Register 4 CSn
#define OSPI_CCCTL5CS_OFS(n)         (0x144 + 0x020 * (n))  // Calibration Control Register 5 CSn
#define OSPI_CCCTL6CS_OFS(n)         (0x148 + 0x020 * (n))  // Calibration Control Register 6 CSn
#define OSPI_CCCTL7CS_OFS(n)         (0x14C + 0x020 * (n))  // Calibration Control Register 7 CSn
#define OSPI_COMSTT_OFS              0x184                  // Common Status Register
#define OSPI_CASTTCS_OFS(n)          (0x188 + 0x004 * (n))  // Calibration Status Register CSn (n=0,1)

#define OSPI_INTS_OFS                0x190                  // Interrupt Status Register
#define OSPI_INTC_OFS                0x194                  // Interrupt Clear Register
#define OSPI_INTE_OFS                0x198                  // Interrupt Enable Register

// =============================================================================
// OSPI_WRAPCFG - Wrapper Configuration Register
// =============================================================================

// DQS Shift Value for CS0 (bits 12:8).
// Specifies the amount of delay applied to the DQS (Data Strobe) input for chip select 0.
// Range: 0 (no delay) to 31 (maximum delay, 31 delay cells). Used to align DQS timing for high-speed operation.
#define OSPI_WRAPCFG_DSSFTCS0_Pos    8
#define OSPI_WRAPCFG_DSSFTCS0_Msk    (0x1FU << OSPI_WRAPCFG_DSSFTCS0_Pos)

// DQS Shift Value for CS1 (bits 28:24).
// Specifies the amount of delay applied to the DQS (Data Strobe) input for chip select 1.
// Range: 0 (no delay) to 31 (maximum delay, 31 delay cells). Used to align DQS timing for high-speed operation.
#define OSPI_WRAPCFG_DSSFTCS1_Pos    24
#define OSPI_WRAPCFG_DSSFTCS1_Msk    (0x1FU << OSPI_WRAPCFG_DSSFTCS1_Pos)

// =============================================================================
// OSPI_COMCFG - Common Configuration Register
// =============================================================================

// Output Enable Assert Extension (bit 16).
// When set to 1, extends the assertion (active phase) of the OE (Output Enable) signal by 1 PCLKA clock cycle.
// 0 = No extension; 1 = Assertion extended by 1 cycle.
#define OSPI_COMCFG_OEASTEX_Pos      16
#define OSPI_COMCFG_OEASTEX_Msk      (1U << OSPI_COMCFG_OEASTEX_Pos)

// Output Enable Negate Extension (bit 17).
// When set to 1, extends the negation (inactive phase) of the OE (Output Enable) signal by 1 PCLKA clock cycle.
// 0 = No extension; 1 = Negation extended by 1 cycle.
#define OSPI_COMCFG_OENEGEX_Pos      17
#define OSPI_COMCFG_OENEGEX_Msk      (1U << OSPI_COMCFG_OENEGEX_Pos)

// =============================================================================
// OSPI_BMCFGCHn - Bridge Map Configuration Register chn (n=0,1)
// The BMCFGCHn register configures the memory-mapped interface behavior for the OSPI bridge channel n (typically, n=0 for CS0, n=1 for CS1).
// It controls write response behavior, write combination mode, write size,
// prefetch settings, and the write combination timer for optimal performance and compatibility with different memory devices.
// =============================================================================

// Write Response Mode (bit 0).
// 0 = Write response by memory device (waits for memory ready).
// 1 = Write response by OSPI controller (does not wait for memory).
#define OSPI_BMCFGCH_WRMD_Pos        0
#define OSPI_BMCFGCH_WRMD_Msk        (1U << OSPI_BMCFGCH_WRMD_Pos)

// Memory Write Combination Enable (bit 7).
// 0 = Disable (each write is issued immediately).
// 1 = Enable (writes are combined into a single burst transaction when possible).
#define OSPI_BMCFGCH_MWRCOMB_Pos     7
#define OSPI_BMCFGCH_MWRCOMB_Msk     (1U << OSPI_BMCFGCH_MWRCOMB_Pos)

// Memory Write Size (bits 15:8).
// Sets the write transaction size in bytes (encoded):
// 0x00 = 4 bytes, 0x01 = 8 bytes, ..., 0x0F = 64 bytes, 0xFF = 2 bytes.
#define OSPI_BMCFGCH_MWRSIZE_Pos     8
#define OSPI_BMCFGCH_MWRSIZE_Msk     (0xFFU << OSPI_BMCFGCH_MWRSIZE_Pos)

// Prefetch Enable (bit 16).
// 0 = Prefetch disabled.
// 1 = Prefetch enabled (read-ahead for improved throughput).
#define OSPI_BMCFGCH_PREEN_Pos       16
#define OSPI_BMCFGCH_PREEN_Msk       (1U << OSPI_BMCFGCH_PREEN_Pos)

// Write Combination Timer (bits 31:24).
// Sets the time (in PCLKA clock cycles) to wait for additional write data before issuing a combined write transaction.
// 0x00 = Timer disabled, 0x01–0xFF = wait up to N cycles.
#define OSPI_BMCFGCH_CMBTIM_Pos      24
#define OSPI_BMCFGCH_CMBTIM_Msk      (0xFFU << OSPI_BMCFGCH_CMBTIM_Pos)

// =============================================================================
// OSPI_CMCFG0CSn - Command Map Configuration Register 0 CSn (n=0,1)
// The CMCFG0CSn register defines the frame and address format, burst/array modes, and address replacement features for each chip select (CSn).
// This allows you to match the OSPI controller’s command/transfer framing to your external flash memory device requirements.
// =============================================================================

// Frame Format (bits 1:0).
// 0 = Standard SPI (1S-1S-1S).
// 1 = Octal DDR (8D-8D-8D) Profile 1.0.
// 2 = Octal DDR (8D-8D-8D) Profile 2.0 (Modified).
// 3 = Octal DDR (8D-8D-8D) Extended Mode.
#define OSPI_CMCFG0CSN_FFMT_Pos      0
#define OSPI_CMCFG0CSN_FFMT_Msk      (0x3U << OSPI_CMCFG0CSN_FFMT_Pos)

// Address Size (bits 3:2).
// 0 = 1 byte, 1 = 2 bytes, 2 = 3 bytes, 3 = 4 bytes.
#define OSPI_CMCFG0CSN_ADDSIZE_Pos   2
#define OSPI_CMCFG0CSN_ADDSIZE_Msk   (0x3U << OSPI_CMCFG0CSN_ADDSIZE_Pos)

// Wrapping Burst Mode Enable (bit 4).
// 0 = Disabled; 1 = Enabled (for supporting continuous burst with wrap).
#define OSPI_CMCFG0CSN_WPBSTMD_Pos   4
#define OSPI_CMCFG0CSN_WPBSTMD_Msk   (1U << OSPI_CMCFG0CSN_WPBSTMD_Pos)

// Array Address Mode (bit 5).
// 0 = Linear addressing; 1 = Array address mode enabled.
#define OSPI_CMCFG0CSN_ARYAMD_Pos    5
#define OSPI_CMCFG0CSN_ARYAMD_Msk    (1U << OSPI_CMCFG0CSN_ARYAMD_Pos)

// Address Replace Enable (bits 23:16).
// Each bit enables address replacement for a corresponding byte position in the address field.
// Bit 16 = enable for address byte 0, Bit 17 = for byte 1, ..., Bit 23 = for byte 7.
#define OSPI_CMCFG0CSN_ADDRPEN_Pos   16
#define OSPI_CMCFG0CSN_ADDRPEN_Msk   (0xFFU << OSPI_CMCFG0CSN_ADDRPEN_Pos)

// Address Replace Code (bits 31:24).
// Value to be used as the replacement byte when address replacement is enabled.
#define OSPI_CMCFG0CSN_ADDRPCD_Pos   24
#define OSPI_CMCFG0CSN_ADDRPCD_Msk   (0xFFU << OSPI_CMCFG0CSN_ADDRPCD_Pos)

// =============================================================================
// OSPI_CMCFG1CSn - Command Map Configuration Register 1 CSn (n=0,1)
// The CMCFG1CSn register sets the command code and dummy cycles (latency cycles) for memory read operations for each chip select (CSn).
// This allows the controller to send the correct command sequence and number of wait cycles,
// matching the requirements of the connected memory device.
// =============================================================================

// Read Command Code (bits 15:0).
// Specifies the opcode(s) used to initiate a read operation on the memory device.
// Typical values: 0x03 (read), 0x0B (fast read), 0xEB, 0xEC, etc.
#define OSPI_CMCFG1CSN_RDCMD_Pos     0
#define OSPI_CMCFG1CSN_RDCMD_Msk     (0xFFFFU << OSPI_CMCFG1CSN_RDCMD_Pos)

// Read Latency (bits 20:16).
// Number of dummy cycles (latency cycles) to insert between command/address phase and data phase.
// Range: 0–31. Set according to device timing requirements.
#define OSPI_CMCFG1CSN_RDLATE_Pos    16
#define OSPI_CMCFG1CSN_RDLATE_Msk    (0x1FU << OSPI_CMCFG1CSN_RDLATE_Pos)

// =============================================================================
// OSPI_CMCFG2CSn - Command Map Configuration Register 2 CSn (n=0,1)
// The CMCFG2CSn register configures the command code and dummy cycles (latency cycles) for memory write operations for each chip select (CSn).
// This allows the OSPI controller to issue the correct command and wait cycles to the connected memory device during writes.
// =============================================================================

// Write Command Code (bits 15:0).
// Specifies the opcode(s) used to initiate a write/program operation on the memory device.
// Typical values: 0x02 (Page Program), 0x38, 0x12, etc.
#define OSPI_CMCFG2CSN_WRCMD_Pos     0
#define OSPI_CMCFG2CSN_WRCMD_Msk     (0xFFFFU << OSPI_CMCFG2CSN_WRCMD_Pos)

// Write Latency (bits 20:16).
// Number of dummy cycles (latency cycles) to insert between command/address phase and data phase for write operations.
// Range: 0–31. Set according to device timing requirements.
#define OSPI_CMCFG2CSN_WRLATE_Pos    16
#define OSPI_CMCFG2CSN_WRLATE_Msk    (0x1FU << OSPI_CMCFG2CSN_WRLATE_Pos)

// =============================================================================
// OSPI_LIOCFGCSn - Link I/O Configuration Register CSn (n=0,1)
// The LIOCFGCSn register configures protocol, timing, and I/O settings for each chip select (CSn) line,
// enabling fine-tuning of the OSPI interface to match the requirements and capabilities of the connected memory device.
// =============================================================================

// Protocol Mode (bits 9:0).
// Selects SPI bus width and protocol (e.g. 1S-1S-1S, 8D-8D-8D, etc.).
// Typical values:
//   0x000 = 1S-1S-1S (standard SPI)
//   0x3B2 = 4S-4D-4D
//   0x3FF = 8D-8D-8D
#define OSPI_LIOCFGCSN_PRTMD_Pos     0
#define OSPI_LIOCFGCSN_PRTMD_Msk     (0x3FFU << OSPI_LIOCFGCSN_PRTMD_Pos)

// Latency Mode (bit 10).
// 0 = Standard mode, 1 = Variable/latency mode (device-specific).
#define OSPI_LIOCFGCSN_LATEMD_Pos    10
#define OSPI_LIOCFGCSN_LATEMD_Msk    (1U << OSPI_LIOCFGCSN_LATEMD_Pos)

// Write Mask Mode (bit 11).
// 0 = Normal write, 1 = Write with mask (for devices supporting masked writes).
#define OSPI_LIOCFGCSN_WRMSKMD_Pos   11
#define OSPI_LIOCFGCSN_WRMSKMD_Msk   (1U << OSPI_LIOCFGCSN_WRMSKMD_Pos)

// Minimum CS Inactive Cycles (bits 19:16).
// Minimum number of PCLKA cycles CS remains inactive between frames (0–15).
#define OSPI_LIOCFGCSN_CSMIN_Pos     16
#define OSPI_LIOCFGCSN_CSMIN_Msk     (0xFU << OSPI_LIOCFGCSN_CSMIN_Pos)

// CS Assert Extension (bit 20).
// 0 = Normal CS assertion; 1 = CS assertion is extended by 1 PCLKA cycle.
#define OSPI_LIOCFGCSN_CSASTEX_Pos   20
#define OSPI_LIOCFGCSN_CSASTEX_Msk   (1U << OSPI_LIOCFGCSN_CSASTEX_Pos)

// CS Negate Extension (bit 21).
// 0 = Normal CS negation; 1 = CS negation is extended by 1 PCLKA cycle.
#define OSPI_LIOCFGCSN_CSNEGEX_Pos   21
#define OSPI_LIOCFGCSN_CSNEGEX_Msk   (1U << OSPI_LIOCFGCSN_CSNEGEX_Pos)

// SDR Output Drive (bit 22).
// 0 = Normal drive; 1 = Stronger drive for SDR signals (device-dependent).
#define OSPI_LIOCFGCSN_SDRDRV_Pos    22
#define OSPI_LIOCFGCSN_SDRDRV_Msk    (1U << OSPI_LIOCFGCSN_SDRDRV_Pos)

// SDR Sampling Mode (bit 23).
// 0 = Standard sampling; 1 = Custom/advanced sampling (device-dependent).
#define OSPI_LIOCFGCSN_SDRSMPMD_Pos  23
#define OSPI_LIOCFGCSN_SDRSMPMD_Msk  (1U << OSPI_LIOCFGCSN_SDRSMPMD_Pos)

// SDR Sampling Shift (bits 27:24).
// Shifts SDR data sampling point (in PCLKA cycles), range 0–15.
#define OSPI_LIOCFGCSN_SDRSMPSFT_Pos 24
#define OSPI_LIOCFGCSN_SDRSMPSFT_Msk (0xFU << OSPI_LIOCFGCSN_SDRSMPSFT_Pos)

// DDR Sampling Extension (bits 31:28).
// Adjusts DDR data sampling point, range 0–15.
#define OSPI_LIOCFGCSN_DDRSMPEX_Pos  28
#define OSPI_LIOCFGCSN_DDRSMPEX_Msk  (0xFU << OSPI_LIOCFGCSN_DDRSMPEX_Pos)

// =============================================================================
// OSPI_BMCTL0 - Bridge Map Control Register 0
// Controls the access enable settings from each system bus channel (ch0) to the OSPI memory areas (CS0/CS1).
// This register is used to enable or disable read/write access to each OSPI memory area during memory-mapped operation.
// =============================================================================

// Channel 0 to CS0 access enable (bits 1:0).
// Controls access from system bus channel 0 to CS0 memory area.
// 0b00: Read/Write disable (no access)
// 0b01: Read enable, Write disable
// 0b10: Read disable, Write enable
// 0b11: Read/Write enable (full access)
#define OSPI_BMCTL0_CH0CS0ACC_Pos    0
#define OSPI_BMCTL0_CH0CS0ACC_Msk    (0x3U << OSPI_BMCTL0_CH0CS0ACC_Pos)

// Channel 0 to CS1 access enable (bits 3:2).
// Controls access from system bus channel 0 to CS1 memory area.
// 0b00: Read/Write disable (no access)
// 0b01: Read enable, Write disable
// 0b10: Read disable, Write enable
// 0b11: Read/Write enable (full access)
#define OSPI_BMCTL0_CH0CS1ACC_Pos    2
#define OSPI_BMCTL0_CH0CS1ACC_Msk    (0x3U << OSPI_BMCTL0_CH0CS1ACC_Pos)

// =============================================================================
// OSPI_BMCTL1 - Bridge Map Control Register 1
// BMCTL1 controls buffer operations for system bus channel 0:
// Manual push of write data from the combination buffer to the OSPI memory device (used in combination mode).
// Clearing the prefetch buffer for channel 0 (when prefetch is enabled).
// =============================================================================

// Memory Write Data Push for channel 0 (bit 8).
// Writing 1 pushes the pending data in the combination buffer to the memory device in combination mode.
// 0: No command
// 1: Push request (self-clearing bit)
#define OSPI_BMCTL1_MWRPUSHCH0_Pos 8
#define OSPI_BMCTL1_MWRPUSHCH0_Msk (1U << OSPI_BMCTL1_MWRPUSHCH0_Pos)

// Prefetch Buffer Clear for channel 0 (bit 10).
// Writing 1 clears the prefetch buffer for channel 0. Should NOT be set during memory access.
// 0: No command
// 1: Clear request (self-clearing bit)
#define OSPI_BMCTL1_PBUFCLRCH0_Pos 10
#define OSPI_BMCTL1_PBUFCLRCH0_Msk (1U << OSPI_BMCTL1_PBUFCLRCH0_Pos)

// =============================================================================
// OSPI_CMCTLCHn - Command Map Control Register chn (n=0,1)
// =============================================================================

#define OSPI_CMCTLCH_XIPENCODE_Pos   0                                      // XIP enable code position
#define OSPI_CMCTLCH_XIPENCODE_Msk   (0xFFU << OSPI_CMCTLCH_XIPENCODE_Pos)  // XIP enable code mask
#define OSPI_CMCTLCH_XIPEXCODE_Pos   8                                      // XIP exit code position
#define OSPI_CMCTLCH_XIPEXCODE_Msk   (0xFFU << OSPI_CMCTLCH_XIPEXCODE_Pos)  // XIP exit code mask
#define OSPI_CMCTLCH_XIPEN_Pos       16                                     // XIP enable position
#define OSPI_CMCTLCH_XIPEN_Msk       (1U << OSPI_CMCTLCH_XIPEN_Pos)         // XIP enable mask

// =============================================================================
// OSPI_CDCTL0/1/2 - Command Manual Control Registers
// =============================================================================

#define OSPI_CDCTL0_TRREQ_Pos        0                                  // Transaction request position
#define OSPI_CDCTL0_TRREQ_Msk        (1U << OSPI_CDCTL0_TRREQ_Pos)      // Transaction request mask
#define OSPI_CDCTL0_PERMD_Pos        1                                  // Periodic mode position
#define OSPI_CDCTL0_PERMD_Msk        (1U << OSPI_CDCTL0_PERMD_Pos)      // Periodic mode mask
#define OSPI_CDCTL0_CSSEL_Pos        3                                  // CS select position
#define OSPI_CDCTL0_CSSEL_Msk        (1U << OSPI_CDCTL0_CSSEL_Pos)      // CS select mask
#define OSPI_CDCTL0_TRNUM_Pos        4                                  // Transaction number position
#define OSPI_CDCTL0_TRNUM_Msk        (0x3U << OSPI_CDCTL0_TRNUM_Pos)    // Transaction number mask
#define OSPI_CDCTL0_PERITV_Pos       16                                 // Periodic interval position
#define OSPI_CDCTL0_PERITV_Msk       (0x1FU << OSPI_CDCTL0_PERITV_Pos)  // Periodic interval mask
#define OSPI_CDCTL0_PERREP_Pos       24                                 // Periodic repeat position
#define OSPI_CDCTL0_PERREP_Msk       (0xFU << OSPI_CDCTL0_PERREP_Pos)   // Periodic repeat mask

#define OSPI_CDCTL1_PEREXP_Msk       0xFFFFFFFFU                        // Periodic expansion mask
#define OSPI_CDCTL2_PERMSK_Msk       0xFFFFFFFFU                        // Periodic mask

// =============================================================================
// OSPI_CDTBUFn, CDABUFn, CDD0BUFn, CDD1BUFn - Command Data Buffers
// =============================================================================

#define OSPI_CDTBUF_CMDSIZE_Pos      0                                   // Command size position
#define OSPI_CDTBUF_CMDSIZE_Msk      (0x3U << OSPI_CDTBUF_CMDSIZE_Pos)   // Command size mask
#define OSPI_CDTBUF_ADDSIZE_Pos      2                                   // Address size position
#define OSPI_CDTBUF_ADDSIZE_Msk      (0x7U << OSPI_CDTBUF_ADDSIZE_Pos)   // Address size mask
#define OSPI_CDTBUF_DATASIZE_Pos     5                                   // Data size position
#define OSPI_CDTBUF_DATASIZE_Msk     (0xFU << OSPI_CDTBUF_DATASIZE_Pos)  // Data size mask
#define OSPI_CDTBUF_LATE_Pos         9                                   // Latency position
#define OSPI_CDTBUF_LATE_Msk         (0x1FU << OSPI_CDTBUF_LATE_Pos)     // Latency mask
#define OSPI_CDTBUF_TRTYPE_Pos       15                                  // Transaction type position
#define OSPI_CDTBUF_TRTYPE_Msk       (1U << OSPI_CDTBUF_TRTYPE_Pos)      // Transaction type mask
#define OSPI_CDTBUF_CMD_Pos          16                                  // Command position
#define OSPI_CDTBUF_CMD_Msk          (0xFFFFU << OSPI_CDTBUF_CMD_Pos)    // Command mask

#define OSPI_CDABUF_ADD_Msk          0xFFFFFFFFU                         // Address buffer mask
#define OSPI_CDD0BUF_DATA_Msk        0xFFFFFFFFU                         // Data0 buffer mask
#define OSPI_CDD1BUF_DATA_Msk        0xFFFFFFFFU                         // Data1 buffer mask

// =============================================================================
// OSPI_LPCTL0/1 - Link Pattern Control Registers
// =============================================================================

#define OSPI_LPCTL0_PATREQ_Pos       0                                  // Pattern request position
#define OSPI_LPCTL0_PATREQ_Msk       (1U << OSPI_LPCTL0_PATREQ_Pos)     // Pattern request mask
#define OSPI_LPCTL0_CSSEL_Pos        3                                  // CS select position
#define OSPI_LPCTL0_CSSEL_Msk        (1U << OSPI_LPCTL0_CSSEL_Pos)      // CS select mask
#define OSPI_LPCTL0_XDPIN_Pos        4                                  // XD pin position
#define OSPI_LPCTL0_XDPIN_Msk        (0x3U << OSPI_LPCTL0_XDPIN_Pos)    // XD pin mask
#define OSPI_LPCTL0_XD1LEN_Pos       16                                 // XD1 length position
#define OSPI_LPCTL0_XD1LEN_Msk       (0x1FU << OSPI_LPCTL0_XD1LEN_Pos)  // XD1 length mask
#define OSPI_LPCTL0_XD1VAL_Pos       23                                 // XD1 value position
#define OSPI_LPCTL0_XD1VAL_Msk       (1U << OSPI_LPCTL0_XD1VAL_Pos)     // XD1 value mask
#define OSPI_LPCTL0_XD2LEN_Pos       24                                 // XD2 length position
#define OSPI_LPCTL0_XD2LEN_Msk       (0x1FU << OSPI_LPCTL0_XD2LEN_Pos)  // XD2 length mask
#define OSPI_LPCTL0_XD2VAL_Pos       31                                 // XD2 value position
#define OSPI_LPCTL0_XD2VAL_Msk       (1U << OSPI_LPCTL0_XD2VAL_Pos)     // XD2 value mask

#define OSPI_LPCTL1_PATREQ_Pos       0                                  // Pattern request position
#define OSPI_LPCTL1_PATREQ_Msk       (0x3U << OSPI_LPCTL1_PATREQ_Pos)   // Pattern request mask
#define OSPI_LPCTL1_CSSEL_Pos        3                                  // CS select position
#define OSPI_LPCTL1_CSSEL_Msk        (1U << OSPI_LPCTL1_CSSEL_Pos)      // CS select mask
#define OSPI_LPCTL1_RSTREP_Pos       4                                  // Reset repeat position
#define OSPI_LPCTL1_RSTREP_Msk       (0x3U << OSPI_LPCTL1_RSTREP_Pos)   // Reset repeat mask
#define OSPI_LPCTL1_RSTWID_Pos       8                                  // Reset width position
#define OSPI_LPCTL1_RSTWID_Msk       (0x7U << OSPI_LPCTL1_RSTWID_Pos)   // Reset width mask
#define OSPI_LPCTL1_RSTSU_Pos        12                                 // Reset setup position
#define OSPI_LPCTL1_RSTSU_Msk        (0x7U << OSPI_LPCTL1_RSTSU_Pos)    // Reset setup mask

// =============================================================================
// CCCTLxCSn - Calibration Control Registers
// =============================================================================

#define OSPI_CCCTL0CSN_CAEN_Pos      0                                        // Calibration enable position
#define OSPI_CCCTL0CSN_CAEN_Msk      (1U << OSPI_CCCTL0CSN_CAEN_Pos)          // Calibration enable mask
#define OSPI_CCCTL0CSN_CANOWR_Pos    1                                        // Calibration no overwrite position
#define OSPI_CCCTL0CSN_CANOWR_Msk    (1U << OSPI_CCCTL0CSN_CANOWR_Pos)        // Calibration no overwrite mask
#define OSPI_CCCTL0CSN_CAITV_Pos     8                                        // Calibration interval position
#define OSPI_CCCTL0CSN_CAITV_Msk     (0x1FU << OSPI_CCCTL0CSN_CAITV_Pos)      // Calibration interval mask
#define OSPI_CCCTL0CSN_CASFTSTA_Pos  16                                       // Calibration shift start position
#define OSPI_CCCTL0CSN_CASFTSTA_Msk  (0x1FU << OSPI_CCCTL0CSN_CASFTSTA_Pos)   // Calibration shift start mask
#define OSPI_CCCTL0CSN_CASFTEND_Pos  24                                       // Calibration shift end position
#define OSPI_CCCTL0CSN_CASFTEND_Msk  (0x1FU << OSPI_CCCTL0CSN_CASFTEND_Pos)   // Calibration shift end mask

#define OSPI_CCCTL1CSN_WPCS1_Pos     1                                        // Write protect CS1 position
#define OSPI_CCCTL1CSN_WPCS1_Msk     (1U << OSPI_CCCTL1CSN_WPCS1_Pos)         // Write protect CS1 mask
#define OSPI_CCCTL1CSN_RSTCS0_Pos    16                                       // Reset CS0 position
#define OSPI_CCCTL1CSN_RSTCS0_Msk    (1U << OSPI_CCCTL1CSN_RSTCS0_Pos)        // Reset CS0 mask

#define OSPI_CCCTL2CSN_CAWRCMD_Pos   0                                        // Calibration write command position
#define OSPI_CCCTL2CSN_CAWRCMD_Msk   (0xFFFFU << OSPI_CCCTL2CSN_CAWRCMD_Pos)  // Calibration write command mask
#define OSPI_CCCTL2CSN_CARDCMD_Pos   16                                       // Calibration read command position
#define OSPI_CCCTL2CSN_CARDCMD_Msk   (0xFFFFU << OSPI_CCCTL2CSN_CARDCMD_Pos)  // Calibration read command mask

#define OSPI_CCCTL3CSN_CAADD_Msk     0xFFFFFFFFU                              // Calibration address mask
#define OSPI_CCCTL4CSN_CADATA_Msk    0xFFFFFFFFU                              // Calibration data mask
#define OSPI_CCCTL5CSN_CADATA_Msk    0xFFFFFFFFU                              // Calibration data mask
#define OSPI_CCCTL6CSN_CADATA_Msk    0xFFFFFFFFU                              // Calibration data mask
#define OSPI_CCCTL7CSN_CADATA_Msk    0xFFFFFFFFU                              // Calibration data mask

// =============================================================================
// COMSTT - Common Status Register
// =============================================================================

#define OSPI_COMSTT_MEMACCCH0_Pos    0                                   // Memory access channel 0 position
#define OSPI_COMSTT_MEMACCCH0_Msk    (1U << OSPI_COMSTT_MEMACCCH0_Pos)   // Memory access channel 0 mask
#define OSPI_COMSTT_PBUFNECH0_Pos    4                                   // Prefetch buffer not empty channel 0 position
#define OSPI_COMSTT_PBUFNECH0_Msk    (1U << OSPI_COMSTT_PBUFNECH0_Pos)   // Prefetch buffer not empty channel 0 mask
#define OSPI_COMSTT_WRBUFNECH0_Pos   6                                   // Write buffer not empty channel 0 position
#define OSPI_COMSTT_WRBUFNECH0_Msk   (1U << OSPI_COMSTT_WRBUFNECH0_Pos)  // Write buffer not empty channel 0 mask
#define OSPI_COMSTT_ECSCS1_Pos       20                                  // ECC status CS1 position
#define OSPI_COMSTT_ECSCS1_Msk       (1U << OSPI_COMSTT_ECSCS1_Pos)      // ECC status CS1 mask
#define OSPI_COMSTT_INTCS1_Pos       21                                  // Interrupt CS1 position
#define OSPI_COMSTT_INTCS1_Msk       (1U << OSPI_COMSTT_INTCS1_Pos)      // Interrupt CS1 mask
#define OSPI_COMSTT_RSTOCS1_Pos      22                                  // Reset output CS1 position
#define OSPI_COMSTT_RSTOCS1_Msk      (1U << OSPI_COMSTT_RSTOCS1_Pos)     // Reset output CS1 mask

// =============================================================================
// CASTTCSn - Calibration Status Register CSn (n=0,1)
// =============================================================================

#define OSPI_CASTTCSN_CASUC_Msk      0xFFFFFFFFU  // Calibration success mask

// =============================================================================
// OSPI_INTS - Interrupt Status Register (INTS)
// =============================================================================

#define OSPI_INTS_CMDCMP_Pos         0                             // Command completed position
#define OSPI_INTS_CMDCMP_Msk         (1U << OSPI_INTS_CMDCMP_Pos)  // Command completed mask (0: No detection, 1: Command completed)

#define OSPI_INTS_PATCMP_Pos         1                             // Pattern completed position
#define OSPI_INTS_PATCMP_Msk         (1U << OSPI_INTS_PATCMP_Pos)  // Pattern completed mask (0: No detection, 1: Pattern completed)

// Bit 2 - Reserved (always reads 0)

#define OSPI_INTS_PERTO_Pos          3                              // Periodic transaction timeout position
#define OSPI_INTS_PERTO_Msk          (1U << OSPI_INTS_PERTO_Pos)    // Periodic transaction timeout mask (0: No detection, 1: Timeout detected)

#define OSPI_INTS_DSTOCS0_Pos        4                              // OM_DQS timeout for slave0 position
#define OSPI_INTS_DSTOCS0_Msk        (1U << OSPI_INTS_DSTOCS0_Pos)  // OM_DQS timeout for slave0 mask

#define OSPI_INTS_DSTOCS1_Pos        5                              // OM_DQS timeout for slave1 position
#define OSPI_INTS_DSTOCS1_Msk        (1U << OSPI_INTS_DSTOCS1_Pos)  // OM_DQS timeout for slave1 mask

// Bits 8:6 - Reserved

#define OSPI_INTS_ECSCS1_Pos         9                             // ECC error detection for slave1 position
#define OSPI_INTS_ECSCS1_Msk         (1U << OSPI_INTS_ECSCS1_Pos)  // ECC error detection for slave1 mask (0: No detection, 1: Detection)

// Bits 12:10 - Reserved

#define OSPI_INTS_INTCS1_Pos         13                            // Interrupt detection for slave1 position
#define OSPI_INTS_INTCS1_Msk         (1U << OSPI_INTS_INTCS1_Pos)  // Interrupt detection for slave1 mask (0: No detection, 1: Detection)

// Bits 19:14 - Reserved

#define OSPI_INTS_BUSERRCH0_Pos      20                               // System bus error for ch0 position
#define OSPI_INTS_BUSERRCH0_Msk      (1U << OSPI_INTS_BUSERRCH0_Pos)  // System bus error for ch0 mask (0: No error, 1: Error detected)

// Bits 27:21 - Reserved

#define OSPI_INTS_CAFAILCS0_Pos      28                               // Calibration failed for slave0 position
#define OSPI_INTS_CAFAILCS0_Msk      (1U << OSPI_INTS_CAFAILCS0_Pos)  // Calibration failed for slave0 mask

#define OSPI_INTS_CAFAILCS1_Pos      29                               // Calibration failed for slave1 position
#define OSPI_INTS_CAFAILCS1_Msk      (1U << OSPI_INTS_CAFAILCS1_Pos)  // Calibration failed for slave1 mask

#define OSPI_INTS_CASUCCS0_Pos       30                               // Calibration success for slave0 position
#define OSPI_INTS_CASUCCS0_Msk       (1U << OSPI_INTS_CASUCCS0_Pos)   // Calibration success for slave0 mask

#define OSPI_INTS_CASUCCS1_Pos       31                               // Calibration success for slave1 position
#define OSPI_INTS_CASUCCS1_Msk       (1U << OSPI_INTS_CASUCCS1_Pos)   // Calibration success for slave1 mask

// =============================================================================
// OSPI_INTC - Interrupt Clear Register (INTC)
// =============================================================================

#define OSPI_INTC_CMDCMPC_Pos        0                              // Command completed interrupt clear position
#define OSPI_INTC_CMDCMPC_Msk        (1U << OSPI_INTC_CMDCMPC_Pos)  // Command completed interrupt clear mask (1: Clear status)

#define OSPI_INTC_PATCMPC_Pos        1                              // Pattern completed interrupt clear position
#define OSPI_INTC_PATCMPC_Msk        (1U << OSPI_INTC_PATCMPC_Pos)  // Pattern completed interrupt clear mask (1: Clear status)

// Bit 2 - Reserved

#define OSPI_INTC_PERTOC_Pos         3                               // Periodic transaction timeout interrupt clear position
#define OSPI_INTC_PERTOC_Msk         (1U << OSPI_INTC_PERTOC_Pos)    // Periodic transaction timeout interrupt clear mask (1: Clear status)

#define OSPI_INTC_DSTOCS0C_Pos       4                               // OM_DQS timeout for slave0 interrupt clear position
#define OSPI_INTC_DSTOCS0C_Msk       (1U << OSPI_INTC_DSTOCS0C_Pos)  // OM_DQS timeout for slave0 interrupt clear mask

#define OSPI_INTC_DSTOCS1C_Pos       5                               // OM_DQS timeout for slave1 interrupt clear position
#define OSPI_INTC_DSTOCS1C_Msk       (1U << OSPI_INTC_DSTOCS1C_Pos)  // OM_DQS timeout for slave1 interrupt clear mask

// Bits 8:6 - Reserved

#define OSPI_INTC_ECSCS1C_Pos        9                              // ECC error detection for slave1 interrupt clear position
#define OSPI_INTC_ECSCS1C_Msk        (1U << OSPI_INTC_ECSCS1C_Pos)  // ECC error detection for slave1 interrupt clear mask

// Bits 12:10 - Reserved

#define OSPI_INTC_INTCS1C_Pos        13                             // Interrupt detection for slave1 interrupt clear position
#define OSPI_INTC_INTCS1C_Msk        (1U << OSPI_INTC_INTCS1C_Pos)  // Interrupt detection for slave1 interrupt clear mask

// Bits 19:14 - Reserved

#define OSPI_INTC_BUSERRCH0C_Pos     20                                // System bus error for ch0 interrupt clear position
#define OSPI_INTC_BUSERRCH0C_Msk     (1U << OSPI_INTC_BUSERRCH0C_Pos)  // System bus error for ch0 interrupt clear mask

// Bits 27:21 - Reserved

#define OSPI_INTC_CAFAILCS0C_Pos     28                                // Calibration failed for slave0 interrupt clear position
#define OSPI_INTC_CAFAILCS0C_Msk     (1U << OSPI_INTC_CAFAILCS0C_Pos)  // Calibration failed for slave0 interrupt clear mask

#define OSPI_INTC_CAFAILCS1C_Pos     29                                // Calibration failed for slave1 interrupt clear position
#define OSPI_INTC_CAFAILCS1C_Pos     29                                // Calibration failed for slave1 interrupt clear position
#define OSPI_INTC_CAFAILCS1C_Msk     (1U << OSPI_INTC_CAFAILCS1C_Pos)  // Calibration failed for slave1 interrupt clear mask

#define OSPI_INTC_CASUCCS0C_Pos      30                                // Calibration success for slave0 interrupt clear position
#define OSPI_INTC_CASUCCS0C_Msk      (1U << OSPI_INTC_CASUCCS0C_Pos)   // Calibration success for slave0 interrupt clear mask

#define OSPI_INTC_CASUCCS1C_Pos      31                                // Calibration success for slave1 interrupt clear position
#define OSPI_INTC_CASUCCS1C_Msk      (1U << OSPI_INTC_CASUCCS1C_Pos)   // Calibration success for slave1 interrupt clear mask

// =============================================================================
// OSPI_INTE - Interrupt Enable Register (INTE)
// =============================================================================

#define OSPI_INTE_CMDCMPE_Pos        0                              // Command completed interrupt enable position
#define OSPI_INTE_CMDCMPE_Msk        (1U << OSPI_INTE_CMDCMPE_Pos)  // Command completed interrupt enable mask

#define OSPI_INTE_PATCMPE_Pos        1                              // Pattern completed interrupt enable position
#define OSPI_INTE_PATCMPE_Msk        (1U << OSPI_INTE_PATCMPE_Pos)  // Pattern completed interrupt enable mask

// Bit 2 - Reserved

#define OSPI_INTE_PERTOE_Pos         3                               // Periodic transaction timeout interrupt enable position
#define OSPI_INTE_PERTOE_Msk         (1U << OSPI_INTE_PERTOE_Pos)    // Periodic transaction timeout interrupt enable mask

#define OSPI_INTE_DSTOCS0E_Pos       4                               // OM_DQS timeout for slave0 interrupt enable position
#define OSPI_INTE_DSTOCS0E_Msk       (1U << OSPI_INTE_DSTOCS0E_Pos)  // OM_DQS timeout for slave0 interrupt enable mask

#define OSPI_INTE_DSTOCS1E_Pos       5                               // OM_DQS timeout for slave1 interrupt enable position
#define OSPI_INTE_DSTOCS1E_Msk       (1U << OSPI_INTE_DSTOCS1E_Pos)  // OM_DQS timeout for slave1 interrupt enable mask

// Bits 8:6 - Reserved

#define OSPI_INTE_ECSCS1E_Pos        9                              // ECC error detection for slave1 interrupt enable position
#define OSPI_INTE_ECSCS1E_Msk        (1U << OSPI_INTE_ECSCS1E_Pos)  // ECC error detection for slave1 interrupt enable mask

// Bits 12:10 - Reserved

#define OSPI_INTE_INTCS1E_Pos        13                             // Interrupt detection for slave1 interrupt enable position
#define OSPI_INTE_INTCS1E_Msk        (1U << OSPI_INTE_INTCS1E_Pos)  // Interrupt detection for slave1 interrupt enable mask

// Bits 19:14 - Reserved

#define OSPI_INTE_BUSERRCH0E_Pos     20                                // System bus error for ch0 interrupt enable position
#define OSPI_INTE_BUSERRCH0E_Msk     (1U << OSPI_INTE_BUSERRCH0E_Pos)  // System bus error for ch0 interrupt enable mask

// Bits 27:21 - Reserved

#define OSPI_INTE_CAFAILCS0E_Pos     28                                // Calibration failed for slave0 interrupt enable position
#define OSPI_INTE_CAFAILCS0E_Msk     (1U << OSPI_INTE_CAFAILCS0E_Pos)  // Calibration failed for slave0 interrupt enable mask

#define OSPI_INTE_CAFAILCS1E_Pos     29                                // Calibration failed for slave1 interrupt enable position
#define OSPI_INTE_CAFAILCS1E_Msk     (1U << OSPI_INTE_CAFAILCS1E_Pos)  // Calibration failed for slave1 interrupt enable mask

#define OSPI_INTE_CASUCCS0E_Pos      30                                // Calibration success for slave0 interrupt enable position
#define OSPI_INTE_CASUCCS0E_Msk      (1U << OSPI_INTE_CASUCCS0E_Pos)   // Calibration success for slave0 interrupt enable mask

#define OSPI_INTE_CASUCCS1E_Pos      31                                // Calibration success for slave1 interrupt enable position
#define OSPI_INTE_CASUCCS1E_Msk      (1U << OSPI_INTE_CASUCCS1E_Pos)   // Calibration success for slave1 interrupt enable mask

#endif                                                                 // RA8M1_OSPI_H
