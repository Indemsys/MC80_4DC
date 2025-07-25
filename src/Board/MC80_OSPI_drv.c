/*-----------------------------------------------------------------------------------------------------
  MC80 OSPI driver implementation - refactored from Renesas FSP r_ospi_b driver
-----------------------------------------------------------------------------------------------------*/

#include "App.h"
#include "MC80_OSPI_drv.h"
#include "RA8M1_OSPI.h"  // For register definitions only
#include "Time_utils.h"  // For MS_TO_TICKS and TICKS_TO_MS macros

/*-----------------------------------------------------------------------------------------------------
  OSPI Wait Loop Debug System - can be disabled with MC80_OSPI_DEBUG_WAIT_LOOPS = 0
-----------------------------------------------------------------------------------------------------*/
#define MC80_OSPI_DEBUG_WAIT_LOOPS (0)  // Set to 0 to disable all debug measurements

/*-----------------------------------------------------------------------------------------------------
  OSPI Erase Debug System - can be disabled with MC80_OSPI_DEBUG_ERASE = 0
-----------------------------------------------------------------------------------------------------*/
#define MC80_OSPI_DEBUG_ERASE (1)  // Set to 0 to disable erase debug output

#if MC80_OSPI_DEBUG_ERASE
  #include "RTT_utils.h"
  #define OSPI_ERASE_DEBUG_PRINT(format, ...) RTT_printf(0, "[OSPI_ERASE] " format "\r\n", ##__VA_ARGS__)
#else
  #define OSPI_ERASE_DEBUG_PRINT(format, ...) ((void)0)
#endif

#if MC80_OSPI_DEBUG_WAIT_LOOPS

  // Maximum precision timing using DWT (Data Watchpoint and Trace) cycle counter
  #define OSPI_DEBUG_TIMER_START()             uint32_t start_cycles = DWT->CYCCNT
  #define OSPI_DEBUG_TIMER_STOP()              uint32_t end_cycles = DWT->CYCCNT
  #define OSPI_DEBUG_CALC_DURATION(start, end) ((end) >= (start) ? ((end) - (start)) : (UINT32_MAX - (start) + (end) + 1))

// Debug loop types enumeration
typedef enum
{
  OSPI_DEBUG_LOOP_DIRECT_TRANSFER_COMPLETION_WAIT = 0,  // Transaction completion wait in direct transfer
  OSPI_DEBUG_LOOP_AUTO_CALIBRATION_WAIT,                // Auto-calibration completion wait
  OSPI_DEBUG_LOOP_XIP_ACCESS_COMPLETION_WAIT,           // Access completion wait in XIP function
  OSPI_DEBUG_LOOP_XIP_ENTER_READ_WAIT,                  // XIP enter read completion wait
  OSPI_DEBUG_LOOP_XIP_EXIT_READ_WAIT,                   // XIP exit read completion wait
  OSPI_DEBUG_LOOP_COUNT                                 // Total number of debug loop types
} T_ospi_debug_loop_type;

// Individual loop measurement data
typedef struct
{
  uint32_t total_calls;      // Total number of times this loop was executed
  uint32_t total_cycles;     // Total CPU cycles spent in this loop
  uint32_t min_cycles;       // Minimum cycles for single loop execution
  uint32_t max_cycles;       // Maximum cycles for single loop execution
  uint32_t last_cycles;      // Cycles from most recent loop execution
  uint32_t avg_cycles;       // Average cycles per loop execution (calculated)
  uint32_t last_iterations;  // Number of iterations in most recent loop execution
} T_ospi_debug_loop_data;

// Complete debug structure - visible in debugger live view
typedef struct
{
  uint32_t               system_clock_hz;                    // System clock frequency for time calculations
  uint32_t               measurement_enabled;                // 1 if measurements active, 0 if disabled
  uint32_t               total_measurements;                 // Total number of loop measurements taken
  T_ospi_debug_loop_data loops[OSPI_DEBUG_LOOP_COUNT];       // Individual loop statistics
  const char            *loop_names[OSPI_DEBUG_LOOP_COUNT];  // Human-readable loop names for debugger
} T_ospi_debug_structure;

// Global debug structure - can be observed in debugger live view
static T_ospi_debug_structure g_ospi_debug = {
  .system_clock_hz     = 240000000UL,  // 240 MHz system clock
  .measurement_enabled = 1,
  .total_measurements  = 0,
  .loops               = { { 0 } },    // Initialize all loop data to zero
  .loop_names          = {
   "DIRECT_TRANSFER_COMPLETION_WAIT",
   "AUTO_CALIBRATION_WAIT",
   "XIP_ACCESS_COMPLETION_WAIT",
   "XIP_ENTER_READ_WAIT",
   "XIP_EXIT_READ_WAIT" }
};

  // Macros for loop measurement - automatically disabled if MC80_OSPI_DEBUG_WAIT_LOOPS = 0
  #define OSPI_DEBUG_LOOP_START(loop_type)  \
    do                                      \
    {                                       \
      if (g_ospi_debug.measurement_enabled) \
      {                                     \
        OSPI_DEBUG_TIMER_START();           \
        uint32_t loop_iterations = 0;

  #define OSPI_DEBUG_LOOP_ITERATION() loop_iterations++

  #define OSPI_DEBUG_LOOP_END(loop_type)                                                          \
    OSPI_DEBUG_TIMER_STOP();                                                                      \
    uint32_t                duration = OSPI_DEBUG_CALC_DURATION(start_cycles, end_cycles);        \
    T_ospi_debug_loop_data *p_loop   = &g_ospi_debug.loops[loop_type];                            \
    p_loop->total_calls++;                                                                        \
    p_loop->total_cycles += duration;                                                             \
    p_loop->last_cycles     = duration;                                                           \
    p_loop->last_iterations = loop_iterations;                                                    \
    if (p_loop->total_calls == 1 || duration < p_loop->min_cycles) p_loop->min_cycles = duration; \
    if (duration > p_loop->max_cycles) p_loop->max_cycles = duration;                             \
    p_loop->avg_cycles = p_loop->total_cycles / p_loop->total_calls;                              \
    g_ospi_debug.total_measurements++;                                                            \
    }                                                                                             \
    }                                                                                             \
    while (0)

#else
  // Debug disabled - all macros become empty
  #define OSPI_DEBUG_LOOP_START(loop_type) \
    do                                     \
    {
  #define OSPI_DEBUG_LOOP_ITERATION() /* empty */
  #define OSPI_DEBUG_LOOP_END(loop_type) \
    }                                    \
    while (0)
#endif

/*-----------------------------------------------------------------------------------------------------
  Static variables for DMA callback support
-----------------------------------------------------------------------------------------------------*/
// RTOS event flags for DMA notifications and OSPI command completion
static TX_EVENT_FLAGS_GROUP g_ospi_dma_event_flags;
static bool                 g_ospi_dma_event_flags_initialized = false;

// DMA event flag definitions
#define OSPI_DMA_EVENT_TRANSFER_COMPLETE                 (0x00000001UL)
#define OSPI_DMA_EVENT_TRANSFER_ERROR                    (0x00000002UL)
#define OSPI_DMA_EVENT_ALL_EVENTS                        (OSPI_DMA_EVENT_TRANSFER_COMPLETE | OSPI_DMA_EVENT_TRANSFER_ERROR)

// Periodic polling event flag definitions (using same event group as DMA)
#define OSPI_CMDCMP_EVENT_COMPLETE                       (0x00000004UL)
#define OSPI_CMDCMP_EVENT_ERROR                          (0x00000008UL)
#define OSPI_CMDCMP_EVENT_ALL_EVENTS                     (OSPI_CMDCMP_EVENT_COMPLETE | OSPI_CMDCMP_EVENT_ERROR)

// DMA completion timeout (1 second = 1000 ms in ThreadX ticks, assuming 1ms tick period)
#define OSPI_DMA_COMPLETION_TIMEOUT_MS                   (1000UL)

// Periodic polling timeout
#define OSPI_CMDCMP_COMPLETION_TIMEOUT_MS                (3500UL)

// Erase operation timeout (10 seconds for complex erase operations)
#define OSPI_ERASE_COMPLETION_TIMEOUT_MS                 (10000UL)

/*-----------------------------------------------------------------------------------------------------
  Macro definitions
-----------------------------------------------------------------------------------------------------*/

// "MC80" in ASCII. Used to determine if the control block is open
#define MC80_OSPI_PRV_OPEN                               (0x4D433830U)  // "MC80"

#define MC80_OSPI_PRV_CHANNELS_PER_UNIT                  (2U)
#define MC80_OSPI_PRV_UNIT_CHANNELS_SHIFT                (MC80_OSPI_PRV_CHANNELS_PER_UNIT)
#define MC80_OSPI_PRV_UNIT_CHANNELS_MASK                 ((1U << MC80_OSPI_PRV_UNIT_CHANNELS_SHIFT) - 1U)

// Mask of all channels for a given OSPI unit
#define MC80_OSPI_PRV_UNIT_MASK(p_ext_cfg)               (MC80_OSPI_PRV_UNIT_CHANNELS_MASK << (((p_ext_cfg)->ospi_unit) * MC80_OSPI_PRV_UNIT_CHANNELS_SHIFT))

// Individual bit mask for a single channel on a given OSPI unit
#define MC80_OSPI_PRV_CH_MASK(p_ext_cfg)                 ((1U << ((p_ext_cfg)->channel)) << (((p_ext_cfg)->ospi_unit) * MC80_OSPI_PRV_UNIT_CHANNELS_SHIFT))

// Indicates the provided protocol mode requires the Data-Strobe signal
#define MC80_OSPI_PRV_PROTOCOL_USES_DS_SIGNAL(protocol)  ((bool)(((uint32_t)(protocol)) & 0x200UL))

// Number of bytes combined into a single transaction for memory-mapped writes
#define MC80_OSPI_PRV_COMBINATION_WRITE_LENGTH           (2U * ((uint8_t)MC80_OSPI_CFG_COMBINATION_FUNCTION + 1U))

#define MC80_OSPI_PRV_BMCTL_DEFAULT_VALUE                (0x0C)

#define MC80_OSPI_PRV_CMCFG_1BYTE_VALUE_MASK             (0xFF00U)
#define MC80_OSPI_PRV_CMCFG_2BYTE_VALUE_MASK             (0xFFFFU)

#define MC80_OSPI_PRV_AUTOCALIBRATION_DATA_SIZE          (0xFU)
#define MC80_OSPI_PRV_AUTOCALIBRATION_LATENCY_CYCLES     (0U)

#define MC80_OSPI_PRV_ADDRESS_REPLACE_VALUE              (0xF0U)
#define MC80_OSPI_PRV_ADDRESS_REPLACE_ENABLE_BITS        (MC80_OSPI_PRV_ADDRESS_REPLACE_VALUE << OSPI_CMCFG0CSN_ADDRPEN_Pos)
#define MC80_OSPI_PRV_ADDRESS_REPLACE_MASK               (~(MC80_OSPI_PRV_ADDRESS_REPLACE_VALUE << 24))

#define MC80_OSPI_PRV_WORD_ACCESS_SIZE                   (4U)
#define MC80_OSPI_PRV_HALF_WORD_ACCESS_SIZE              (2U)

#define MC80_OSPI_PRV_DIRECT_ADDR_AND_DATA_MASK          (7U)
#define MC80_OSPI_PRV_PAGE_SIZE_BYTES                    (256U)

#define MC80_OSPI_PRV_DIRECT_CMD_SIZE_MASK               (0x3U)

#define MC80_OSPI_PRV_CDTBUF_CMD_OFFSET                  (16U)
#define MC80_OSPI_PRV_CDTBUF_CMD_UPPER_OFFSET            (24U)
#define MC80_OSPI_PRV_CDTBUF_CMD_1B_VALUE_MASK           (0xFFU)
#define MC80_OSPI_PRV_CDTBUF_CMD_1B_VALUE_SHIFT          (8U)
#define MC80_OSPI_PRV_CDTBUF_CMD_2B_VALUE_MASK           (0xFFFFU)

// BMCTL0 register values - bits 4-7 must always be set to 1 per documentation
#define MC80_OSPI_PRV_BMCTL0_RESERVED_BITS               (0xF0)  // Bits 4-7 must be 1
#define MC80_OSPI_PRV_BMCTL0_DISABLED_VALUE              (0xF0)  // 0b1111'0000 - Reserved bits + disabled
#define MC80_OSPI_PRV_BMCTL0_READ_ONLY_VALUE             (0xF5)  // 0b1111'0101 - Reserved bits + read-only
#define MC80_OSPI_PRV_BMCTL0_WRITE_ONLY_VALUE            (0xFA)  // 0b1111'1010 - Reserved bits + write-only
#define MC80_OSPI_PRV_BMCTL0_READ_WRITE_VALUE            (0xFF)  // 0b1111'1111 - Reserved bits + read/write

#define MC80_OSPI_PRV_BMCTL1_CLEAR_PREFETCH_MASK         (0x03 << OSPI_BMCTL1_PBUFCLRCH0_Pos)
#define MC80_OSPI_PRV_BMCTL1_PUSH_COMBINATION_WRITE_MASK (0x03 << OSPI_BMCTL1_MWRPUSHCH0_Pos)

#define MC80_OSPI_PRV_COMSTT_MEMACCCH_MASK               (0x03 << OSPI_COMSTT_MEMACCCH0_Pos)

#define MC80_OSPI_SOFTWARE_DELAY                         (50U)

// These are used as modulus checking, make sure they are powers of 2
#define MC80_OSPI_PRV_CPU_ACCESS_LENGTH                  (8U)
#define MC80_OSPI_PRV_CPU_ACCESS_ALIGNMENT               (8U)

#define MC80_OSPI_PRV_PROTOCOL_USES_DS_MASK              (0x200U)

#define MC80_OSPI_PRV_UINT32_BITS                        (32)

#define MC80_OSPI_MAX_WRITE_ENABLE_LOOPS                 (5)

// Number of address bytes in 4 byte address mode
#define MC80_OSPI_4_BYTE_ADDRESS                         (4U)

// Size of 64-byte block for optimized memory-mapped write operations
// This value is coordinated with MC80_OSPI_CFG_COMBINATION_FUNCTION (64-byte)
// for optimal hardware performance and combination write functionality
#define MC80_OSPI_BLOCK_WRITE_SIZE                       (64U)

// Size of block for optimized memory-mapped read operations
// Using large blocks for maximum DMA performance while staying within uint16_t limit
#define MC80_OSPI_BLOCK_READ_SIZE                        (32768U)  // 32KB blocks for optimal performance

// Timeout for waiting device ready status before write operation (1 second in ThreadX ticks)
#define MC80_OSPI_DEVICE_READY_TIMEOUT_MS                (1000UL)

// Periodic polling configuration for status monitoring
// OSPI timer: (CPU_CLK/BSP_CFG_OCTA_DIV)/2 = (240MHz/2)/2 = 60MHz, formula: 2^(PERITV+1) cycles
#define MC80_OSPI_PERIODIC_INTERVAL                      (0x06U)  // PERITV = 6: 2^7 = 128 cycles = 2.13µs @ 60MHz
#define MC80_OSPI_PERIODIC_REPETITIONS_MAX               (0x0FU)  // PERREP = 15: 32768 repetitions (2^15), total time = 2.13µs × 32768 = 69.8ms
#define MC80_OSPI_PERIODIC_MODE_ENABLE                   (0x01U)  // PERMD = 1: Enable periodic mode

// Timeout for periodic polling completion (slightly more than hardware polling time: 69.8ms + margin = 100ms)
#define OSPI_PERIODIC_POLLING_TIMEOUT_MS                 (100UL)

// Common periodic polling values for flash status monitoring
#define MC80_OSPI_WAIT_WIP_CLEAR_EXPECTED                (0x00000000U)  // WIP bit = 0 (device ready)
#define MC80_OSPI_WAIT_WIP_CLEAR_MASK                    (0xFFFFFFFEU)  // Compare only WIP bit (bit 0), ignore all others
#define MC80_OSPI_WAIT_WEL_SET_EXPECTED                  (0x00000002U)  // WEL bit = 1 (write enabled)
#define MC80_OSPI_WAIT_WEL_SET_MASK                      (0xFFFFFFFDU)  // Compare only WEL bit (bit 1), ignore all others

/*-----------------------------------------------------------------------------------------------------
  Static function prototypes
-----------------------------------------------------------------------------------------------------*/
static bool                                _Mc80_ospi_status_sub(T_mc80_ospi_instance_ctrl *p_ctrl, uint8_t bit_pos);
static fsp_err_t                           _Mc80_ospi_protocol_specific_settings(T_mc80_ospi_instance_ctrl *p_ctrl);
static fsp_err_t                           _Mc80_ospi_memory_mapped_write_enable(T_mc80_ospi_instance_ctrl *p_ctrl);
static void                                _Mc80_ospi_direct_transfer(T_mc80_ospi_instance_ctrl *p_ctrl, T_mc80_ospi_direct_transfer *const p_transfer, T_mc80_ospi_direct_transfer_dir direction);
static T_mc80_ospi_xspi_command_set const *_Mc80_ospi_command_set_get(T_mc80_ospi_instance_ctrl *p_ctrl);
static void                                _Mc80_ospi_xip(T_mc80_ospi_instance_ctrl *p_ctrl, bool is_entering);
void                                       OSPI_dma_callback(dmac_callback_args_t *p_args);
static fsp_err_t                           _Ospi_dma_event_flags_initialize(void);
static void                                _Ospi_dma_event_flags_cleanup(void);
static fsp_err_t                           _Mc80_ospi_periodic_status_start(T_mc80_ospi_instance_ctrl *p_ctrl, uint32_t expected_value, uint32_t comparison_mask);
static void                                _Mc80_ospi_periodic_status_stop(T_mc80_ospi_instance_ctrl *p_ctrl);
void                                       ospi_cmdcmp_isr(void);

