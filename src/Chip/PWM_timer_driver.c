#include "App.h"

/*
  ===============================================================================
  PWM Timer Driver for MC80 - Motor Phase Pin Assignment
  ===============================================================================

  Motor Driver 1 (MD1) - Pin Connections:
  ─────────────────────────────────────────
  • Phase U High (UH): P415 (GTIOC0A) - GPT0 Channel A output
  • Phase U Low  (UL): P414 (GTIOC0B) - GPT0 Channel B output
  • Phase V High (VH): P105 (GTIOC1A) - GPT1 Channel A output
  • Phase V Low  (VL): P208 (GTIOC1B) - GPT1 Channel B output
  • Phase W High (WH): P113 (GTIOC2A) - GPT2 Channel A output
  • Phase W Low  (WL): P114 (GTIOC2B) - GPT2 Channel B output

  Motor Driver 2 (MD2) - Pin Connections:
  ─────────────────────────────────────────
  • Phase U High (UH): P300 (GTIOC3A) - GPT3 Channel A output
  • Phase U Low  (UL): P112 (GTIOC3B) - GPT3 Channel B output
  • Phase V High (VH): P205 (GTIOC4A) - GPT4 Channel A output
  • Phase V Low  (VL): P204 (GTIOC4B) - GPT4 Channel B output
  • Phase W High (WH): P700 (GTIOC5A) - GPT5 Channel A output
  • Phase W Low  (WL): P701 (GTIOC5B) - GPT5 Channel B output  PWM Operation:
  ─────────────────
  • GPT0-GPT5 are used for triangle PWM generation with dead time
  • Motor Driver 1 uses GPT0, GPT1, GPT2 for phases U, V, W respectively
  • Motor Driver 2 uses GPT3, GPT4, GPT5 for phases U, V, W respectively
  • Each phase has complementary outputs (High/Low) with dead time protection
  • Pin functions can be switched between GPIO and PWM modes via PFS registers
  • Universal function _Set_pwm_pin_output_mode() controls all motor phases

  ===============================================================================
*/

// ---- ADC trigger timing advance constants ----
// ADC trigger advance values for optimal sampling timing
// With FRQ_PCLKD_MHZ MHz clock, each tick = 8.33 ns
#define ADC_TRIGGER_ADVANCE_nS    133                                                // ADC_TRIGGER_ADVANCE_nS ns advance time for optimal ADC sampling
#define ADC_TRIGGER_ADVANCE_TICKS ((ADC_TRIGGER_ADVANCE_nS * FRQ_PCLKD_MHZ) / 1000)  // Calculate ticks from nanoseconds

// Debug support for PWM values
// #define PWM_DEBUG_VALUES          1  // Set to 1 to enable debug storage, 0 to disable

#if PWM_DEBUG_VALUES
// Debug array to store comparator values for debugging purposes
// Index: [motor][phase]
uint32_t g_debug_gtccr0_values[DRIVER_COUNT][PHASE_COUNT] = {0};

// Debug array to store GTCCR[PHASE_COUNT] register values for debugging purposes
// Index: [motor][phase]
uint32_t g_debug_gtccr2_values[DRIVER_COUNT][PHASE_COUNT] = {0};
#endif

volatile uint8_t gpt0_out_mode     = FL_PHASE_PWM_ON;
volatile uint8_t gpt1_out_mode     = FL_PHASE_PWM_ON;
volatile uint8_t gpt2_out_mode     = FL_PHASE_PWM_ON;
volatile uint8_t gpt3_out_mode     = FL_PHASE_PWM_ON;  // Motor Driver 2 Phase U
volatile uint8_t gpt4_out_mode     = FL_PHASE_PWM_ON;  // Motor Driver 2 Phase V
volatile uint8_t gpt5_out_mode     = FL_PHASE_PWM_ON;  // Motor Driver 2 Phase W

// Global phase state arrays for diagnostic access
// Index: [motor][phase]
volatile uint8_t g_phase_state_0_percent[DRIVER_COUNT][PHASE_COUNT]   = {{0}};  // 0% duty state: GTIOA=LOW, GTIOB=HIGH
volatile uint8_t g_phase_state_100_percent[DRIVER_COUNT][PHASE_COUNT] = {{0}};  // 100% duty state: GTIOA=HIGH, GTIOB=LOW
volatile uint8_t g_phase_state_pwm_mode[DRIVER_COUNT][PHASE_COUNT]    = {{0}};  // PWM mode: both force modes disabled
volatile uint8_t g_phase_state_Z_stage[DRIVER_COUNT][PHASE_COUNT]     = {{0}};  // Z-state: both signals LOW (high impedance)

