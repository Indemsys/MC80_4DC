#ifndef MC80_H
#define MC80_H

// Universal macro for compiler-specif#define THREAD_PREEMPT_VT100_MANAGER    21  // Can be preempted by priorities 0-20c forced inline optimization
#ifdef __ICCARM__        // IAR Compiler
  #define FORCE_INLINE_PRAGMA _Pragma("inline=forced")
  #define FORCE_INLINE_ATTR   static
#elif defined(__GNUC__)  // GCC Compiler
  #define FORCE_INLINE_PRAGMA
  #define FORCE_INLINE_ATTR static __attribute__((always_inline)) inline
#else                    // Other compilers
  #define FORCE_INLINE_PRAGMA
  #define FORCE_INLINE_ATTR static inline
#endif

// Debug mode configuration - uncomment to enable ADC sampling debug mode
// When enabled: disables GUI creation and enables GTADSM1 output on P705 pin
// #define DEBUG_ADC_SAMPLING_MODE

// Thread stack sizes (in bytes)
// Stack size determines how much memory is allocated for each thread's stack.
// Values must be multiples of 4 bytes and should account for:
// - Local variables size
// - Function call depth (each call uses ~16-32 bytes)
// - Interrupt context saving (~200-400 bytes)
// - Safety margin (recommended 20-30% extra)
// Range: 512-8192 bytes typical, minimum 256 bytes
#define MAIN_THREAD_STACK_SIZE               2048  // Main control logic thread
#define LOGGER_THREAD_STACK_SIZE             2048  // File system operations thread
#define IDLE_THREAD_STACK_SIZE               2048  // Background maintenance thread
#define MOTOR_THREAD_STACK_SIZE              2048  // Motor control thread
#define CAN_THREAD_STACK_SIZE                1024  // CAN communication thread
#define CAN_RX_THREAD_STACK_SIZE             1024  // CAN receive thread
#define FREEMASTER_THREAD_STACK_SIZE         2048  // FreeMaster communication thread
#define VT100_MANAGER_THREAD_STACK_SIZE      2048  // VT100 manager thread
#define VT100_THREAD_STACK_SIZE              4096  // VT100 task thread
#define TMC6200_MONITORING_THREAD_STACK_SIZE 1024  // TMC6200 driver monitoring thread

// Thread priorities (0-31, where 0 is highest priority)
// Lower numerical values indicate higher priority threads.
// Threads with the same priority use round-robin scheduling.
// Priority 0 is reserved for the timer thread.
// Range: 1-31 for user threads
// Guidelines:
// - Real-time control: 1-5 (highest)
// - Communication: 6-15 (medium)
// - Background tasks: 16-30 (lowest)
// - Idle thread: 31 (system idle)
#define THREAD_PRIORITY_MAIN                 1   // Highest - main control
#define THREAD_PRIORITY_MOTOR                1   // Highest - motor control (same as main)
#define THREAD_PRIORITY_CAN                  2   // High - communication
#define THREAD_PRIORITY_CAN_RX               3   // High - CAN receive
#define THREAD_PRIORITY_TMC6200_MONITORING   10  // Medium - TMC6200 driver monitoring
#define THREAD_PRIORITY_FREEMASTER           15  // Medium - FreeMaster communication
#define THREAD_PRIORITY_LOGGER               20  // Low - background logging
#define THREAD_PRIORITY_VT100_MANAGER        21  // Low - VT100 manager
#define THREAD_PRIORITY_VT100                22  // Low - VT100 tasks
#define THREAD_PRIORITY_IDLE                 31  // Lowest - system idle

