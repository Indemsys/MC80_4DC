#include "App.h"

#define REF_TIME_INTERVAL_MS 100
#define CPU_USAGE_FLTR_LEN   128

TX_THREAD      IDLE_thread;
static uint8_t IDLE_thread_stack[IDLE_THREAD_STACK_SIZE] BSP_PLACE_IN_SECTION(".stack.IDLE_thread") BSP_ALIGN_VARIABLE(BSP_STACK_ALIGNMENT);

static void _IDLE_thread_func(ULONG thread_input);

uint32_t              g_ref_time;
T_run_average_int32_N filter_cpu_usage;
volatile uint32_t     g_aver_cpu_usage;
volatile uint32_t     g_cpu_usage;
int32_t               cpu_usage_arr[CPU_USAGE_FLTR_LEN];
volatile uint32_t     idle_counter = 0;

typedef struct
{
  bool     request;
  uint8_t  ptype;
  uint8_t  media_type;
  char    *file_name;
  bool     operation_completed;
  uint32_t operation_result;
} T_SaveParams;

T_SaveParams    g_save_params = { 0 };
static TX_MUTEX save_params_mutex;

/*-----------------------------------------------------------------------------------------------------
  Creates and initializes IDLE thread

  Parameters:

  Return:
-----------------------------------------------------------------------------------------------------*/
void IDLE_thread_create(void)
{
  tx_thread_create(
  &IDLE_thread,
  (CHAR *)"IDLE",
  _IDLE_thread_func,
  (ULONG)NULL,
  &IDLE_thread_stack,
  IDLE_THREAD_STACK_SIZE,
  THREAD_PRIORITY_IDLE,
  THREAD_PREEMPT_IDLE,
  THREAD_TIME_SLICE_IDLE,
  TX_AUTO_START);
}

/*-----------------------------------------------------------------------------------------------------
  Initializes the DWT cycle counter
  Returns 1 if successful, 0 if failed

  Parameters:

  Return:
-----------------------------------------------------------------------------------------------------*/
static uint32_t DWT_Init(void)
{
// DWT and DCB register addresses
#define DWT_CONTROL   (*((volatile uint32_t *)0xE0001000))
#define DWT_CYCCNT    (*((volatile uint32_t *)0xE0001004))
#define DEMCR         (*((volatile uint32_t *)0xE000EDFC))

// Control bits
#define DEMCR_TRCENA  (1u << 24)
#define DWT_CYCCNTENA (1u << 0)

  // Enable trace and debug (DWT requires this)
  DEMCR |= DEMCR_TRCENA;

  // Reset cycle counter
  DWT_CYCCNT = 0;

  // Enable cycle counter
  DWT_CONTROL |= DWT_CYCCNTENA;

  // Check if counter is enabled
  return (DWT_CONTROL & DWT_CYCCNTENA) ? 1 : 0;
}

/*-----------------------------------------------------------------------------------------------------
  Measures the duration of a time interval specified in milliseconds
  Uses DWT_CYCCNT, accounting for single overflow of the counter
  Function samples the counter at start and end, and calculates the time difference

  Parameters:
    time_delay_ms - Time delay in milliseconds

  Return:
    Time difference in microseconds
-----------------------------------------------------------------------------------------------------*/
uint64_t Measure_reference_time_interval(uint32_t time_delay_ms)
{
  uint32_t cycle_start;
  uint32_t cycle_end;
  uint64_t diff_cycles;
  uint64_t diff_us;
  // Initialize DWT if not already initialized
  static uint8_t dwt_initialized = 0;
  if (!dwt_initialized)
  {
    dwt_initialized = DWT_Init();
    if (!dwt_initialized)
    {
      return 0;  // Failed to initialize DWT
    }
  }

  // Read CYCLECOUNTER at the start
  cycle_start = DWT->CYCCNT;

  // Delay for the specified time
  DELAY_ms(time_delay_ms);

  // Read CYCLECOUNTER at the end
  cycle_end = DWT->CYCCNT;

  // Calculate the difference in cycle counts, handling potential overflow
  if (cycle_end >= cycle_start)
  {
    // No overflow
    diff_cycles = (uint64_t)(cycle_end - cycle_start);
  }
  else
  {
    // Overflow occurred
    diff_cycles = (uint64_t)(0xFFFFFFFF - cycle_start) + (uint64_t)cycle_end + 1;
  }

  // Calculate the time difference in microseconds
  diff_us = diff_cycles / FRQ_CPUCLK_MHZ;

  return diff_us;
}

/*-----------------------------------------------------------------------------------------------------
  Initializes mutex for protecting save parameters

  Parameters:

  Return:
-----------------------------------------------------------------------------------------------------*/
void Init_save_params_mutex(void)
{
  tx_mutex_create(&save_params_mutex, "Save Params Mutex", TX_NO_INHERIT);
}

