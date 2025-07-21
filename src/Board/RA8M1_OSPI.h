#ifndef RA8M1_OSPI_H
#define RA8M1_OSPI_H

// =============================================================================
// RA8M1 OSPI (Octal Serial Peripheral Interface) Register Map
// =============================================================================

#define OSPI0_B_BASE                  0x40268000UL  // OSPI0 base address (secure)
#define OSPI0_B_NS_BASE               0x50268000UL  // OSPI0 base address (non-secure)

// =============================================================================
// Register Offsets
// =============================================================================

#define OSPI_WRAPCFG_OFS              0x000                  // Wrapper Configuration Register
#define OSPI_COMCFG_OFS               0x004                  // Common Configuration Register
#define OSPI_BMCFGCH_OFS(n)           (0x008 + 0x004 * (n))  // Bridge Map Configuration Register chn (n=0,1)
#define OSPI_CMCFG0CS_OFS(n)          (0x010 + 0x010 * (n))  // Command Map Configuration Register 0 CSn (n=0,1)
#define OSPI_CMCFG1CS_OFS(n)          (0x014 + 0x010 * (n))  // Command Map Configuration Register 1 CSn
#define OSPI_CMCFG2CS_OFS(n)          (0x018 + 0x010 * (n))  // Command Map Configuration Register 2 CSn
#define OSPI_LIOCFGCS_OFS(n)          (0x050 + 0x004 * (n))  // Link I/O Configuration Register CSn
#define OSPI_BMCTL0_OFS               0x060                  // Bridge Map Control Register 0
#define OSPI_BMCTL1_OFS               0x064                  // Bridge Map Control Register 1
#define OSPI_CMCTLCH_OFS(n)           (0x068 + 0x004 * (n))  // Command Map Control Register chn (n=0,1)
#define OSPI_CDCTL0_OFS               0x070                  // Command Manual Control Register 0
#define OSPI_CDCTL1_OFS               0x074                  // Command Manual Control Register 1
#define OSPI_CDCTL2_OFS               0x078                  // Command Manual Control Register 2
#define OSPI_CDTBUF_OFS(n)            (0x080 + 0x010 * (n))  // Command Transaction Buffer (n=0..3)
#define OSPI_CDABUF_OFS(n)            (0x084 + 0x010 * (n))  // Command Address Buffer
#define OSPI_CDD0BUF_OFS(n)           (0x088 + 0x010 * (n))  // Command Data0 Buffer
#define OSPI_CDD1BUF_OFS(n)           (0x08C + 0x010 * (n))  // Command Data1 Buffer
#define OSPI_LPCTL0_OFS               0x100                  // Link Pattern Control Register 0
#define OSPI_LPCTL1_OFS               0x104                  // Link Pattern Control Register 1
#define OSPI_LIOCTL_OFS               0x108                  // Link I/O Control Register

// Calibration/Status block
#define OSPI_CCCTL0CS_OFS(n)          (0x130 + 0x020 * (n))  // Calibration Control Register 0 CSn (n=0,1)
#define OSPI_CCCTL1CS_OFS(n)          (0x134 + 0x020 * (n))  // Calibration Control Register 1 CSn
#define OSPI_CCCTL2CS_OFS(n)          (0x138 + 0x020 * (n))  // Calibration Control Register 2 CSn
#define OSPI_CCCTL3CS_OFS(n)          (0x13C + 0x020 * (n))  // Calibration Control Register 3 CSn
#define OSPI_CCCTL4CS_OFS(n)          (0x140 + 0x020 * (n))  // Calibration Control Register 4 CSn
#define OSPI_CCCTL5CS_OFS(n)          (0x144 + 0x020 * (n))  // Calibration Control Register 5 CSn
#define OSPI_CCCTL6CS_OFS(n)          (0x148 + 0x020 * (n))  // Calibration Control Register 6 CSn
#define OSPI_CCCTL7CS_OFS(n)          (0x14C + 0x020 * (n))  // Calibration Control Register 7 CSn
#define OSPI_COMSTT_OFS               0x184                  // Common Status Register
#define OSPI_CASTTCS_OFS(n)           (0x188 + 0x004 * (n))  // Calibration Status Register CSn (n=0,1)

#define OSPI_INTS_OFS                 0x190                  // Interrupt Status Register
#define OSPI_INTC_OFS                 0x194                  // Interrupt Clear Register
#define OSPI_INTE_OFS                 0x198                  // Interrupt Enable Register

// =============================================================================
// OSPI_WRAPCFG - Wrapper Configuration Register
// =============================================================================

// DQS Shift Value for CS0 (bits 12:8).
// Specifies the amount of delay applied to the DQS (Data Strobe) input for chip select 0.
// Range: 0 (no delay) to 31 (maximum delay, 31 delay cells). Used to align DQS timing for high-speed operation.
#define OSPI_WRAPCFG_DSSFTCS0_Pos     8
#define OSPI_WRAPCFG_DSSFTCS0_Msk     (0x1FU << OSPI_WRAPCFG_DSSFTCS0_Pos)

// DQS Shift Value for CS1 (bits 28:24).
// Specifies the amount of delay applied to the DQS (Data Strobe) input for chip select 1.
// Range: 0 (no delay) to 31 (maximum delay, 31 delay cells). Used to align DQS timing for high-speed operation.
#define OSPI_WRAPCFG_DSSFTCS1_Pos     24
#define OSPI_WRAPCFG_DSSFTCS1_Msk     (0x1FU << OSPI_WRAPCFG_DSSFTCS1_Pos)

// =============================================================================
// OSPI_COMCFG - Common Configuration Register
// =============================================================================

// Output Enable Assert Extension (bit 16).
// When set to 1, extends the assertion (active phase) of the OE (Output Enable) signal by 1 PCLKA clock cycle.
// 0 = No extension; 1 = Assertion extended by 1 cycle.
#define OSPI_COMCFG_OEASTEX_Pos       16
#define OSPI_COMCFG_OEASTEX_Msk       (1U << OSPI_COMCFG_OEASTEX_Pos)

// Output Enable Negate Extension (bit 17).
// When set to 1, extends the negation (inactive phase) of the OE (Output Enable) signal by 1 PCLKA clock cycle.
// 0 = No extension; 1 = Negation extended by 1 cycle.
#define OSPI_COMCFG_OENEGEX_Pos       17
#define OSPI_COMCFG_OENEGEX_Msk       (1U << OSPI_COMCFG_OENEGEX_Pos)

// =============================================================================
// OSPI_BMCFGCHn - Bridge Map Configuration Register chn (n=0,1)
// The BMCFGCHn register configures the memory-mapped interface behavior for the OSPI bridge channel n (typically, n=0 for CS0, n=1 for CS1).
// It controls write response behavior, write combination mode, write size,
// prefetch settings, and the write combination timer for optimal performance and compatibility with different memory devices.
// =============================================================================

// Write Response Mode (bit 0).
// 0 = Write response by memory device (waits for memory ready).
// 1 = Write response by OSPI controller (does not wait for memory).
#define OSPI_BMCFGCH_WRMD_Pos         0
#define OSPI_BMCFGCH_WRMD_Msk         (1U << OSPI_BMCFGCH_WRMD_Pos)

// Memory Write Combination Enable (bit 7).
// 0 = Disable (each write is issued immediately).
// 1 = Enable (writes are combined into a single burst transaction when possible).
#define OSPI_BMCFGCH_MWRCOMB_Pos      7
#define OSPI_BMCFGCH_MWRCOMB_Msk      (1U << OSPI_BMCFGCH_MWRCOMB_Pos)