static uint32_t g_pwm_period_ticks = 0;                // GPT timer period register value in PCLKD clock ticks for triangle PWM mode
static uint32_t pwm_indx_to_comp[PWM_STEP_COUNT];      // PWM modulation index to comparator value lookup table

// Global array for GPT timer register access
// Index: [motor][phase]
static R_GPT0_Type *const g_gpt_registers[DRIVER_COUNT][PHASE_COUNT] =
{
 {R_GPT0, R_GPT1, R_GPT2},  // Motor 1: GPT0-GPT2 for phases U, V, W
 {R_GPT3, R_GPT4, R_GPT5}   // Motor 2: GPT3-GPT5 for phases U, V, W
};

// Forward static declarations
static void            _PWM_triangle_buffered_init(R_GPT0_Type *R_GPT);
static void            _PWM_set_ADC_trigger(T_pwm_adc_trigger_select adc_module);
static void            _Fill_PWM_comparator_table(void);
FORCE_INLINE_ATTR void _Set_pwm_enhanced(uint8_t motor, uint8_t phase, uint32_t pwm_level, uint8_t output_enable);
#ifdef DEBUG_ADC_SAMPLING_MODE
static void _Init_gpt0_gtadsmr1(void);
#endif

/*-----------------------------------------------------------------------------------------------------
  Configure timer for symmetric triangle PWM mode 2 with dead time and enhanced safety features
  Incorporates advanced features from Synergy implementation adapted for RA8M1 architecture:
  - Triangle PWM mode 2 for enhanced double buffer support
  - Simultaneous high-level output disable protection (GRPABH)
  - Dead time insertion with automatic up/down synchronization
  - Buffered operation for smooth PWM updates

  Parameters:
    R_GPT - pointer to timer register struct

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _PWM_triangle_buffered_init(R_GPT0_Type *R_GPT)
{
  // Enable write access to GPT registers (RA8M1 equivalent of Synergy GTWP)
  // Note: RA8M1 does not have GTWP register, write protection is handled differently

  // Stop counter first using typed temporary variable
  T_gtcr_bits gtcr_temp         = *(T_gtcr_bits *)&R_GPT->GTCR;
  gtcr_temp.CST                 = 0;                   // Stop counter
  R_GPT->GTCR                   = *(uint32_t *)&gtcr_temp;
  R_GPT->GTCNT                  = 0;                   // Reset counter
  R_GPT->GTPR                   = g_pwm_period_ticks;  // Set period
  R_GPT->GTIOR                  = 0;                   // Clear output settings

  R_GPT->GTUPSR                 = 0;                   // Up count source select
  R_GPT->GTDNSR                 = 0;                   // Down count source select

  // Set up/down count direction and glitch-free resume bits using typed temporary variable
  T_gtuddtyc_bits gtuddtyc_temp = *(T_gtuddtyc_bits *)&R_GPT->GTUDDTYC;
  gtuddtyc_temp.UDF             = 1;  // Up/Down Count Forcible Update: 1 = Immediate update of UD bit
                                      // Alternatives: 0 = Update UD at count overflow/underflow only
  gtuddtyc_temp.UD              = 1;  // Up/Down Count Direction: 1 = Up count (increment)
                                      // Alternatives: 0 = Down count (decrement)
  gtuddtyc_temp.OADTYR          = 1;  // GTIOA Glitch-free Resume: 1 = Enable glitch-free switching for GTIOA
  gtuddtyc_temp.OBDTYR          = 1;  // GTIOB Glitch-free Resume: 1 = Enable glitch-free switching for GTIOB
  R_GPT->GTUDDTYC               = *(uint32_t *)&gtuddtyc_temp;

  // Configure timer control using typed temporary variable
  gtcr_temp                     = *(T_gtcr_bits *)&R_GPT->GTCR;
  gtcr_temp.TPCS                = 0;  // Timer Prescaler Clock Select: 0 = PCLKD/1 (no prescaling)
                                      // Alternatives: 1=PCLKD/4, 2=PCLKD/16, 3=PCLKD/64, 4=PCLKD/256, 5=PCLKD/1024
  gtcr_temp.MD                  = 5;  // Mode Select: 5 = Triangle PWM mode 2 (enhanced double buffer support)
                                      // Alternatives: 0=Periodic, 1=One-shot, 2=One-shot pulse, 4=Triangle PWM mode 1, 6=Triangle PWM mode 3
  R_GPT->GTCR                   = *(uint32_t *)&gtcr_temp;

  // Configure safety features using typed temporary variable
  T_gtintad_bits gtintad_temp   = *(T_gtintad_bits *)&R_GPT->GTINTAD;
  gtintad_temp.GRPABH           = 1;  // Enable simultaneous high-level output disable request
                                      // Safety feature: if both outputs go high simultaneously, disable outputs
  R_GPT->GTINTAD                = *(uint32_t *)&gtintad_temp;

  // Configure software start using typed temporary variable
  T_gtssr_bits gtssr_temp       = *(T_gtssr_bits *)&R_GPT->GTSSR;
  gtssr_temp.CSTRT              = 1;  // Software Start Enable: 1 = Enable software start trigger via GTSTR register
                                      // Alternatives: 0 = Disable software start (can still use external triggers)
  R_GPT->GTSSR                  = *(uint32_t *)&gtssr_temp;

  // Configure software stop using typed temporary variable
  T_gtpsr_bits gtpsr_temp       = *(T_gtpsr_bits *)&R_GPT->GTPSR;
  gtpsr_temp.CSTOP              = 1;  // Software Stop Enable: 1 = Enable software stop trigger via GTSTP register
                                      // Alternatives: 0 = Disable software stop (can still use external triggers)
  R_GPT->GTPSR                  = *(uint32_t *)&gtpsr_temp;

  // Configure software clear using typed temporary variable
  T_gtcsr_bits gtcsr_temp       = *(T_gtcsr_bits *)&R_GPT->GTCSR;
  gtcsr_temp.CCLR               = 1;  // Software Clear Enable: 1 = Enable software clear trigger via GTCLR register
                                      // Alternatives: 0 = Disable software clear (can still use external triggers)
  R_GPT->GTCSR                  = *(uint32_t *)&gtcsr_temp;

  R_GPT->GTST                   = 0;  // Clear status

  // Configure dead time control using typed temporary variable
  T_gtdtcr_bits gtdtcr_temp     = *(T_gtdtcr_bits *)&R_GPT->GTDTCR;
  gtdtcr_temp.TDE               = 1;  // Dead Time Enable: 1 = Enable dead time insertion for complementary outputs
                                      // Alternatives: 0 = Disable dead time (normal PWM operation)
  R_GPT->GTDTCR                 = *(uint32_t *)&gtdtcr_temp;
  // Set dead time value using typed temporary variable
  T_gtdvu_bits gtdvu_temp       = *(T_gtdvu_bits *)&R_GPT->GTDVU;
  gtdvu_temp.GTDVU              = PWM_DEAD_TIME_VAL;  // Dead Time Up-count Value: Clock cycles for dead time insertion
                                                      // Range: 0-1023 PCLKD cycles, prevents shoot-through in bridge circuits
  R_GPT->GTDVU                  = *(uint32_t *)&gtdvu_temp;

  // Configure buffer enable using typed temporary variable
  T_gtber_bits gtber_temp       = *(T_gtber_bits *)&R_GPT->GTBER;
  gtber_temp.BD0                = 0;                    // BD[3:0] Buffer Disable: 0 = Enable buffered write for GTCCRA (double buffer)
                                                        // Alternatives: 1 = Disable buffer (direct write), bits BD1-BD3 control GTCCRB-GTCCRD
  gtber_temp.CCRA               = 1;                    // GTCCRA Buffer Transfer Timing: 1 = Single buffer mode (transfer at trough/crest)
                                                        // Alternatives: 0 = Double buffer mode (transfer at trough and crest)
  R_GPT->GTBER                  = *(uint32_t *)&gtber_temp;

  R_GPT->GTCCR[0]               = MIN_PWM_COMPARE_VAL;  // GTCCRA equivalent
  R_GPT->GTCCR[2]               = MIN_PWM_COMPARE_VAL;  // GTCCRC equivalent
  // Configure I/O control using typed temporary variable
  T_gtior_bits gtior_temp       = *(T_gtior_bits *)&R_GPT->GTIOR;
  gtior_temp.GTIOA              = 0x7;   // GTIOA Output Function: 0x7 = High at compare match during up-count, Low at compare match during down-count
                                         // Alternatives: 0x0=Initial Low, 0x1=Initial High, 0x5=Toggle at compare match, 0x6=Low/High, 0x8=High/Low, etc.
  gtior_temp.GTIOB              = 0x1B;  // GTIOB Output Function: 0x1B = Low at compare match during up-count, High at compare match during down-count
                                         // Alternatives: 0x0=Initial Low, 0x1=Initial High, 0x5=Toggle at compare match, 0x17=High/Low, 0x18=Low/High, etc.
  gtior_temp.OADFLT             = 0;     // GTIOA Disable Value: 0 = Low level output when OAE=0
                                         // Alternatives: 1 = High level output when OAE=0
  gtior_temp.OBDFLT             = 0;     // GTIOB Disable Value: 0 = Low level output when OBE=0
                                         // Alternatives: 1 = High level output when OBE=0
  gtior_temp.OADF               = 2;     // GTIOA Disable Source: 2 = Dead time error or GPT disable
                                         // Alternatives: 0=Disable prohibited, 1=Dead time error, 3=Independent disable
  gtior_temp.OBDF               = 2;     // GTIOB Disable Source: 2 = Dead time error or GPT disable
                                         // Alternatives: 0=Disable prohibited, 1=Dead time error, 3=Independent disable
  gtior_temp.OAE                = 1;     // GTIOA Output Enable: 1 = Enable GTIOA pin output
                                         // Alternatives: 0 = Disable GTIOA pin output (pin uses OADFLT level)
  gtior_temp.OBE                = 1;     // GTIOB Output Enable: 1 = Enable GTIOB pin output
                                         // Alternatives: 0 = Disable GTIOB pin output (pin uses OBDFLT level)
  R_GPT->GTIOR                  = *(uint32_t *)&gtior_temp;

  __DSB();                               // Data sync barrier
}

/*-----------------------------------------------------------------------------------------------------
  Program dual-buffer ADC trigger synchronized with triangle PWM for enhanced sampling control
  Configures two independent ADC triggers:
  - Trigger A: fires during up-counting phase (at crest timing)
  - Trigger B: fires during down-counting phase (at trough timing)
  This provides flexible ADC sampling at optimal points in the PWM cycle

  Parameters:
    adc_module - ADC module selection (PWM_ADC0_TRIGGER for GPT0/ADC0, PWM_ADC1_TRIGGER for GPT4/ADC1)

  Return: none
-----------------------------------------------------------------------------------------------------*/
// clang-format off
static void _PWM_set_ADC_trigger(T_pwm_adc_trigger_select adc_module)
{
  R_GPT0_Type *gpt_reg;

  // Select GPT timer based on ADC module
  if (adc_module == PWM_ADC0_TRIGGER)
  {
    gpt_reg = R_GPT0;  // Use GPT0 for ADC0 trigger
  }
  else if (adc_module == PWM_ADC1_TRIGGER)
  {
    gpt_reg = R_GPT4;  // Use GPT4 for ADC1 trigger
  }
  else
  {
    return;  // Invalid parameter
  }
  // Configure buffer transfer timing using typed temporary variable
  T_gtber_bits gtber_temp     = *(T_gtber_bits *)&gpt_reg->GTBER;

  // Configure ADC Trigger A (up-counting phase)
  gtber_temp.ADTTA            = 0;  // GTADTRA Buffer Transfer Timing: 0 = No transfer (triangle/saw wave mode)
                                    // Alternatives: 1 = Transfer at crest (triangle PWM peak), 2 = Transfer at trough, 3 = Transfer at both
  gtber_temp.ADTDA            = 0;  // GTADTRA Double Buffer Operation: 0 = Single buffer mode (GTADTDBRA → GTADTRA)
                                    // Alternatives: 1 = Double buffer mode (GTADTDBRA → GTADTDBRA → GTADTRA)

  // Configure ADC Trigger B (down-counting phase)
  gtber_temp.ADTTB            = 0;  // GTADTRB Buffer Transfer Timing: 0 = No transfer (triangle/saw wave mode)
                                    // Alternatives: 1 = Transfer at crest, 2 = Transfer at trough, 3 = Transfer at both
  gtber_temp.ADTDB            = 0;  // GTADTRB Double Buffer Operation: 0 = Single buffer mode (GTADTDBRB → GTADTRB)
                                    // Alternatives: 1 = Double buffer mode (GTADTDBRB → GTADTDBRB → GTADTRB)

  gpt_reg->GTBER              = *(uint32_t *)&gtber_temp;                        // Set trigger timing values (advance timing for optimal ADC sampling)
  gpt_reg->GTADTRA            = g_pwm_period_ticks - ADC_TRIGGER_ADVANCE_TICKS;  // ADC Trigger A timing: ~ADC_TRIGGER_ADVANCE_nS ns before peak of triangle wave
  gpt_reg->GTADTRB            = ADC_TRIGGER_ADVANCE_TICKS;                       // ADC Trigger B timing: ~ADC_TRIGGER_ADVANCE_nS ns after trough of triangle wave

  // Buffering for ADC trigger timing control is not applied here
  // gpt_reg->GTADTBRA           = g_pwm_period_ticks - ADC_TRIGGER_ADVANCE_TICKS;
  // gpt_reg->GTADTBRB           = ADC_TRIGGER_ADVANCE_TICKS;

  // Configure ADC interrupt enable using typed temporary variable
  T_gtintad_bits gtintad_temp = *(T_gtintad_bits *)&gpt_reg->GTINTAD;
  gtintad_temp.ADTRAUEN       = 1;  // ADC Trigger A Up-count Enable: 1 = Generate ADC trigger A during up-count only
                                    // Alternatives: 0 = Disable ADC trigger A during up-count
                                    // Note: Enables ADC trigger signal only during up-counting phase to avoid duplicate triggers
  gtintad_temp.ADTRBDEN       = 1;  // ADC Trigger B Down-count Enable: 1 = Generate ADC trigger B during down-count only
                                    // Alternatives: 0 = Disable ADC trigger B during down-count
                                    // Note: Enables ADC trigger signal only during down-counting phase for dual-trigger operation
  gpt_reg->GTINTAD            = *(uint32_t *)&gtintad_temp;
}
// clang-format on