// Thread preemption thresholds (0-31, must be >= thread priority)
// Determines the lowest priority level that can preempt this thread.
// When set equal to thread priority: thread can be preempted by any higher priority
// When set higher: thread runs without preemption until voluntary suspension
// Setting threshold = 31 disables preemption completely for the thread
// Range: thread_priority to 31
// Use cases:
// - threshold = priority: normal preemptive behavior
// - threshold > priority: critical sections without mutex overhead
#define THREAD_PREEMPT_MAIN                  1   // Can be preempted by priority 0 only
#define THREAD_PREEMPT_MOTOR                 1   // Can be preempted by priority 0 only
#define THREAD_PREEMPT_CAN                   2   // Can be preempted by priorities 0-1
#define THREAD_PREEMPT_CAN_RX                3   // Can be preempted by priorities 0-2
#define THREAD_PREEMPT_TMC6200_MONITORING    10  // Can be preempted by priorities 0-9
#define THREAD_PREEMPT_FREEMASTER            15  // Can be preempted by priorities 0-14
#define THREAD_PREEMPT_LOGGER                20  // Can be preempted by priorities 0-19
#define THREAD_PREEMPT_VT100_MANAGER         21  // Can be preempted by priorities 0-20
#define THREAD_PREEMPT_VT100                 22  // Can be preempted by priorities 0-21
#define THREAD_PREEMPT_IDLE                  31  // Can be preempted by any priority

// Thread time slices (in timer ticks, 0 = no time slicing)
// Time slice defines how long a thread can run before being preempted by
// another thread of the same priority (round-robin scheduling).
// One tick = 1ms typically (depends on TX_TIMER_TICKS_PER_SECOND)
// Range: 0-65535 ticks
// Values:
// - 0: No time slicing, thread runs until suspension/completion
// - 1-10: Very responsive, high context switch overhead
// - 10-100: Balanced responsiveness and efficiency (recommended)
// - >100: Lower responsiveness, better efficiency
#define THREAD_TIME_SLICE_IDLE               1   // 1ms time slice for idle (quick bursts)
#define THREAD_TIME_SLICE_LOGGER             1   // 1ms time slice for logger (quick bursts)
#define THREAD_TIME_SLICE_MAIN               10  // 10ms time slice for main thread
#define THREAD_TIME_SLICE_MOTOR              10  // 10ms time slice for motor thread
#define THREAD_TIME_SLICE_CAN                10  // 10ms time slice for CAN thread
#define THREAD_TIME_SLICE_CAN_RX             10  // 10ms time slice for CAN RX thread
#define THREAD_TIME_SLICE_FREEMASTER         10  // 10ms time slice for FreeMaster
#define THREAD_TIME_SLICE_VT100_MANAGER      10  // 10ms time slice for VT100 manager
#define THREAD_TIME_SLICE_VT100              10  // 10ms time slice for VT100 tasks
#define THREAD_TIME_SLICE_TMC6200_MONITORING 10  // 10ms time slice for  TMC6200 monitoring

// Сруктура Code Flash памяти кода
// 0x02000000...0x0200FFFF - стирание по 8192   байта, запись по 128 байт
// 0x02010000...0x021F7FFF - стирание по 32768  байта, запись по 128 байт

#define CODE_FLASH_START                     (0x02000000)
#define CODE_FLASH_END                       (0x021F7FFF)
#define CODE_FLASH_EBLOCK_SZ                 32768  // Размер стираемого блока
#define CODE_FLASH_WR_SZ                     128    // Размер записываемого блока

// Сруктура Data Flash памяти данных
// 0x40100000...0x4010FFFF - стирание по 64 байта, запись по 4 байта

#define DATA_FLASH_START                     (0x27000000)
#define DATA_FLASH_SIZE                      (0x00003000)

#define DATA_FLASH_EBLOCK_SZ                 64  // Размер стираемого блока
#define DATA_FLASH_WR_SZ                     4   // Размер записываемого блока

#define STANDBY_SECURE_SRAM_START            (0x26000000)
#define STANDBY_SECURE_SRAM_SIZE             (0x00000400)


// OSPI Flash memory addresses
#define OSPI_BASE_ADDRESS        BSP_FEATURE_OSPI_B_DEVICE_0_START_ADDRESS  // Unified base address


// Конфигурация клоков
// CPUCLK = 240 МГц
// ICLK   = 240 МГц
// PCLKA  = 120 МГц
// PCLKB  = 60  МГц
// PCLKC  = 60  МГц
// PCLKD  = 120 МГц
// PCLKE  = 240 МГц
// SDCLK  = 120 МГц
// BCLK   = 120 МГц
// EBCLK  = 60  МГц
// FCLK   = 60  МГц