/*-----------------------------------------------------------------------------------------------------
  Private global variables
-----------------------------------------------------------------------------------------------------*/

// Bit-flags specifying which channels are open so the module can be stopped when all are closed
static uint32_t g_mc80_ospi_channels_open_flags = 0;

/*-----------------------------------------------------------------------------------------------------
  OSPI command completion interrupt service routine for periodic status polling.

  This ISR is called when the OSPI peripheral completes a periodic status command.
  It sets the appropriate event flag to wake up tasks waiting for write completion.

  Parameters: None

  Return: None
-----------------------------------------------------------------------------------------------------*/
void ospi_cmdcmp_isr(void)
{
  // Since we use only OSPI0 in this project, directly access R_XSPI0
  R_XSPI0_Type *const p_reg = R_XSPI0;

  // Read interrupt status to check what kind of interrupt occurred
  uint32_t ints_status      = p_reg->INTS;

  // Check if CMDCMP interrupt occurred
  if (ints_status & OSPI_INTS_CMDCMP_Msk)
  {
    // Clear the command completion interrupt flag
    p_reg->INTC = OSPI_INTC_CMDCMPC_Msk;

    // Set command completion event flag if RTOS is initialized
    if (g_ospi_dma_event_flags_initialized)
    {
      tx_event_flags_set(&g_ospi_dma_event_flags, OSPI_CMDCMP_EVENT_COMPLETE, TX_OR);
    }
  }

  // Check for any errors (optional - can help with debugging)
  if (ints_status & (OSPI_INTS_CAFAILCS0_Msk | OSPI_INTS_CAFAILCS1_Msk))
  {
    // Clear error flags for both channels
    p_reg->INTC = OSPI_INTC_CAFAILCS0C_Msk | OSPI_INTC_CAFAILCS1C_Msk;

    // Set error event flag if RTOS is initialized
    if (g_ospi_dma_event_flags_initialized)
    {
      tx_event_flags_set(&g_ospi_dma_event_flags, OSPI_CMDCMP_EVENT_ERROR, TX_OR);
    }
  }

  // Clear interrupt flag in ICU (essential for proper interrupt handling)
  R_ICU->IELSR_b[OSPI_CMDCMP_IRQn].IR = 0;
}

