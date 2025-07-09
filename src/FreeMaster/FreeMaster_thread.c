#include "App.h"
#include "freemaster_cfg.h"
#include "freemaster.h"
#include "freemaster_tsa.h"

#define FM_PIPE_RX_BUF_SIZE 64
#define FM_PIPE_TX_BUF_SIZE 1024
#define FM_PIPE_MAX_STR_LEN 512

#define FM_PIPE_PORT_NUM    0
#define FM_PIPE_CALLBACK    0
#define FM_PIPE_TYPE        FMSTR_PIPE_TYPE_ANSI_TERMINAL

// Structure for FreeMaster pipe data - single memory allocation
// Total size: 64 + 1024 + sizeof(T_logger_record) + 512 = ~1600+ bytes
typedef struct
{
  uint8_t         rx_buffer[FM_PIPE_RX_BUF_SIZE];  // RX buffer (64 bytes)
  uint8_t         tx_buffer[FM_PIPE_TX_BUF_SIZE];  // TX buffer (1024 bytes)
  T_logger_record log_rec;                         // Logger record
  char            log_str[FM_PIPE_MAX_STR_LEN];    // Log string buffer (512 bytes)
} T_freemaster_pipe_data;

FMSTR_HPIPE             fm_pipe = NULL;
T_logger_record        *p_log_rec;
char                   *log_str;
T_freemaster_pipe_data *fm_pipe_data = NULL;  // Single allocation for all pipe data
uint8_t                 f_unsent_record;
uint32_t                g_freemaster_interface_type;

static uint8_t   freemaster_stack[FREEMASTER_THREAD_STACK_SIZE] BSP_PLACE_IN_SECTION(".stack.freemaster_thread") BSP_ALIGN_VARIABLE(BSP_STACK_ALIGNMENT);
static TX_THREAD freemaster_thread;

extern T_freemaster_serial_driver *frm_serial_drv;

static void Thread_FreeMaster(ULONG initial_data);
static void _Free_freemaster_pipe_data(void);
static void _Create_freemaster_pipe(void);

/*-----------------------------------------------------------------------------------------------------
  Вызывается из контекста удаляемой задачи после того как она выключена

  \param thread_ptr
  \param condition
-----------------------------------------------------------------------------------------------------*/
static void FreeMaster_entry_exit_notify(TX_THREAD *thread_ptr, UINT condition)
{
  if (condition == TX_THREAD_ENTRY)
  {
  }
  else if (condition == TX_THREAD_EXIT)
  {
    FreeMaster_task_delete();
  }
}

/*-----------------------------------------------------------------------------------------------------


  \param void
-----------------------------------------------------------------------------------------------------*/
void FreeMaster_task_delete(void)
{
  tx_thread_terminate(&freemaster_thread);
  tx_thread_delete(&freemaster_thread);

  // Free pipe data using dedicated function
  _Free_freemaster_pipe_data();
}

/*-----------------------------------------------------------------------------------------------------


  \param str
  \param max_str_len
  \param p_log_rec
-----------------------------------------------------------------------------------------------------*/
void Format_log_string(char *str, uint32_t max_str_len, T_logger_record *p_log_rec)
{
  uint64_t t64 = p_log_rec->delta_time;
  uint32_t t32;
  uint32_t time_msec = t64 % 1000000ull;
  t32                = (uint32_t)(t64 / 1000000ull);
  uint32_t time_sec  = t32 % 60;
  uint32_t time_min  = (t32 / 60) % 60;
  uint32_t time_hour = (t32 / (60 * 60)) % 24;
  uint32_t time_day  = t32 / (60 * 60 * 24);

  snprintf(str, max_str_len, "%03d d %02d h %02d m %02d s %06d us |", time_day, time_hour, time_min, time_sec, time_msec);
  uint32_t len = strlen(str);

  if (p_log_rec->line_num != 0)
  {
    snprintf(&str[len], max_str_len - len, " %s (%s %d)\n\r",
             p_log_rec->msg,
             p_log_rec->func_name,
             p_log_rec->line_num);
  }
  else
  {
    snprintf(&str[len], max_str_len - len, " %s\n\r",
             p_log_rec->msg);
  }
}

/*-----------------------------------------------------------------------------------------------------
  Пересылаем данные из лога приложения в канал FreeMaster

  \param void
-----------------------------------------------------------------------------------------------------*/
void Freemaster_send_log_to_pipe(void)
{
  if (f_unsent_record != 0)
  {
    if (FMSTR_PipePuts(fm_pipe, log_str) != FMSTR_TRUE) return;
    f_unsent_record = 0;
  }

  while (AppLog_get_tail_record(p_log_rec) == RES_OK)
  {
    Format_log_string(log_str, FM_PIPE_MAX_STR_LEN, p_log_rec);
    if (FMSTR_PipePuts(fm_pipe, log_str) != FMSTR_TRUE)
    {
      f_unsent_record = 1;
      return;
    }
  }
}