#define FRQ_CPUCLK_MHZ                       240  // МГц
#define FRQ_ICLK_MHZ                         240  // МГц
#define FRQ_PCLKA_MHZ                        120  // МГц
#define FRQ_PCLKB_MHZ                        60   // МГц
#define FRQ_PCLKC_MHZ                        60   // МГц
#define FRQ_PCLKD_MHZ                        120  // МГц
#define FRQ_PCLKE_MHZ                        240  // МГц
#define FRQ_SDCLK_MHZ                        120  // МГц
#define FRQ_BCLK_MHZ                         120  // МГц
#define FRQ_EBCLK_MHZ                        60   // МГц
#define FRQ_FCLK_MHZ                         60   // МГц

#define AGT_PERIOD_US                        100

// PWM Configuration Constants
#define MIN_PWM_PULSE_nS                     7000ll  // Minimum PWM pulse width in nanoseconds
#define PWM_DEAD_TIME_nS                     100ll   // Dead time from high switch off to low switch on in nanoseconds

// Safety check: MIN_PWM_PULSE_nS must be greater than or equal to PWM_DEAD_TIME_nS
// to ensure proper operation of power switches and prevent shoot-through
#if MIN_PWM_PULSE_nS <= PWM_DEAD_TIME_nS
  #error "MIN_PWM_PULSE_nS must be greater than PWM_DEAD_TIME_nS for safe operation"
#endif

#define MIN_PWM_COMPARE_VAL    ((MIN_PWM_PULSE_nS * FRQ_PCLKD_MHZ) / 2000ll)
#define PWM_DEAD_TIME_VAL      ((PWM_DEAD_TIME_nS * FRQ_PCLKD_MHZ) / 1000ll)
#define PWM_STEP_COUNT         200  // Minimum PWM cycle count
#define HALF_PWM_STEP_COUNT    (PWM_STEP_COUNT / 2)
#define PWM_100                PWM_STEP_COUNT

#define PSEL_03                3  // PFS peripheral select value for GPT

// Выходы
#define IO_EXTENDER_CS         R_PORT4->PODR_b.PODR6
#define MOTOR_DRV1_CS          R_PORT6->PODR_b.PODR12
#define MOTOR_DRV2_CS          R_PORT7->PODR_b.PODR4
#define LCD_CS                 R_PORT2->PODR_b.PODR6
#define LCD_BLK                R_PORT7->PODR_b.PODR10
#define LCD_RST                R_PORT7->PODR_b.PODR3
#define LCD_DC                 R_PORT7->PODR_b.PODR5
#define MOTOR_DRV1_EN          R_PORT9->PODR_b.PODR5  // Выход - разрешение драйвера 1
#define MOTOR_DRV2_EN          R_PORT3->PODR_b.PODR1  // Выход - разрешение драйвера 2

// Входы
#define MANUAL_ENCODER_A_INPUT (R_PORT6->PIDR_b.PIDR5)
#define MANUAL_ENCODER_B_INPUT (R_PORT6->PIDR_b.PIDR4)
#define MANUAL_ENCODER_SWITCH  (R_PORT2->PIDR_b.PIDR1)
#define MOTOR_DRV2_FAULT_STATE (R_PORT7->PIDR_b.PIDR8)  // Вход - ошибка в драйвере 2
#define MOTOR_DRV1_FAULT_STATE (R_PORT7->PIDR_b.PIDR9)  // Вход - ошибка в драйвере 1
#define MOTOR_DRV1_EN_STATE    (R_PORT9->PIDR_b.PIDR5)  // Вход - разрешение драйвера 1
#define MOTOR_DRV2_EN_STATE    (R_PORT3->PIDR_b.PIDR1)  // Вход - разрешение драйвера 2

// clang-format off
// ADC multiplexer control signals
#define ADC_MUX_SEL               R_PORT4->PODR_b.PODR13  // Output - P413: SEL line for U9/U11 mux (motor 1/2 selection)
#define ADC_MUX_A0                R_PORT6->PODR_b.PODR13  // Output - P613: A0 for U12 mux (phase selection bit 0)
#define ADC_MUX_A1                R_PORT6->PODR_b.PODR14  // Output - P614: A1 for U12 mux (phase selection bit 1)

// Motor selection macros for AN000, AN001, AN002, AN100, AN101, AN102
#define ADC_SELECT_MOTOR1()     do { ADC_MUX_SEL = 0; } while (0)  // SEL=0 -> Motor 1 signals
#define ADC_SELECT_MOTOR2()     do { ADC_MUX_SEL = 1; } while (0)  // SEL=1 -> Motor 2 signals