// Memory Write Size (bits 15:8).
// Sets the write transaction size in bytes (encoded):
// 0x00 = 4 bytes, 0x01 = 8 bytes, ..., 0x0F = 64 bytes, 0xFF = 2 bytes.
#define OSPI_BMCFGCH_MWRSIZE_Pos      8
#define OSPI_BMCFGCH_MWRSIZE_Msk      (0xFFU << OSPI_BMCFGCH_MWRSIZE_Pos)

// Prefetch Enable (bit 16).
// 0 = Prefetch disabled.
// 1 = Prefetch enabled (read-ahead for improved throughput).
#define OSPI_BMCFGCH_PREEN_Pos        16
#define OSPI_BMCFGCH_PREEN_Msk        (1U << OSPI_BMCFGCH_PREEN_Pos)

// Write Combination Timer (bits 31:24).
// Sets the time (in PCLKA clock cycles) to wait for additional write data before issuing a combined write transaction.
// 0x00 = Timer disabled, 0x01–0xFF = wait up to N cycles.
#define OSPI_BMCFGCH_CMBTIM_Pos       24
#define OSPI_BMCFGCH_CMBTIM_Msk       (0xFFU << OSPI_BMCFGCH_CMBTIM_Pos)

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
#define OSPI_CMCFG0CSN_FFMT_Pos       0
#define OSPI_CMCFG0CSN_FFMT_Msk       (0x3U << OSPI_CMCFG0CSN_FFMT_Pos)

// Address Size (bits 3:2).
// 0 = 1 byte, 1 = 2 bytes, 2 = 3 bytes, 3 = 4 bytes.
#define OSPI_CMCFG0CSN_ADDSIZE_Pos    2
#define OSPI_CMCFG0CSN_ADDSIZE_Msk    (0x3U << OSPI_CMCFG0CSN_ADDSIZE_Pos)

// Wrapping Burst Mode Enable (bit 4).
// 0 = Disabled; 1 = Enabled (for supporting continuous burst with wrap).
#define OSPI_CMCFG0CSN_WPBSTMD_Pos    4
#define OSPI_CMCFG0CSN_WPBSTMD_Msk    (1U << OSPI_CMCFG0CSN_WPBSTMD_Pos)

// Array Address Mode (bit 5).
// 0 = Linear addressing; 1 = Array address mode enabled.
#define OSPI_CMCFG0CSN_ARYAMD_Pos     5
#define OSPI_CMCFG0CSN_ARYAMD_Msk     (1U << OSPI_CMCFG0CSN_ARYAMD_Pos)

// Address Replace Enable (bits 23:16).
// Each bit enables address replacement for a corresponding byte position in the address field.
// Bit 16 = enable for address byte 0, Bit 17 = for byte 1, ..., Bit 23 = for byte 7.
#define OSPI_CMCFG0CSN_ADDRPEN_Pos    16
#define OSPI_CMCFG0CSN_ADDRPEN_Msk    (0xFFU << OSPI_CMCFG0CSN_ADDRPEN_Pos)

// Address Replace Code (bits 31:24).
// Value to be used as the replacement byte when address replacement is enabled.
#define OSPI_CMCFG0CSN_ADDRPCD_Pos    24
#define OSPI_CMCFG0CSN_ADDRPCD_Msk    (0xFFU << OSPI_CMCFG0CSN_ADDRPCD_Pos)

// =============================================================================
// OSPI_CMCFG1CSn - Command Map Configuration Register 1 CSn (n=0,1)
// The CMCFG1CSn register sets the command code and dummy cycles (latency cycles) for memory read operations for each chip select (CSn).
// This allows the controller to send the correct command sequence and number of wait cycles,
// matching the requirements of the connected memory device.
// =============================================================================

// Read Command Code (bits 15:0).
// Specifies the opcode(s) used to initiate a read operation on the memory device.
// Typical values: 0x03 (read), 0x0B (fast read), 0xEB, 0xEC, etc.
#define OSPI_CMCFG1CSN_RDCMD_Pos      0
#define OSPI_CMCFG1CSN_RDCMD_Msk      (0xFFFFU << OSPI_CMCFG1CSN_RDCMD_Pos)

// Read Latency (bits 20:16).
// Number of dummy cycles (latency cycles) to insert between command/address phase and data phase.
// Range: 0–31. Set according to device timing requirements.
#define OSPI_CMCFG1CSN_RDLATE_Pos     16
#define OSPI_CMCFG1CSN_RDLATE_Msk     (0x1FU << OSPI_CMCFG1CSN_RDLATE_Pos)

// =============================================================================
// OSPI_CMCFG2CSn - Command Map Configuration Register 2 CSn (n=0,1)
// The CMCFG2CSn register configures the command code and dummy cycles (latency cycles) for memory write operations for each chip select (CSn).
// This allows the OSPI controller to issue the correct command and wait cycles to the connected memory device during writes.
// =============================================================================

// Write Command Code (bits 15:0).
// Specifies the opcode(s) used to initiate a write/program operation on the memory device.
// Typical values: 0x02 (Page Program), 0x38, 0x12, etc.
#define OSPI_CMCFG2CSN_WRCMD_Pos      0
#define OSPI_CMCFG2CSN_WRCMD_Msk      (0xFFFFU << OSPI_CMCFG2CSN_WRCMD_Pos)

// Write Latency (bits 20:16).
// Number of dummy cycles (latency cycles) to insert between command/address phase and data phase for write operations.
// Range: 0–31. Set according to device timing requirements.
#define OSPI_CMCFG2CSN_WRLATE_Pos     16
#define OSPI_CMCFG2CSN_WRLATE_Msk     (0x1FU << OSPI_CMCFG2CSN_WRLATE_Pos)

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
#define OSPI_LIOCFGCSN_PRTMD_Pos      0
#define OSPI_LIOCFGCSN_PRTMD_Msk      (0x3FFU << OSPI_LIOCFGCSN_PRTMD_Pos)

// Latency Mode (bit 10).
// 0 = Standard mode, 1 = Variable/latency mode (device-specific).
#define OSPI_LIOCFGCSN_LATEMD_Pos     10
#define OSPI_LIOCFGCSN_LATEMD_Msk     (1U << OSPI_LIOCFGCSN_LATEMD_Pos)

// Write Mask Mode (bit 11).
// 0 = Normal write, 1 = Write with mask (for devices supporting masked writes).
#define OSPI_LIOCFGCSN_WRMSKMD_Pos    11
#define OSPI_LIOCFGCSN_WRMSKMD_Msk    (1U << OSPI_LIOCFGCSN_WRMSKMD_Pos)

// Minimum CS Inactive Cycles (bits 19:16).
// Minimum number of PCLKA cycles CS remains inactive between frames (0–15).
#define OSPI_LIOCFGCSN_CSMIN_Pos      16
#define OSPI_LIOCFGCSN_CSMIN_Msk      (0xFU << OSPI_LIOCFGCSN_CSMIN_Pos)

// CS Assert Extension (bit 20).
// 0 = Normal CS assertion; 1 = CS assertion is extended by 1 PCLKA cycle.
#define OSPI_LIOCFGCSN_CSASTEX_Pos    20
#define OSPI_LIOCFGCSN_CSASTEX_Msk    (1U << OSPI_LIOCFGCSN_CSASTEX_Pos)

// CS Negate Extension (bit 21).
// 0 = Normal CS negation; 1 = CS negation is extended by 1 PCLKA cycle.
#define OSPI_LIOCFGCSN_CSNEGEX_Pos    21
#define OSPI_LIOCFGCSN_CSNEGEX_Msk    (1U << OSPI_LIOCFGCSN_CSNEGEX_Pos)