/*-----------------------------------------------------------------------------------------------------
  Open the xSPI device. After the driver is open, the xSPI device can be accessed like internal flash memory.

  Parameters:
    p_ctrl - Pointer to the control structure
    p_cfg  - Pointer to the configuration structure

  Return:
    FSP_SUCCESS              - Configuration was successful
    FSP_ERR_ASSERTION        - The parameter p_ctrl or p_cfg is NULL
    FSP_ERR_ALREADY_OPEN     - Driver has already been opened with the same p_ctrl
    FSP_ERR_CALIBRATE_FAILED - Failed to perform auto-calibrate
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_open(T_mc80_ospi_instance_ctrl *const p_ctrl, T_mc80_ospi_cfg const *const p_cfg)
{
  fsp_err_t                             ret          = FSP_SUCCESS;
  const T_mc80_ospi_extended_cfg *const p_cfg_extend = p_cfg->p_extend;

  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl || NULL == p_cfg || NULL == p_cfg->p_extend)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN == p_ctrl->open)
    {
      return FSP_ERR_ALREADY_OPEN;
    }
  }

  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (!(MC80_OSPI_PERIPHERAL_CHANNEL_MASK & (1U << p_cfg_extend->ospi_unit)) ||
        (0 != (g_mc80_ospi_channels_open_flags & MC80_OSPI_PRV_CH_MASK(p_cfg_extend))))
    {
      if (!(MC80_OSPI_PERIPHERAL_CHANNEL_MASK & (1U << p_cfg_extend->ospi_unit)))
      {
        return FSP_ERR_ASSERTION;
      }
      else
      {
        return FSP_ERR_ALREADY_OPEN;
      }
    }
  }

  R_XSPI0_Type *p_reg = (R_XSPI0_Type *)(MC80_OSPI0_BASE_ADDRESS + p_cfg_extend->ospi_unit * MC80_OSPI_UNIT_ADDRESS_OFFSET);

  // Enable clock to the xSPI block
  R_BSP_MODULE_START(FSP_IP_OSPI, p_cfg_extend->ospi_unit);

  // Initialize control block
  p_ctrl->p_cfg                         = p_cfg;
  p_ctrl->p_reg                         = p_reg;
  p_ctrl->spi_protocol                  = p_cfg->spi_protocol;
  p_ctrl->channel                       = p_cfg_extend->channel;
  p_ctrl->ospi_unit                     = p_cfg_extend->ospi_unit;

  // Initialize transfer instance
  transfer_instance_t const *p_transfer = p_cfg_extend->p_lower_lvl_transfer;

  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE && NULL == p_transfer)
  {
    return FSP_ERR_ASSERTION;
  }

  p_transfer->p_api->open(p_transfer->p_ctrl, p_transfer->p_cfg);

  // Disable memory-mapping for this slave. It will be enabled later on after initialization
  // Note: Preserve bits 4-7 which must always be 1
  if (MC80_OSPI_DEVICE_NUMBER_0 == p_ctrl->channel)
  {
    p_reg->BMCTL0 = (p_reg->BMCTL0 & ~OSPI_BMCTL0_CH0CS0ACC_Msk) | MC80_OSPI_PRV_BMCTL0_RESERVED_BITS;
  }
  else
  {
    p_reg->BMCTL0 = (p_reg->BMCTL0 & ~OSPI_BMCTL0_CH0CS1ACC_Msk) | MC80_OSPI_PRV_BMCTL0_RESERVED_BITS;
  }

  // Perform xSPI Initial configuration as described in hardware manual
  // Set xSPI protocol mode
  uint32_t liocfg = ((uint32_t)p_cfg->spi_protocol) << OSPI_LIOCFGCSN_PRTMD_Pos;
  // NOTE: Do NOT write to LIOCFGCS yet - we need to add timing settings first

  // Set xSPI drive/sampling timing
  if (MC80_OSPI_DEVICE_NUMBER_0 == p_ctrl->channel)
  {
    p_reg->WRAPCFG = ((uint32_t)p_cfg_extend->data_latch_delay_clocks << OSPI_WRAPCFG_DSSFTCS0_Pos) & OSPI_WRAPCFG_DSSFTCS0_Msk;
  }
  else
  {
    p_reg->WRAPCFG = ((uint32_t)p_cfg_extend->data_latch_delay_clocks << OSPI_WRAPCFG_DSSFTCS1_Pos) & OSPI_WRAPCFG_DSSFTCS1_Msk;
  }

  // Set minimum cycles between xSPI frames
  liocfg |= ((uint32_t)p_cfg_extend->p_timing_settings->command_to_command_interval << OSPI_LIOCFGCSN_CSMIN_Pos) & OSPI_LIOCFGCSN_CSMIN_Msk;

  // Set CS asserting extension in cycles
  liocfg |= ((uint32_t)p_cfg_extend->p_timing_settings->cs_pulldown_lead << OSPI_LIOCFGCSN_CSASTEX_Pos) & OSPI_LIOCFGCSN_CSASTEX_Msk;

  // Set CS releasing extension in cycles
  liocfg |= ((uint32_t)p_cfg_extend->p_timing_settings->cs_pullup_lag << OSPI_LIOCFGCSN_CSNEGEX_Pos) & OSPI_LIOCFGCSN_CSNEGEX_Msk;

  // Set SDR and DDR timing
  liocfg |= ((uint32_t)p_cfg_extend->p_timing_settings->sdr_drive_timing << OSPI_LIOCFGCSN_SDRDRV_Pos) & OSPI_LIOCFGCSN_SDRDRV_Msk;
  liocfg |= ((uint32_t)p_cfg_extend->p_timing_settings->sdr_sampling_edge << OSPI_LIOCFGCSN_SDRSMPMD_Pos) & OSPI_LIOCFGCSN_SDRSMPMD_Msk;
  liocfg |= ((uint32_t)p_cfg_extend->p_timing_settings->sdr_sampling_delay << OSPI_LIOCFGCSN_SDRSMPSFT_Pos) & OSPI_LIOCFGCSN_SDRSMPSFT_Msk;
  liocfg |= ((uint32_t)p_cfg_extend->p_timing_settings->ddr_sampling_extension << OSPI_LIOCFGCSN_DDRSMPEX_Pos) & OSPI_LIOCFGCSN_DDRSMPEX_Msk;

  // Set xSPI CSn signal timings (write complete configuration at once)
  p_reg->LIOCFGCS[p_cfg_extend->channel] = liocfg;

  // Set xSPI memory-mapping operation
  ret                                    = _Mc80_ospi_protocol_specific_settings(p_ctrl);

  // Return response after issuing write transaction to xSPI bus, Enable prefetch function and combination if desired
  const uint32_t bmcfgch                 = (0 << OSPI_BMCFGCH_WRMD_Pos) |
                           ((MC80_OSPI_CFG_COMBINATION_FUNCTION << OSPI_BMCFGCH_MWRCOMB_Pos) &
                            (OSPI_BMCFGCH_MWRCOMB_Msk | OSPI_BMCFGCH_MWRSIZE_Msk)) |
                           ((MC80_OSPI_CFG_PREFETCH_FUNCTION << OSPI_BMCFGCH_PREEN_Pos) &
                            OSPI_BMCFGCH_PREEN_Msk);

  // Both of these should have the same configuration and it affects all OSPI slave channels
  p_reg->BMCFGCH[0] = bmcfgch;
  p_reg->BMCFGCH[1] = bmcfgch;

  // Re-activate memory-mapped mode in Read/Write
  // Note: Preserve bits 4-7 which must always be 1
  if (0 == p_ctrl->channel)
  {
    p_reg->BMCTL0 = (p_reg->BMCTL0 & ~OSPI_BMCTL0_CH0CS0ACC_Msk) | OSPI_BMCTL0_CH0CS0ACC_Msk | MC80_OSPI_PRV_BMCTL0_RESERVED_BITS;
  }
  else
  {
    p_reg->BMCTL0 = (p_reg->BMCTL0 & ~OSPI_BMCTL0_CH0CS1ACC_Msk) | OSPI_BMCTL0_CH0CS1ACC_Msk | MC80_OSPI_PRV_BMCTL0_RESERVED_BITS;
  }

  if (FSP_SUCCESS == ret)
  {
    // Initialize RTOS event flags for DMA synchronization
    _Ospi_dma_event_flags_initialize();

    // Configure OSPI command completion interrupt
    NVIC_SetPriority(OSPI_CMDCMP_IRQn, 5);    // Set interrupt priority (0=highest, 15=lowest)
    R_ICU->IELSR_b[OSPI_CMDCMP_IRQn].IR = 0;  // Clear any pending interrupt flag
    NVIC_ClearPendingIRQ(OSPI_CMDCMP_IRQn);   // Clear NVIC pending interrupt
    NVIC_EnableIRQ(OSPI_CMDCMP_IRQn);         // Enable OSPI command completion interrupt

    p_ctrl->open = MC80_OSPI_PRV_OPEN;
    g_mc80_ospi_channels_open_flags |= MC80_OSPI_PRV_CH_MASK(p_cfg_extend);
  }
  else if (0 == (g_mc80_ospi_channels_open_flags & MC80_OSPI_PRV_UNIT_MASK(p_cfg_extend)))
  {
    // If the open fails and no other channels are open, stop the module
    R_BSP_MODULE_STOP(FSP_IP_OSPI, p_cfg_extend->ospi_unit);
  }

  return ret;
}

/*-----------------------------------------------------------------------------------------------------
  Writes raw data directly to the OctaFlash using direct transfer interface.
  This function uses the current protocol's write command to send data directly to the flash device.
  Large transfers are automatically split into multiple 8-byte chunks with proper address increment.

  Parameters:
    p_ctrl           - Pointer to the control structure
    p_src            - Source data buffer
    address          - Starting address in flash memory
    bytes            - Number of bytes to write (any size)
    read_after_write - Read after write flag (not used in this implementation)

  Return:
    FSP_SUCCESS       - Data was written successfully
    FSP_ERR_ASSERTION - A required pointer is NULL or invalid parameters
    FSP_ERR_NOT_OPEN  - Driver is not opened
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_direct_write(T_mc80_ospi_instance_ctrl *p_ctrl,
                                 uint8_t const *const       p_src,
                                 uint32_t const             address,
                                 uint32_t const             bytes,
                                 bool const                 read_after_write)
{
  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl || NULL == p_src || 0 == bytes)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }
  }

  // Get current command set for the active protocol
  T_mc80_ospi_xspi_command_set const *p_cmd_set = p_ctrl->p_cmd_set;
  if (NULL == p_cmd_set)
  {
    return FSP_ERR_ASSERTION;
  }

  uint32_t bytes_remaining = bytes;
  uint32_t current_address = address;
  uint32_t offset          = 0;

  // Process data in chunks of up to 8 bytes
  while (bytes_remaining > 0)
  {
    uint32_t chunk_size;
    if (bytes_remaining > 8)
    {
      chunk_size = 8;
    }
    else
    {
      chunk_size = bytes_remaining;
    }

    // Prepare direct transfer structure for this chunk
    T_mc80_ospi_direct_transfer direct_transfer = {
      .command        = p_cmd_set->program_command,
      .command_length = (uint8_t)p_cmd_set->command_bytes,
      .address        = current_address,                           // Use current address
      .address_length = (uint8_t)(p_cmd_set->address_bytes + 1U),  // Include address
      .data_length    = (uint8_t)chunk_size,
      .dummy_cycles   = p_cmd_set->program_dummy_cycles,
      .data_u64       = 0,
    };

    // Copy data from source buffer to transfer structure
    for (uint32_t i = 0; i < chunk_size; i++)
    {
      direct_transfer.data_bytes[i] = p_src[offset + i];
    }

    // Execute the direct transfer
    _Mc80_ospi_direct_transfer(p_ctrl, &direct_transfer, MC80_OSPI_DIRECT_TRANSFER_DIR_WRITE);

    // Move to next chunk with incremented address
    offset += chunk_size;
    current_address += chunk_size;
    bytes_remaining -= chunk_size;
  }

  return FSP_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Reads raw data directly from the OctaFlash using direct transfer interface.
  This function uses the current protocol's read command to receive data directly from the flash device.
  Large transfers are automatically split into multiple 8-byte chunks with proper address increment.

  Parameters:
    p_ctrl  - Pointer to the control structure
    p_dest  - Destination data buffer
    address - Starting address in flash memory
    bytes   - Number of bytes to read (any size)

  Return:
    FSP_SUCCESS       - Data was read successfully
    FSP_ERR_ASSERTION - A required pointer is NULL or invalid parameters
    FSP_ERR_NOT_OPEN  - Driver is not opened
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_direct_read(T_mc80_ospi_instance_ctrl *p_ctrl, uint8_t *const p_dest, uint32_t const address, uint32_t const bytes)
{
  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl || NULL == p_dest || 0 == bytes)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }
  }

  // Get current command set for the active protocol
  T_mc80_ospi_xspi_command_set const *p_cmd_set = p_ctrl->p_cmd_set;
  if (NULL == p_cmd_set)
  {
    return FSP_ERR_ASSERTION;
  }

  uint32_t bytes_remaining = bytes;
  uint32_t current_address = address;
  uint32_t offset          = 0;

  // Process data in chunks of up to 8 bytes
  while (bytes_remaining > 0)
  {
    uint32_t chunk_size;
    if (bytes_remaining > 8)
    {
      chunk_size = 8;
    }
    else
    {
      chunk_size = bytes_remaining;
    }

    // Prepare direct transfer structure for this chunk
    T_mc80_ospi_direct_transfer direct_transfer = {
      .command        = p_cmd_set->read_command,
      .command_length = (uint8_t)p_cmd_set->command_bytes,
      .address        = current_address,                           // Use current address
      .address_length = (uint8_t)(p_cmd_set->address_bytes + 1U),  // Include address
      .data_length    = (uint8_t)chunk_size,
      .dummy_cycles   = p_cmd_set->read_dummy_cycles,
      .data_u64       = 0,
    };

    // Execute the direct transfer
    _Mc80_ospi_direct_transfer(p_ctrl, &direct_transfer, MC80_OSPI_DIRECT_TRANSFER_DIR_READ);

    // Copy data from transfer structure to destination buffer
    for (uint32_t i = 0; i < chunk_size; i++)
    {
      p_dest[offset + i] = direct_transfer.data_bytes[i];
    }

    // Move to next chunk with incremented address
    offset += chunk_size;
    current_address += chunk_size;
    bytes_remaining -= chunk_size;
  }

  return FSP_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Read/Write raw data directly with the OctaFlash.

  Parameters:
    p_ctrl     - Pointer to the control structure
    p_transfer - Pointer to transfer structure
    direction  - Transfer direction

  Return:
    FSP_SUCCESS       - The flash was programmed successfully
    FSP_ERR_ASSERTION - A required pointer is NULL
    FSP_ERR_NOT_OPEN  - Driver is not opened
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_direct_transfer(T_mc80_ospi_instance_ctrl *p_ctrl, T_mc80_ospi_direct_transfer *const p_transfer, T_mc80_ospi_direct_transfer_dir direction)
{
  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl || NULL == p_transfer || 0 == p_transfer->command_length)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }
  }

  _Mc80_ospi_direct_transfer(p_ctrl, p_transfer, direction);

  return FSP_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Enters XIP (execute in place) mode.

  NOTE: For the MX25UM25645G flash memory chip used in this project, XIP mode manipulation
  is not required. The chip can operate in continuous memory-mapped mode without explicit
  XIP enter/exit commands. This function is provided for compatibility but may not be
  necessary for normal operation.

  Parameters:
    p_ctrl - Pointer to the control structure

  Return:
    FSP_SUCCESS        - XiP mode was entered successfully
    FSP_ERR_ASSERTION  - A required pointer is NULL
    FSP_ERR_NOT_OPEN   - Driver is not opened
    FSP_ERR_UNSUPPORTED- XiP support is not enabled
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_xip_enter(T_mc80_ospi_instance_ctrl *p_ctrl)
{
  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl || NULL == p_ctrl->p_cfg)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }
  }

  // Enter XIP mode
  _Mc80_ospi_xip(p_ctrl, true);

  return FSP_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Exits XIP (execute in place) mode.

  NOTE: For the MX25UM25645G flash memory chip used in this project, XIP mode manipulation
  is not required. The chip can operate in continuous memory-mapped mode without explicit
  XIP enter/exit commands. This function is provided for compatibility but may not be
  necessary for normal operation.

  Parameters:
    p_ctrl - Pointer to the control structure

  Return:
    FSP_SUCCESS        - XiP mode was exited successfully
    FSP_ERR_ASSERTION  - A required pointer is NULL
    FSP_ERR_NOT_OPEN   - Driver is not opened
    FSP_ERR_UNSUPPORTED- XiP support is not enabled
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_xip_exit(T_mc80_ospi_instance_ctrl *p_ctrl)
{
  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl || NULL == p_ctrl->p_cfg)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }
  }

  // Exit XIP mode
  _Mc80_ospi_xip(p_ctrl, false);

  return FSP_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Description: High-performance memory-mapped read from flash using DMA transfer with block optimization

  This function performs optimized memory-mapped reading from OSPI flash memory using DMA
  for maximum performance. The function processes data in blocks to handle any size transfer
  while respecting DMA limitations.

  Block-Based Read Operation:
  - Data is processed in optimized blocks (32KB) for maximum DMA performance
  - Block size (MC80_OSPI_BLOCK_READ_SIZE) is chosen to maximize throughput while staying within DMA limits
  - DMA length field is uint16_t (max 65535), so blocks are limited to 32KB for safety margin
  - Each block is read using a separate DMA transfer for reliable completion
  - Final partial block (if any) is handled separately to ensure complete data transfer
  - Supports any total size from 1 byte to multiple megabytes
  - Hardware prefetch function optimizes sequential read operations automatically

  DMA Limitation Handling:
  - Solves critical issue where DMA length field (uint16_t) was truncated for large transfers
  - Previous implementation: bytes > 65535 would be silently truncated to bytes & 0xFFFF
  - New implementation: safely handles transfers of any size up to 4GB through block processing
  - Each block transfer is verified for completion before proceeding to next block

  Operation sequence:
  1. Validates input parameters (pointers, size limits)
  2. Calculates memory-mapped address based on channel and input address
  3. Processes data in blocks of up to 32KB each
  4. For each block:
     a. Configures DMA for memory-to-memory transfer from flash to destination buffer
     b. Clears DMA completion event flags
     c. Starts DMA transfer asynchronously
     d. Waits for DMA completion using RTOS event flags (callback-driven)
     e. Verifies transfer success using infoGet API
  5. Returns success when all blocks are verified complete

  Performance Benefits:
  - DMA handles data transfer without CPU intervention for each block
  - Maximum throughput for large data transfers through optimal block sizing
  - CPU can perform other tasks during transfer
  - RTOS-based asynchronous completion notification
  - Hardware-optimized memory access patterns
  - Eliminates data corruption from DMA length truncation

  Parameters: p_ctrl - Pointer to OSPI instance control structure
              p_dest - Destination buffer for read data
              address - Physical flash address to read from (0x00000000-based)
              bytes - Number of bytes to read (any size up to 4GB)

  Return: FSP_SUCCESS - Data read successfully
          FSP_ERR_ASSERTION - Invalid parameters
          FSP_ERR_NOT_OPEN - Driver not opened
          FSP_ERR_TIMEOUT - DMA completion timeout
          FSP_ERR_TRANSFER_ABORTED - DMA transfer did not complete properly
          FSP_ERR_NOT_INITIALIZED - RTOS event flags not initialized
          Error codes from DMA operations
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_memory_mapped_read(T_mc80_ospi_instance_ctrl *const p_ctrl, uint8_t *const p_dest, uint32_t const address, uint32_t const bytes)
{
  // Parameter validation
  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if ((NULL == p_ctrl) || (NULL == p_dest) || (0 == bytes))
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }
  }

  fsp_err_t err = FSP_SUCCESS;

  // Calculate memory-mapped address based on channel
  uint32_t memory_mapped_address;
  if (p_ctrl->channel == MC80_OSPI_DEVICE_NUMBER_0)
  {
    memory_mapped_address = MC80_OSPI_DEVICE_0_START_ADDRESS + address;
  }
  else
  {
    memory_mapped_address = MC80_OSPI_DEVICE_1_START_ADDRESS + address;
  }

  // Get DMA transfer instance from OSPI configuration
  T_mc80_ospi_extended_cfg const *p_cfg_extend = p_ctrl->p_cfg->p_extend;
  transfer_instance_t const      *p_transfer   = p_cfg_extend->p_lower_lvl_transfer;

  // Process data in blocks to handle any size transfer while respecting DMA limitations
  uint32_t bytes_remaining                     = bytes;
  uint32_t offset                              = 0;

  while (bytes_remaining > 0)
  {
    // Determine current block size (32KB or remaining bytes, whichever is smaller)
    uint32_t current_block_size;
    if (bytes_remaining >= MC80_OSPI_BLOCK_READ_SIZE)
    {
      current_block_size = MC80_OSPI_BLOCK_READ_SIZE;
    }
    else
    {
      current_block_size = bytes_remaining;
    }

    // Configure DMA for current block transfer
    p_transfer->p_cfg->p_info->p_src                         = (void const *)(memory_mapped_address + offset);
    p_transfer->p_cfg->p_info->p_dest                        = (uint8_t *)p_dest + offset;
    p_transfer->p_cfg->p_info->transfer_settings_word_b.size = TRANSFER_SIZE_1_BYTE;
    p_transfer->p_cfg->p_info->transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL;
    p_transfer->p_cfg->p_info->length                        = (uint16_t)current_block_size;  // Safe cast - always <= 32KB

    // Reconfigure DMA with current block settings
    err                                                      = p_transfer->p_api->reconfigure(p_transfer->p_ctrl, p_transfer->p_cfg->p_info);
    if (FSP_SUCCESS != err)
    {
      break;  // Exit on DMA configuration error
    }

    // Clear DMA event flags before starting transfer
    Mc80_ospi_dma_transfer_reset_flags();

    // Start DMA transfer for current block
    err = p_transfer->p_api->softwareStart(p_transfer->p_ctrl, TRANSFER_START_MODE_REPEAT);
    if (FSP_SUCCESS != err)
    {
      break;  // Exit on DMA start error
    }

    // Wait for current block DMA completion using RTOS event flags (asynchronous)
    err = Mc80_ospi_dma_wait_for_completion(MS_TO_TICKS(OSPI_DMA_COMPLETION_TIMEOUT_MS));
    if (FSP_SUCCESS != err)
    {
      break;  // DMA transfer failed or timed out
    }

    // Verify current block transfer completion status using infoGet
    transfer_properties_t transfer_properties = { 0U };
    err                                       = p_transfer->p_api->infoGet(p_transfer->p_ctrl, &transfer_properties);
    if (FSP_SUCCESS != err)
    {
      break;  // Exit on transfer status error
    }

    // Check if current block transfer completed successfully (remaining length should be 0)
    if (transfer_properties.transfer_length_remaining > 0)
    {
      err = FSP_ERR_TRANSFER_ABORTED;  // Transfer did not complete properly
      break;
    }

    // Move to next block
    offset += current_block_size;
    bytes_remaining -= current_block_size;
  }

  return err;
}

/*-----------------------------------------------------------------------------------------------------
  Universal flash programming function with automatic alignment and size handling.

  This function performs high-performance flash programming using the OSPI memory-mapped mode
  combined with DMA (Direct Memory Access) for efficient data transfer. The function automatically
  handles alignment and accepts any size data with any address offset.

  Universal Write Operation Features:
  - Accepts any destination address (automatic 64-byte alignment handling)
  - Accepts any data size (no alignment restrictions)
  - Automatically creates temporary 64-byte buffer for unaligned addresses
  - Fills non-target bytes with 0xFF (erased flash state) in alignment buffer
  - All data is written in optimized 64-byte blocks for maximum hardware performance
  - Each block is written using a separate DMA transfer with write enable sequence

  64-byte Alignment Algorithm:
  1. For unaligned addresses or partial blocks:
     - Use temporary buffer aligned to 64-byte boundary
     - Fill buffer with 0xFF (erased state) and copy user data to correct offset
     - Write full 64-byte aligned block to flash
  2. For aligned full blocks:
     - Write directly from source buffer to flash without temporary buffer
  3. Continue until all data is written with proper address progression

  Example: Write 100 bytes starting at address 0x800000E0 (32 bytes into 64-byte block)
  - Block 1: temp_buffer[0-31]=0xFF, temp_buffer[32-63]=data[0-31] → write 64 bytes at 0x800000C0
  - Block 2: temp_buffer[0-35]=data[32-67], temp_buffer[36-63]=0xFF → write 64 bytes at 0x80000100
  - Block 3: temp_buffer[0-31]=data[68-99], temp_buffer[32-63]=0xFF → write 64 bytes at 0x80000140
  Protocol and Operation Method:
  - Uses memory-mapped write mode where flash appears as regular system memory
  - Flash device is accessed via memory-mapped addresses (0x80000000 for Device 0, 0x90000000 for Device 1)
  - Automatically uses the current active protocol's write commands configured in hardware registers
  - Protocol commands are pre-configured in CMCFG2 register during driver initialization
  - Supports both Standard SPI (1S-1S-1S) and Octal DDR (8D-8D-8D) protocols transparently

  DMA Transfer Mechanism:
  1. Configures DMAC (Direct Memory Access Controller) for bufferable write operations
  2. Sets up block-mode transfer from source buffer to memory-mapped flash address
  3. Enables OSPI DMA Bufferable Write (DMBWR) for optimal performance
  4. Processes data in 64-byte blocks with separate DMA transfers for each block
  5. Clears DMA completion event flags before starting each transfer
  6. Starts DMA transfer in repeat mode for continuous data streaming
  7. Waits for DMA completion using RTOS event flags (asynchronous, 1-second timeout)
  8. Verifies transfer completion using infoGet API to check remaining transfer length

  Parameters:
    p_ctrl     - Pointer to the control structure
    p_src      - Source data buffer
    p_dest     - Destination address in flash (memory-mapped address, any alignment)
    byte_count - Number of bytes to write (any size, no restrictions)

  Return:
    FSP_SUCCESS            - The flash was programmed successfully
    FSP_ERR_ASSERTION      - p_ctrl, p_dest or p_src is NULL
    FSP_ERR_NOT_OPEN       - Driver is not opened
    FSP_ERR_DEVICE_BUSY    - Another Write/Erase transaction is in progress
    FSP_ERR_WRITE_FAILED   - Write operation failed
    FSP_ERR_TIMEOUT        - Device ready timeout (1 second) or DMA completion timeout
    FSP_ERR_TRANSFER_ABORTED - DMA transfer did not complete properly
    FSP_ERR_NOT_INITIALIZED - RTOS event flags not initialized
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_memory_mapped_write(T_mc80_ospi_instance_ctrl *p_ctrl, uint8_t const *const p_src, uint8_t *const p_dest, uint32_t byte_count)
{
  // Variable declarations
  fsp_err_t                       err = FSP_SUCCESS;
  R_XSPI0_Type                   *p_reg;
  T_mc80_ospi_extended_cfg const *p_cfg_extend;
  transfer_instance_t const      *p_transfer;
  dmac_extended_cfg_t const      *p_dmac_extend;
  R_DMAC0_Type                   *p_dma_reg;
  uint32_t                        bytes_remaining;
  uint32_t                        current_block_size;
  transfer_properties_t           transfer_properties;
  uint8_t                         combo_bytes;
  fsp_err_t                       status_err;

  // 64-byte alignment handling variables
  uint32_t dest_addr = (uint32_t)p_dest;
  uint8_t  temp_buffer[MC80_OSPI_BLOCK_WRITE_SIZE];  // 64-byte temporary buffer
  uint32_t src_offset = 0;

  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl || NULL == p_src || NULL == p_dest || 0 == byte_count)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }

    // Additional safety check: ensure byte_count doesn't cause integer overflow
    if (byte_count > UINT32_MAX - dest_addr)
    {
      return FSP_ERR_ASSERTION;
    }
  }

  // Initialize variables
  p_reg                      = p_ctrl->p_reg;
  p_cfg_extend               = p_ctrl->p_cfg->p_extend;
  p_transfer                 = p_cfg_extend->p_lower_lvl_transfer;

  // Enable Octa-SPI DMA Bufferable Write
  p_dmac_extend              = p_transfer->p_cfg->p_extend;
  p_dma_reg                  = R_DMAC0 + (sizeof(R_DMAC0_Type) * p_dmac_extend->channel);
  p_dma_reg->DMBWR           = R_DMAC0_DMBWR_BWE_Msk;

  // Process data with 64-byte alignment for optimal performance
  bytes_remaining            = byte_count;

  // Handle data transfer with automatic 64-byte alignment
  uint32_t current_dest_addr = dest_addr;  // Track current destination address

  while (bytes_remaining > 0)
  {
    uint8_t const *write_src;
    uint8_t       *write_dest;

    // Calculate 64-byte alignment for current address
    uint32_t current_alignment_offset = current_dest_addr & (MC80_OSPI_BLOCK_WRITE_SIZE - 1);
    uint32_t current_aligned_addr     = current_dest_addr & ~(MC80_OSPI_BLOCK_WRITE_SIZE - 1);

    // Check if we need to handle alignment or partial block
    if (current_alignment_offset != 0 || bytes_remaining < MC80_OSPI_BLOCK_WRITE_SIZE)
    {
      // Use temporary buffer for alignment or partial block
      uint32_t available_space_in_block = MC80_OSPI_BLOCK_WRITE_SIZE - current_alignment_offset;
      uint32_t bytes_to_copy = (bytes_remaining < available_space_in_block) ? bytes_remaining : available_space_in_block;

      // Always use full 64-byte block for proper alignment
      current_block_size = MC80_OSPI_BLOCK_WRITE_SIZE;

      // Fill buffer with 0xFF (erased flash state)
      memset(temp_buffer, 0xFF, MC80_OSPI_BLOCK_WRITE_SIZE);

      // Copy user data to aligned position in buffer
      memcpy(&temp_buffer[current_alignment_offset], &p_src[src_offset], bytes_to_copy);

      write_src  = temp_buffer;
      write_dest = (uint8_t *)current_aligned_addr;

      // Update for next iteration
      src_offset += bytes_to_copy;
      bytes_remaining -= bytes_to_copy;
      current_dest_addr += bytes_to_copy;
    }
    else
    {
      // Direct transfer for aligned full blocks
      write_src          = p_src + src_offset;
      write_dest         = (uint8_t *)current_dest_addr;
      current_block_size = MC80_OSPI_BLOCK_WRITE_SIZE;

      // Update for next iteration
      src_offset += MC80_OSPI_BLOCK_WRITE_SIZE;
      bytes_remaining -= MC80_OSPI_BLOCK_WRITE_SIZE;
      current_dest_addr += MC80_OSPI_BLOCK_WRITE_SIZE;
    }

    // Configure DMA for current block transfer
    p_transfer->p_cfg->p_info->p_src                         = write_src;
    p_transfer->p_cfg->p_info->p_dest                        = write_dest;
    p_transfer->p_cfg->p_info->transfer_settings_word_b.size = TRANSFER_SIZE_1_BYTE;
    p_transfer->p_cfg->p_info->transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL;
    p_transfer->p_cfg->p_info->length                        = (uint16_t)current_block_size;

    // Reconfigure DMA with current block settings
    err                                                      = p_transfer->p_api->reconfigure(p_transfer->p_ctrl, p_transfer->p_cfg->p_info);
    if (FSP_SUCCESS != err)
    {
      break;
    }

    // Enable write for current block
    err = _Mc80_ospi_memory_mapped_write_enable(p_ctrl);
    if (FSP_SUCCESS != err)
    {
      break;
    }

    // Clear DMA event flags before starting transfer
    Mc80_ospi_dma_transfer_reset_flags();

    // Start DMA transfer for current block
    err = p_transfer->p_api->softwareStart(p_transfer->p_ctrl, TRANSFER_START_MODE_REPEAT);
    if (FSP_SUCCESS != err)
    {
      break;
    }

    // Wait for current block DMA completion
    err = Mc80_ospi_dma_wait_for_completion(MS_TO_TICKS(OSPI_DMA_COMPLETION_TIMEOUT_MS));
    if (FSP_SUCCESS != err)
    {
      break;  // DMA transfer failed or timed out
    }

    // Verify current block transfer completion status using infoGet
    transfer_properties.transfer_length_remaining = 0U;  // Reset structure
    err                                           = p_transfer->p_api->infoGet(p_transfer->p_ctrl, &transfer_properties);
    if (FSP_SUCCESS != err)
    {
      break;
    }

    // Check if current block transfer completed successfully
    if (transfer_properties.transfer_length_remaining > 0)
    {
      err = FSP_ERR_TRANSFER_ABORTED;  // Transfer did not complete properly
      break;
    }

    // Handle combination write function for current block if needed
    if (MC80_OSPI_COMBINATION_FUNCTION_DISABLE != MC80_OSPI_CFG_COMBINATION_FUNCTION)
    {
      combo_bytes = (uint8_t)(2U * ((uint8_t)MC80_OSPI_CFG_COMBINATION_FUNCTION + 1U));
      if (current_block_size < combo_bytes)
      {
        p_reg->BMCTL1 = MC80_OSPI_PRV_BMCTL1_PUSH_COMBINATION_WRITE_MASK;
      }
    }

    // Wait for device ready status after completing block write operation
    // Use hardware periodic polling to check write completion
    if (_Mc80_ospi_periodic_status_start(p_ctrl, MC80_OSPI_WAIT_WIP_CLEAR_EXPECTED, MC80_OSPI_WAIT_WIP_CLEAR_MASK) == FSP_SUCCESS)
    {
      // Wait for periodic polling completion using hardware automation
      status_err = Mc80_ospi_cmdcmp_wait_for_completion(MS_TO_TICKS(OSPI_PERIODIC_POLLING_TIMEOUT_MS));
      _Mc80_ospi_periodic_status_stop(p_ctrl);

      if (FSP_SUCCESS != status_err)
      {
        err = status_err;  // Timeout or error in periodic polling
        break;
      }
    }
    else
    {
      // Failed to start periodic status polling, exit with error
      err = FSP_ERR_WRITE_FAILED;
      break;
    }

    // Move to next block
    // Note: offset updates are handled in the alignment logic above
  }

  // Disable Octa-SPI DMA Bufferable Write
  p_dma_reg->DMBWR = 0U;

  return err;
}

/*-----------------------------------------------------------------------------------------------------
  Universal flash erase function that accepts any size and address, automatically optimizing
  erase operations by aligning to sector boundaries and using the most efficient commands.

  This function performs automatic alignment and optimal erase sequence selection:
  1. Aligns start address DOWN to sector boundary (4KB sectors)
  2. Aligns end address UP to sector boundary
  3. Calculates total erase size needed for complete coverage
  4. Automatically selects optimal erase commands (64KB blocks vs 4KB sectors)
  5. Executes erase operations in order of decreasing size for maximum efficiency

  IMPORTANT: This function always erases complete sectors/blocks. Data outside the requested
  range but within the same sectors will be lost. This is standard flash memory behavior.

  Algorithm Example:
  - Request: address=0x1800, byte_count=70KB
  - Aligned: start=0x1000, end=0x13000 (total=72KB to erase)
  - Operations: 1×64KB block (0x1000-0x11000) + 2×4KB sectors (0x11000-0x13000)

  Parameters:
    p_ctrl           - Pointer to the control structure
    p_device_address - Starting address in flash memory (will be aligned down to sector boundary)
    byte_count       - Number of bytes to erase (region will be expanded to sector boundaries)

  Return:
    FSP_SUCCESS         - Flash erase completed successfully
    FSP_ERR_ASSERTION   - Invalid parameters (NULL pointers or zero byte_count)
    FSP_ERR_NOT_OPEN    - Driver is not opened
    FSP_ERR_DEVICE_BUSY - Flash device is busy with another operation
    FSP_ERR_WRITE_FAILED - Erase operation failed or timeout occurred
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_erase(T_mc80_ospi_instance_ctrl *p_ctrl, uint8_t *const p_device_address, uint32_t byte_count)
{
  // Variable declarations
  uint32_t                            chip_address_base;
  uint32_t                            start_address;      // Combined start address (raw and aligned)
  uint32_t                            total_erase_size;
  uint32_t                            current_address;
  T_mc80_ospi_xspi_command_set const *p_cmd_set;
  T_mc80_ospi_erase_command const    *p_erase_list;
  uint8_t                             erase_list_length;
  uint16_t                            block_erase_command;
  uint16_t                            sector_erase_command;
  fsp_err_t                           err;
  T_mc80_ospi_direct_transfer         direct_command;
  uint32_t                            erase_start_time;
  fsp_err_t                           status_err;
  bool                                write_in_progress;
  uint32_t                            elapsed_time;
  uint32_t                            index;

  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl || NULL == p_device_address || 0 == byte_count)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }
  }

  // Calculate chip address base for proper addressing
  if (p_ctrl->channel)
  {
    chip_address_base = MC80_OSPI_DEVICE_1_START_ADDRESS;
  }
  else
  {
    chip_address_base = MC80_OSPI_DEVICE_0_START_ADDRESS;
  }

  // Convert memory-mapped address to chip address and align to sector boundaries
  start_address = (uint32_t)p_device_address - chip_address_base;

  // Calculate aligned erase region (align start down, end up to sector boundaries)
  uint32_t end_address   = start_address + byte_count;

  // Check for address overflow
  if (end_address < start_address)
  {
    return FSP_ERR_ASSERTION;  // Address overflow detected
  }

  start_address          = start_address & ~(MX25UM25645G_SECTOR_SIZE - 1);                                 // Round down to 4KB boundary
  end_address            = (end_address + MX25UM25645G_SECTOR_SIZE - 1) & ~(MX25UM25645G_SECTOR_SIZE - 1);  // Round up to 4KB boundary

  // Calculate total size to erase and set starting address
  total_erase_size = end_address - start_address;
  current_address  = start_address;

  // Get command set for erase operations
  p_cmd_set         = p_ctrl->p_cmd_set;
  if (NULL == p_cmd_set)
  {
    return FSP_ERR_ASSERTION;  // Command set not initialized
  }

  p_erase_list      = p_cmd_set->p_erase_commands->p_table;
  erase_list_length = p_cmd_set->p_erase_commands->length;

  // Find available erase commands (64KB block and 4KB sector)
  block_erase_command  = 0;  // 64KB block erase
  sector_erase_command = 0;  // 4KB sector erase

  for (index = 0; index < erase_list_length; index++)
  {
    if (p_erase_list[index].size == MX25UM25645G_BLOCK_SIZE)
    {
      block_erase_command = p_erase_list[index].command;  // 64KB block erase
    }
    else if (p_erase_list[index].size == MX25UM25645G_SECTOR_SIZE)
    {
      sector_erase_command = p_erase_list[index].command;  // 4KB sector erase
    }
  }

  // Process erase operations in optimal order (largest blocks first)
  err = FSP_SUCCESS;
  while (total_erase_size > 0 && FSP_SUCCESS == err)
  {
    // Select optimal erase operation based on remaining size and alignment
    uint16_t erase_command;
    uint32_t erase_size;

    if (total_erase_size >= MX25UM25645G_BLOCK_SIZE &&
        (current_address & (MX25UM25645G_BLOCK_SIZE - 1)) == 0 &&
        block_erase_command != 0)
    {
      // Use 64KB block erase (optimal for large areas)
      erase_command = block_erase_command;
      erase_size    = MX25UM25645G_BLOCK_SIZE;
    }
    else if (sector_erase_command != 0)
    {
      // Use 4KB sector erase (for remaining areas)
      erase_command = sector_erase_command;
      erase_size    = MX25UM25645G_SECTOR_SIZE;
    }
    else
    {
      // No suitable erase command found
      return FSP_ERR_ASSERTION;
    }

    // Execute write enable command before erase
    err = _Mc80_ospi_memory_mapped_write_enable(p_ctrl);
    if (FSP_SUCCESS != err)
    {
      break;
    }

    direct_command.command        = erase_command;
    direct_command.command_length = (uint8_t)p_cmd_set->command_bytes;
    direct_command.address        = current_address;
    direct_command.address_length = (uint8_t)(p_cmd_set->address_bytes + 1U);
    direct_command.data_length    = 0;

    // Execute erase command
    _Mc80_ospi_direct_transfer(p_ctrl, &direct_command, MC80_OSPI_DIRECT_TRANSFER_DIR_WRITE);

    // Monitor erase completion with extended timeout and retry logic
    erase_start_time = tx_time_get();

    while (true)
    {
      // Start periodic polling cycle for erase completion monitoring
      status_err = _Mc80_ospi_periodic_status_start(p_ctrl, MC80_OSPI_WAIT_WIP_CLEAR_EXPECTED, MC80_OSPI_WAIT_WIP_CLEAR_MASK);
      if (FSP_SUCCESS != status_err)
      {
        err = FSP_ERR_WRITE_FAILED;  // Failed to start periodic status polling
        break;
      }

      // Wait for periodic polling completion using hardware automation
      status_err = Mc80_ospi_cmdcmp_wait_for_completion(MS_TO_TICKS(OSPI_PERIODIC_POLLING_TIMEOUT_MS));
      _Mc80_ospi_periodic_status_stop(p_ctrl);

      // Check device status directly to see if write is still in progress
      write_in_progress = _Mc80_ospi_status_sub(p_ctrl, p_ctrl->p_cfg->write_status_bit);
      if (!write_in_progress)
      {
        // Write completed successfully (status check shows ready)
        status_err = FSP_SUCCESS;
        break;
      }

      // Check if maximum erase timeout exceeded (calculate inline)
      elapsed_time = tx_time_get() - erase_start_time;
      if (elapsed_time >= MS_TO_TICKS(OSPI_ERASE_COMPLETION_TIMEOUT_MS))
      {
        err = FSP_ERR_TIMEOUT;  // Maximum erase timeout exceeded
        break;
      }
    }

    // Calculate erase operation time
    elapsed_time = tx_time_get() - erase_start_time;
    uint32_t erase_time_ms = TICKS_TO_MS(elapsed_time);

    if (FSP_SUCCESS != status_err && FSP_ERR_TIMEOUT != err)
    {
      err = status_err;  // Timeout or error in periodic polling
      break;
    }

    // Move to next erase block
    current_address += erase_size;
    total_erase_size -= erase_size;
  }

  // If prefetch is enabled, flush the prefetch caches after erase operations
  if (MC80_OSPI_CFG_PREFETCH_FUNCTION && FSP_SUCCESS == err)
  {
    R_XSPI0_Type *const p_reg = p_ctrl->p_reg;
    p_reg->BMCTL1             = MC80_OSPI_PRV_BMCTL1_CLEAR_PREFETCH_MASK;
  }

  return err;
}

/*-----------------------------------------------------------------------------------------------------
  Gets the write or erase status of the flash.

  Parameters:
    p_ctrl   - Pointer to the control structure
    p_status - Pointer to status structure

  Return:
    FSP_SUCCESS       - The write status is in p_status
    FSP_ERR_ASSERTION - p_ctrl or p_status is NULL
    FSP_ERR_NOT_OPEN  - Driver is not opened
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_status_get(T_mc80_ospi_instance_ctrl *p_ctrl, T_mc80_ospi_status *const p_status)
{
  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl || NULL == p_status)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }
  }

  // Read device status
  p_status->write_in_progress = _Mc80_ospi_status_sub(p_ctrl, p_ctrl->p_cfg->write_status_bit);

  return FSP_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Selects the bank to access. Bank value should be 0 or 1.

  Parameters:
    p_ctrl - Pointer to the control structure
    bank   - Bank value (0 or 1)

  Return:
    FSP_ERR_UNSUPPORTED - This function is unsupported
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_bank_set(T_mc80_ospi_instance_ctrl *p_ctrl, uint32_t bank)
{
  return FSP_ERR_UNSUPPORTED;
}

/*-----------------------------------------------------------------------------------------------------
  Sets the SPI protocol. Dynamically switches between Standard SPI (1S-1S-1S) and Octal DDR (8D-8D-8D)
  modes by updating OSPI hardware registers and selecting appropriate command set from MC80_OSPI_config.c.

  === Mc80_ospi_spi_protocol_set() Function Operation ===

  The Mc80_ospi_spi_protocol_set() function allows dynamic switching between different OSPI protocols
  during runtime. It uses the command set table from this configuration file to reconfigure the
  peripheral for the new protocol.

  How it works:
  1. Accepts a new protocol type (e.g., MC80_OSPI_PROTOCOL_1S_1S_1S or MC80_OSPI_PROTOCOL_8D_8D_8D)
  2. Searches through g_OSPI_command_set_table[] to find matching protocol configuration
  3. Updates the peripheral registers with new timing, commands, and protocol settings
  4. Switches between Standard SPI mode and high-speed Octal DDR mode seamlessly

  Configuration Elements Used:

  * g_OSPI_command_set_table[] - Main protocol configuration array containing:
    - Protocol definitions (1S-1S-1S for Standard SPI, 8D-8D-8D for Octal DDR)
    - Command formats and dummy cycle counts
    - Address and data phase configurations
    - Frame formats and latency modes

  * g_OSPI_command_set - Table descriptor pointing to the command set array
    - Used by _Mc80_ospi_command_set_get() to locate protocol configurations
    - Length field ensures safe iteration through available protocols

  * Protocol-specific configurations:
    - Standard SPI (1S-1S-1S): Uses single-line commands with 1-byte opcodes
    - Octal DDR (8D-8D-8D): Uses 8-line double data rate with 2-byte opcodes

  * Command mappings for each protocol:
    - Read commands (FAST_READ4B vs 8READ_DTR)
    - Write commands (PP4B vs PP4B_STR)
    - Status commands (RDSR vs RDSR_STR)
    - Dummy cycle requirements (8 cycles for SPI, 20 cycles for DDR)

  * Erase command tables:
    - g_OSPI_command_set_initial_erase_table for Standard SPI mode
    - g_OSPI_command_set_high_speed_erase_table for Octal DDR mode (empty - erase only in SPI)

  The function enables seamless protocol switching for performance optimization:
  - Start with reliable Standard SPI for initialization
  - Switch to high-speed Octal DDR for bulk data operations
  - Return to Standard SPI for erase operations (required by flash device)

  Parameters:
    p_ctrl       - Pointer to the control structure
    spi_protocol - SPI protocol to set

  Return:
    FSP_SUCCESS              - SPI protocol updated on MPU peripheral
    FSP_ERR_ASSERTION        - A required pointer is NULL
    FSP_ERR_NOT_OPEN         - Driver is not opened
    FSP_ERR_CALIBRATE_FAILED - Failed to perform auto-calibrate
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_spi_protocol_set(T_mc80_ospi_instance_ctrl *p_ctrl, T_mc80_ospi_protocol spi_protocol)
{
  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }
  }

  // Save the old protocol in case of an undefined command set
  T_mc80_ospi_protocol old_protocol = p_ctrl->spi_protocol;
  p_ctrl->spi_protocol              = spi_protocol;

  // Update the SPI protocol and its associated registers
  fsp_err_t err                     = _Mc80_ospi_protocol_specific_settings(p_ctrl);

  if (FSP_ERR_INVALID_MODE == err)
  {
    // Restore the original spi protocol. Nothing else has been changed in this case
    p_ctrl->spi_protocol = old_protocol;
  }

  return err;
}

/*-----------------------------------------------------------------------------------------------------
  Safely switches SPI protocol with complete flash device reset and reinitialization sequence.

  This function performs a complete protocol switch sequence including:
  1. Flash device hardware reset via RSTEN/RST commands
  2. Flash CR2 register configuration for new protocol mode
  3. Controller protocol settings update
  4. Verification of successful protocol switch

  Protocol Switch Sequence:
  - For switch to 8D-8D-8D: Reset flash → Set CR2=0x02 → Switch controller to 8D-8D-8D
  - For switch to 1S-1S-1S: Reset flash → Verify CR2=0x00 → Switch controller to 1S-1S-1S

  The function ensures the flash device and controller are synchronized and operating
  in the same protocol mode after the switch completes.

  Parameters:
    p_ctrl       - Pointer to the control structure
    new_protocol - Target SPI protocol to switch to

  Return:
    FSP_SUCCESS              - Protocol switched successfully
    FSP_ERR_ASSERTION        - A required pointer is NULL
    FSP_ERR_NOT_OPEN         - Driver is not opened
    FSP_ERR_INVALID_MODE     - Unsupported protocol specified
    FSP_ERR_DEVICE_BUSY      - Flash device is busy
    FSP_ERR_WRITE_FAILED     - Failed to configure flash device
    FSP_ERR_CALIBRATE_FAILED - Failed to perform auto-calibrate
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_spi_protocol_switch_safe(T_mc80_ospi_instance_ctrl *p_ctrl, T_mc80_ospi_protocol new_protocol)
{
  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }
    // Validate protocol
    if (new_protocol != MC80_OSPI_PROTOCOL_1S_1S_1S && new_protocol != MC80_OSPI_PROTOCOL_8D_8D_8D)
    {
      return FSP_ERR_INVALID_MODE;
    }
  }

  fsp_err_t err = FSP_SUCCESS;

  // Check if we're already in the target protocol
  if (p_ctrl->spi_protocol == new_protocol)
  {
    return FSP_SUCCESS;  // Already in target protocol
  }

  // Step 1: Reset flash device to known state (always start in SPI mode)
  err = Mc80_ospi_hardware_reset(p_ctrl);
  if (FSP_SUCCESS != err)
  {
    return err;
  }

  // Step 2: Set controller to Standard SPI mode first (for flash configuration)
  if (p_ctrl->spi_protocol != MC80_OSPI_PROTOCOL_1S_1S_1S)
  {
    err = Mc80_ospi_spi_protocol_set(p_ctrl, MC80_OSPI_PROTOCOL_1S_1S_1S);
    if (FSP_SUCCESS != err)
    {
      return err;
    }
  }

  // Step 3: Wait for flash device to be ready
  R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);  // Allow flash reset to complete

  // Step 4: Configure flash device for target protocol
  if (new_protocol == MC80_OSPI_PROTOCOL_8D_8D_8D)
  {
    // Configure flash to OPI DTR mode by setting CR2 = 0x02

    // Enable writing
    T_mc80_ospi_direct_transfer write_enable_cmd = {
      .command        = MX25_CMD_WREN,
      .command_length = 1,
      .address_length = 0,
      .data_length    = 0,
      .dummy_cycles   = 0,
    };

    _Mc80_ospi_direct_transfer(p_ctrl, &write_enable_cmd, MC80_OSPI_DIRECT_TRANSFER_DIR_WRITE);

    // Write CR2 to enable OPI DTR mode
    T_mc80_ospi_direct_transfer write_cr2_cmd = {
      .command        = MX25_CMD_WRCR2,
      .command_length = 1,
      .address        = 0x00000000,  // CR2 address
      .address_length = 4,           // 4-byte address required for CR2
      .data           = 0x02,        // Enable OPI DTR mode
      .data_length    = 1,
      .dummy_cycles   = 0,
    };

    _Mc80_ospi_direct_transfer(p_ctrl, &write_cr2_cmd, MC80_OSPI_DIRECT_TRANSFER_DIR_WRITE);

    // Wait for flash configuration to complete
    R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
  }

  // Step 5: Switch controller to target protocol
  err = Mc80_ospi_spi_protocol_set(p_ctrl, new_protocol);
  if (FSP_SUCCESS != err)
  {
    return err;
  }

  // Step 6: Verify protocol switch was successful
  if (new_protocol == MC80_OSPI_PROTOCOL_8D_8D_8D)
  {
    // Try to read CR2 in 8D-8D-8D mode to verify
    T_mc80_ospi_direct_transfer read_cr2_cmd = {
      .command        = MX25_OPI_RDCR2_DTR,
      .command_length = 2,
      .address        = 0x00000000,
      .address_length = 4,
      .data_length    = 2,  // Read 2 bytes in OPI DTR mode
      .dummy_cycles   = 4,  // 4 dummy cycles for status in DDR
    };

    _Mc80_ospi_direct_transfer(p_ctrl, &read_cr2_cmd, MC80_OSPI_DIRECT_TRANSFER_DIR_READ);

    uint8_t cr2_value = (uint8_t)(read_cr2_cmd.data & 0xFF);
    if (cr2_value != 0x02)
    {
      return FSP_ERR_WRITE_FAILED;  // Protocol switch verification failed
    }
  }

  return FSP_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Close the OSPI driver module.

  Parameters:
    p_ctrl - Pointer to the control structure

  Return:
    FSP_SUCCESS       - Configuration was successful
    FSP_ERR_ASSERTION - p_ctrl is NULL
    FSP_ERR_NOT_OPEN  - Driver is not opened
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_close(T_mc80_ospi_instance_ctrl *p_ctrl)
{
  fsp_err_t err = FSP_SUCCESS;

  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl || NULL == p_ctrl->p_cfg || NULL == p_ctrl->p_cfg->p_extend)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }
  }

  T_mc80_ospi_extended_cfg const *p_cfg_extend = p_ctrl->p_cfg->p_extend;

  // Close transfer instance
  transfer_instance_t const *p_transfer        = p_cfg_extend->p_lower_lvl_transfer;
  p_transfer->p_api->close(p_transfer->p_ctrl);

  // Cleanup RTOS resources if all channels are being closed
  if (0 == ((g_mc80_ospi_channels_open_flags & ~MC80_OSPI_PRV_CH_MASK(p_cfg_extend)) & MC80_OSPI_PRV_UNIT_MASK(p_cfg_extend)))
  {
    // Disable OSPI command completion interrupt in NVIC
    NVIC_DisableIRQ(OSPI_CMDCMP_IRQn);
    NVIC_ClearPendingIRQ(OSPI_CMDCMP_IRQn);   // Clear NVIC pending interrupt
    R_ICU->IELSR_b[OSPI_CMDCMP_IRQn].IR = 0;  // Clear any pending interrupt flag

    _Ospi_dma_event_flags_cleanup();
  }

  p_ctrl->open = 0U;
  g_mc80_ospi_channels_open_flags &= ~MC80_OSPI_PRV_CH_MASK(p_cfg_extend);

  // Disable clock to the OSPI block if all channels are closed
  if (0 == (g_mc80_ospi_channels_open_flags & MC80_OSPI_PRV_UNIT_MASK(p_cfg_extend)))
  {
    R_BSP_MODULE_STOP(FSP_IP_OSPI, p_cfg_extend->ospi_unit);
  }

  return err;
}

/*-----------------------------------------------------------------------------------------------------
  Perform initialization based on SPI/OPI protocol

  Parameters:
    p_ctrl - Pointer to OSPI specific control structure

  Return:
    FSP_SUCCESS              - Protocol based settings completed successfully
    FSP_ERR_CALIBRATE_FAILED - Auto-Calibration failed
-----------------------------------------------------------------------------------------------------*/
static fsp_err_t _Mc80_ospi_protocol_specific_settings(T_mc80_ospi_instance_ctrl *p_ctrl)
{
  R_XSPI0_Type *const p_reg                     = p_ctrl->p_reg;
  fsp_err_t           ret                       = FSP_SUCCESS;

  // Get the command set for the configured protocol and save it to the control struct
  T_mc80_ospi_xspi_command_set const *p_cmd_set = _Mc80_ospi_command_set_get(p_ctrl);
  if (NULL == p_cmd_set)
  {
    return FSP_ERR_INVALID_MODE;
  }

  p_ctrl->p_cmd_set = p_cmd_set;

  // Update the SPI protocol and latency mode
  uint32_t liocfg   = p_reg->LIOCFGCS[p_ctrl->channel] & ~(OSPI_LIOCFGCSN_LATEMD_Msk | OSPI_LIOCFGCSN_PRTMD_Msk);
  liocfg |= (((uint32_t)p_ctrl->spi_protocol << OSPI_LIOCFGCSN_PRTMD_Pos) & OSPI_LIOCFGCSN_PRTMD_Msk);
  liocfg |= (((uint32_t)p_cmd_set->latency_mode << OSPI_LIOCFGCSN_LATEMD_Pos) & OSPI_LIOCFGCSN_LATEMD_Msk);
  p_reg->LIOCFGCS[p_ctrl->channel] = liocfg;

  // Specifies the read/write commands and Read dummy clocks for Device
  uint32_t cmcfg0                  = ((uint32_t)(p_cmd_set->address_msb_mask << OSPI_CMCFG0CSN_ADDRPEN_Pos)) |
                    ((uint32_t)(p_cmd_set->frame_format << OSPI_CMCFG0CSN_FFMT_Pos)) |
                    (((uint32_t)p_cmd_set->address_bytes << OSPI_CMCFG0CSN_ADDSIZE_Pos) &
                     OSPI_CMCFG0CSN_ADDSIZE_Msk);

  // When using 4-byte addressing, always mask off the most-significant nybble to remove the system bus offset from
  // the transmitted addresses. Ex. CS1 starts at 0x9000_0000 so it needs to mask off bits [31:28]
  if (p_cmd_set->address_bytes == MC80_OSPI_ADDRESS_BYTES_4)
  {
    cmcfg0 |= MC80_OSPI_PRV_ADDRESS_REPLACE_ENABLE_BITS;
  }

  // Apply the frame format setting and update the register
  cmcfg0 |= (uint32_t)(p_cmd_set->frame_format << OSPI_CMCFG0CSN_FFMT_Pos);
  p_reg->CMCFGCS[p_ctrl->channel].CMCFG0 = cmcfg0;

  // Cache the appropriate command values for later use
  uint16_t read_command                  = p_cmd_set->read_command;
  uint16_t write_command                 = p_cmd_set->program_command;

  // If no length is specified or if the command byte length is 1, move the command to the upper byte
  if (MC80_OSPI_COMMAND_BYTES_1 == p_cmd_set->command_bytes)
  {
    read_command  = (uint16_t)((read_command & MC80_OSPI_PRV_CDTBUF_CMD_1B_VALUE_MASK) << MC80_OSPI_PRV_CDTBUF_CMD_1B_VALUE_SHIFT);
    write_command = (uint16_t)((write_command & MC80_OSPI_PRV_CDTBUF_CMD_1B_VALUE_MASK) << MC80_OSPI_PRV_CDTBUF_CMD_1B_VALUE_SHIFT);
  }

  const uint8_t read_dummy_cycles        = p_cmd_set->read_dummy_cycles;
  const uint8_t write_dummy_cycles       = p_cmd_set->program_dummy_cycles;

  p_reg->CMCFGCS[p_ctrl->channel].CMCFG1 = (uint32_t)(((uint32_t)(read_command) << OSPI_CMCFG1CSN_RDCMD_Pos) |
                                                      ((uint32_t)(read_dummy_cycles << OSPI_CMCFG1CSN_RDLATE_Pos) &
                                                       OSPI_CMCFG1CSN_RDLATE_Msk));

  p_reg->CMCFGCS[p_ctrl->channel].CMCFG2 = (uint32_t)(((uint32_t)(write_command) << OSPI_CMCFG2CSN_WRCMD_Pos) |
                                                      ((uint32_t)(write_dummy_cycles << OSPI_CMCFG2CSN_WRLATE_Pos) &
                                                       OSPI_CMCFG2CSN_WRLATE_Msk));

  return ret;
}

