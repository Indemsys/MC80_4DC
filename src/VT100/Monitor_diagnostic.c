#include "App.h"

static void        Diagnostic_Show_pin_states(uint8_t keycode);
static void        Diagnostic_Show_task_states(uint8_t keycode);
static void        Diagnostic_Show_heap_state(uint8_t keycode);
static const char *_Get_task_state_str(UINT state);

//-------------------------------------------------------------------------------------
// Menu item structure for diagnostic menu
//
//  but   - key code for menu item selection
//  func  - pointer to the function to execute for this menu item
//  psubmenu - pointer to the submenu structure (if any)
//-------------------------------------------------------------------------------------
// clang-format off
const T_VT100_Menu_item MENU_DIAGNOSTIC_ITEMS[] =
{
  { '1', Diagnostic_Show_pin_states,   0                 },
  { '2', Diagnostic_Show_task_states,  0                 },
  { '3', Diagnostic_Show_heap_state,   0                 },
  { '4', Diagnostic_TMC6200,           0                 },
  { '5', Diagnostic_Motor,             0                 },
  { '6', Diagnostic_CAN,               0                 },
  { 'R', 0,                            0                 },
  { 'M', 0,                            (void *)&MENU_MAIN},
  { 0 } // End of menu
};
// clang-format on

const T_VT100_Menu MENU_DIAGNOSTIC = {
  "Application diagnostic menu",
  "\033[5C Diagnostic menu\r\n"
  "\033[5C <1> - MCU pin states\r\n"
  "\033[5C <2> - RTOS task states\r\n"
  "\033[5C <3> - RTOS heap (byte pool) state\r\n"
  "\033[5C <4> - TMC6200 driver state\r\n"
  "\033[5C <5> - Motor diagnostic\r\n"
  "\033[5C <6> - CAN diagnostic\r\n"
  "\033[5C <R> - Display previous menu\r\n"
  "\033[5C <M> - Display main menu\r\n",
  MENU_DIAGNOSTIC_ITEMS,
};

/*-----------------------------------------------------------------------------------------------------
  Show MCU pin states.

  Parameters:
    keycode   Not used.

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void Diagnostic_Show_pin_states(uint8_t keycode)
{
  GET_MCBL;
  uint32_t pin_count = Get_board_pin_count();  // Get total number of board pins
  char     pin_config_str[256];                // Buffer for pin configuration string

  MPRINTF("\r\n---------------------------------------------------\r\n");
  MPRINTF("MCU Pin States:\r\n");
  MPRINTF("---------------------------------------------------\r\n\r\n");

  for (uint32_t i = 0; i < pin_count; i++)
  {
    Get_board_pin_conf_str(i, pin_config_str);  // Get pin configuration as string
    MPRINTF("%s\r\n", pin_config_str);          // Print pin configuration
  }
  MPRINTF("---------------------------------------------------\r\n");

  // Wait for ESC
  uint8_t b;
  while (1)
  {
    if (WAIT_CHAR(&b, ms_to_ticks(100000)) == RES_OK)
    {
      if (b == VT100_ESC)
      {
        break;
      }
    }
  }
}

// clang-format off
/*-----------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------*/
static const char *_Get_task_state_str(UINT state)
{
  switch (state)
  {
    case TX_READY:           return "Ready     ";
    case TX_COMPLETED:       return "Completed ";
    case TX_TERMINATED:      return "Terminated";
    case TX_SUSPENDED:       return "Suspended ";
    case TX_SLEEP:           return "Sleep     ";
    case TX_QUEUE_SUSP:      return "Queue Susp";
    case TX_SEMAPHORE_SUSP:  return "Sem Susp  ";
    case TX_EVENT_FLAG:      return "EventFlg  ";
    case TX_BLOCK_MEMORY:    return "BlkMem    ";
    case TX_BYTE_MEMORY:     return "BytMem    ";
    case TX_FILE:            return "File      ";
    case TX_TCP_IP:          return "TCP/IP    ";
    default:                 return "Unknown   ";
  }
}
// clang-format on