// SDR Output Drive (bit 22).
// 0 = Normal drive; 1 = Stronger drive for SDR signals (device-dependent).
#define OSPI_LIOCFGCSN_SDRDRV_Pos     22
#define OSPI_LIOCFGCSN_SDRDRV_Msk     (1U << OSPI_LIOCFGCSN_SDRDRV_Pos)

// SDR Sampling Mode (bit 23).
// 0 = Standard sampling; 1 = Custom/advanced sampling (device-dependent).
#define OSPI_LIOCFGCSN_SDRSMPMD_Pos   23
#define OSPI_LIOCFGCSN_SDRSMPMD_Msk   (1U << OSPI_LIOCFGCSN_SDRSMPMD_Pos)

// SDR Sampling Shift (bits 27:24).
// Shifts SDR data sampling point (in PCLKA cycles), range 0–15.
#define OSPI_LIOCFGCSN_SDRSMPSFT_Pos  24
#define OSPI_LIOCFGCSN_SDRSMPSFT_Msk  (0xFU << OSPI_LIOCFGCSN_SDRSMPSFT_Pos)

// DDR Sampling Extension (bits 31:28).
// Adjusts DDR data sampling point, range 0–15.
#define OSPI_LIOCFGCSN_DDRSMPEX_Pos   28
#define OSPI_LIOCFGCSN_DDRSMPEX_Msk   (0xFU << OSPI_LIOCFGCSN_DDRSMPEX_Pos)

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
#define OSPI_BMCTL0_CH0CS0ACC_Pos     0
#define OSPI_BMCTL0_CH0CS0ACC_Msk     (0x3U << OSPI_BMCTL0_CH0CS0ACC_Pos)

// Channel 0 to CS1 access enable (bits 3:2).
// Controls access from system bus channel 0 to CS1 memory area.
// 0b00: Read/Write disable (no access)
// 0b01: Read enable, Write disable
// 0b10: Read disable, Write enable
// 0b11: Read/Write enable (full access)
#define OSPI_BMCTL0_CH0CS1ACC_Pos     2
#define OSPI_BMCTL0_CH0CS1ACC_Msk     (0x3U << OSPI_BMCTL0_CH0CS1ACC_Pos)

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
#define OSPI_BMCTL1_MWRPUSHCH0_Pos    8
#define OSPI_BMCTL1_MWRPUSHCH0_Msk    (1U << OSPI_BMCTL1_MWRPUSHCH0_Pos)

// Prefetch Buffer Clear for channel 0 (bit 10).
// Writing 1 clears the prefetch buffer for channel 0. Should NOT be set during memory access.
// 0: No command
// 1: Clear request (self-clearing bit)
#define OSPI_BMCTL1_PBUFCLRCH0_Pos    10
#define OSPI_BMCTL1_PBUFCLRCH0_Msk    (1U << OSPI_BMCTL1_PBUFCLRCH0_Pos)

// =============================================================================
// OSPI_CMCTLCHn – Command Map Control Register for Channel n (n = 0, 1)
// CMCTLCHn controls the XiP (Execute in Place) mode for each OSPI channel.
// Allows enabling XiP mode and setting enter/exit codes for XiP operation.
// =============================================================================

// XiP Mode Enter Code (bits 0–7).
// Sets the code to enter XiP mode in memory-mapping mode.
// 0x00–0xFF: User-defined code
#define OSPI_CMCTLCHn_XIPENCODE_Pos   0
#define OSPI_CMCTLCHn_XIPENCODE_Msk   (0xFFU << OSPI_CMCTLCHn_XIPENCODE_Pos)

// XiP Mode Exit Code (bits 8–15).
// Sets the code to exit XiP mode in memory-mapping mode.
// 0x00–0xFF: User-defined code
#define OSPI_CMCTLCHn_XIPEXCODE_Pos   8
#define OSPI_CMCTLCHn_XIPEXCODE_Msk   (0xFFU << OSPI_CMCTLCHn_XIPEXCODE_Pos)

// XiP Mode Enable (bit 16).
// Enables or disables XiP mode in memory-mapping mode.
// 0: Disable XiP mode (exit code is inserted in the latency field)
// 1: Enable XiP mode (enter code is inserted in the latency field)
// This bit is automatically cleared when the XiP disable pattern is transmitted.
#define OSPI_CMCTLCHn_XIPEN_Pos       16
#define OSPI_CMCTLCHn_XIPEN_Msk       (1U << OSPI_CMCTLCHn_XIPEN_Pos)

// Bits [31:17] – Reserved. Always read as 0, should be written as 0.

// =============================================================================
// OSPI_CDCTL0 – Command Manual Control Register 0
// CDCTL0 controls manual and periodic command transactions on the OSPI interface.
// It allows you to issue manual commands, configure periodic polling, and select the
// target chip select and number of commands.
// =============================================================================

// Transaction Request (bit 0).
// Write 1 to start a manual command transaction. Cleared to 0 when the transaction completes or is canceled.
// 0: No transaction
// 1: Request transaction
#define OSPI_CDCTL0_TRREQ_Pos         0
#define OSPI_CDCTL0_TRREQ_Msk         (1U << OSPI_CDCTL0_TRREQ_Pos)

// Periodic Mode Enable (bit 1).
// Enables periodic transaction mode for status polling operations.
// 0: Direct manual-command mode
// 1: Periodic manual-command mode
#define OSPI_CDCTL0_PERMD_Pos         1
#define OSPI_CDCTL0_PERMD_Msk         (1U << OSPI_CDCTL0_PERMD_Pos)

// Chip Select (bit 3).
// Selects which memory device (CS0 or CS1) the command will be issued to.
// 0: CS0
// 1: CS1
#define OSPI_CDCTL0_CSSEL_Pos         3
#define OSPI_CDCTL0_CSSEL_Msk         (1U << OSPI_CDCTL0_CSSEL_Pos)

// Transaction Number (bits 4–5).
// Number of commands to issue in normal manual-command mode (0–3).
// 00: 1 command (buffer 0)
// 01: 2 commands (buffers 0–1)
// 10: 3 commands (buffers 0–2)
// 11: 4 commands (buffers 0–3)
#define OSPI_CDCTL0_TRNUM_Pos         4
#define OSPI_CDCTL0_TRNUM_Msk         (0x3U << OSPI_CDCTL0_TRNUM_Pos)

// Periodic Transaction Interval (bits 16–20).
// Interval between periodic transactions (in periodic mode).
// Should be longer than 4 times the CPU bus cycle.
// 0x00: 2 cycles
// 0x01: 4 cycles
// ...
// 0x1E: 2,147,483,648 cycles
// 0x1F: 4,294,967,296 cycles
#define OSPI_CDCTL0_PERITV_Pos        16
#define OSPI_CDCTL0_PERITV_Msk        (0x1FU << OSPI_CDCTL0_PERITV_Pos)

// Periodic Transaction Repeat (bits 24–27).
// Number of repetitions for periodic transactions (in periodic mode).
// 0x0: 1 time
// 0x1: 2 times
// ...
// 0xE: 16,384 times
// 0xF: 32,768 times
#define OSPI_CDCTL0_PERREP_Pos        24
#define OSPI_CDCTL0_PERREP_Msk        (0xFU << OSPI_CDCTL0_PERREP_Pos)

// All other bits are reserved. Always read as 0, should be written as 0.

// =============================================================================
// OSPI_CDCTL1 – Command Manual Control Register 1
// CDCTL1 configures the expected value for periodic transactions in periodic manual-command mode.
// The register is used for status polling operations, where read data is compared with this expected value.
// =============================================================================