/*-----------------------------------------------------------------------------------------------------
  Request to save settings

  Logic:
  1. Acquire mutex to protect the save structure.
  2. If an active save request already exists:
     - Release mutex.
     - Wait 1 tick before rechecking.
     - Repeat the process until the current request is processed.
  3. Set new request parameters (type, media, filename).
  4. Release mutex.

  Parameters:
    ptype      - Parameter type (e.g., APPLICATION_PARAMS)
    media_type - Media type: MEDIA_TYPE_FILE or MEDIA_TYPE_DATAFLASH
    file_name  - Filename for saving (NULL - use default name)

  Return:
-----------------------------------------------------------------------------------------------------*/
void Request_save_settings(uint8_t ptype, uint8_t media_type, char *file_name)
{
  tx_mutex_get(&save_params_mutex, TX_WAIT_FOREVER);
  while (g_save_params.request)  // Wait until current request is processed
  {
    tx_mutex_put(&save_params_mutex);
    tx_thread_sleep(1);          // Pause before rechecking
    tx_mutex_get(&save_params_mutex, TX_WAIT_FOREVER);
  }
  g_save_params.request             = true;
  g_save_params.ptype               = ptype;
  g_save_params.media_type          = media_type;
  g_save_params.file_name           = file_name;
  g_save_params.operation_completed = false;
  g_save_params.operation_result    = RES_ERROR;
  tx_mutex_put(&save_params_mutex);
}

/*-----------------------------------------------------------------------------------------------------
  Wait for save operation completion within specified timeout

  Parameters:
    result - Pointer to store operation result (RES_OK or error code)
    timeout_ms - Timeout in milliseconds to wait for completion

  Return:
    true if operation completed within timeout, false if timeout occurred
-----------------------------------------------------------------------------------------------------*/
bool Check_save_operation_status(uint32_t *result, uint32_t timeout_ms)
{
  bool     completed     = false;
  uint32_t timeout_ticks = timeout_ms / 10;  // Convert ms to ticks (assuming 10ms per tick)
  uint32_t elapsed_ticks = 0;

  while (elapsed_ticks < timeout_ticks)
  {
    tx_mutex_get(&save_params_mutex, TX_WAIT_FOREVER);
    completed = g_save_params.operation_completed;
    if (completed)
    {
      if (result != NULL)
      {
        *result = g_save_params.operation_result;
      }
      // Reset completion flag after reading result
      g_save_params.operation_completed = false;
      tx_mutex_put(&save_params_mutex);
      return true;  // Operation completed successfully
    }
    tx_mutex_put(&save_params_mutex);

    // Wait 10ms before checking again
    tx_thread_sleep(1);
    elapsed_ticks++;
  }

  return false;  // Timeout occurred
}

/*-----------------------------------------------------------------------------------------------------
  IDLE thread entry point

  Parameters:
    thread_input - Thread input parameter

  Return:
-----------------------------------------------------------------------------------------------------*/
static void _IDLE_thread_func(ULONG thread_input)
{
  uint32_t t;
  uint32_t dt;

  __disable_interrupt();
  g_ref_time = Measure_reference_time_interval(REF_TIME_INTERVAL_MS);
  __enable_interrupt();

  g_cpu_usage           = 0;
  g_aver_cpu_usage      = 0;

  filter_cpu_usage.init = 0;
  filter_cpu_usage.len  = CPU_USAGE_FLTR_LEN;
  filter_cpu_usage.arr  = cpu_usage_arr;
  g_aver_cpu_usage      = RunAverageFilter_int32_N(&filter_cpu_usage, g_cpu_usage);

  for (;;)
  {
    t = Measure_reference_time_interval(REF_TIME_INTERVAL_MS);

    if (t < g_ref_time)
    {
      g_ref_time = t;
      dt         = 0;
    }
    else
    {
      dt = t - g_ref_time;
    }
    g_cpu_usage      = (1000ul * dt) / g_ref_time;
    g_aver_cpu_usage = RunAverageFilter_int32_N(&filter_cpu_usage, g_cpu_usage);
    // Check for save settings request
    tx_mutex_get(&save_params_mutex, TX_WAIT_FOREVER);
    if (g_save_params.request)
    {
      g_save_params.request             = false;  // Reset the request flag
      uint32_t save_result              = Save_settings(g_save_params.ptype, g_save_params.media_type, g_save_params.file_name);

      // Store operation result and mark as completed
      g_save_params.operation_result    = save_result;
      g_save_params.operation_completed = true;
    }
    tx_mutex_put(&save_params_mutex);

    idle_counter++;
  }
}