/*-----------------------------------------------------------------------------------------------------
  Show RTOS task states.

  Parameters:
    keycode   Not used.

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void Diagnostic_Show_task_states(uint8_t keycode)
{
  GET_MCBL;
  extern TX_THREAD *_tx_thread_created_ptr;    // Head of created thread list
  extern ULONG      _tx_thread_created_count;  // Number of created threads

  // Объявление функции анализа стека ThreadX
  extern void _tx_thread_stack_analyze(TX_THREAD * thread_ptr);

  typedef struct
  {
    CHAR *name;
    UINT  state;
    ULONG run_count;
    UINT  priority;
    UINT  preemption_threshold;
    ULONG time_slice;
    VOID *stack_start;    // Stack start address (top)
    VOID *stack_end;      // Stack end address (bottom)
    ULONG stack_size;     // Stack size in bytes
    VOID *stack_ptr;      // Current stack pointer
    VOID *stack_highest;  // Highest used stack pointer (lowest address reached)
  } T_TaskInfo;

  ULONG       count = _tx_thread_created_count;
  T_TaskInfo *tasks = (T_TaskInfo *)App_malloc(sizeof(T_TaskInfo) * count);
  if (!tasks)
  {
    MPRINTF("\r\n[ERROR] Not enough memory for task info!\r\n");
    tx_thread_sleep(ms_to_ticks(2000));  // Give user time to read error
    return;
  }
  ULONG      real_count = 0;
  TX_THREAD *thread     = _tx_thread_created_ptr;

  for (ULONG i = 0; (thread != TX_NULL) && (i < count) && (real_count < 32); i++)
  {
    CHAR      *name                 = (CHAR *)"";
    UINT       state                = 0;
    ULONG      run_count            = 0;
    UINT       priority             = 0;
    UINT       preemption_threshold = 0;
    ULONG      time_slice           = 0;
    TX_THREAD *next                 = TX_NULL;
    // Инициализация для анализа стека, если вдруг не была выполнена при создании задачи
    if (thread->tx_thread_stack_highest_ptr == TX_NULL)
    {
      thread->tx_thread_stack_highest_ptr = thread->tx_thread_stack_end;
    }
    // Перед сбором информации анализируем стек задачи для корректного used_stack
    _tx_thread_stack_analyze(thread);
    UINT status = tx_thread_info_get(thread, &name, &state, &run_count, &priority, &preemption_threshold, &time_slice, &next, TX_NULL);
    if (status == TX_SUCCESS)
    {
      tasks[real_count].name                 = name;
      tasks[real_count].state                = state;
      tasks[real_count].run_count            = run_count;
      tasks[real_count].priority             = priority;
      tasks[real_count].preemption_threshold = preemption_threshold;
      tasks[real_count].time_slice           = time_slice;
      tasks[real_count].stack_start          = thread->tx_thread_stack_start;
      tasks[real_count].stack_end            = thread->tx_thread_stack_end;
      tasks[real_count].stack_size           = thread->tx_thread_stack_size;
      tasks[real_count].stack_ptr            = thread->tx_thread_stack_ptr;
      tasks[real_count].stack_highest        = thread->tx_thread_stack_highest_ptr;
      real_count++;
    }
    thread = thread->tx_thread_created_next;
    if (thread == _tx_thread_created_ptr)
    {
      break;
    }
  }

  // Find max name length
  int name_col_width = 4;  // min width for "Name"
  for (ULONG i = 0; i < real_count; i++)
  {
    int         len = 0;
    const char *p   = tasks[i].name;
    while (p && *p)
    {
      len++;
      p++;
    }
    if (len > name_col_width)
    {
      name_col_width = len;
    }
  }
  if (name_col_width < 8) name_col_width = 8;    // reasonable min width
  if (name_col_width > 64) name_col_width = 64;  // reasonable max width

  // Sort by priority
  for (ULONG i = 0; i < real_count; i++)
  {
    for (ULONG j = i + 1; j < real_count; j++)
    {
      if (tasks[j].priority < tasks[i].priority)
      {
        T_TaskInfo tmp = tasks[i];
        tasks[i]       = tasks[j];
        tasks[j]       = tmp;
      }
    }
  }

  // Print panel header
  MPRINTF("\r\n================ RTOS Task States ================\r\n\r\n");

  // Print table header
  int total_width = name_col_width + 3 + 12 + 3 + 8 + 3 + 8 + 3 + 10 + 3 + 10 + 3 + 10 + 3 + 7 + 2;
  for (int i = 0; i < total_width; i++) MPRINTF("-");
  MPRINTF("\r\n");
  MPRINTF(" %s%-*s | %12s | %8s | %8s | %10s | %10s | %10s | %7s\r\n",
          "", name_col_width, "Name", "State", "Priority", "RunCnt", "StackAddr", "StackSize", "UsedStack", "Stack % ");
  for (int i = 0; i < total_width; i++) MPRINTF("-");
  MPRINTF("\r\n");

  for (ULONG i = 0; i < real_count; i++)
  {
    const char *state_str  = _Get_task_state_str(tasks[i].state);
    ULONG       used_stack = 0;
    ULONG       start      = (ULONG)tasks[i].stack_start;
    ULONG       end        = (ULONG)tasks[i].stack_end;
    ULONG       high       = (ULONG)tasks[i].stack_highest;
    if (high >= start && high <= end)
    {
      used_stack = end - high;
    }
    else
    {
      used_stack = 0;
    }
    uint32_t stack_percent = 0;
    if (tasks[i].stack_size > 0)
    {
      stack_percent = (used_stack * 100) / tasks[i].stack_size;
    }
    MPRINTF(" %-*s | %12s | %8u | %8lu | 0x%08lX | %10lu | %10lu | %6lu\r\n",
            name_col_width, tasks[i].name,
            state_str,
            tasks[i].priority,
            tasks[i].run_count,
            (ULONG)tasks[i].stack_start,
            tasks[i].stack_size,
            used_stack,
            stack_percent);
  }
  for (int i = 0; i < total_width; i++) MPRINTF("-");
  MPRINTF("\r\n");

  App_free(tasks);

  // Wait for ESC
  uint8_t b;
  while (1)
  {
    if (WAIT_CHAR(&b, ms_to_ticks(100000)) == RES_OK)
    {
      if (b == VT100_ESC)
      {
        break;
      }
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Show RTOS heap (byte pool) state.

  Parameters:
    keycode   Not used.

  Return:
    None
-----------------------------------------------------------------------------------------------------*/
static void Diagnostic_Show_heap_state(uint8_t keycode)
{
  GET_MCBL;

  // Show statistics for the main application byte pool (g_app_pool)
  extern TX_BYTE_POOL g_app_pool;
  CHAR               *pool_name       = NULL;  // Имя пула (строка, заданная при создании пула)
  ULONG               avail_bytes     = 0;     // Количество свободных (доступных для аллокации) байт в пуле
  ULONG               fragments       = 0;     // Количество фрагментов памяти в пуле (чем больше, тем выше фрагментация)
  TX_THREAD          *first_suspended = NULL;  // Указатель на первый поток, ожидающий освобождения памяти в пуле (если есть)
  ULONG               suspended_count = 0;     // Количество потоков, ожидающих память из пула (обычно 0, если нет блокировок)
  TX_BYTE_POOL       *next_pool       = NULL;  // Указатель на следующий пул в системе (если используется несколько пулов)

  UINT status                         = tx_byte_pool_info_get(&g_app_pool, &pool_name, &avail_bytes, &fragments, &first_suspended, &suspended_count, &next_pool);

  MPRINTF("\r\n================ RTOS Heap (Byte Pool) State ================\r\n\r\n");
  if (status == TX_SUCCESS)
  {
    ULONG    pool_size    = g_app_pool.tx_byte_pool_size;                                                       // Общий размер пула в байтах (выделено под пул при инициализации)
    void    *pool_start   = g_app_pool.tx_byte_pool_start;                                                      // Начальный адрес области памяти, выделенной под пул
    void    *pool_end     = (void *)((uintptr_t)g_app_pool.tx_byte_pool_start + g_app_pool.tx_byte_pool_size);  // Конечный адрес области памяти пула (не включительно)
    ULONG    used_bytes   = pool_size - avail_bytes;                                                            // Количество занятых байт в пуле
    uint32_t used_percent = 0;                                                                                  // Процент занятой памяти
    if (pool_size > 0)
    {
      used_percent = (uint32_t)(((uint64_t)used_bytes * 100) / pool_size);
    }
    // Табличный выровненный вывод
    MPRINTF(" %-20s : %s\r\n", "Pool name", pool_name ? pool_name : "(null)");
    MPRINTF(" %-20s : 0x%08lX\r\n", "Start address", (unsigned long)(uintptr_t)pool_start);
    MPRINTF(" %-20s : 0x%08lX\r\n\r\n", "End address", (unsigned long)(uintptr_t)pool_end);
    MPRINTF(" %-20s : %10lu bytes\r\n", "Total size", (unsigned long)pool_size);
    MPRINTF(" %-20s : %10lu bytes\r\n", "Used", (unsigned long)used_bytes);            // Сколько байт занято
    MPRINTF(" %-20s : %10lu bytes\r\n", "Available", (unsigned long)avail_bytes);
    MPRINTF(" %-20s : %9lu %%\r\n\r\n", "Used percent", (unsigned long)used_percent);  // Процент занятой памяти
    MPRINTF(" %-20s : %10lu\r\n", "Fragments", (unsigned long)fragments);
    MPRINTF(" %-20s : %10lu\r\n", "Suspended threads", (unsigned long)suspended_count);
// Попробовать получить статистику аллокаций/освобождений, если поддерживается
#ifdef TX_BYTE_POOL_ENABLE_PERFORMANCE_INFO
    ULONG allocates = 0, releases = 0, fragments_searched = 0, merges = 0, splits = 0, suspensions = 0, timeouts = 0;  // Счетчики производительности пула
    // allocates          - Общее количество успешных аллокаций памяти из пула с момента создания
    // releases           - Общее количество освобождений (возвратов памяти) в пул
    // fragments_searched - Сколько фрагментов памяти было просмотрено при поиске подходящего блока (характеризует фрагментацию)
    // merges             - Сколько раз происходило слияние соседних свободных блоков (дефрагментация)
    // splits             - Сколько раз происходило разделение блока на части при аллокации
    // suspensions        - Сколько раз потоки были заблокированы в ожидании памяти из пула
    // timeouts           - Сколько раз ожидание памяти завершалось по таймауту (без выделения)
    if (tx_byte_pool_performance_info_get(&g_app_pool, &allocates, &releases, &fragments_searched, &merges, &splits, &suspensions, &timeouts) == TX_SUCCESS)
    {
      MPRINTF(" %-20s : %10lu\r\n", "Allocations", (unsigned long)allocates);
      MPRINTF(" %-20s : %10lu\r\n", "Releases", (unsigned long)releases);
      MPRINTF(" %-20s : %10lu\r\n", "Merges", (unsigned long)merges);
      MPRINTF(" %-20s : %10lu\r\n", "Splits", (unsigned long)splits);
      MPRINTF(" %-20s : %10lu\r\n", "Fragments searched", (unsigned long)fragments_searched);
      MPRINTF(" %-20s : %10lu\r\n", "Suspensions", (unsigned long)suspensions);
      MPRINTF(" %-20s : %10lu\r\n", "Timeouts", (unsigned long)timeouts);
    }
#endif
  }
  else
  {
    MPRINTF("[ERROR] tx_byte_pool_info_get failed!\r\n");
  }
  MPRINTF("------------------------------------------------------------\r\n");

  // Wait for ESC
  uint8_t b;
  while (1)
  {
    if (WAIT_CHAR(&b, ms_to_ticks(100000)) == RES_OK)
    {
      if (b == VT100_ESC)
      {
        break;
      }
    }
  }
}