// Periodic Transaction Expected Value (bits 0–31).
// Sets the expected value to compare with the read value in periodic manual-command mode.
// For example, if comparing 1 byte, configure the lower byte.
#define OSPI_CDCTL1_PEREXP_Pos        0
#define OSPI_CDCTL1_PEREXP_Msk        (0xFFFFFFFFUL << OSPI_CDCTL1_PEREXP_Pos)

// =============================================================================
// OSPI_CDCTL2 – Command Manual Control Register 2
// CDCTL2 sets the mask value for periodic transactions in periodic manual-command mode.
// Any bit set to 1 in this register will cause the corresponding bit in the expected value (CDCTL1) to be ignored during comparison.
// =============================================================================

// Periodic Transaction Masked Value (bits 0–31).
// Sets the mask for the expected value in periodic manual-command mode.
// When a bit is set to 1, the corresponding bit in CDCTL1.PEREXP is ignored during comparison.
// For 8D-8D-8D mode, only even byte-pairs are transferred; unused bits should be masked (e.g., 0xFFFFFF00 for 1 byte).
#define OSPI_CDCTL2_PERMSK_Pos        0
#define OSPI_CDCTL2_PERMSK_Msk        (0xFFFFFFFFUL << OSPI_CDCTL2_PERMSK_Pos)

// =============================================================================
// OSPI_CDTBUFn – Command Manual Type Buffer n (n = 0 to 3)
// CDTBUFn configures the command, address, data size, latency, and type for each manual command buffer.
// This register defines the fields for a manual command issued through the OSPI interface.
// =============================================================================

// Command Size (bits 0–1).
// Specifies the number of bytes in the command phase.
// In 8D-8D-8D mode, this must be set to 2 (0b10).
// Both command size and address size must not be set to zero simultaneously.
// 00: 0 bytes (No command phase)
// 01: 1 byte
// 10: 2 bytes
#define OSPI_CDTBUFn_CMDSIZE_Pos      0
#define OSPI_CDTBUFn_CMDSIZE_Msk      (0x3U << OSPI_CDTBUFn_CMDSIZE_Pos)

// Address Size (bits 2–4).
// Specifies the number of bytes in the address phase.
// 000: 0 bytes (No address phase)
// 001: 1 byte
// 010: 2 bytes
// 011: 3 bytes
// 100: 4 bytes
#define OSPI_CDTBUFn_ADDSIZE_Pos      2
#define OSPI_CDTBUFn_ADDSIZE_Msk      (0x7U << OSPI_CDTBUFn_ADDSIZE_Pos)

// Data Size (bits 5–8).
// Specifies the number of bytes in the data phase.
// In 8D-8D-8D mode, only even byte counts are valid for data transfer.
// 0x0: 0 bytes (No data phase)
// 0x1: 1 byte
// ...
// 0x8: 8 bytes
#define OSPI_CDTBUFn_DATASIZE_Pos     5
#define OSPI_CDTBUFn_DATASIZE_Msk     (0xFU << OSPI_CDTBUFn_DATASIZE_Pos)

// Latency Cycle (bits 9–13).
// Specifies the number of dummy cycles (latency cycles) inserted.
// 0x0: No latency
// 0x1: 1 cycle
// ...
// 0x1F: 31 cycles
#define OSPI_CDTBUFn_LATE_Pos         9
#define OSPI_CDTBUFn_LATE_Msk         (0x1FU << OSPI_CDTBUFn_LATE_Pos)

// Reserved (bit 14).
// Always read as 0; write value should be 0.

// Transaction Type (bit 15).
// Selects the type of transaction for this buffer.
// 0: Read transaction (readout from slave device)
// 1: Not read transaction (typically write)
#define OSPI_CDTBUFn_TRTYPE_Pos       15
#define OSPI_CDTBUFn_TRTYPE_Msk       (1U << OSPI_CDTBUFn_TRTYPE_Pos)

// Command (bits 16–31).
// Specifies the command code to be issued during the manual command transaction.
// The number of bytes used is determined by CMDSIZE.
// For 8D-8D-8D mode, the meaning of bits varies by profile.
#define OSPI_CDTBUFn_CMD_Pos          16
#define OSPI_CDTBUFn_CMD_Msk          (0xFFFFU << OSPI_CDTBUFn_CMD_Pos)

// =============================================================================
// OSPI_CDABUFn – Command Manual Address Buffer n (n = 0 to 3)
// CDABUFn sets the address field used for manual command operations via the OSPI interface.
// For profile 2.0 (8D-8D-8D), this field may be used as part of the command & modifier field.
// =============================================================================

// Address (bits 0–31).
// Sets the address to be used in manual-command mode.
// For 1S-1S-1S, 4S-4D-4D, and 8D-8D-8D profile 1.0: this is the address field.
// For 8D-8D-8D profile 2.0: this field is used as the lower 4 bytes of the command & modifier field.
#define OSPI_CDABUFn_ADD_Pos          0
#define OSPI_CDABUFn_ADD_Msk          (0xFFFFFFFFUL << OSPI_CDABUFn_ADD_Pos)

// =============================================================================
// OSPI_CDD0BUFn – Command Manual Data 0 Buffer n (n = 0 to 3)
// CDD0BUFn is used to set or retrieve data for manual command operations via the OSPI interface.
// For write transactions, this register provides the write data.
// For read transactions, the received data is stored in this register after the operation.
// =============================================================================

// Data (bits 0–31).
// Sets the data to be written or read in manual-command mode.
// For write transactions: set the value to be transmitted.
// For read transactions: the received data will be stored here.
#define OSPI_CDD0BUFn_DATA_Pos        0
#define OSPI_CDD0BUFn_DATA_Msk        (0xFFFFFFFFUL << OSPI_CDD0BUFn_DATA_Pos)

// =============================================================================
// OSPI_CDD1BUFn – Command Manual Data 1 Buffer n (n = 0 to 3)
// CDD1BUFn is used to set or retrieve additional data for manual command operations via the OSPI interface.
// For write transactions, this register provides additional write data.
// For read transactions, additional received data is stored in this register after the operation.
// =============================================================================

// Data (bits 0–31).
// Sets the data to be written or read in manual-command mode.
// For write transactions: set the value to be transmitted.
// For read transactions: the received data will be stored here.
#define OSPI_CDD1BUFn_DATA_Pos        0
#define OSPI_CDD1BUFn_DATA_Msk        (0xFFFFFFFFUL << OSPI_CDD1BUFn_DATA_Pos)

// =============================================================================
// OSPI_LPCTL0 – Link Pattern Control Register 0
// LPCTL0 is used to configure and request transmission of the XiP Disable pattern on the OSPI link.
// It controls the pins, timing, and values of the pattern phases sent to the external memory.
// =============================================================================

// Pattern Request (bit 0).
// Write 1 to start transmission of the XiP Disable pattern. Cleared to 0 when the pattern completes.
// 0: No request (XiP Disable pattern not issued)
// 1: Request XiP Disable pattern
#define OSPI_LPCTL0_PATREQ_Pos        0
#define OSPI_LPCTL0_PATREQ_Msk        (1U << OSPI_LPCTL0_PATREQ_Pos)

// Chip Select (bit 3).
// Selects the target memory (chip select) for pattern transmission.
// 0: slave0 (CS0)
// 1: slave1 (CS1)
#define OSPI_LPCTL0_CSSEL_Pos         3
#define OSPI_LPCTL0_CSSEL_Msk         (1U << OSPI_LPCTL0_CSSEL_Pos)

