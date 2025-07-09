#include "App.h"

TX_EVENT_FLAGS_GROUP g_main_event_flags;
TX_THREAD            Main_thread;
static uint8_t       Main_thread_stack[MAIN_THREAD_STACK_SIZE] BSP_PLACE_IN_SECTION(".stack.Main_thread") BSP_ALIGN_VARIABLE(BSP_STACK_ALIGNMENT);

// Forward declarations
static void Main_thread_func(ULONG thread_input);

/*-----------------------------------------------------------------------------------------------------
  Creates the main thread and initializes system components

  Parameters:
    first_unused_memory - Pointer to the first unused memory block for threadX

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Main_thread_create(void* first_unused_memory)
{
  // Initialize SEGGER RTT
  SEGGER_RTT_Init();

  // Initialize memory pools
  App_memory_pools_creation(first_unused_memory);

  // Initialize each kernel object
  tx_thread_create(
  &Main_thread,
  (CHAR*)"MAIN",
  Main_thread_func,
  (ULONG)NULL,
  &Main_thread_stack,
  MAIN_THREAD_STACK_SIZE,
  THREAD_PRIORITY_MAIN,
  THREAD_PREEMPT_MAIN,
  THREAD_TIME_SLICE_MAIN,
  TX_AUTO_START);
}

/*-----------------------------------------------------------------------------------------------------
  Set event flags for the main thread from other tasks.

  Parameters:
    flags - Bitmask of event flags to set

  Return:
    TX_SUCCESS on success, error code otherwise
-----------------------------------------------------------------------------------------------------*/
uint32_t Main_thread_set_event(ULONG flags)
{
  return tx_event_flags_set(&g_main_event_flags, flags, TX_OR);
}

/*-----------------------------------------------------------------------------------------------------
  Initialize synchronization objects

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
void Init_synchronization_objects(void)
{
  tx_event_flags_create(&g_main_event_flags, "main_event_flags");
  Init_save_params_mutex();
}

/*-----------------------------------------------------------------------------------------------------
  Main thread entry point. Initializes system modules and runs the main loop.

  Parameters:
    None

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void Main_thread_func(ULONG thread_input)
{
  uint32_t restore_res;
  ULONG    actual_flags;

  // Initialize ELC driver before other subsystems that might use it
  R_ELC_Open(g_elc.p_ctrl, g_elc.p_cfg);

  Init_synchronization_objects();

  // Initialize system error flags with default values
  App_init_error_flags();

  IDLE_thread_create();

  Logger_init();
  nx_crypto_initialize();
  RTC_init();
  Init_SD_card_file_system();
  Flash_driver_init();

  restore_res = Restore_settings(APPLICATION_PARAMS);  // Restore application parameters from DataFlash or file
  if (restore_res != RES_OK)
  {
    Led_blink_set_patterns_GR(&LED_PATTERN_OFF, &LED_PATTERN_ERROR);
  }
  else
  {
    // Check for emergency stop status during initialization
    if (App_is_emergency_stop_active())
    {
      Led_blink_set_patterns_GR(&LED_PATTERN_OFF, &LED_PATTERN_ERROR);
    }
    else
    {
      Led_blink_set_patterns_GR(&LED_PATTERN_WORK_NORMAL, &LED_PATTERN_OFF);
    }
  }

  Logger_thread_create();

  SPI0_open();
  Configure_IO_extender();
  Manual_encode_init();
  VT100_task_manager_initialization();  // Initialize VT100 terminal engine before communication channels can create VT100 tasks
  Thread_FreeMaster_create();           // Create FreeMaster communication thread
  Init_USB_stack();

#ifdef DEBUG_ADC_SAMPLING_MODE
  // Debug mode: configure P705 for GTADSM1 output instead of LCD_DC
  Config_pin_P705_GTADSM1_mode();
#else
  // Normal mode: start GUI
  GUI_start();
#endif
  Can_thread_create();

  Motor_thread_create();
  // Initialize CAN message handler
  Can_message_handler_init();


  while (1)
  {
    static uint32_t can_check_counter = 0;

    tx_event_flags_get(&g_main_event_flags, 0xFFFFFFFF, TX_OR_CLEAR, &actual_flags, MS_TO_TICKS(10));  // 10 ms tick timeout

    // Check for emergency stop status and update encoder lighting accordingly
    if (App_is_emergency_stop_active())
    {
      // Emergency stop active - show alarm pattern on encoder lighting
      Led_blink_set_patterns_GR(&LED_PATTERN_OFF, &LED_PATTERN_ERROR);
    }
    else
    {
      // Normal operation - show work pattern on encoder lighting
      Led_blink_set_patterns_GR(&LED_PATTERN_WORK_NORMAL, &LED_PATTERN_OFF);
    }

    // CAN LED control based on communication status
    if (App_is_can_bus_error_set())
    {
      // CAN error detected - LED off
      Led_blink_set_pattern(LED_BLINK_CAN, &LED_PATTERN_CAN_ERROR);
    }
    else
    {
      Led_blink_set_pattern(LED_BLINK_CAN, &LED_PATTERN_CAN_ACTIVITY);
    }

    // RS485 LED - turn off for now (no RS485 implementation yet)
    Led_blink_set_pattern(LED_BLINK_RS485, &LED_PATTERN_RS485_OFF);

    Led_blink_func();

    // Check CAN communication status every 50ms (5 x 10ms)
    can_check_counter++;
    if (can_check_counter >= 5)
    {
      can_check_counter = 0;
      Can_check_communication_timeout();
    }
  }
}