/*-------------------------------------------------------------------------------------------------------------
  Цикл движка FreeMaster
-------------------------------------------------------------------------------------------------------------*/
static void Thread_FreeMaster(ULONG initial_data)
{
  uint16_t app_command;
  uint8_t  res;
  if (initial_data == FREEMASTER_ON_NET)
  {
    // Ожидаем инициализации стека сетевого стека BSD
    // TODO: Replace g_BSD_initialised with proper initialization check
    // while (g_BSD_initialised == 0)
    // {
    //   Wait_ms(10);
    // }

    if (!FMSTR_Init((void *)&FMSTR_NET))
    {
      APPLOG("FreeMaster: Failed to initialize NET interface");
      return;
    }
    APPLOG("FreeMaster: Successfully started on NET interface");
  }
  else if (initial_data == FREEMASTER_ON_SERIAL)
  {
    frm_serial_drv = Get_FreeMaster_usb_vcom1_driver();
    if (frm_serial_drv->_init(&frm_serial_drv->drv_cbl_ptr, frm_serial_drv) != RES_OK)
    {
      APPLOG("FreeMaster: Failed to initialize VCOM1 driver");
      return;
    }
    tx_thread_identify()->driver = (ULONG)(frm_serial_drv);
    if (!FMSTR_Init((void *)&FMSTR_SERIAL))
    {
      APPLOG("FreeMaster: Failed to initialize SERIAL interface");
      return;
    }
    APPLOG("FreeMaster: Successfully started on SERIAL interface (VCOM1)");
  }
  else
  {
    APPLOG("FreeMaster: Invalid interface type: %d", (int)initial_data);
    return;
  }

  // Create FreeMaster pipe
  _Create_freemaster_pipe();

  while (1)
  {
    app_command = FMSTR_GetAppCmd();

    if (app_command != FMSTR_APPCMDRESULT_NOCMD)
    {
      res = Freemaster_Command_Manager(app_command);
      FMSTR_AppCmdAck(res);
    }
    FMSTR_Poll();
    if (fm_pipe != NULL)
    {
      Freemaster_send_log_to_pipe();
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Free FreeMaster pipe data and reset all related pointers

  \param void
-----------------------------------------------------------------------------------------------------*/
static void _Free_freemaster_pipe_data(void)
{
  if (fm_pipe_data != NULL)
  {
    App_free(fm_pipe_data);
    fm_pipe_data = NULL;
  }

  // Reset global pointers
  p_log_rec = NULL;
  log_str   = NULL;
  fm_pipe   = NULL;
}

/*-----------------------------------------------------------------------------------------------------
  Create FreeMaster pipe with memory allocation

  \param void
-----------------------------------------------------------------------------------------------------*/
static void _Create_freemaster_pipe(void)
{
  FMSTR_ADDR      pipeRxBuff;
  FMSTR_PIPE_SIZE pipeRxSize;
  FMSTR_ADDR      pipeTxBuff;
  FMSTR_PIPE_SIZE pipeTxSize;

  // Single memory allocation for all pipe data
  fm_pipe_data = App_malloc(sizeof(T_freemaster_pipe_data));
  if (fm_pipe_data != NULL)
  {
    // Initialize local pointers to allocated memory
    pipeRxBuff = fm_pipe_data->rx_buffer;
    pipeRxSize = FM_PIPE_RX_BUF_SIZE;
    pipeTxBuff = fm_pipe_data->tx_buffer;
    pipeTxSize = FM_PIPE_TX_BUF_SIZE;
    p_log_rec  = &fm_pipe_data->log_rec;
    log_str    = fm_pipe_data->log_str;

    // Create FreeMaster pipe
    fm_pipe    = FMSTR_PipeOpen(FM_PIPE_PORT_NUM, FM_PIPE_CALLBACK, pipeRxBuff, pipeRxSize, pipeTxBuff, pipeTxSize, FM_PIPE_TYPE, "SysLog");
    if (fm_pipe != NULL)
    {
      APPLOG("FreeMaster: Pipe 'SysLog' created successfully");
    }
    else
    {
      APPLOG("FreeMaster: Failed to create pipe 'SysLog'");
      // Free allocated memory since pipe creation failed but continue task execution
      _Free_freemaster_pipe_data();
    }
  }
  else
  {
    APPLOG("FreeMaster: Failed to allocate memory for pipe data (%d bytes)", sizeof(T_freemaster_pipe_data));
    // Continue task execution even without pipe
  }
}

/*-----------------------------------------------------------------------------------------------------


  \param void
-----------------------------------------------------------------------------------------------------*/
void Determine_FreeMaster_interface_type(void)
{
  if (wvar.usb_mode == USB_MODE_VCOM_AND_FREEMASTER_PORT)
  {
    g_freemaster_interface_type = FREEMASTER_ON_SERIAL;
  }
  else if (wvar.usb_mode == USB_MODE_RNDIS)
  {
    g_freemaster_interface_type = FREEMASTER_ON_NET;
  }
  else if (wvar.usb_mode == USB_MODE_HOST_ECM)
  {
    g_freemaster_interface_type = FREEMASTER_ON_NET;
  }
  else
  {
    // No suitable interface for FreeMaster
    g_freemaster_interface_type = FREEMASTER_NO_INTERFACE;
  }
}

/*-----------------------------------------------------------------------------------------------------


  \param void

  \return uint32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t Thread_FreeMaster_create(void)
{
  UINT res;

  Determine_FreeMaster_interface_type();

  // Check if FreeMaster has a suitable interface
  if (g_freemaster_interface_type == FREEMASTER_NO_INTERFACE)
  {
    APPLOG("FreeMaster: No suitable interface available (USB mode: %d)", wvar.usb_mode);
    return RES_OK;  // Return success but don't create thread
  }
  res = tx_thread_create(
  &freemaster_thread,
  (CHAR *)"FreeMaster",
  Thread_FreeMaster,
  (ULONG)g_freemaster_interface_type,
  freemaster_stack,
  FREEMASTER_THREAD_STACK_SIZE,
  THREAD_PRIORITY_FREEMASTER,
  THREAD_PRIORITY_FREEMASTER,
  THREAD_TIME_SLICE_FREEMASTER,
  TX_AUTO_START);

  if (res != TX_SUCCESS)
  {
    APPLOG("FreeMaster task creation failed: %d", res);
    return RES_ERROR;
  }

  tx_thread_entry_exit_notify(&freemaster_thread, FreeMaster_entry_exit_notify);

  return RES_OK;
}