// XiP Disable Pattern Pin (bits 4–5).
// Selects the number of pins used for the XiP Disable pattern output.
// 00: 1 pin
// 01: 2 pins
// 10: 4 pins
// 11: 8 pins
#define OSPI_LPCTL0_XDPIN_Pos         4
#define OSPI_LPCTL0_XDPIN_Msk         (0x3U << OSPI_LPCTL0_XDPIN_Pos)

// XiP Disable Pattern 1st Phase Length (bits 16–20).
// Number of cycles for the 1st phase of the XiP Disable pattern.
// 0x0: 0 cycles
// ...
// 0x1F: 31 cycles
#define OSPI_LPCTL0_XD1LEN_Pos        16
#define OSPI_LPCTL0_XD1LEN_Msk        (0x1FU << OSPI_LPCTL0_XD1LEN_Pos)

// XiP Disable Pattern 1st Phase Value (bit 23).
// Output value for the 1st phase.
// 0: Low drive
// 1: High drive
#define OSPI_LPCTL0_XD1VAL_Pos        23
#define OSPI_LPCTL0_XD1VAL_Msk        (1U << OSPI_LPCTL0_XD1VAL_Pos)

// XiP Disable Pattern 2nd Phase Length (bits 24–28).
// Number of cycles for the 2nd phase of the XiP Disable pattern.
// 0x00: 0 cycles
// ...
// 0x1F: 31 cycles
#define OSPI_LPCTL0_XD2LEN_Pos        24
#define OSPI_LPCTL0_XD2LEN_Msk        (0x1FU << OSPI_LPCTL0_XD2LEN_Pos)

// XiP Disable Pattern 2nd Phase Value (bit 31).
// Output value for the 2nd phase.
// 0: Low drive
// 1: High drive
#define OSPI_LPCTL0_XD2VAL_Pos        31
#define OSPI_LPCTL0_XD2VAL_Msk        (1U << OSPI_LPCTL0_XD2VAL_Pos)

// All other bits are reserved. Always read as 0, should be written as 0.

// =============================================================================
// OSPI_LPCTL1 – Link Pattern Control Register 1
// LPCTL1 is used to request and configure reset or CS-only patterns on the OSPI link.
// It controls the chip select target, number of repeats, timing, and data output setup for reset signaling.
// =============================================================================

// Pattern Request (bits 0–1).
// Requests transmission of a pattern. Cleared to 00b when the pattern completes.
// 00: No request
// 01: Request Reset pattern
// 10: Request CS-only pattern
// 11: Setting prohibited
#define OSPI_LPCTL1_PATREQ_Pos        0
#define OSPI_LPCTL1_PATREQ_Msk        (0x3U << OSPI_LPCTL1_PATREQ_Pos)

// Chip Select (bit 3).
// Selects the target memory (chip select) for pattern transmission.
// 0: slave0 (CS0)
// 1: slave1 (CS1)
#define OSPI_LPCTL1_CSSEL_Pos         3
#define OSPI_LPCTL1_CSSEL_Msk         (1U << OSPI_LPCTL1_CSSEL_Pos)

// Reset Pattern Repeat (bits 4–5).
// Number of times to toggle CS from LOW to HIGH during reset pattern.
// 00: 4 times (per protocol)
// 01: 5 times
// 10: 6 times
// 11: 7 times
#define OSPI_LPCTL1_RSTREP_Pos        4
#define OSPI_LPCTL1_RSTREP_Msk        (0x3U << OSPI_LPCTL1_RSTREP_Pos)

// Reset Pattern Width (bits 8–10).
// Width (in cycles) of each CS toggle during reset/CS-only pattern.
// 000: 2 cycles
// 001: 4 cycles
// ...
// 110: 128 cycles
// 111: 256 cycles
#define OSPI_LPCTL1_RSTWID_Pos        8
#define OSPI_LPCTL1_RSTWID_Msk        (0x7U << OSPI_LPCTL1_RSTWID_Pos)

// Reset Pattern Data Output Setup Time (bits 12–14).
// Number of setup cycles for data output, based on CS edge during reset pattern.
// Should be less than the pattern width.
// 000: 1 cycle
// 001: 2 cycles
// ...
// 110: 7 cycles
// 111: 8 cycles
#define OSPI_LPCTL1_RSTSU_Pos         12
#define OSPI_LPCTL1_RSTSU_Msk         (0x7U << OSPI_LPCTL1_RSTSU_Pos)

// All other bits are reserved. Always read as 0, should be written as 0.

// =============================================================================
// OSPI_LIOCTL – Link I/O Control Register
// LIOCTL controls the output value of the Write Protect (WP) and Reset (RST) pins for the OSPI slave devices.
// It allows the firmware to manually drive these control lines, if supported by the attached xSPI memory.
// =============================================================================

// Write Protect for Slave 1 (bit 1).
// Controls the output value of the OM_WP1 port (Write Protect for slave1).
// 0: Drive Low level
// 1: Drive High level
#define OSPI_LIOCTL_WPCS1_Pos         1
#define OSPI_LIOCTL_WPCS1_Msk         (1U << OSPI_LIOCTL_WPCS1_Pos)

// Reset Drive for Slave 0 (bit 16).
// Controls the output value of the OM_RESET port (Reset for slave0).
// 0: Drive Low level
// 1: Drive High level
#define OSPI_LIOCTL_RSTCS0_Pos        16
#define OSPI_LIOCTL_RSTCS0_Msk        (1U << OSPI_LIOCTL_RSTCS0_Pos)

// Bit 0 and Bit 17 are always read as 1, and should be written as 1.
// All other bits are reserved. Always read as 0, should be written as 0.

// =============================================================================
// OSPI_CCCTL0CSn – Command Calibration Control Register 0 CSn (n = 0, 1)
// CCCTL0CSn configures and enables automatic calibration for each xSPI slave channel (chip select).
// It sets up calibration intervals, DQS shift range, and enables or disables the calibration sequence.
// =============================================================================

// Automatic Calibration Enable (bit 0).
// Enables automatic calibration sequence for the target chip select.
// 0: Disable automatic calibration
// 1: Enable automatic calibration
#define OSPI_CCCTL0CSn_CAEN_Pos       0
#define OSPI_CCCTL0CSn_CAEN_Msk       (1U << OSPI_CCCTL0CSn_CAEN_Pos)

// Calibration No Write Mode (bit 1).
// Selects whether the calibration sequence omits the write command.
// 0: Calibration with write command
// 1: Calibration without write command (for devices with fixed calibration data)
#define OSPI_CCCTL0CSn_CANOWR_Pos     1
#define OSPI_CCCTL0CSn_CANOWR_Msk     (1U << OSPI_CCCTL0CSn_CANOWR_Pos)

// Calibration Interval (bits 8–12).
// Interval (in cycles) between calibration patterns.
// 0x00: 2 cycles
// 0x01: 4 cycles
// ...
// 0x1F: 4,294,967,296 cycles
#define OSPI_CCCTL0CSn_CAITV_Pos      8
#define OSPI_CCCTL0CSn_CAITV_Msk      (0x1FU << OSPI_CCCTL0CSn_CAITV_Pos)

// Calibration OM_DQS Shift Start Value (bits 16–20).
// Start value for OM_DQS shift during calibration.
#define OSPI_CCCTL0CSn_CASFTSTA_Pos   16
#define OSPI_CCCTL0CSn_CASFTSTA_Msk   (0x1FU << OSPI_CCCTL0CSn_CASFTSTA_Pos)