/*-----------------------------------------------------------------------------------------------------
  Gets device status.

  Parameters:
    p_ctrl - Pointer to a driver handle
    bit_pos         - Write-in-progress bit position

  Return:
    True if busy, false if not
-----------------------------------------------------------------------------------------------------*/
static bool _Mc80_ospi_status_sub(T_mc80_ospi_instance_ctrl *p_ctrl, uint8_t bit_pos)
{
  T_mc80_ospi_xspi_command_set const *p_cmd_set = p_ctrl->p_cmd_set;

  // Skip status check if no command was specified
  if (0 == p_cmd_set->status_command)
  {
    return false;
  }

  T_mc80_ospi_direct_transfer direct_command = {
    .command        = p_cmd_set->status_command,
    .command_length = (uint8_t)p_cmd_set->command_bytes,
    .address_length = 0U,
    .address        = 0U,
    .data_length    = 1U,
    .dummy_cycles   = p_cmd_set->status_dummy_cycles,
  };

  if (p_cmd_set->status_needs_address)
  {
    direct_command.address_length = (uint8_t)(p_cmd_set->status_address_bytes + 1U);
    direct_command.address        = p_cmd_set->status_address;
  }

  // 8D-8D-8D mode requires an address for any kind of read. If the address wasn't set by the configuration
  // set it to the general address length
  if ((direct_command.address_length != 0) && (MC80_OSPI_PROTOCOL_8D_8D_8D == p_ctrl->spi_protocol))
  {
    direct_command.address_length = (uint8_t)(p_cmd_set->address_bytes + 1U);
  }

  _Mc80_ospi_direct_transfer(p_ctrl, &direct_command, MC80_OSPI_DIRECT_TRANSFER_DIR_READ);

  return (direct_command.data >> bit_pos) & 1U;
}