// Phase voltage selection macros for AN005/AN006
#define ADC_SELECT_PHASE_U()    do { ADC_MUX_A1 = 0; ADC_MUX_A0 = 0; } while (0)  // A1:A0 = 00 -> U phase
#define ADC_SELECT_PHASE_V()    do { ADC_MUX_A1 = 0; ADC_MUX_A0 = 1; } while (0)  // A1:A0 = 01 -> V phase
#define ADC_SELECT_PHASE_W()    do { ADC_MUX_A1 = 1; ADC_MUX_A0 = 0; } while (0)  // A1:A0 = 10 -> W phase
#define ADC_SELECT_PHASE_GND()  do { ADC_MUX_A1 = 1; ADC_MUX_A0 = 1; } while (0)  // A1:A0 = 11 -> GND calibration

// Motor Driver 1 (MD1) PWM Phase Output Control Macros
#define MD1_PHASE_UH              R_PORT4->PODR_b.PODR15        // P415 (GTIOC0A) - MD1 Phase UH
#define MD1_PHASE_UL              R_PORT4->PODR_b.PODR14        // P414 (GTIOC0B) - MD1 Phase UL
#define MD1_PHASE_VH              R_PORT1->PODR_b.PODR5         // P105 (GTIOC1A) - MD1 Phase VH
#define MD1_PHASE_VL              R_PORT2->PODR_b.PODR8         // P208 (GTIOC1B) - MD1 Phase VL
#define MD1_PHASE_WH              R_PORT1->PODR_b.PODR13        // P113 (GTIOC2A) - MD1 Phase WH
#define MD1_PHASE_WL              R_PORT1->PODR_b.PODR14        // P114 (GTIOC2B) - MD1 Phase WL

// Motor Driver 2 (MD2) PWM Phase Output Control Macros
#define MD2_PHASE_UH              R_PORT3->PODR_b.PODR0         // P300 (GTIOC3A) - MD2 Phase UH
#define MD2_PHASE_UL              R_PORT1->PODR_b.PODR12        // P112 (GTIOC3B) - MD2 Phase UL
#define MD2_PHASE_VH              R_PORT2->PODR_b.PODR5         // P205 (GTIOC4A) - MD2 Phase VH
#define MD2_PHASE_VL              R_PORT2->PODR_b.PODR4         // P204 (GTIOC4B) - MD2 Phase VL
#define MD2_PHASE_WH              R_PORT7->PODR_b.PODR0         // P700 (GTIOC5A) - MD2 Phase WH
#define MD2_PHASE_WL              R_PORT7->PODR_b.PODR1         // P701 (GTIOC5B) - MD2 Phase WL

// Motor and Phase identification constants
#define MOTOR_1                   1
#define MOTOR_2                   2
#define PHASE_U                   1
#define PHASE_V                   2
#define PHASE_W                   3

// PFS Register Macros for _Set_pwm_pin_output_mode function
// Motor Driver 1 (MD1)
#define MD1_PHASE_U_PFS_H         R_PFS->PORT[4].PIN[15].PmnPFS // P415 - MD1 Phase U High
#define MD1_PHASE_U_PFS_L         R_PFS->PORT[4].PIN[14].PmnPFS // P414 - MD1 Phase U Low
#define MD1_PHASE_V_PFS_H         R_PFS->PORT[1].PIN[5].PmnPFS  // P105 - MD1 Phase V High
#define MD1_PHASE_V_PFS_L         R_PFS->PORT[2].PIN[8].PmnPFS  // P208 - MD1 Phase V Low
#define MD1_PHASE_W_PFS_H         R_PFS->PORT[1].PIN[13].PmnPFS // P113 - MD1 Phase W High
#define MD1_PHASE_W_PFS_L         R_PFS->PORT[1].PIN[14].PmnPFS // P114 - MD1 Phase W Low