// Calibration OM_DQS Shift End Value (bits 24–28).
// End value for OM_DQS shift during calibration. Must be equal or greater than start value.
#define OSPI_CCCTL0CSn_CASFTEND_Pos   24
#define OSPI_CCCTL0CSn_CASFTEND_Msk   (0x1FU << OSPI_CCCTL0CSn_CASFTEND_Pos)

// All other bits are reserved. Always read as 0, should be written as 0.

// =============================================================================
// OSPI_CCCTL1CSn – Command Calibration Control Register 1 CSn (n = 0, 1)
// CCCTL1CSn configures the command, address, data sizes, and latency cycles for read and write calibration frames for each xSPI slave channel.
// =============================================================================

// Command Size (bits 0–1).
// Sets the size of the command field for the calibration sequence.
// In 8D-8D-8D mode, must be set to 2 (0b10).
// 00: 0 bytes (No command phase)
// 01: 1 byte
// 10: 2 bytes
// 11: Setting prohibited
#define OSPI_CCCTL1CSn_CACMDSIZE_Pos  0
#define OSPI_CCCTL1CSn_CACMDSIZE_Msk  (0x3U << OSPI_CCCTL1CSn_CACMDSIZE_Pos)

// Address Size (bits 2–4).
// Sets the size of the address field for the calibration sequence.
// 000: 0 bytes (No address phase)
// 001: 1 byte
// 010: 2 bytes
// 011: 3 bytes
// 100: 4 bytes
#define OSPI_CCCTL1CSn_CAADDSIZE_Pos  2
#define OSPI_CCCTL1CSn_CAADDSIZE_Msk  (0x7U << OSPI_CCCTL1CSn_CAADDSIZE_Pos)

// Data Size (bits 5–8).
// Sets the size of the data field for calibration.
// In 8D-8D-8D mode, must be an even value.
// 0x0: 1 byte
// 0x1: 2 bytes
// ...
// 0xF: 16 bytes
#define OSPI_CCCTL1CSn_CADATASIZE_Pos 5
#define OSPI_CCCTL1CSn_CADATASIZE_Msk (0xFU << OSPI_CCCTL1CSn_CADATASIZE_Pos)

// Write Latency Cycle (bits 16–20).
// Number of latency cycles for the write calibration frame.
// 0x00: No latency
// 0x01: 1 cycle
// ...
// 0x1F: 31 cycles
#define OSPI_CCCTL1CSn_CAWRLATE_Pos   16
#define OSPI_CCCTL1CSn_CAWRLATE_Msk   (0x1FU << OSPI_CCCTL1CSn_CAWRLATE_Pos)

// Read Latency Cycle (bits 24–28).
// Number of latency cycles for the read calibration frame.
// 0x00: No latency
// 0x01: 1 cycle
// ...
// 0x1F: 31 cycles
#define OSPI_CCCTL1CSn_CARDLATE_Pos   24
#define OSPI_CCCTL1CSn_CARDLATE_Msk   (0x1FU << OSPI_CCCTL1CSn_CARDLATE_Pos)

// All other bits are reserved. Always read as 0, should be written as 0.

// =============================================================================
// OSPI_CCCTL2CSn – Command Calibration Control Register 2 CSn (n = 0, 1)
// CCCTL2CSn sets the command values for the calibration read and write patterns for each xSPI slave channel.
// =============================================================================

// Calibration Pattern Write Command (bits 0–15).
// Sets the command code for the calibration pattern write operation.
#define OSPI_CCCTL2CSn_CAWRCMD_Pos    0
#define OSPI_CCCTL2CSn_CAWRCMD_Msk    (0xFFFFU << OSPI_CCCTL2CSn_CAWRCMD_Pos)

// Calibration Pattern Read Command (bits 16–31).
// Sets the command code for the calibration pattern read operation.
#define OSPI_CCCTL2CSn_CARDCMD_Pos    16
#define OSPI_CCCTL2CSn_CARDCMD_Msk    (0xFFFFU << OSPI_CCCTL2CSn_CARDCMD_Pos)

// =============================================================================
// OSPI_CCCTL3CSn – Command Calibration Control Register 3 CSn (n = 0, 1)
// CCCTL3CSn sets the address used for the calibration pattern operation for each xSPI slave channel.
// =============================================================================

// Calibration Pattern Address (bits 0–31).
// Sets the address for the calibration pattern operation during calibration sequences.
#define OSPI_CCCTL3CSn_CAADD_Pos      0
#define OSPI_CCCTL3CSn_CAADD_Msk      (0xFFFFFFFFUL << OSPI_CCCTL3CSn_CAADD_Pos)

// =============================================================================
// OSPI_CCCTL4CSn – Command Calibration Control Register 4 CSn (n = 0, 1)
// CCCTL4CSn sets the data value for the calibration pattern operation for each xSPI slave channel.
// =============================================================================

// Calibration Pattern Data (bits 0–31).
// Sets the data value for the calibration pattern during calibration sequences.
#define OSPI_CCCTL4CSn_CADATA_Pos     0
#define OSPI_CCCTL4CSn_CADATA_Msk     (0xFFFFFFFFUL << OSPI_CCCTL4CSn_CADATA_Pos)

// =============================================================================
// OSPI_CCCTL5CSn – Command Calibration Control Register 5 CSn (n = 0, 1)
// CCCTL5CSn sets the second data value for the calibration pattern operation for each xSPI slave channel.
// =============================================================================

// Calibration Pattern Data (bits 0–31).
// Sets the second data value for the calibration pattern during calibration sequences.
#define OSPI_CCCTL5CSn_CADATA_Pos     0
#define OSPI_CCCTL5CSn_CADATA_Msk     (0xFFFFFFFFUL << OSPI_CCCTL5CSn_CADATA_Pos)

// =============================================================================
// OSPI_CCCTL6CSn – Command Calibration Control Register 6 CSn (n = 0, 1)
// CCCTL6CSn sets the third data value for the calibration pattern operation for each xSPI slave channel.
// =============================================================================

// Calibration Pattern Data (bits 0–31).
// Sets the third data value for the calibration pattern during calibration sequences.
#define OSPI_CCCTL6CSn_CADATA_Pos     0
#define OSPI_CCCTL6CSn_CADATA_Msk     (0xFFFFFFFFUL << OSPI_CCCTL6CSn_CADATA_Pos)

// =============================================================================
// OSPI_CCCTL7CSn – Command Calibration Control Register 7 CSn (n = 0, 1)
// CCCTL7CSn sets the fourth data value for the calibration pattern operation for each xSPI slave channel.
// =============================================================================

// Calibration Pattern Data (bits 0–31).
// Sets the fourth data value for the calibration pattern during calibration sequences.
#define OSPI_CCCTL7CSn_CADATA_Pos     0
#define OSPI_CCCTL7CSn_CADATA_Msk     (0xFFFFFFFFUL << OSPI_CCCTL7CSn_CADATA_Pos)

// =============================================================================
// OSPI_COMSTT - OSPI Common Status Register
// COMSTT monitors the overall status of the xSPI Master.
// This register contains various status bits related to memory access, prefetch, buffer status, and external signals.
// =============================================================================

// Memory access ongoing from channel 0 (bit 0).
// Indicates whether system bus bridge channel 0 is accessing the memory.
// 0: Not accessing
// 1: Accessing
#define OSPI_COMSTT_MEMACCCH0_Pos     0
#define OSPI_COMSTT_MEMACCCH0_Msk     (1U << OSPI_COMSTT_MEMACCCH0_Pos)