/*-----------------------------------------------------------------------------------------------------
  Send Write enable command to the OctaFlash

  Parameters:
    p_ctrl - Pointer to OSPI specific control structure

  Return:
    FSP_SUCCESS         - Write enable operation completed
    FSP_ERR_NOT_ENABLED - Write enable failed
-----------------------------------------------------------------------------------------------------*/
static fsp_err_t _Mc80_ospi_memory_mapped_write_enable(T_mc80_ospi_instance_ctrl *p_ctrl)
{
  T_mc80_ospi_xspi_command_set const *const p_cmd_set = p_ctrl->p_cmd_set;

  // If the command is 0x00, then skip sending the write enable
  if (0 == p_cmd_set->write_enable_command)
  {
    return FSP_SUCCESS;
  }

  T_mc80_ospi_direct_transfer direct_command = {
    .command        = p_cmd_set->write_enable_command,
    .command_length = (uint8_t)p_cmd_set->command_bytes,
    .address_length = 0,
    .address        = 0,
    .data_length    = 0,
    .dummy_cycles   = 0,
  };

  _Mc80_ospi_direct_transfer(p_ctrl, &direct_command, MC80_OSPI_DIRECT_TRANSFER_DIR_WRITE);

  // Verify write is enabled using hardware periodic polling
  fsp_err_t status_err = _Mc80_ospi_periodic_status_start(p_ctrl, MC80_OSPI_WAIT_WEL_SET_EXPECTED, MC80_OSPI_WAIT_WEL_SET_MASK);
  if (FSP_SUCCESS != status_err)
  {
    return FSP_ERR_NOT_ENABLED;  // Failed to start periodic status polling
  }

  // Wait for periodic polling completion using hardware automation
  status_err = Mc80_ospi_cmdcmp_wait_for_completion(MS_TO_TICKS(OSPI_PERIODIC_POLLING_TIMEOUT_MS));
  _Mc80_ospi_periodic_status_stop(p_ctrl);

  if (FSP_SUCCESS != status_err)
  {
    return FSP_ERR_NOT_ENABLED;  // Write enable verification failed or timed out
  }

  return FSP_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Direct transfer implementation - executes manual OSPI commands outside of memory-mapped mode.

  This function performs direct communication with the OSPI flash device using the manual command
  interface. It bypasses the automatic memory-mapped mode and allows sending custom commands
  with specific parameters for read/write operations, status checks, and device configuration.

  Operation sequence:
  1. Configures the Command Data Buffer (CDTBUF) with command parameters:
     - Command code and length (1 or 2 bytes)
     - Address and address length
     - Data length and dummy cycles
     - Transfer direction (read/write)
  2. Sets up manual command control (CDCTL0) to select the target chip
  3. Waits for any ongoing transactions to complete
  4. Loads command data into the command buffer registers
  5. For write operations: loads data into CDD0/CDD1 registers
  6. Initiates the transaction by setting TRREQ bit
  7. Waits for transaction completion
  8. For read operations: retrieves data from CDD0/CDD1 registers
  9. Clears interrupt flags

  The function handles both 1-byte and 2-byte command formats, adjusting the command positioning
  in the register based on the command length. For data transfers larger than 4 bytes, it uses
  both CDD0 and CDD1 registers to handle up to 8 bytes of data.

  Parameters:
    p_ctrl - Pointer to OSPI specific control structure
    p_transfer      - Pointer to transfer structure
    direction       - Transfer direction

  Return:
    void
-----------------------------------------------------------------------------------------------------*/
static void _Mc80_ospi_direct_transfer(T_mc80_ospi_instance_ctrl         *p_ctrl,
                                       T_mc80_ospi_direct_transfer *const p_transfer,
                                       T_mc80_ospi_direct_transfer_dir    direction)
{
  R_XSPI0_Type *const             p_reg   = p_ctrl->p_reg;
  const T_mc80_ospi_device_number channel = p_ctrl->channel;

  // Build the Command Data Buffer configuration register value
  // This register defines the complete transaction format including command, address, data sizes and dummy cycles
  uint32_t cdtbuf0 =
  (((uint32_t)p_transfer->command_length << OSPI_CDTBUFn_CMDSIZE_Pos) & OSPI_CDTBUFn_CMDSIZE_Msk) |
  (((uint32_t)p_transfer->address_length << OSPI_CDTBUFn_ADDSIZE_Pos) & OSPI_CDTBUFn_ADDSIZE_Msk) |
  (((uint32_t)p_transfer->data_length << OSPI_CDTBUFn_DATASIZE_Pos) & OSPI_CDTBUFn_DATASIZE_Msk) |
  (((uint32_t)p_transfer->dummy_cycles << OSPI_CDTBUFn_LATE_Pos) & OSPI_CDTBUFn_LATE_Msk) |
  (((uint32_t)direction << OSPI_CDTBUFn_TRTYPE_Pos) & OSPI_CDTBUFn_TRTYPE_Msk);

  // Handle command code positioning based on command length
  // 1-byte commands go in upper byte, 2-byte commands use both bytes
  if (1 == p_transfer->command_length)
  {
    cdtbuf0 |= (p_transfer->command & MC80_OSPI_PRV_CDTBUF_CMD_1B_VALUE_MASK) << MC80_OSPI_PRV_CDTBUF_CMD_UPPER_OFFSET;
  }
  else
  {
    cdtbuf0 |= (p_transfer->command & MC80_OSPI_PRV_CDTBUF_CMD_2B_VALUE_MASK) << MC80_OSPI_PRV_CDTBUF_CMD_OFFSET;
  }

  // Configure manual command control: cancel ongoing transactions, select target channel
  p_reg->CDCTL0       = ((((uint32_t)channel) << OSPI_CDCTL0_CSSEL_Pos) & OSPI_CDCTL0_CSSEL_Msk);

  // Load the transaction configuration and address into command buffer
  p_reg->CDBUF[0].CDT = cdtbuf0;              // Command Data Transaction register
  p_reg->CDBUF[0].CDA = p_transfer->address;  // Command Data Address register

  // For write operations: load data into the data registers before starting transaction
  if (MC80_OSPI_DIRECT_TRANSFER_DIR_WRITE == direction)
  {
    p_reg->CDBUF[0].CDD0 = (uint32_t)(p_transfer->data_u64 & UINT32_MAX);                    // Lower 32 bits
    if (p_transfer->data_length > sizeof(uint32_t))
    {
      p_reg->CDBUF[0].CDD1 = (uint32_t)(p_transfer->data_u64 >> MC80_OSPI_PRV_UINT32_BITS);  // Upper 32 bits
    }
  }

  // Start the transaction and wait for completion
  p_reg->CDCTL0_b.TRREQ = 1;  // Initiate transaction
  OSPI_DEBUG_LOOP_START(OSPI_DEBUG_LOOP_DIRECT_TRANSFER_COMPLETION_WAIT);
  while (p_reg->CDCTL0_b.TRREQ != 0)
  {
    OSPI_DEBUG_LOOP_ITERATION();
    __NOP();  // Breakpoint for transaction completion wait
  }
  OSPI_DEBUG_LOOP_END(OSPI_DEBUG_LOOP_DIRECT_TRANSFER_COMPLETION_WAIT);

  // For read operations: retrieve data from the data registers after transaction completes
  if (MC80_OSPI_DIRECT_TRANSFER_DIR_READ == direction)
  {
    p_transfer->data_u64 = p_reg->CDBUF[0].CDD0;                                                // Read lower 32 bits
    if (p_transfer->data_length > sizeof(uint32_t))
    {
      p_transfer->data_u64 |= (((uint64_t)p_reg->CDBUF[0].CDD1) << MC80_OSPI_PRV_UINT32_BITS);  // Combine with upper 32 bits
    }
  }

  // Clear interrupt flags to prepare for next transaction
  p_reg->INTC = p_reg->INTS;
}

/*-----------------------------------------------------------------------------------------------------
  Get command set for current protocol

  Parameters:
    p_ctrl - Pointer to OSPI specific control structure

  Return:
    Pointer to command set structure, or NULL if not found
-----------------------------------------------------------------------------------------------------*/
static T_mc80_ospi_xspi_command_set const *_Mc80_ospi_command_set_get(T_mc80_ospi_instance_ctrl *p_ctrl)
{
  T_mc80_ospi_extended_cfg const *p_cfg_extend = p_ctrl->p_cfg->p_extend;

  if (NULL == p_cfg_extend->p_xspi_command_set)
  {
    return NULL;
  }

  T_mc80_ospi_xspi_command_set *p_cmd_set;
  for (uint32_t i = 0; i < p_cfg_extend->p_xspi_command_set->length; i++)
  {
    p_cmd_set = &((T_mc80_ospi_xspi_command_set *)p_cfg_extend->p_xspi_command_set->p_table)[i];
    if (p_cmd_set->protocol == p_ctrl->spi_protocol)
    {
      return p_cmd_set;
    }
  }

  // If the protocol isn't found, return NULL
  return NULL;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Automatic calibration sequence for OSPI timing parameters in high-speed protocols.

  This function performs automatic calibration of read data strobe timing for reliable high-speed
  OSPI communication, particularly critical for 8D-8D-8D (Octal DDR) protocol operation.

  Calibration Process:
  1. Validates that no calibration is currently in progress (CAEN bit check)
  2. Saves current timing parameters before calibration (if p_calibration_data provided)
  3. Extracts current protocol parameters (command format, address size, dummy cycles)
  4. Configures calibration control registers (CCCTL1-3) with:
     - Command size and format (1 or 2 bytes, positioned correctly)
     - Address size from current protocol settings
     - Read command from active command set
     - Dummy cycles matching protocol requirements
     - Calibration data size (0xF = 15 bytes)
     - Preamble pattern address for known data reading
  5. Sets preamble patterns in CCCTL4-7 registers for data comparison:
     - Uses patterns from configuration (p_autocalibration_preamble_patterns)
     - Falls back to default patterns if configuration pointer is NULL
     - Patterns must match data at the specified calibration address in flash
     - Default patterns: 0xFFFF0000, 0x000800FF, 0x00FFF700, 0xF700F708
     - Write patterns to flash at calibration address before running calibration
  6. Sets calibration parameters in CCCTL0:
     - CAITV: Calibration interval (0x1F cycles)
     - CANOWR: No overwrite mode enabled
     - CASFTEND: Shift end value (0x1F)
  7. Starts automatic calibration by setting CAEN bit
  8. Waits for hardware to complete calibration process by monitoring INTS register:
     - CASUCCS flag indicates successful calibration
     - CAFAIL flag indicates calibration failure
     - Timeout protection prevents infinite waiting
  9. Disables calibration by clearing CAEN bit
  10. Checks calibration result and clears appropriate interrupt flags
  11. Saves timing parameters after calibration (if p_calibration_data provided)

  Hardware Operation:
  The OSPI peripheral automatically:
  - Tests multiple data strobe timing delays
  - Reads known pattern data from the specified address
  - Compares received data against expected pattern
  - Selects optimal timing that provides reliable data reception
  - Updates internal timing registers with calibrated values

  Calibrated Parameters Storage:
  Results are automatically written to hardware registers by the OSPI peripheral:
  - CASTTCSn (0x188 + 0x004*n): Calibration Status Register - contains success flags for each
    tested OM_DQS shift value, indicating which timing delays passed validation
  - Internal OM_DQS timing registers: Hardware automatically selects and applies the optimal
    data strobe timing delay from the successful calibration range
  - LIOCFGCS register timing fields: May be updated with optimized sampling delays

  The calibration process tests all possible data strobe (OM_DQS) timing delays and identifies
  which ones provide reliable data reception. The hardware then automatically configures the
  optimal timing for subsequent high-speed read operations.

  Protocol Compatibility:
  - Standard SPI (1S-1S-1S): Basic calibration for timing optimization
  - Octal DDR (8D-8D-8D): Critical for high-speed data integrity
  - Automatically uses current protocol's command format and timing

  Prerequisites:
  - Flash device must be in appropriate protocol mode
  - Preamble pattern address must contain valid, known data
  - Device must not be busy with other operations

  Parameters: p_ctrl - Pointer to OSPI instance control structure
              p_calibration_data - Pointer to structure for storing calibration data (can be NULL)

  Return: FSP_SUCCESS - Auto-calibration completed successfully
          FSP_ERR_DEVICE_BUSY - Auto-calibration already in progress
          FSP_ERR_CALIBRATE_FAILED - Auto-calibration failed
          FSP_ERR_ASSERTION - Invalid parameter
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_auto_calibrate(T_mc80_ospi_instance_ctrl *p_ctrl, T_mc80_ospi_calibration_data *p_calibration_data)
{
  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }
  }

  R_XSPI0_Type *const             p_OSPI        = p_ctrl->p_reg;
  fsp_err_t                       ret           = FSP_SUCCESS;
  T_mc80_ospi_extended_cfg const *p_cfg_extend  = p_ctrl->p_cfg->p_extend;

  T_mc80_ospi_xspi_command_set const *p_cmd_set = p_ctrl->p_cmd_set;

  T_mc80_ospi_device_number channel             = p_ctrl->channel;

  // Check that calibration is not in progress
  if (0 != p_OSPI->CCCTLCS[channel].CCCTL0_b.CAEN)
  {
    return FSP_ERR_DEVICE_BUSY;
  }

  // Save timing parameters before calibration if structure provided
  if (NULL != p_calibration_data)
  {
    // Clear the structure
    memset(p_calibration_data, 0, sizeof(T_mc80_ospi_calibration_data));
    p_calibration_data->channel = (uint8_t)channel;

    // Save DQS shift value from WRAPCFG register
    if (MC80_OSPI_DEVICE_NUMBER_0 == channel)
    {
      p_calibration_data->before_calibration.wrapcfg_dssft = (p_OSPI->WRAPCFG & OSPI_WRAPCFG_DSSFTCS0_Msk) >> OSPI_WRAPCFG_DSSFTCS0_Pos;
    }
    else
    {
      p_calibration_data->before_calibration.wrapcfg_dssft = (p_OSPI->WRAPCFG & OSPI_WRAPCFG_DSSFTCS1_Msk) >> OSPI_WRAPCFG_DSSFTCS1_Pos;
    }

    // Save SDR and DDR timing parameters from LIOCFGCS register
    uint32_t liocfg_value                                   = p_OSPI->LIOCFGCS[channel];
    p_calibration_data->before_calibration.liocfg_sdrsmpsft = (liocfg_value & OSPI_LIOCFGCSN_SDRSMPSFT_Msk) >> OSPI_LIOCFGCSN_SDRSMPSFT_Pos;
    p_calibration_data->before_calibration.liocfg_ddrsmpex  = (liocfg_value & OSPI_LIOCFGCSN_DDRSMPEX_Msk) >> OSPI_LIOCFGCSN_DDRSMPEX_Pos;

    // Save current calibration status
    p_calibration_data->before_calibration.casttcs_value    = p_OSPI->CASTTCS[channel];
  }

  const uint8_t command_bytes     = (uint8_t)p_cmd_set->command_bytes;
  uint16_t      read_command      = p_cmd_set->read_command;
  const uint8_t read_dummy_cycles = p_cmd_set->read_dummy_cycles;
  const uint8_t address_bytes     = (uint8_t)(p_cmd_set->address_bytes + 1U);

  // If using 1 command byte, shift the read command over as the peripheral expects
  if (1U == command_bytes)
  {
    read_command = (uint16_t)((read_command & MC80_OSPI_PRV_CDTBUF_CMD_1B_VALUE_MASK) << MC80_OSPI_PRV_CDTBUF_CMD_1B_VALUE_SHIFT);
  }

  p_OSPI->CCCTLCS[channel].CCCTL1 = (((uint32_t)command_bytes << OSPI_CCCTL1CSn_CACMDSIZE_Pos) & OSPI_CCCTL1CSn_CACMDSIZE_Msk) | (((uint32_t)address_bytes << OSPI_CCCTL1CSn_CAADDSIZE_Pos) & OSPI_CCCTL1CSn_CAADDSIZE_Msk) | (0xFU << OSPI_CCCTL1CSn_CADATASIZE_Pos) | (0U << OSPI_CCCTL1CSn_CAWRLATE_Pos) | (((uint32_t)read_dummy_cycles << OSPI_CCCTL1CSn_CARDLATE_Pos) & OSPI_CCCTL1CSn_CARDLATE_Msk);
  p_OSPI->CCCTLCS[channel].CCCTL2 = (uint32_t)read_command << OSPI_CCCTL2CSn_CARDCMD_Pos;

  // Set calibration address - use flash start address if preamble address is invalid
  uint32_t calibration_address    = (uint32_t)p_cfg_extend->p_autocalibration_preamble_pattern_addr;
  if (calibration_address == 0)
  {
    // Use start of flash memory if no specific pattern address is configured
    if (channel == MC80_OSPI_DEVICE_NUMBER_0)
    {
      calibration_address = 0x00000000;
    }
    else
    {
      calibration_address = 0x10000000;
    }
  }
  else
  {
    // Convert XIP address to physical flash address for calibration
    if (channel == MC80_OSPI_DEVICE_NUMBER_0)
    {
      if (calibration_address >= MC80_OSPI_DEVICE_0_START_ADDRESS)
      {
        calibration_address = calibration_address - MC80_OSPI_DEVICE_0_START_ADDRESS;
      }
    }
    else
    {
      if (calibration_address >= MC80_OSPI_DEVICE_1_START_ADDRESS)
      {
        calibration_address = calibration_address - MC80_OSPI_DEVICE_1_START_ADDRESS;
      }
    }
  }
  p_OSPI->CCCTLCS[channel].CCCTL3 = calibration_address;

  // Set preamble patterns from configuration or use defaults
  if (NULL != p_cfg_extend->p_autocalibration_preamble_patterns)
  {
    p_OSPI->CCCTLCS[channel].CCCTL4 = p_cfg_extend->p_autocalibration_preamble_patterns[0];  // Pattern 0
    p_OSPI->CCCTLCS[channel].CCCTL5 = p_cfg_extend->p_autocalibration_preamble_patterns[1];  // Pattern 1
    p_OSPI->CCCTLCS[channel].CCCTL6 = p_cfg_extend->p_autocalibration_preamble_patterns[2];  // Pattern 2
    p_OSPI->CCCTLCS[channel].CCCTL7 = p_cfg_extend->p_autocalibration_preamble_patterns[3];  // Pattern 3
  }
  else
  {
    // Use default patterns if none provided
    p_OSPI->CCCTLCS[channel].CCCTL4 = MC80_OSPI_DEFAULT_PREAMBLE_PATTERN_0;  // Pattern 0
    p_OSPI->CCCTLCS[channel].CCCTL5 = MC80_OSPI_DEFAULT_PREAMBLE_PATTERN_1;  // Pattern 1
    p_OSPI->CCCTLCS[channel].CCCTL6 = MC80_OSPI_DEFAULT_PREAMBLE_PATTERN_2;  // Pattern 2
    p_OSPI->CCCTLCS[channel].CCCTL7 = MC80_OSPI_DEFAULT_PREAMBLE_PATTERN_3;  // Pattern 3
  }

  // Configure auto-calibration parameters using defined constants
  p_OSPI->CCCTLCS[channel].CCCTL0 = (MC80_OSPI_CALIBRATION_INTERVAL_MAX << OSPI_CCCTL0CSn_CAITV_Pos) |          // CAITV: Interval between calibration patterns (2^(4+1) = 32 cycles)
                                    (MC80_OSPI_CALIBRATION_NO_OVERWRITE_ENABLE << OSPI_CCCTL0CSn_CANOWR_Pos) |  // CANOWR: No overwrite mode enabled
                                    (MC80_OSPI_CALIBRATION_SHIFT_START_MIN << OSPI_CCCTL0CSn_CASFTSTA_Pos) |    // CASFTSTA: Start shift value (0)
                                    (MC80_OSPI_CALIBRATION_SHIFT_END_MAX << OSPI_CCCTL0CSn_CASFTEND_Pos);       // CASFTEND: Maximum OM_DQS shift value

  // Start auto-calibration
  p_OSPI->CCCTLCS[channel].CCCTL0_b.CAEN = 1;

  // Wait for calibration to complete by checking interrupt flags with timeout
  uint32_t       timeout_count           = 0;
  const uint32_t CALIBRATION_TIMEOUT     = 100000;  // Timeout counter limit

  OSPI_DEBUG_LOOP_START(OSPI_DEBUG_LOOP_AUTO_CALIBRATION_WAIT);
  while ((0 == ((p_OSPI->INTS >> (OSPI_INTS_CASUCCS0_Pos + channel)) & 0x01)) &&
         (0 == ((p_OSPI->INTS >> (OSPI_INTS_CAFAILCS0_Pos + channel)) & 0x01)) &&
         (timeout_count < CALIBRATION_TIMEOUT))
  {
    OSPI_DEBUG_LOOP_ITERATION();
    __NOP();  // Breakpoint for calibration wait
    timeout_count++;
  }
  OSPI_DEBUG_LOOP_END(OSPI_DEBUG_LOOP_AUTO_CALIBRATION_WAIT);

  // Disable automatic calibration
  p_OSPI->CCCTLCS[channel].CCCTL0_b.CAEN = 0;

  // Check calibration result
  if (timeout_count >= CALIBRATION_TIMEOUT)
  {
    // Calibration timed out
    ret = FSP_ERR_CALIBRATE_FAILED;
  }
  else if (1 == ((p_OSPI->INTS >> (OSPI_INTS_CASUCCS0_Pos + channel)) & 0x01))
  {
    // Calibration succeeded
    ret          = FSP_SUCCESS;
    // Clear automatic calibration success status
    p_OSPI->INTC = (uint32_t)1 << (OSPI_INTS_CASUCCS0_Pos + channel);
  }
  else if (1 == ((p_OSPI->INTS >> (OSPI_INTS_CAFAILCS0_Pos + channel)) & 0x01))
  {
    // Calibration failed
    ret          = FSP_ERR_CALIBRATE_FAILED;
    // Clear automatic calibration failure status
    p_OSPI->INTC = (uint32_t)1 << (OSPI_INTS_CAFAILCS0_Pos + channel);
  }
  else
  {
    // Unexpected state - neither success nor failure flag set
    ret = FSP_ERR_CALIBRATE_FAILED;
  }

  // Save timing parameters after calibration if structure provided
  if (NULL != p_calibration_data)
  {
    // Set calibration result
    p_calibration_data->calibration_success = (FSP_SUCCESS == ret);

    // Save DQS shift value from WRAPCFG register
    if (MC80_OSPI_DEVICE_NUMBER_0 == channel)
    {
      p_calibration_data->after_calibration.wrapcfg_dssft = (p_OSPI->WRAPCFG & OSPI_WRAPCFG_DSSFTCS0_Msk) >> OSPI_WRAPCFG_DSSFTCS0_Pos;
    }
    else
    {
      p_calibration_data->after_calibration.wrapcfg_dssft = (p_OSPI->WRAPCFG & OSPI_WRAPCFG_DSSFTCS1_Msk) >> OSPI_WRAPCFG_DSSFTCS1_Pos;
    }

    // Save SDR and DDR timing parameters from LIOCFGCS register
    uint32_t liocfg_value                                  = p_OSPI->LIOCFGCS[channel];
    p_calibration_data->after_calibration.liocfg_sdrsmpsft = (liocfg_value & OSPI_LIOCFGCSN_SDRSMPSFT_Msk) >> OSPI_LIOCFGCSN_SDRSMPSFT_Pos;
    p_calibration_data->after_calibration.liocfg_ddrsmpex  = (liocfg_value & OSPI_LIOCFGCSN_DDRSMPEX_Msk) >> OSPI_LIOCFGCSN_DDRSMPEX_Pos;

    // Save calibration status after calibration
    p_calibration_data->after_calibration.casttcs_value    = p_OSPI->CASTTCS[channel];
  }

  return ret;
}

