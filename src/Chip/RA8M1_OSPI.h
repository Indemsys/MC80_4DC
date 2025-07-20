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

#define OSPI_WRAPCFG_DSSFTCS0_Pos    8                                     // DSS shift CS0 position
#define OSPI_WRAPCFG_DSSFTCS0_Msk    (0x1FU << OSPI_WRAPCFG_DSSFTCS0_Pos)  // DSS shift CS0 mask
#define OSPI_WRAPCFG_DSSFTCS1_Pos    24                                    // DSS shift CS1 position
#define OSPI_WRAPCFG_DSSFTCS1_Msk    (0x1FU << OSPI_WRAPCFG_DSSFTCS1_Pos)  // DSS shift CS1 mask

// =============================================================================
// OSPI_COMCFG - Common Configuration Register
// =============================================================================

#define OSPI_COMCFG_OEASTEX_Pos      16                               // OE assert extension position
#define OSPI_COMCFG_OEASTEX_Msk      (1U << OSPI_COMCFG_OEASTEX_Pos)  // OE assert extension mask
#define OSPI_COMCFG_OENEGEX_Pos      17                               // OE negate extension position
#define OSPI_COMCFG_OENEGEX_Msk      (1U << OSPI_COMCFG_OENEGEX_Pos)  // OE negate extension mask

// =============================================================================
// OSPI_BMCFGCHn - Bridge Map Configuration Register chn (n=0,1)
// =============================================================================

#define OSPI_BMCFGCH_WRMD_Pos        0                                    // Write mode position
#define OSPI_BMCFGCH_WRMD_Msk        (1U << OSPI_BMCFGCH_WRMD_Pos)        // Write mode mask
#define OSPI_BMCFGCH_MWRCOMB_Pos     7                                    // Multi-write combine position
#define OSPI_BMCFGCH_MWRCOMB_Msk     (1U << OSPI_BMCFGCH_MWRCOMB_Pos)     // Multi-write combine mask
#define OSPI_BMCFGCH_MWRSIZE_Pos     8                                    // Multi-write size position
#define OSPI_BMCFGCH_MWRSIZE_Msk     (0xFFU << OSPI_BMCFGCH_MWRSIZE_Pos)  // Multi-write size mask
#define OSPI_BMCFGCH_PREEN_Pos       16                                   // Prefetch enable position
#define OSPI_BMCFGCH_PREEN_Msk       (1U << OSPI_BMCFGCH_PREEN_Pos)       // Prefetch enable mask
#define OSPI_BMCFGCH_CMBTIM_Pos      24                                   // Combine timer position
#define OSPI_BMCFGCH_CMBTIM_Msk      (0xFFU << OSPI_BMCFGCH_CMBTIM_Pos)   // Combine timer mask

// =============================================================================
// OSPI_CMCFG0CSn - Command Map Configuration Register 0 CSn (n=0,1)
// =============================================================================

#define OSPI_CMCFG0CSN_FFMT_Pos      0                                      // Flash format position
#define OSPI_CMCFG0CSN_FFMT_Msk      (0x3U << OSPI_CMCFG0CSN_FFMT_Pos)      // Flash format mask
#define OSPI_CMCFG0CSN_ADDSIZE_Pos   2                                      // Address size position
#define OSPI_CMCFG0CSN_ADDSIZE_Msk   (0x3U << OSPI_CMCFG0CSN_ADDSIZE_Pos)   // Address size mask
#define OSPI_CMCFG0CSN_WPBSTMD_Pos   4                                      // Write protect burst mode position
#define OSPI_CMCFG0CSN_WPBSTMD_Msk   (1U << OSPI_CMCFG0CSN_WPBSTMD_Pos)     // Write protect burst mode mask
#define OSPI_CMCFG0CSN_ARYAMD_Pos    5                                      // Array mode position
#define OSPI_CMCFG0CSN_ARYAMD_Msk    (1U << OSPI_CMCFG0CSN_ARYAMD_Pos)      // Array mode mask
#define OSPI_CMCFG0CSN_ADDRPEN_Pos   16                                     // Address phase enable position
#define OSPI_CMCFG0CSN_ADDRPEN_Msk   (0xFFU << OSPI_CMCFG0CSN_ADDRPEN_Pos)  // Address phase enable mask
#define OSPI_CMCFG0CSN_ADDRPCD_Pos   24                                     // Address phase code position
#define OSPI_CMCFG0CSN_ADDRPCD_Msk   (0xFFU << OSPI_CMCFG0CSN_ADDRPCD_Pos)  // Address phase code mask

// =============================================================================
// OSPI_CMCFG1CSn - Command Map Configuration Register 1 CSn (n=0,1)
// =============================================================================

#define OSPI_CMCFG1CSN_RDCMD_Pos     0                                      // Read command position
#define OSPI_CMCFG1CSN_RDCMD_Msk     (0xFFFFU << OSPI_CMCFG1CSN_RDCMD_Pos)  // Read command mask
#define OSPI_CMCFG1CSN_RDLATE_Pos    16                                     // Read latency position
#define OSPI_CMCFG1CSN_RDLATE_Msk    (0x1FU << OSPI_CMCFG1CSN_RDLATE_Pos)   // Read latency mask