// Prefetch Buffer Not Empty for channel 0 (bit 4).
// Indicates whether the prefetch buffer for channel 0 contains data.
// 0: Empty
// 1: Not empty
#define OSPI_COMSTT_PBUFNECH0_Pos     4
#define OSPI_COMSTT_PBUFNECH0_Msk     (1U << OSPI_COMSTT_PBUFNECH0_Pos)

// Write Buffer Not Empty for channel 0 (bit 6).
// Indicates whether the write buffer for channel 0 contains data.
// 0: Empty
// 1: Not empty
#define OSPI_COMSTT_WRBUFNECH0_Pos    6
#define OSPI_COMSTT_WRBUFNECH0_Msk    (1U << OSPI_COMSTT_WRBUFNECH0_Pos)

// ECS monitor for slave 1 (bit 20).
// Indicates the value of the OM_ECSINT1 port.
// 0: Low level
// 1: High level
#define OSPI_COMSTT_ECSCS1_Pos        20
#define OSPI_COMSTT_ECSCS1_Msk        (1U << OSPI_COMSTT_ECSCS1_Pos)

// INT monitor for slave 1 (bit 21).
// Indicates the value of the OM_ECSINT1 port.
// 0: Low level
// 1: High level
#define OSPI_COMSTT_INTCS1_Pos        21
#define OSPI_COMSTT_INTCS1_Msk        (1U << OSPI_COMSTT_INTCS1_Pos)

// RSTO monitor for slave 1 (bit 22).
// Indicates the value of the OM_RSTO1 port.
// 0: Low level
// 1: High level
#define OSPI_COMSTT_RSTOCS1_Pos       22
#define OSPI_COMSTT_RSTOCS1_Msk       (1U << OSPI_COMSTT_RSTOCS1_Pos)

// Note: Bits 3:1, 5, and 19:7, and 31:23 are reserved and read as 0.

// =============================================================================
// OSPI_CASTTCSn - OSPI Calibration Status Register CSn (n = 0, 1)
// CASTTCSn indicates calibration success for each OM_DQS shift value for slave n.
// This register is updated after each calibration sequence.
// =============================================================================

// Calibration Success for each OM_DQS shift value (bits 0..31).
// Each bit CASUC[x] indicates calibration success for OM_DQS shift value = x.
// 0: Calibration failed for this shift value
// 1: Calibration succeeded for this shift value
#define OSPI_CASTTCSn_CASUC_Pos       0
#define OSPI_CASTTCSn_CASUC_Msk       (0xFFFFFFFFUL << OSPI_CASTTCSn_CASUC_Pos)

// =============================================================================
// OSPI_INTS - OSPI Interrupt Status Register
// INTS indicates interrupt status flags for xSPI Master, including command completion, pattern completion, errors, and calibration status.
// Each flag is set when the corresponding event occurs, and is cleared by writing 1 to the corresponding bit in the INTC register.
// =============================================================================

// Command Completed (bit 0).
// Set to 1 when a manual command transaction completes.
// 0: No detection
// 1: Detection
#define OSPI_INTS_CMDCMP_Pos          0
#define OSPI_INTS_CMDCMP_Msk          (1U << OSPI_INTS_CMDCMP_Pos)

// Pattern Completed (bit 1).
// Set to 1 when the requested pattern is completed.
// 0: No detection
// 1: Detection
#define OSPI_INTS_PATCMP_Pos          1
#define OSPI_INTS_PATCMP_Msk          (1U << OSPI_INTS_PATCMP_Pos)

// Periodic Transaction Timeout (bit 3).
// Set to 1 when a timeout occurs in periodic manual-command mode.
// 0: No detection
// 1: Detection
#define OSPI_INTS_PERTO_Pos           3
#define OSPI_INTS_PERTO_Msk           (1U << OSPI_INTS_PERTO_Pos)

// OM_DQS Timeout for slave0 (bit 4).
// Set to 1 when OM_DQS is lost in a read transaction for slave0.
// 0: No detection
// 1: Detection
#define OSPI_INTS_DSTOCS0_Pos         4
#define OSPI_INTS_DSTOCS0_Msk         (1U << OSPI_INTS_DSTOCS0_Pos)

// OM_DQS Timeout for slave1 (bit 5).
// Set to 1 when OM_DQS is lost in a read transaction for slave1.
// 0: No detection
// 1: Detection
#define OSPI_INTS_DSTOCS1_Pos         5
#define OSPI_INTS_DSTOCS1_Msk         (1U << OSPI_INTS_DSTOCS1_Pos)

// ECC error detection for slave1 (bit 9).
// Set to 1 when a falling edge is detected on OM_ECSINT1 port.
// 0: No detection
// 1: Detection
#define OSPI_INTS_ECSCS1_Pos          9
#define OSPI_INTS_ECSCS1_Msk          (1U << OSPI_INTS_ECSCS1_Pos)

// Interrupt detection for slave1 (bit 13).
// Set to 1 when a falling edge is detected on OM_ECSINT1 port for slave1.
// 0: No detection
// 1: Detection
#define OSPI_INTS_INTCS1_Pos          13
#define OSPI_INTS_INTCS1_Msk          (1U << OSPI_INTS_INTCS1_Pos)

// System bus error for channel 0 (bit 20).
// Set to 1 when an error response occurs on system bus channel 0.
// 0: No detection
// 1: Detection
#define OSPI_INTS_BUSERRCH0_Pos       20
#define OSPI_INTS_BUSERRCH0_Msk       (1U << OSPI_INTS_BUSERRCH0_Pos)

// Calibration failed for slave0 (bit 28).
// Set to 1 when calibration fails for slave0.
// 0: No detection
// 1: Detection
#define OSPI_INTS_CAFAILCS0_Pos       28
#define OSPI_INTS_CAFAILCS0_Msk       (1U << OSPI_INTS_CAFAILCS0_Pos)

// Calibration failed for slave1 (bit 29).
// Set to 1 when calibration fails for slave1.
// 0: No detection
// 1: Detection
#define OSPI_INTS_CAFAILCS1_Pos       29
#define OSPI_INTS_CAFAILCS1_Msk       (1U << OSPI_INTS_CAFAILCS1_Pos)

// Calibration success for slave0 (bit 30).
// Set to 1 when calibration succeeds for slave0.
// 0: No detection
// 1: Detection
#define OSPI_INTS_CASUCCS0_Pos        30
#define OSPI_INTS_CASUCCS0_Msk        (1U << OSPI_INTS_CASUCCS0_Pos)

// Calibration success for slave1 (bit 31).
// Set to 1 when calibration succeeds for slave1.
// 0: No detection
// 1: Detection
#define OSPI_INTS_CASUCCS1_Pos        31
#define OSPI_INTS_CASUCCS1_Msk        (1U << OSPI_INTS_CASUCCS1_Pos)

// Reserved bits: 2, 6-8, 10-12, 14-19, 21-27 (read as 0).

// =============================================================================
// OSPI_INTC - OSPI Interrupt Clear Register
// INTC clears the interrupt flags of the xSPI Master.
// Writing 1 to any bit clears the corresponding interrupt status flag in INTS.
// Writing 0 has no effect.
// =============================================================================

// Command Completed interrupt clear (bit 0).
// Write 1 to clear the Command Completed interrupt flag (INTS.CMDCMP).
#define OSPI_INTC_CMDCMPC_Pos         0
#define OSPI_INTC_CMDCMPC_Msk         (1U << OSPI_INTC_CMDCMPC_Pos)

// Pattern Completed interrupt clear (bit 1).
// Write 1 to clear the Pattern Completed interrupt flag (INTS.PATCMP).
#define OSPI_INTC_PATCMPC_Pos         1
#define OSPI_INTC_PATCMPC_Msk         (1U << OSPI_INTC_PATCMPC_Pos)