/*-----------------------------------------------------------------------------------------------------
  Description: Read JEDEC identification data from flash device using RDID command (0x9F)
               Returns Manufacturer ID, Memory Type, and Memory Density

  Parameters: p_ctrl - Pointer to instance control structure
              p_id - Pointer to buffer for ID data (minimum 3 bytes)
              id_length - Number of ID bytes to read (typically 3)

  Return: FSP_SUCCESS - ID read successfully
          FSP_ERR_ASSERTION - Invalid parameters
          Error code from direct transfer operation
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_read_id(T_mc80_ospi_instance_ctrl *const p_ctrl, uint8_t *const p_id, uint32_t id_length)
{
  // Parameter validation
  if ((NULL == p_ctrl) || (NULL == p_id) || (0 == id_length) || (id_length > 8))
  {
    return FSP_ERR_ASSERTION;
  }

  // Ensure the instance is open
  if (MC80_OSPI_PRV_OPEN != p_ctrl->open)
  {
    return FSP_ERR_NOT_OPEN;
  }

  // Clear ID buffer
  for (uint32_t i = 0; i < id_length; i++)
  {
    p_id[i] = 0;
  }

  // Setup direct transfer structure for RDID command
  T_mc80_ospi_direct_transfer transfer = { 0 };
  transfer.command                     = MX25_CMD_RDID;       // 0x9F - Read Identification command
  transfer.command_length              = 1;                   // Single byte command
  transfer.address                     = 0;                   // No address for RDID
  transfer.address_length              = 0;                   // No address bytes
  transfer.data_length                 = (uint8_t)id_length;  // Number of ID bytes to read
  transfer.dummy_cycles                = 0;                   // No dummy cycles for RDID

  // Execute read command using direct transfer
  fsp_err_t err                        = Mc80_ospi_direct_transfer(p_ctrl, &transfer, MC80_OSPI_DIRECT_TRANSFER_DIR_READ);
  if (FSP_SUCCESS != err)
  {
    return err;
  }

  // Copy received data to output buffer
  for (uint32_t i = 0; i < id_length; i++)
  {
    p_id[i] = transfer.data_bytes[i];
  }

  return FSP_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Configures the device to enter or exit XiP mode

  NOTE: For the MX25UM25645G flash memory chip used in this project, XIP mode manipulation
  is not required. The chip can operate in continuous memory-mapped mode without explicit
  XIP enter/exit commands. This function is provided for compatibility but may not be
  necessary for normal operation with our memory chip.

  Parameters:
    p_ctrl - Pointer to the instance ctrl struct
    is_entering     - true if entering XiP mode, false if exiting

  Return:
    void
-----------------------------------------------------------------------------------------------------*/
static void _Mc80_ospi_xip(T_mc80_ospi_instance_ctrl *p_ctrl, bool is_entering)
{
  R_XSPI0_Type *const    p_reg = p_ctrl->p_reg;
  const T_mc80_ospi_cfg *p_cfg = p_ctrl->p_cfg;
  volatile uint8_t      *p_dummy_read_address;
  volatile uint8_t       dummy_read = 0;

  if (MC80_OSPI_DEVICE_NUMBER_0 == p_ctrl->channel)
  {
    p_dummy_read_address = (volatile uint8_t *)MC80_OSPI_DEVICE_0_START_ADDRESS;
  }
  else
  {
    p_dummy_read_address = (volatile uint8_t *)MC80_OSPI_DEVICE_1_START_ADDRESS;
  }

// Clear the pre-fetch buffer for this bank so the next read is guaranteed to use the XiP code
#if MC80_OSPI_CFG_PREFETCH_FUNCTION
  p_reg->BMCTL1 |= 0x03U << OSPI_BMCTL1_PBUFCLRCH0_Pos;
#endif

  // Wait for any on-going access to complete
  OSPI_DEBUG_LOOP_START(OSPI_DEBUG_LOOP_XIP_ACCESS_COMPLETION_WAIT);
  while ((p_reg->COMSTT & MC80_OSPI_PRV_COMSTT_MEMACCCH_MASK) != 0)
  {
    OSPI_DEBUG_LOOP_ITERATION();
    __NOP();  // Breakpoint for access completion wait
  }
  OSPI_DEBUG_LOOP_END(OSPI_DEBUG_LOOP_XIP_ACCESS_COMPLETION_WAIT);

  if (is_entering)
  {
    // Change memory-mapping to read-only mode (preserve bits 4-7)
    p_reg->BMCTL0          = MC80_OSPI_PRV_BMCTL0_READ_ONLY_VALUE;

    // Configure XiP codes and enable
    const uint32_t cmctlch = OSPI_CMCTLCHn_XIPEN_Msk |
                             ((uint32_t)(p_cfg->xip_enter_command << OSPI_CMCTLCHn_XIPENCODE_Pos)) |
                             ((uint32_t)(p_cfg->xip_exit_command << OSPI_CMCTLCHn_XIPEXCODE_Pos));

    // XiP enter/exit codes are configured only for memory mapped operations and affects both OSPI slave channels
    p_reg->CMCTLCH[0] = cmctlch;
    p_reg->CMCTLCH[1] = cmctlch;

    // Perform a read to send the enter code. All further reads will use the enter code and will not send a read command code
    dummy_read        = *p_dummy_read_address;

    // Wait for the read to complete
    OSPI_DEBUG_LOOP_START(OSPI_DEBUG_LOOP_XIP_ENTER_READ_WAIT);
    while ((p_reg->COMSTT & MC80_OSPI_PRV_COMSTT_MEMACCCH_MASK) != 0)
    {
      OSPI_DEBUG_LOOP_ITERATION();
      __NOP();  // Breakpoint for XIP enter read wait
    }
    OSPI_DEBUG_LOOP_END(OSPI_DEBUG_LOOP_XIP_ENTER_READ_WAIT);
  }
  else
  {
    // Disable XiP
    p_reg->CMCTLCH[0] &= ~OSPI_CMCTLCHn_XIPEN_Msk;
    p_reg->CMCTLCH[1] &= ~OSPI_CMCTLCHn_XIPEN_Msk;

    // Perform a read to send the exit code. All further reads will not send an exit code
    dummy_read = *p_dummy_read_address;

    // Wait for the read to complete
    OSPI_DEBUG_LOOP_START(OSPI_DEBUG_LOOP_XIP_EXIT_READ_WAIT);
    while ((p_reg->COMSTT & MC80_OSPI_PRV_COMSTT_MEMACCCH_MASK) != 0)
    {
      OSPI_DEBUG_LOOP_ITERATION();
      __NOP();  // Breakpoint for XIP exit read wait
    }
    OSPI_DEBUG_LOOP_END(OSPI_DEBUG_LOOP_XIP_EXIT_READ_WAIT);

    // Change memory-mapping back to R/W mode (preserve bits 4-7)
    p_reg->BMCTL0 = MC80_OSPI_PRV_BMCTL0_READ_WRITE_VALUE;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Perform hardware reset of the flash memory device using LIOCTL register RSTCS0/RSTCS1 bit.

  This function performs a true hardware reset by toggling the reset pin through the OSPI controller.
  The reset sequence:
  1. Assert reset pin (set RST bit to 1)
  2. Hold reset for minimum 20us
  3. Deassert reset pin (set RST bit to 0)
  4. Wait 100us for flash device initialization

  After hardware reset, the flash device returns to its default state (SPI mode, CR2=0x00).
  This provides a more reliable reset than software commands.

  Parameters: p_ctrl - Pointer to OSPI instance control structure

  Return: FSP_SUCCESS - Reset completed successfully
          FSP_ERR_ASSERTION - Invalid parameters
          FSP_ERR_NOT_OPEN - Driver is not opened
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_hardware_reset(T_mc80_ospi_instance_ctrl *const p_ctrl)
{
  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }
  }

  R_XSPI0_Type *const p_reg = p_ctrl->p_reg;

  // Hardware reset sequence using LIOCTL register RSTCS0 bit
  // Assert reset (clear RSTCS0 bit - active low reset)
  p_reg->LIOCTL &= ~OSPI_LIOCTL_RSTCS0_Msk;

  // Hold reset for minimum 20us (using 50us for safety margin)
  R_BSP_SoftwareDelay(50, BSP_DELAY_UNITS_MICROSECONDS);

  // Deassert reset (set RSTCS0 bit - release reset)
  p_reg->LIOCTL |= OSPI_LIOCTL_RSTCS0_Msk;

  // Wait for flash device initialization to complete (minimum 100us)
  R_BSP_SoftwareDelay(200, BSP_DELAY_UNITS_MICROSECONDS);

  return FSP_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  DMA callback function for OSPI operations

  This callback is called when DMA transfer completes and sends RTOS event notifications to
  wake up waiting tasks. The callback is safe to call from interrupt context as it only
  sends RTOS notifications.

  The p_args parameter contains a dmac_callback_args_t structure with a single p_context field.
  This context pointer can be configured in MC80_OSPI_config.c in the g_transfer_OSPI_extend
  structure's .p_context field to pass custom data to the callback. The context can point to:
  - A control structure for state management
  - A synchronization object (mutex, semaphore, event flags)
  - Custom user data for callback processing
  - NULL if no context is needed (current configuration)

  The dmac_int_isr() function creates the callback args structure and copies the configured
  p_context value from the DMA configuration into p_args->p_context before calling this callback.

  Parameters:
    p_args - Pointer to dmac_callback_args_t structure containing:
             p_args->p_context - User-configurable context pointer set in MC80_OSPI_config.c

  Return:
    void
-----------------------------------------------------------------------------------------------------*/
void OSPI_dma_callback(dmac_callback_args_t *p_args)
{
  // Send RTOS event notification if event flags are initialized
  if (g_ospi_dma_event_flags_initialized)
  {
    // Set completion event flag - this will wake up any waiting tasks
    tx_event_flags_set(&g_ospi_dma_event_flags, OSPI_DMA_EVENT_TRANSFER_COMPLETE, TX_OR);
  }
}

/*-----------------------------------------------------------------------------------------------------
  Reset DMA transfer status flags

  Parameters:
    None

  Return:
    void
-----------------------------------------------------------------------------------------------------*/
void Mc80_ospi_dma_transfer_reset_flags(void)
{
  // Clear RTOS event flags if initialized
  if (g_ospi_dma_event_flags_initialized)
  {
    tx_event_flags_set(&g_ospi_dma_event_flags, ~OSPI_DMA_EVENT_ALL_EVENTS, TX_AND);
  }
}

/*-----------------------------------------------------------------------------------------------------
  Initialize RTOS event flags for DMA notifications

  This function must be called after RTOS kernel initialization but before using DMA operations.
  It creates the event flags group used for task synchronization with DMA completion.

  Parameters:
    None

  Return:
    FSP_SUCCESS - Event flags initialized successfully
    FSP_ERR_INTERNAL - Failed to create event flags group
-----------------------------------------------------------------------------------------------------*/
static fsp_err_t _Ospi_dma_event_flags_initialize(void)
{
  if (!g_ospi_dma_event_flags_initialized)
  {
    UINT tx_result = tx_event_flags_create(&g_ospi_dma_event_flags, "OSPI_DMA_Events");
    if (TX_SUCCESS == tx_result)
    {
      g_ospi_dma_event_flags_initialized = true;
      return FSP_SUCCESS;
    }
    else
    {
      return FSP_ERR_INTERNAL;
    }
  }

  return FSP_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Cleanup RTOS resources for OSPI DMA operations

  This internal function cleans up the RTOS event flags group when the driver is closed.
  It should only be called when no other tasks are using the OSPI driver.

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void _Ospi_dma_event_flags_cleanup(void)
{
  if (g_ospi_dma_event_flags_initialized)
  {
    tx_event_flags_delete(&g_ospi_dma_event_flags);
    g_ospi_dma_event_flags_initialized = false;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Get RTOS event flags group for DMA operations

  This function provides access to the event flags group for tasks that need to wait for
  DMA completion. Tasks can use tx_event_flags_get() to wait for specific events.

  Example usage in task:
  ULONG actual_flags;
  UINT result = tx_event_flags_get(&flags_group, OSPI_DMA_EVENT_TRANSFER_COMPLETE,
                                   TX_OR_CLEAR, &actual_flags, TX_WAIT_FOREVER);

  Parameters:
    pp_event_flags - Pointer to store the event flags group pointer

  Return:
    FSP_SUCCESS - Event flags group is available
    FSP_ERR_NOT_INITIALIZED - Event flags not initialized, call Mc80_ospi_dma_event_flags_initialize() first
    FSP_ERR_ASSERTION - Invalid parameter
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_dma_get_event_flags(TX_EVENT_FLAGS_GROUP **pp_event_flags)
{
  if (NULL == pp_event_flags)
  {
    return FSP_ERR_ASSERTION;
  }

  if (!g_ospi_dma_event_flags_initialized)
  {
    return FSP_ERR_NOT_INITIALIZED;
  }

  *pp_event_flags = &g_ospi_dma_event_flags;
  return FSP_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Wait for DMA transfer completion using RTOS event flags

  This function provides a blocking wait for DMA transfer completion using RTOS synchronization.
  It's more efficient than polling and allows the task to be suspended until the transfer completes.

  Parameters:
    timeout_ticks - Maximum time to wait in RTOS ticks (TX_WAIT_FOREVER for infinite wait)

  Return:
    FSP_SUCCESS - Transfer completed successfully
    FSP_ERR_TIMEOUT - Timeout occurred before transfer completion
    FSP_ERR_NOT_INITIALIZED - RTOS support not initialized
    FSP_ERR_INTERNAL - RTOS error occurred
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_dma_wait_for_completion(ULONG timeout_ticks)
{
  if (!g_ospi_dma_event_flags_initialized)
  {
    return FSP_ERR_NOT_INITIALIZED;
  }

  ULONG actual_flags;
  UINT  tx_result = tx_event_flags_get(&g_ospi_dma_event_flags,
                                       OSPI_DMA_EVENT_TRANSFER_COMPLETE,
                                       TX_OR_CLEAR,
                                       &actual_flags,
                                       timeout_ticks);

  if (TX_SUCCESS == tx_result)
  {
    return FSP_SUCCESS;
  }
  else if (TX_NO_EVENTS == tx_result)
  {
    return FSP_ERR_TIMEOUT;
  }
  else
  {
    return FSP_ERR_INTERNAL;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Description: Captures a snapshot of all OSPI peripheral registers for debugging and analysis

  This function reads and stores the current state of all OSPI (XSPI) peripheral registers
  for both channels. This is useful for debugging protocol switching, calibration issues,
  timing analysis, and general troubleshooting of OSPI operations.

  Register Categories Captured:
  - Control registers (LIOCTL, WRAPCFG, BMCTL0, BMCTL1)
  - Channel configuration (LIOCFGCS, CMCFGCS for both channels)
  - Calibration registers (CCCTLCS for both channels)
  - Status and interrupt registers (INTS, COMSTT)
  - Command buffer registers (CDBUF for both channels)
  - Timing and protocol configuration

  Usage Examples:
  - Before/after protocol switching to verify register changes
  - During calibration to analyze timing parameters
  - When debugging read/write failures
  - For performance analysis and optimization

  Parameters:
    p_ctrl - Pointer to OSPI instance control structure
    p_snapshot - Pointer to structure to store register snapshot

  Return:
    FSP_SUCCESS - Register snapshot captured successfully
    FSP_ERR_ASSERTION - Invalid parameters
    FSP_ERR_NOT_OPEN - Driver is not opened
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_capture_register_snapshot(T_mc80_ospi_instance_ctrl *const p_ctrl, T_mc80_ospi_register_snapshot *const p_snapshot)
{
  // Parameter validation
  if ((NULL == p_ctrl) || (NULL == p_snapshot))
  {
    return FSP_ERR_ASSERTION;
  }

  // Ensure the instance is open
  if (MC80_OSPI_PRV_OPEN != p_ctrl->open)
  {
    return FSP_ERR_NOT_OPEN;
  }

  R_XSPI0_Type *const p_reg = p_ctrl->p_reg;

  // Clear the snapshot structure
  memset(p_snapshot, 0, sizeof(T_mc80_ospi_register_snapshot));

  // Capture timestamp
  p_snapshot->timestamp        = tx_time_get();
  p_snapshot->channel          = p_ctrl->channel;
  p_snapshot->current_protocol = p_ctrl->spi_protocol;

  // === Control and Configuration Registers ===
  p_snapshot->lioctl           = p_reg->LIOCTL;
  p_snapshot->wrapcfg          = p_reg->WRAPCFG;
  p_snapshot->comcfg           = p_reg->COMCFG;
  p_snapshot->bmcfgch[0]       = p_reg->BMCFGCH[0];
  p_snapshot->bmcfgch[1]       = p_reg->BMCFGCH[1];
  p_snapshot->bmctl0           = p_reg->BMCTL0;
  p_snapshot->bmctl1           = p_reg->BMCTL1;
  p_snapshot->abmcfg           = p_reg->ABMCFG;

  // === Channel Configuration Registers (Both Channels) ===
  for (int ch = 0; ch < 2; ch++)
  {
    p_snapshot->liocfgcs[ch]       = p_reg->LIOCFGCS[ch];
    p_snapshot->cmcfgcs[ch].cmcfg0 = p_reg->CMCFGCS[ch].CMCFG0;
    p_snapshot->cmcfgcs[ch].cmcfg1 = p_reg->CMCFGCS[ch].CMCFG1;
    p_snapshot->cmcfgcs[ch].cmcfg2 = p_reg->CMCFGCS[ch].CMCFG2;
  }

  // === Calibration Control Registers (Both Channels) ===
  for (int ch = 0; ch < 2; ch++)
  {
    p_snapshot->ccctlcs[ch].ccctl0 = p_reg->CCCTLCS[ch].CCCTL0;
    p_snapshot->ccctlcs[ch].ccctl1 = p_reg->CCCTLCS[ch].CCCTL1;
    p_snapshot->ccctlcs[ch].ccctl2 = p_reg->CCCTLCS[ch].CCCTL2;
    p_snapshot->ccctlcs[ch].ccctl3 = p_reg->CCCTLCS[ch].CCCTL3;
    p_snapshot->ccctlcs[ch].ccctl4 = p_reg->CCCTLCS[ch].CCCTL4;
    p_snapshot->ccctlcs[ch].ccctl5 = p_reg->CCCTLCS[ch].CCCTL5;
    p_snapshot->ccctlcs[ch].ccctl6 = p_reg->CCCTLCS[ch].CCCTL6;
    p_snapshot->ccctlcs[ch].ccctl7 = p_reg->CCCTLCS[ch].CCCTL7;
  }

  // === Status and Interrupt Registers ===
  p_snapshot->ints   = p_reg->INTS;
  p_snapshot->intc   = 0;  // INTC is write-only register, set to 0 for reference
  p_snapshot->inte   = p_reg->INTE;
  p_snapshot->comstt = p_reg->COMSTT;
  p_snapshot->verstt = p_reg->VERSTT;

  // === Calibration Status Registers ===
  for (int ch = 0; ch < 2; ch++)
  {
    p_snapshot->casttcs[ch] = p_reg->CASTTCS[ch];
  }

  // === Command Buffer Registers (Both Channels) ===
  for (int ch = 0; ch < 2; ch++)
  {
    p_snapshot->cdbuf[ch].cdt  = p_reg->CDBUF[ch].CDT;
    p_snapshot->cdbuf[ch].cda  = p_reg->CDBUF[ch].CDA;
    p_snapshot->cdbuf[ch].cdd0 = p_reg->CDBUF[ch].CDD0;
    p_snapshot->cdbuf[ch].cdd1 = p_reg->CDBUF[ch].CDD1;
  }

  // === Manual Command Control ===
  p_snapshot->cdctl0 = p_reg->CDCTL0;
  p_snapshot->cdctl1 = p_reg->CDCTL1;
  p_snapshot->cdctl2 = p_reg->CDCTL2;

  // === Link Pattern Control Registers ===
  p_snapshot->lpctl0 = p_reg->LPCTL0;
  p_snapshot->lpctl1 = p_reg->LPCTL1;

  // === XIP Control Registers ===
  for (int ch = 0; ch < 2; ch++)
  {
    p_snapshot->cmctlch[ch] = p_reg->CMCTLCH[ch];
  }

  return FSP_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Start periodic status polling using OSPI hardware automation.

  This function configures the OSPI peripheral to automatically and periodically
  read the flash status register at 100µs intervals for up to 32768 repetitions.
  The polling stops automatically when the specified condition is met.

  Usage Examples:
  1. Wait for WIP bit to clear (write/erase completion):
     _Mc80_ospi_periodic_status_start(p_ctrl, 0x00000000U, 0xFFFFFFFEU);
     or use convenience function: _Mc80_ospi_wait_wip_clear(p_ctrl);

  2. Wait for WEL bit to be set (write enable confirmation):
     _Mc80_ospi_periodic_status_start(p_ctrl, 0x00000002U, 0xFFFFFFFDU);
     or use convenience function: _Mc80_ospi_wait_wel_set(p_ctrl);

  3. Wait for specific status pattern (custom condition):
     _Mc80_ospi_periodic_status_start(p_ctrl, 0x00000040U, 0xFFFFFFBFU); // Wait for bit 6 set

  4. Wait for multiple bits (e.g., WIP=0 AND WEL=1):
     _Mc80_ospi_periodic_status_start(p_ctrl, 0x00000002U, 0xFFFFFFFCU); // Compare bits 0,1

  Parameters:
    p_ctrl - Pointer to the control structure
    expected_value - Expected value for comparison (when condition is met, polling stops)
    comparison_mask - Mask for bit comparison (0 = compare bit, 1 = ignore bit)

  Return:
    FSP_SUCCESS       - Periodic polling started successfully
    FSP_ERR_ASSERTION - Invalid parameters
    FSP_ERR_NOT_OPEN  - Driver is not opened
-----------------------------------------------------------------------------------------------------*/
static fsp_err_t _Mc80_ospi_periodic_status_start(T_mc80_ospi_instance_ctrl *p_ctrl, uint32_t expected_value, uint32_t comparison_mask)
{
  R_XSPI0_Type *const                 p_reg        = p_ctrl->p_reg;
  T_mc80_ospi_xspi_command_set const *p_cmd_set    = p_ctrl->p_cmd_set;
  uint32_t                            cdctl0_value = 0;
  uint16_t                            command;
  uint8_t                             command_length;
  uint8_t                             dummy_cycles;
  bool                                is_8d_mode;
  uint8_t                             address_length = 0U;
  uint32_t                            address        = 0U;
  uint8_t                             data_length;
  uint32_t                            cdtbuf0;

  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl)
    {
      return FSP_ERR_ASSERTION;
    }
    if (MC80_OSPI_PRV_OPEN != p_ctrl->open)
    {
      return FSP_ERR_NOT_OPEN;
    }
  }

  // Skip if no status command is defined
  if (0 == p_cmd_set->status_command)
  {
    return FSP_SUCCESS;
  }

  // First, clear any existing configuration
  p_reg->CDCTL0 = 0;

  // Set channel selection
  cdctl0_value |= (((uint32_t)p_ctrl->channel << OSPI_CDCTL0_CSSEL_Pos) & OSPI_CDCTL0_CSSEL_Msk);

  // Set periodic mode enable
  cdctl0_value |= (MC80_OSPI_PERIODIC_MODE_ENABLE << OSPI_CDCTL0_PERMD_Pos) & OSPI_CDCTL0_PERMD_Msk;

  // Set periodic interval (100µs)
  cdctl0_value |= (MC80_OSPI_PERIODIC_INTERVAL << OSPI_CDCTL0_PERITV_Pos) & OSPI_CDCTL0_PERITV_Msk;

  // Set maximum repetitions (32768)
  cdctl0_value |= (MC80_OSPI_PERIODIC_REPETITIONS_MAX << OSPI_CDCTL0_PERREP_Pos) & OSPI_CDCTL0_PERREP_Msk;

  // Prepare command parameters for status reading
  command        = p_cmd_set->status_command;
  command_length = (uint8_t)p_cmd_set->command_bytes;
  dummy_cycles   = p_cmd_set->status_dummy_cycles;
  is_8d_mode     = (MC80_OSPI_PROTOCOL_8D_8D_8D == p_ctrl->spi_protocol);

  // Configure address parameters
  if (p_cmd_set->status_needs_address)
  {
    address_length = (uint8_t)(p_cmd_set->status_address_bytes + 1U);
    address        = p_cmd_set->status_address;
  }
  else if (is_8d_mode)
  {
    // 8D-8D-8D mode requires an address for any read operation
    address_length = (uint8_t)(p_cmd_set->address_bytes + 1U);
  }

  // Configure data length: 8D-8D-8D mode requires 2-byte reads, SPI mode uses 1 byte
  data_length = is_8d_mode ? 2U : 1U;

  // Build the Command Data Buffer configuration register value
  cdtbuf0 =
  (((uint32_t)command_length << OSPI_CDTBUFn_CMDSIZE_Pos) & OSPI_CDTBUFn_CMDSIZE_Msk) |
  (((uint32_t)address_length << OSPI_CDTBUFn_ADDSIZE_Pos) & OSPI_CDTBUFn_ADDSIZE_Msk) |
  (((uint32_t)data_length << OSPI_CDTBUFn_DATASIZE_Pos) & OSPI_CDTBUFn_DATASIZE_Msk) |
  (((uint32_t)dummy_cycles << OSPI_CDTBUFn_LATE_Pos) & OSPI_CDTBUFn_LATE_Msk) |
  (((uint32_t)MC80_OSPI_DIRECT_TRANSFER_DIR_READ << OSPI_CDTBUFn_TRTYPE_Pos) & OSPI_CDTBUFn_TRTYPE_Msk);

  // Handle command code positioning based on command length
  if (1 == command_length)
  {
    cdtbuf0 |= (command & MC80_OSPI_PRV_CDTBUF_CMD_1B_VALUE_MASK) << MC80_OSPI_PRV_CDTBUF_CMD_UPPER_OFFSET;
  }
  else
  {
    cdtbuf0 |= (command & MC80_OSPI_PRV_CDTBUF_CMD_2B_VALUE_MASK) << MC80_OSPI_PRV_CDTBUF_CMD_OFFSET;
  }

  // Configure command buffer for status reading (use buffer 0 like in direct transfer)
  p_reg->CDBUF[0].CDT = cdtbuf0;

  if (address_length > 0)
  {
    p_reg->CDBUF[0].CDA = address;
  }

  // Configure periodic transaction expected value (CDCTL1.PEREXP)
  // Set the expected value passed as parameter
  p_reg->CDCTL1                       = expected_value;

  // Configure periodic transaction mask (CDCTL2.PERMSK)
  // Set the comparison mask passed as parameter
  // Bits set to 1 are IGNORED, bits set to 0 are COMPARED
  p_reg->CDCTL2                       = comparison_mask;        // Clear any existing interrupts before starting

  p_reg->INTC                         = OSPI_INTC_CMDCMPC_Msk;  // Clear OSPI interrupt flags
  R_ICU->IELSR_b[OSPI_CMDCMP_IRQn].IR = 0;                      // Clear any pending interrupt flag
  NVIC_ClearPendingIRQ(OSPI_CMDCMP_IRQn);                       // Clear NVIC pending interrupt
  p_reg->INTE |= OSPI_INTE_CMDCMPE_Msk;                         // Enable command completion interrupt

  // Write periodic control configuration and start periodic polling
  p_reg->CDCTL0 = cdctl0_value | OSPI_CDCTL0_TRREQ_Msk;

  return FSP_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------
  Stop periodic status polling.

  This function stops the OSPI hardware periodic polling and disables the
  command completion interrupt.

  Parameters:
    p_ctrl - Pointer to the control structure

  Return: None
-----------------------------------------------------------------------------------------------------*/
static void _Mc80_ospi_periodic_status_stop(T_mc80_ospi_instance_ctrl *p_ctrl)
{
  if (MC80_OSPI_CFG_PARAM_CHECKING_ENABLE)
  {
    if (NULL == p_ctrl || MC80_OSPI_PRV_OPEN != p_ctrl->open)
    {
      return;
    }
  }

  R_XSPI0_Type *const p_reg = p_ctrl->p_reg;

  // Disable periodic mode
  p_reg->CDCTL0 &= ~OSPI_CDCTL0_PERMD_Msk;

  // Clear periodic transaction registers
  p_reg->CDCTL1 = 0U;  // Clear expected value
  p_reg->CDCTL2 = 0U;  // Clear mask value

  // Disable command completion interrupt
  p_reg->INTE &= ~OSPI_INTE_CMDCMPE_Msk;

  // Clear any pending interrupt flags
  p_reg->INTC |= OSPI_INTC_CMDCMPC_Msk;
}

/*-----------------------------------------------------------------------------------------------------
  Wait for WIP (Write In Progress) bit to clear using hardware periodic polling.

  This is a convenience function that calls _Mc80_ospi_periodic_status_start with predefined
  parameters for waiting until the flash device finishes write/erase operations.

  Parameters:
    p_ctrl - Pointer to the control structure

  Return:
    FSP_SUCCESS       - Periodic polling started successfully
    FSP_ERR_ASSERTION - Invalid parameters
    FSP_ERR_NOT_OPEN  - Driver is not opened
-----------------------------------------------------------------------------------------------------*/
fsp_err_t Mc80_ospi_cmdcmp_wait_for_completion(ULONG timeout_ticks)
{
  if (!g_ospi_dma_event_flags_initialized)
  {
    return FSP_ERR_NOT_INITIALIZED;
  }

  ULONG actual_flags = 0;
  UINT  status       = tx_event_flags_get(&g_ospi_dma_event_flags,
                                          OSPI_CMDCMP_EVENT_ALL_EVENTS,
                                          TX_OR_CLEAR,
                                          &actual_flags,
                                          timeout_ticks);

  if (TX_SUCCESS != status)
  {
    return FSP_ERR_TIMEOUT;
  }

  if (actual_flags & OSPI_CMDCMP_EVENT_ERROR)
  {
    return FSP_ERR_WRITE_FAILED;
  }

  return FSP_SUCCESS;
}

#if MC80_OSPI_DEBUG_WAIT_LOOPS

/*-----------------------------------------------------------------------------------------------------
  Reset all OSPI wait loop debug statistics to zero.

  This function clears all accumulated timing data and resets counters to allow fresh measurements.
  Useful for benchmarking specific operations or clearing old data.

  Parameters: None

  Return: None
-----------------------------------------------------------------------------------------------------*/
void Mc80_ospi_debug_reset_statistics(void)
{
  // Preserve system settings but clear all measurement data
  uint32_t     saved_clock   = g_ospi_debug.system_clock_hz;
  uint32_t     saved_enabled = g_ospi_debug.measurement_enabled;
  const char **saved_names   = (const char **)g_ospi_debug.loop_names;

  // Clear all statistics
  memset(&g_ospi_debug, 0, sizeof(g_ospi_debug));

  // Restore settings
  g_ospi_debug.system_clock_hz     = saved_clock;
  g_ospi_debug.measurement_enabled = saved_enabled;
  memcpy((void *)g_ospi_debug.loop_names, saved_names, sizeof(g_ospi_debug.loop_names));

  // Initialize min_cycles to maximum value for proper minimum detection
  for (uint32_t i = 0; i < OSPI_DEBUG_LOOP_COUNT; i++)
  {
    g_ospi_debug.loops[i].min_cycles = UINT32_MAX;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Enable or disable OSPI wait loop timing measurements.

  Parameters:
    enable - true to enable measurements, false to disable

  Return: Previous enable state (true/false)
-----------------------------------------------------------------------------------------------------*/
bool Mc80_ospi_debug_enable_measurements(bool enable)
{
  bool previous_state              = (g_ospi_debug.measurement_enabled != 0);
  g_ospi_debug.measurement_enabled = enable ? 1 : 0;
  return previous_state;
}

/*-----------------------------------------------------------------------------------------------------
  Get pointer to the debug statistics structure for live monitoring in debugger.

  This function provides access to the complete debug structure containing all timing measurements.
  The returned pointer can be added to debugger watch window for live monitoring of loop performance.

  Example debugger usage:
  1. Add "g_ospi_debug" to watch window for live view
  2. Expand structure to see individual loop statistics
  3. Monitor .last_cycles for real-time timing
  4. Check .max_cycles for worst-case performance
  5. Use .avg_cycles for typical performance analysis

  Parameters: None

  Return: Pointer to debug statistics structure (never NULL)
-----------------------------------------------------------------------------------------------------*/
T_ospi_debug_structure *Mc80_ospi_debug_get_statistics(void)
{
  return &g_ospi_debug;
}

/*-----------------------------------------------------------------------------------------------------
  Calculate time in microseconds from CPU cycles using current system clock.

  Parameters:
    cycles - Number of CPU cycles

  Return: Time in microseconds (as floating-point for sub-microsecond precision)
-----------------------------------------------------------------------------------------------------*/
float Mc80_ospi_debug_cycles_to_microseconds(uint32_t cycles)
{
  return ((float)cycles * 1000000.0f) / (float)g_ospi_debug.system_clock_hz;
}

#endif  // MC80_OSPI_DEBUG_WAIT_LOOPS