// Motor Driver 2 (MD2)
#define MD2_PHASE_U_PFS_H         R_PFS->PORT[3].PIN[0].PmnPFS  // P300 - MD2 Phase U High
#define MD2_PHASE_U_PFS_L         R_PFS->PORT[1].PIN[12].PmnPFS // P112 - MD2 Phase U Low
#define MD2_PHASE_V_PFS_H         R_PFS->PORT[2].PIN[5].PmnPFS  // P205 - MD2 Phase V High
#define MD2_PHASE_V_PFS_L         R_PFS->PORT[2].PIN[4].PmnPFS  // P204 - MD2 Phase V Low
#define MD2_PHASE_W_PFS_H         R_PFS->PORT[7].PIN[0].PmnPFS  // P700 - MD2 Phase W High
#define MD2_PHASE_W_PFS_L         R_PFS->PORT[7].PIN[1].PmnPFS  // P701 - MD2 Phase W Low
// clang-format on

#define BIT(n)                 (1u << n)
#define LSHIFT(v, n)           (((unsigned int)(v) << n))

#define RES_OK                 (0)
#define RES_ERROR              (1)

#define DELAY_1us              Delay_m85(160)         // ~1           мкс при частоте   480 МГц
#define DELAY_2us              Delay_m85(320)         // ~2           мкс при частоте   480 МГц
#define DELAY_4us              Delay_m85(640)         // ~4           мкс при частоте   480 МГц
#define DELAY_8us              Delay_m85(1200)        // ~8           мкс при частоте   480 МГц
#define DELAY_32us             Delay_m85(4800)        // ~32          мкс при частоте   480 МГц
#define DELAY_100us            Delay_m85(16000)       // ~100         мкс при частоте   480 МГц
#define DELAY_ms(x)            Delay_m85(x * 160000)  // ~1000*x      мкс при частоте 480 МГц

extern void Delay_m85(int cnt);                       // Задержка на (cnt * 3) + 2 тактов . Передача нуля не допускается

typedef struct
{
  uint32_t cycles;
  uint32_t ticks;

} T_sys_timestump;

#include "jansson.h"
#include "Params_Types.h"
#include "Parameters_manager.h"
#include "Parameters_serializer.h"
#include "Parameters_deserializer.h"
#include "MC80_Params.h"
#include "App_utils.h"
#include "CRC_utils.h"
#include "String_utils.h"
#include "DSP_Filters.h"
#include "Time_utils.h"
#include "compress.h"
#include "NV_store.h"

#include "MC80V1_pins.h"
#include "ADC_driver.h"
#include "SPI0_bus.h"
#include "RTC_driver.h"
#include "Flash_driver.h"
#include "PWM_timer_driver.h"
#include "mx25um25645g.h"
#include "OSPI_config.h"
#include "LevelX_config.h"


#include "Memory_manager.h"
#include "Logger.h"

#include "FS_init.h"
#include "FS_utils.h"
#include "SPI_Display.h"
#include "MotDrv_TMC6200.h"
#include "IO_extender.h"

#include "HMI.h"
#include "Logger_task.h"
#include "TMC6200_Monitoring_task.h"
#include "Motor_Driver_task.h"
#include "Main_task.h"
#include "CAN_task.h"
#include "Manual_Encoder.h"
#include "IDLE_task.h"
#include "USB_transfer_obj.h"
#include "USB_init.h"
#include "USB_cdc_acm.h"
#include "USB_storage.h"
#include "freemaster.h"
#include "FreeMaster_thread.h"
#include "FreeMaster_command_handler.h"
#include "Monitor_VT100_manager.h"
#include "Monitor_utilites.h"
#include "Monitor_access_control.h"
#include "Monitor_TMC6200.h"
#include "Monitor_diagnostic.h"
#include "Monitor_main_menu.h"
#include "Monitor_params_editor.h"
#include "Monitor_log_viewer.h"
#include "Monitor_USB_drv.h"
#include "Led_blink.h"
#include "Monitor_Motor.h"
#include "Monitor_CAN.h"
#include "Monitor_LittleFS.h"
#include "Monitor_OSPI.h"
#include "Monitor_RTT.h"
#include "CAN_protocol.h"
#include "System_error_flags.h"
#include "CAN_message_handler.h"
#include "CAN_parameter_exchange.h"
#include "Motor_Soft_Start.h"


extern uint8_t g_file_system_ready;

uint32_t Main_thread_set_event(ULONG flags);

#endif