/*-----------------------------------------------------------------------------------------------------
  Start PHASE_COUNT*DRIVER_COUNT-phase PWM (both motors)
  Parameters: none
  Return: none
-----------------------------------------------------------------------------------------------------*/
void PWM_start(void)
{
  // Writing 1 to each bit starts the corresponding timer
  T_reg_GTSTR gtstr_temp;
  gtstr_temp.reg       = 0;               // Initialize register value
  gtstr_temp.bf.CSTRT0 = 1;               // Start GPT0 timer
  gtstr_temp.bf.CSTRT1 = 1;               // Start GPT1 timer
  gtstr_temp.bf.CSTRT2 = 1;               // Start GPT2 timer
  gtstr_temp.bf.CSTRT3 = 1;               // Start GPT3 timer
  gtstr_temp.bf.CSTRT4 = 1;               // Start GPT4 timer
  gtstr_temp.bf.CSTRT5 = 1;               // Start GPT5 timer
  R_GPT0->GTSTR        = gtstr_temp.reg;  // Write to GTSTR register
  __DSB();                                // Data sync barrier to ensure all timers start simultaneously
}

/*-----------------------------------------------------------------------------------------------------
  Stop PHASE_COUNT*DRIVER_COUNT-phase PWM (both motors)
  Parameters: none
  Return: none
-----------------------------------------------------------------------------------------------------*/
void PWM_stop(void)
{
  // Writing 1 to each bit stops the corresponding timer
  T_reg_GTSTP gtstp_temp;
  gtstp_temp.reg       = 0;               // Initialize register value
  gtstp_temp.bf.CSTOP0 = 1;               // Stop GPT0 timer
  gtstp_temp.bf.CSTOP1 = 1;               // Stop GPT1 timer
  gtstp_temp.bf.CSTOP2 = 1;               // Stop GPT2 timer
  gtstp_temp.bf.CSTOP3 = 1;               // Stop GPT3 timer
  gtstp_temp.bf.CSTOP4 = 1;               // Stop GPT4 timer
  gtstp_temp.bf.CSTOP5 = 1;               // Stop GPT5 timer
  R_GPT0->GTSTP        = gtstp_temp.reg;  // Write to GTSTP register
  __DSB();                                // Data sync barrier to ensure all timers stop simultaneously
}