// Periodic transaction timeout interrupt clear (bit 3).
// Write 1 to clear the Periodic Transaction Timeout interrupt flag (INTS.PERTO).
#define OSPI_INTC_PERTOC_Pos          3
#define OSPI_INTC_PERTOC_Msk          (1U << OSPI_INTC_PERTOC_Pos)

// OM_DQS timeout for slave0 interrupt clear (bit 4).
// Write 1 to clear the OM_DQS timeout interrupt flag for slave0 (INTS.DSTOCS0).
#define OSPI_INTC_DSTOCS0C_Pos        4
#define OSPI_INTC_DSTOCS0C_Msk        (1U << OSPI_INTC_DSTOCS0C_Pos)

// OM_DQS timeout for slave1 interrupt clear (bit 5).
// Write 1 to clear the OM_DQS timeout interrupt flag for slave1 (INTS.DSTOCS1).
#define OSPI_INTC_DSTOCS1C_Pos        5
#define OSPI_INTC_DSTOCS1C_Msk        (1U << OSPI_INTC_DSTOCS1C_Pos)

// ECC error detection for slave1 interrupt clear (bit 9).
// Write 1 to clear the ECC error detection interrupt flag for slave1 (INTS.ECSCS1).
#define OSPI_INTC_ECSCS1C_Pos         9
#define OSPI_INTC_ECSCS1C_Msk         (1U << OSPI_INTC_ECSCS1C_Pos)

// Interrupt detection for slave1 interrupt clear (bit 13).
// Write 1 to clear the interrupt detection flag for slave1 (INTS.INTCS1).
#define OSPI_INTC_INTCS1C_Pos         13
#define OSPI_INTC_INTCS1C_Msk         (1U << OSPI_INTC_INTCS1C_Pos)

// System bus error for ch0 interrupt clear (bit 20).
// Write 1 to clear the system bus error interrupt flag for channel 0 (INTS.BUSERRCH0).
#define OSPI_INTC_BUSERRCH0C_Pos      20
#define OSPI_INTC_BUSERRCH0C_Msk      (1U << OSPI_INTC_BUSERRCH0C_Pos)

// Calibration failed for slave0 interrupt clear (bit 28).
// Write 1 to clear the calibration failed interrupt flag for slave0 (INTS.CAFAILCS0).
#define OSPI_INTC_CAFAILCS0C_Pos      28
#define OSPI_INTC_CAFAILCS0C_Msk      (1U << OSPI_INTC_CAFAILCS0C_Pos)

// Calibration failed for slave1 interrupt clear (bit 29).
// Write 1 to clear the calibration failed interrupt flag for slave1 (INTS.CAFAILCS1).
#define OSPI_INTC_CAFAILCS1C_Pos      29
#define OSPI_INTC_CAFAILCS1C_Msk      (1U << OSPI_INTC_CAFAILCS1C_Pos)

// Calibration success for slave0 interrupt clear (bit 30).
// Write 1 to clear the calibration success interrupt flag for slave0 (INTS.CASUCCS0).
#define OSPI_INTC_CASUCCS0C_Pos       30
#define OSPI_INTC_CASUCCS0C_Msk       (1U << OSPI_INTC_CASUCCS0C_Pos)

// Calibration success for slave1 interrupt clear (bit 31).
// Write 1 to clear the calibration success interrupt flag for slave1 (INTS.CASUCCS1).
#define OSPI_INTC_CASUCCS1C_Pos       31
#define OSPI_INTC_CASUCCS1C_Msk       (1U << OSPI_INTC_CASUCCS1C_Pos)

// Reserved bits: 2, 6-8, 10-12, 14-19, 21-27 (write 0).

// =============================================================================
// OSPI_INTE - OSPI Interrupt Enable Register
// INTE enables or disables interrupt sources for xSPI Master.
// Each bit enables the corresponding interrupt in the INTS register.
// =============================================================================

// Command Completed interrupt enable (bit 0).
// 0: Disabled
// 1: Enabled
#define OSPI_INTE_CMDCMPE_Pos         0
#define OSPI_INTE_CMDCMPE_Msk         (1U << OSPI_INTE_CMDCMPE_Pos)

// Pattern Completed interrupt enable (bit 1).
// 0: Disabled
// 1: Enabled
#define OSPI_INTE_PATCMPE_Pos         1
#define OSPI_INTE_PATCMPE_Msk         (1U << OSPI_INTE_PATCMPE_Pos)

// Periodic transaction timeout interrupt enable (bit 3).
// 0: Disabled
// 1: Enabled
#define OSPI_INTE_PERTOE_Pos          3
#define OSPI_INTE_PERTOE_Msk          (1U << OSPI_INTE_PERTOE_Pos)

// OM_DQS timeout for slave0 interrupt enable (bit 4).
// 0: Disabled
// 1: Enabled
#define OSPI_INTE_DSTOCS0E_Pos        4
#define OSPI_INTE_DSTOCS0E_Msk        (1U << OSPI_INTE_DSTOCS0E_Pos)

// OM_DQS timeout for slave1 interrupt enable (bit 5).
// 0: Disabled
// 1: Enabled
#define OSPI_INTE_DSTOCS1E_Pos        5
#define OSPI_INTE_DSTOCS1E_Msk        (1U << OSPI_INTE_DSTOCS1E_Pos)

// ECC error detection for slave1 interrupt enable (bit 9).
// 0: Disabled
// 1: Enabled
#define OSPI_INTE_ECSCS1E_Pos         9
#define OSPI_INTE_ECSCS1E_Msk         (1U << OSPI_INTE_ECSCS1E_Pos)

// Interrupt detection for slave1 interrupt enable (bit 13).
// 0: Disabled
// 1: Enabled
#define OSPI_INTE_INTCS1E_Pos         13
#define OSPI_INTE_INTCS1E_Msk         (1U << OSPI_INTE_INTCS1E_Pos)

// System bus error for channel 0 interrupt enable (bit 20).
// 0: Disabled
// 1: Enabled
#define OSPI_INTE_BUSERRCH0E_Pos      20
#define OSPI_INTE_BUSERRCH0E_Msk      (1U << OSPI_INTE_BUSERRCH0E_Pos)

// Calibration failed for slave0 interrupt enable (bit 28).
// 0: Disabled
// 1: Enabled
#define OSPI_INTE_CAFAILCS0E_Pos      28
#define OSPI_INTE_CAFAILCS0E_Msk      (1U << OSPI_INTE_CAFAILCS0E_Pos)

// Calibration failed for slave1 interrupt enable (bit 29).
// 0: Disabled
// 1: Enabled
#define OSPI_INTE_CAFAILCS1E_Pos      29
#define OSPI_INTE_CAFAILCS1E_Msk      (1U << OSPI_INTE_CAFAILCS1E_Pos)

// Calibration success for slave0 interrupt enable (bit 30).
// 0: Disabled
// 1: Enabled
#define OSPI_INTE_CASUCCS0E_Pos       30
#define OSPI_INTE_CASUCCS0E_Msk       (1U << OSPI_INTE_CASUCCS0E_Pos)

// Calibration success for slave1 interrupt enable (bit 31).
// 0: Disabled
// 1: Enabled
#define OSPI_INTE_CASUCCS1E_Pos       31
#define OSPI_INTE_CASUCCS1E_Msk       (1U << OSPI_INTE_CASUCCS1E_Pos)

// Reserved bits: 2, 6-8, 10-12, 14-19, 21-27 (read as 0, write 0).

#endif  // RA8M1_OSPI_H