// =============================================================================
// OSPI_CMCFG2CSn - Command Map Configuration Register 2 CSn (n=0,1)
// =============================================================================

#define OSPI_CMCFG2CSN_WRCMD_Pos     0                                      // Write command position
#define OSPI_CMCFG2CSN_WRCMD_Msk     (0xFFFFU << OSPI_CMCFG2CSN_WRCMD_Pos)  // Write command mask
#define OSPI_CMCFG2CSN_WRLATE_Pos    16                                     // Write latency position
#define OSPI_CMCFG2CSN_WRLATE_Msk    (0x1FU << OSPI_CMCFG2CSN_WRLATE_Pos)   // Write latency mask

// =============================================================================
// OSPI_LIOCFGCSn - Link I/O Configuration Register CSn (n=0,1)
// =============================================================================

#define OSPI_LIOCFGCSN_PRTMD_Pos     0                                       // Protocol mode position
#define OSPI_LIOCFGCSN_PRTMD_Msk     (0x3FFU << OSPI_LIOCFGCSN_PRTMD_Pos)    // Protocol mode mask
#define OSPI_LIOCFGCSN_LATEMD_Pos    10                                      // Latency mode position
#define OSPI_LIOCFGCSN_LATEMD_Msk    (1U << OSPI_LIOCFGCSN_LATEMD_Pos)       // Latency mode mask
#define OSPI_LIOCFGCSN_WRMSKMD_Pos   11                                      // Write mask mode position
#define OSPI_LIOCFGCSN_WRMSKMD_Msk   (1U << OSPI_LIOCFGCSN_WRMSKMD_Pos)      // Write mask mode mask
#define OSPI_LIOCFGCSN_CSMIN_Pos     16                                      // CS minimum period position
#define OSPI_LIOCFGCSN_CSMIN_Msk     (0xFU << OSPI_LIOCFGCSN_CSMIN_Pos)      // CS minimum period mask
#define OSPI_LIOCFGCSN_CSASTEX_Pos   20                                      // CS assert extension position
#define OSPI_LIOCFGCSN_CSASTEX_Pos   20                                      // CS assert extension position
#define OSPI_LIOCFGCSN_CSASTEX_Msk   (1U << OSPI_LIOCFGCSN_CSASTEX_Pos)      // CS assert extension mask
#define OSPI_LIOCFGCSN_CSNEGEX_Pos   21                                      // CS negate extension position
#define OSPI_LIOCFGCSN_CSNEGEX_Msk   (1U << OSPI_LIOCFGCSN_CSNEGEX_Pos)      // CS negate extension mask
#define OSPI_LIOCFGCSN_SDRDRV_Pos    22                                      // SDR drive position
#define OSPI_LIOCFGCSN_SDRDRV_Msk    (1U << OSPI_LIOCFGCSN_SDRDRV_Pos)       // SDR drive mask
#define OSPI_LIOCFGCSN_SDRSMPMD_Pos  23                                      // SDR sample mode position
#define OSPI_LIOCFGCSN_SDRSMPMD_Msk  (1U << OSPI_LIOCFGCSN_SDRSMPMD_Pos)     // SDR sample mode mask
#define OSPI_LIOCFGCSN_SDRSMPSFT_Pos 24                                      // SDR sample shift position
#define OSPI_LIOCFGCSN_SDRSMPSFT_Msk (0xFU << OSPI_LIOCFGCSN_SDRSMPSFT_Pos)  // SDR sample shift mask
#define OSPI_LIOCFGCSN_DDRSMPEX_Pos  28                                      // DDR sample extension position
#define OSPI_LIOCFGCSN_DDRSMPEX_Msk  (0xFU << OSPI_LIOCFGCSN_DDRSMPEX_Pos)   // DDR sample extension mask

// =============================================================================
// OSPI_BMCTL0 - Bridge Map Control Register 0
// =============================================================================

#define OSPI_BMCTL0_CH0CS0ACC_Pos    0                                    // Channel 0 CS0 access position
#define OSPI_BMCTL0_CH0CS0ACC_Msk    (0x3U << OSPI_BMCTL0_CH0CS0ACC_Pos)  // Channel 0 CS0 access mask
#define OSPI_BMCTL0_CH0CS1ACC_Pos    2                                    // Channel 0 CS1 access position
#define OSPI_BMCTL0_CH0CS1ACC_Msk    (0x3U << OSPI_BMCTL0_CH0CS1ACC_Pos)  // Channel 0 CS1 access mask

// =============================================================================
// OSPI_BMCTL1 - Bridge Map Control Register 1
// =============================================================================

#define OSPI_BMCTL1_MWRPUSHCH0_Pos   8                                   // Multi-write push channel 0 position
#define OSPI_BMCTL1_MWRPUSHCH0_Msk   (1U << OSPI_BMCTL1_MWRPUSHCH0_Pos)  // Multi-write push channel 0 mask
#define OSPI_BMCTL1_PBUFCLRCH0_Pos   10                                  // Prefetch buffer clear channel 0 position
#define OSPI_BMCTL1_PBUFCLRCH0_Msk   (1U << OSPI_BMCTL1_PBUFCLRCH0_Pos)  // Prefetch buffer clear channel 0 mask

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