/*-----------------------------------------------------------------------------------------------------
  Fill PWM comparator lookup table for different modulation values
  Calculates comparator values for each PWM step to optimize real-time performance

  Parameters: none
  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Fill_PWM_comparator_table(void)
{  // Fill array with comparator data for different modulation values
  for (uint32_t i = 0; i < PWM_STEP_COUNT; i++)
  {
    int32_t comp_val = (g_pwm_period_ticks * (PWM_STEP_COUNT - i)) / PWM_STEP_COUNT;

    // Prevent PWM from stopping and prevent pulse from being too short
    if (comp_val < MIN_PWM_COMPARE_VAL)
    {
      comp_val = MIN_PWM_COMPARE_VAL;
    }
    else if ((g_pwm_period_ticks - comp_val) < MIN_PWM_COMPARE_VAL)
    {
      comp_val = g_pwm_period_ticks - MIN_PWM_COMPARE_VAL;
    }
    pwm_indx_to_comp[i] = comp_val;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Initialize GPT0-GPT5 for PHASE_COUNT*DRIVER_COUNT-phase triangle PWM (DRIVER_COUNT motors x PHASE_COUNT phases each)
  Parameters:
    freq - PWM frequency (Hz)
  Return:
    RES_OK or RES_ERROR
-----------------------------------------------------------------------------------------------------*/
uint32_t Init_PWM_triangle_buffered(uint32_t freq)
{
  g_pwm_period_ticks = (FRQ_PCLKD_MHZ * 1000000) / (freq * 2) - 1;

  if (g_pwm_period_ticks < PWM_STEP_COUNT)
  {
    // Prevent erroneously selecting too high PWM frequency
    APPLOG("Wrong PWM frequency value %d", freq);
    return RES_ERROR;
  }
  // Fill PWM comparator lookup table for optimal real-time performance
  _Fill_PWM_comparator_table();

  // Clear bits to enable module clocks (0=Enable, 1=Disable/Stop)
  T_reg_MSTPCRE mstpcre_temp;
  mstpcre_temp.reg          = R_MSTP->MSTPCRE;   // Read current register value
  mstpcre_temp.bf.MSTPCRE31 = 0;                 // Enable GPT0 module clock
  mstpcre_temp.bf.MSTPCRE30 = 0;                 // Enable GPT1 module clock
  mstpcre_temp.bf.MSTPCRE29 = 0;                 // Enable GPT2 module clock
  mstpcre_temp.bf.MSTPCRE28 = 0;                 // Enable GPT3 module clock
  mstpcre_temp.bf.MSTPCRE27 = 0;                 // Enable GPT4 module clock
  mstpcre_temp.bf.MSTPCRE26 = 0;                 // Enable GPT5 module clock
  R_MSTP->MSTPCRE           = mstpcre_temp.reg;  // Write back modified register value

  // Initialize timers for Motor Driver 1 (MD1)
  _PWM_triangle_buffered_init(R_GPT0);  // MD1 Phase U
  _PWM_triangle_buffered_init(R_GPT1);  // MD1 Phase V
  _PWM_triangle_buffered_init(R_GPT2);  // MD1 Phase W
  // Initialize timers for Motor Driver 2 (MD2)
  _PWM_triangle_buffered_init(R_GPT3);  // MD2 Phase U
  _PWM_triangle_buffered_init(R_GPT4);  // MD2 Phase V
  _PWM_triangle_buffered_init(R_GPT5);  // MD2 Phase W

  _PWM_set_ADC_trigger(PWM_ADC0_TRIGGER);
  _PWM_set_ADC_trigger(PWM_ADC1_TRIGGER);

  // Set ADC PWM frequency for dynamic EMA coefficient calculation
  Adc_driver_set_pwm_frequency(freq);

#ifdef DEBUG_ADC_SAMPLING_MODE
  // Initialize GTADSMR register for GPT0 to configure ADC conversion start request signal monitoring
  _Init_gpt0_gtadsmr1();
#endif

  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Update PWM level for all phases (DRIVER_COUNT motors × PHASE_COUNT phases) using global control structure
  This function is called from ADC interrupts to safely reprogram timer registers during optimal timing windows

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
void Pwm_update_all_phases_callback(void)
{
  for (uint8_t motor = 0; motor < DRIVER_COUNT; motor++)
  {
    for (uint8_t phase = 0; phase < PHASE_COUNT; phase++)
    {
      // Use enhanced PWM control with step-based duty cycle and output enable control
      _Set_pwm_enhanced(motor, phase, g_pwm_phase_control.pwm_level[motor][phase], g_pwm_phase_control.output_state[motor][phase]);
    }
  }
}

#ifdef DEBUG_ADC_SAMPLING_MODE
/*-----------------------------------------------------------------------------------------------------
  Initialize GTADSMR register for GPT0 to configure ADC conversion start request signal monitoring

  Parameters: none

  Return: none
-----------------------------------------------------------------------------------------------------*/
static void _Init_gpt0_gtadsmr1(void)
{
  T_gtadsmr_bits gtadsmr_cfg = {0};

  // Enable ADC start signal monitoring for GPT0
  gtadsmr_cfg.ADSMEN1        = 1;                          // Enable ADC start request monitoring for GPT0
  gtadsmr_cfg.ADSMS1         = 0;                          // Set ADC start timing (0: at valley, 1: at peak)

  R_GPT0->GTADSMR            = *(uint32_t *)&gtadsmr_cfg;  // Configure GTADSMR register for GPT0
}
#endif                                                     // DEBUG_ADC_SAMPLING_MODE

/*-----------------------------------------------------------------------------------------------------
  Description:
    Enhanced PWM switching with two-stage mode transitions for safe motor control.
    Implements safe switching between PWM modes (0%, 100%, PWM) using two-stage logic:
    - Stage 1: Force both outputs to LOW (safe intermediate state)
    - Stage 2: Set target output configuration (0%, 100%, or enable PWM mode)
    Current state is determined by reading OADTY and OBDTY bits from GTUDDTYC register.
    This prevents shoot-through current by ensuring safe transitions through LOW-LOW state.

  Parameters:
    motor        - motor identifier (0-(DRIVER_COUNT-1))
    phase        - phase identifier (0-(PHASE_COUNT-1))
    pwm_level    - PWM duty cycle in steps (0-PWM_STEP_COUNT)
    output_enable - output enable flag (0=switches OFF, 1=switches enabled)

  Return:
    none
-----------------------------------------------------------------------------------------------------*/
FORCE_INLINE_PRAGMA
FORCE_INLINE_ATTR void _Set_pwm_enhanced(uint8_t motor, uint8_t phase, uint32_t pwm_level, uint8_t output_enable)
{
  R_GPT0_Type *gpt_reg = g_gpt_registers[motor][phase];
  // If output is being disabled, force both switches OFF (high-Z state for coast mode)
  if (output_enable == PHASE_OUTPUT_DISABLE)
  {
    T_gtuddtyc_bits gtuddtyc_temp = *(T_gtuddtyc_bits *)&gpt_reg->GTUDDTYC;

    // Both switches OFF (high-Z state)
    gtuddtyc_temp.OADTY           = 0x2;  // GTIOA = LOW (upper switch OFF)
    gtuddtyc_temp.OADTYF          = 1;    // Enable force mode for GTIOA
    gtuddtyc_temp.OBDTY           = 0x2;  // GTIOB = LOW (lower switch OFF)
    gtuddtyc_temp.OBDTYF          = 1;    // Enable force mode for GTIOB

    gpt_reg->GTUDDTYC             = *(uint32_t *)&gtuddtyc_temp;

    // Update global state arrays for diagnostic display (Z-state: both switches OFF)
    g_phase_state_0_percent[motor][phase]   = 0;  // Not in 0% duty state
    g_phase_state_100_percent[motor][phase] = 0;  // Not in 100% duty state
    g_phase_state_pwm_mode[motor][phase]    = 0;  // Not in PWM mode
    g_phase_state_Z_stage[motor][phase]     = 1;  // In Z-state (high impedance)
    return;
  }
  T_gtuddtyc_bits gtuddtyc_temp     = *(T_gtuddtyc_bits *)&gpt_reg->GTUDDTYC;
  // Read current state from OADTY and OBDTY bits to determine transition stage
  uint8_t current_state_0_percent   = (gtuddtyc_temp.OADTY == 0x2) && (gtuddtyc_temp.OBDTY == 0x3);  // 0% duty state: GTIOA=LOW, GTIOB=HIGH
  uint8_t current_state_100_percent = (gtuddtyc_temp.OADTY == 0x3) && (gtuddtyc_temp.OBDTY == 0x2);  // 100% duty state: GTIOA=HIGH, GTIOB=LOW
  uint8_t current_state_pwm_mode    = (gtuddtyc_temp.OADTYF == 0) && (gtuddtyc_temp.OBDTYF == 0);    // PWM mode: both force modes disabled
  uint8_t current_state_Z_stage     = (gtuddtyc_temp.OADTY == 0x2) && (gtuddtyc_temp.OBDTY == 0x2);  // Z-state: both signals LOW (high impedance)

  // Store current states in global arrays for diagnostic access
  g_phase_state_0_percent[motor][phase]   = current_state_0_percent;
  g_phase_state_100_percent[motor][phase] = current_state_100_percent;
  g_phase_state_pwm_mode[motor][phase]    = current_state_pwm_mode;
  g_phase_state_Z_stage[motor][phase]     = current_state_Z_stage;

  if (pwm_level == 0)
  {
    // Target: 0% duty (GTIOA=LOW, GTIOB=HIGH)
    if (current_state_0_percent)
    {
      return;  // Already in target state
    }
    if (current_state_100_percent || current_state_pwm_mode)
    {
      // Stage 1: Set both signals to LOW (first stage)
      gtuddtyc_temp.OADTY  = 0x2;  // GTIOA = LOW
      gtuddtyc_temp.OADTYF = 1;    // Enable force mode for GTIOA
      gtuddtyc_temp.OBDTY  = 0x2;  // GTIOB = LOW
      gtuddtyc_temp.OBDTYF = 1;    // Enable force mode for GTIOB
    }
    else if (current_state_Z_stage)
    {
      // Stage 2: Set GTIOB to HIGH to complete 0% duty transition
      gtuddtyc_temp.OBDTY = 0x3;  // GTIOB = HIGH (lower switch ON)
      // GTIOA remains LOW (upper switch OFF)
    }
  }
  else if (pwm_level == PWM_STEP_COUNT)
  {
    // Target: 100% duty (GTIOA=HIGH, GTIOB=LOW)
    if (current_state_100_percent)
    {
      return;  // Already in target state
    }
    if (current_state_0_percent || current_state_pwm_mode)
    {
      // Stage 1: Set both signals to LOW (first stage)
      gtuddtyc_temp.OADTY  = 0x2;  // GTIOA = LOW
      gtuddtyc_temp.OADTYF = 1;    // Enable force mode for GTIOA
      gtuddtyc_temp.OBDTY  = 0x2;  // GTIOB = LOW
      gtuddtyc_temp.OBDTYF = 1;    // Enable force mode for GTIOB
    }
    else if (current_state_Z_stage)
    {
      // Stage 2: Set GTIOA to HIGH to complete 100% duty transition
      gtuddtyc_temp.OADTY = 0x3;  // GTIOA = HIGH (upper switch ON)
      // GTIOB remains LOW (lower switch OFF)
    }
  }
  else
  {
    // Target: Normal PWM operation (1 to PWM_STEP_COUNT-1)
    if (current_state_pwm_mode)
    {
      // Already in PWM mode, just update comparator values
      uint32_t comp_value = pwm_indx_to_comp[pwm_level];
      gpt_reg->GTCCR[0]   = comp_value;                  // GTCCRA - primary comparator register
      gpt_reg->GTCCR[2]   = comp_value;                  // GTCCRC - secondary comparator register
#if PWM_DEBUG_VALUES
      g_debug_gtccr0_values[motor][phase] = comp_value;  // Store debug value
      g_debug_gtccr2_values[motor][phase] = comp_value;  // Store debug value
#endif
      return;
    }
    if (current_state_0_percent || current_state_100_percent)
    {
      // Stage 1: Set both signals to LOW (first stage)
      gtuddtyc_temp.OADTY  = 0x2;  // GTIOA = LOW
      gtuddtyc_temp.OADTYF = 1;    // Enable force mode for GTIOA
      gtuddtyc_temp.OBDTY  = 0x2;  // GTIOB = LOW
      gtuddtyc_temp.OBDTYF = 1;    // Enable force mode for GTIOB
    }
    else if (current_state_Z_stage)
    {
      // Stage 2: Enable PWM mode
      // Set PWM comparator value
      uint32_t comp_value = pwm_indx_to_comp[pwm_level];
      gpt_reg->GTCCR[0]   = comp_value;                  // GTCCRA - primary comparator register
      gpt_reg->GTCCR[2]   = comp_value;                  // GTCCRC - secondary comparator register
#if PWM_DEBUG_VALUES
      g_debug_gtccr0_values[motor][phase] = comp_value;  // Store debug value
      g_debug_gtccr2_values[motor][phase] = comp_value;  // Store debug value
#endif

      // Clear force duty values and enable PWM mode
      gtuddtyc_temp.OADTY  = 0;  // Clear GTIOA force duty value
      gtuddtyc_temp.OBDTY  = 0;  // Clear GTIOB force duty value
      gtuddtyc_temp.OADTYF = 0;  // Disable force mode for GTIOA (enable PWM)
      gtuddtyc_temp.OBDTYF = 0;  // Disable force mode for GTIOB (enable PWM)
    }
  }
  // Apply the configuration
  gpt_reg->GTUDDTYC = *(uint32_t *)&gtuddtyc_temp;
}
