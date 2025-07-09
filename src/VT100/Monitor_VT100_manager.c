#include "App.h"

TX_MUTEX         vt100_mutex;
T_VT100_task_cbl vt100_task_cbls[VT100_TASKS_MAX_NUM];

static uint8_t   VT100_stacks[VT100_MANAGER_THREAD_STACK_SIZE] BSP_PLACE_IN_SECTION(".stack.vt100_manager") BSP_ALIGN_VARIABLE(BSP_STACK_ALIGNMENT);
static TX_THREAD vt100_manager_thread;

T_vt100_man_msg vt100_man_queue_buf[VT100_MAG_QUEUE_BUF_LEN];
TX_QUEUE        vt100_man_queue;

/*-----------------------------------------------------------------------------------------------------
  Main screen callback for monitor VT100

  Parameters:
    void

  Return:
    __weak void
-----------------------------------------------------------------------------------------------------*/
__weak void Application_monitor_main_screen(void)
{
}

/*-----------------------------------------------------------------------------------------------------
  Send message to VT100 task manager

  Parameters:
    msg_ptr - pointer to message
-----------------------------------------------------------------------------------------------------*/
void Send_message_to_VT100_task_manager(T_vt100_man_msg *msg_ptr)
{
  tx_queue_send(&vt100_man_queue, msg_ptr, TX_NO_WAIT);  // Send message to queue
}

/*-----------------------------------------------------------------------------------------------------
  Manager task receives messages about VT100 task creation and deletion together with driver tasks

  Parameters:
    arg - not used
-----------------------------------------------------------------------------------------------------*/
static void VT100_task_manager_thread(ULONG arg)
{
  T_vt100_man_msg      msg;
  T_vt100_man_callback func;

  while (tx_queue_receive(&vt100_man_queue, &msg, TX_WAIT_FOREVER) == TX_SUCCESS)
  {
    func = (T_vt100_man_callback)msg.arg1;
    func(&msg);
  }
}

/*-----------------------------------------------------------------------------------------------------
  Initialization of VT100 monitor manager

  Parameters:
    void

  Return:
    uint32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t VT100_task_manager_initialization(void)
{
  uint32_t res;
  res = tx_mutex_create(&vt100_mutex, "vt100_mutex", TX_INHERIT);
  if (res != TX_SUCCESS)
  {
    goto err_exit;
  }

  res = tx_queue_create(&vt100_man_queue, (CHAR *)"VT100_manager", sizeof(T_vt100_man_msg) / sizeof(uint32_t), vt100_man_queue_buf, sizeof(T_vt100_man_msg) * VT100_MAG_QUEUE_BUF_LEN);
  if (res != TX_SUCCESS)
  {
    goto err_exit;
  }
  res = tx_thread_create(
  &vt100_manager_thread,
  (CHAR *)"VT100 manager",
  VT100_task_manager_thread,
  0,
  VT100_stacks,
  VT100_MANAGER_THREAD_STACK_SIZE,
  THREAD_PRIORITY_VT100_MANAGER,
  THREAD_PREEMPT_VT100_MANAGER,
  THREAD_TIME_SLICE_VT100_MANAGER,
  TX_AUTO_START);
  if (res == TX_SUCCESS)
  {
    EAPPLOG("VT100 manager task created");
  }
  else
  {
    EAPPLOG("VT100 manager task creation error: %04X", res);
    goto err_exit;
  }

  return RES_OK;

err_exit:
  tx_mutex_delete(&vt100_mutex);
  tx_queue_delete(&vt100_man_queue);
  tx_thread_delete(&vt100_manager_thread);
  EAPPLOG("VT100 manager creating error %d.", res);
  return RES_ERROR;
}

/*-------------------------------------------------------------------------------------------------------------
  VT100 monitor task

  initial_data contains the index of the task in the VT100 task control block array
-------------------------------------------------------------------------------------------------------------*/
static void Task_VT100(ULONG arg)
{
  uint8_t b;

  // Pass pointers to monitor and driver structures to the task structure
  T_monitor_cbl *monitor_cbl        = (T_monitor_cbl *)(arg);

  tx_thread_identify()->environment = (ULONG)(monitor_cbl);
  tx_thread_identify()->driver      = (ULONG)(monitor_cbl->pdrv);

  GET_MCBL;

  fx_directory_default_set(&fat_fs_media, "/");

  do
  {
    MPRINTF(VT100_CLEAR_AND_HOME);
  } while (Access_control() != RES_OK);

  // Clear screen
  MPRINTF(VT100_CLEAR_AND_HOME);
  Goto_main_menu();
  do
  {
    if (VT100_wait_special_key(&b, ms_to_ticks(1000)) == RES_OK)
    {
      if (b != 0)
      {
        if ((b == 0x1B) && (mcbl->Monitor_func != Edit_func))
        {
          MPRINTF(VT100_CLEAR_AND_HOME);
          // Entry_check();
          Goto_main_menu();
        }
        else
        {
          if (mcbl->Monitor_func)
          {
            mcbl->Monitor_func(b);  // Main loop key handler
          }
        }
      }
    }

    if (mcbl->menu_trace[mcbl->menu_nesting] == &MENU_MAIN)
    {
      rtc_time_t curr_time;

      VT100_set_cursor_pos(15, 0);
      MPRINTF(VT100_CLR_LINE "Software     : %s\r\n", wvar.software_version);
      MPRINTF(VT100_CLR_LINE "Hardware     : %s\r\n", wvar.hardware_version);
      MPRINTF(VT100_CLR_LINE "Compile time : %s %s\r\n", Get_build_date(), Get_build_time());
      MPRINTF(VT100_CLR_LINE "Up time      : %d s. CPU usage %d%% \r\n",
              _tx_time_get() / TX_TIMER_TICKS_PER_SECOND,
              g_aver_cpu_usage / 10);

      RTC_get_system_DateTime(&curr_time);
      MPRINTF(VT100_CLR_LINE "Date Time    : %04d.%02d.%02d  %02d:%02d:%02d\r\n", curr_time.tm_year, curr_time.tm_mon, curr_time.tm_mday, curr_time.tm_hour, curr_time.tm_min, curr_time.tm_sec);

      {
        uint32_t ab1;
        uint32_t fragments;
        App_get_RAM_pool_statistic(&ab1, &fragments);
        if (g_file_system_ready)
        {
          MPRINTF(VT100_CLR_LINE "Chip RAM free: %d bytes, SD card: Operating\r\n", ab1);
        }
        else
        {
          MPRINTF(VT100_CLR_LINE "Chip RAM free: %d bytes, SD card: Non-Operating\r\n", ab1);
        }
      }

      Application_monitor_main_screen();
    }
  } while (1);
}

/*-----------------------------------------------------------------------------------------------------
  Create VT100 task. No more than VT100_TASKS_MAX_NUM tasks can be created. Task does not start automatically. Use Task_VT100_start to start it.

  Parameters:
    serial_drv_ptr - pointer to serial driver
    task_instance_index_ptr - pointer to index variable

  Return:
    uint32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t Task_VT100_create(T_serial_io_driver *serial_drv_ptr, int32_t *task_instance_index_ptr)
{
  UINT    res;
  int32_t instance_indx = -1;

  if ((task_instance_index_ptr == 0) || (serial_drv_ptr == 0))
  {
    return RES_ERROR;
  }

  if (tx_mutex_get(&vt100_mutex, MS_TO_TICKS(1000)) != TX_SUCCESS)
  {
    return RES_ERROR;
  }

  // Find free slot for task
  for (uint32_t i = 0; i < VT100_TASKS_MAX_NUM; i++)
  {
    if (vt100_task_cbls[i].taken == 0)
    {
      instance_indx                                  = i;
      vt100_task_cbls[instance_indx].taken           = 1;

      // Allocate memory for monitor structure
      vt100_task_cbls[instance_indx].monitor_cbl_ptr = App_malloc(sizeof(T_monitor_cbl));
      if (vt100_task_cbls[instance_indx].monitor_cbl_ptr == NULL)
      {
        break;
      }

      // Allocate memory for thread structure
      vt100_task_cbls[instance_indx].VT100_thread_prt = App_malloc(sizeof(TX_THREAD));
      if (vt100_task_cbls[instance_indx].VT100_thread_prt == NULL)
      {
        break;
      }

      // Allocate memory for thread stack
      vt100_task_cbls[instance_indx].VT100_thread_stack = App_malloc(VT100_THREAD_STACK_SIZE);
      if (vt100_task_cbls[instance_indx].VT100_thread_stack == NULL)
      {
        break;
      }

      vt100_task_cbls[instance_indx].monitor_cbl_ptr->pdrv = serial_drv_ptr;
      snprintf(vt100_task_cbls[instance_indx].task_name, VT100_TASK_NAME_MAX_SZ, "VT100_%d", i);
      break;
    }
  }
  if (instance_indx == -1)
  {
    goto err_exit;
  }

  // Initialize driver
  if (serial_drv_ptr->_init(&serial_drv_ptr->drv_cbl_ptr, serial_drv_ptr) != RES_OK)
  {
    vt100_task_cbls[instance_indx].taken = 0;
    goto err_exit;
  }
  res = tx_thread_create(
  vt100_task_cbls[instance_indx].VT100_thread_prt,
  (CHAR *)vt100_task_cbls[instance_indx].task_name,
  Task_VT100,
  (ULONG)(vt100_task_cbls[instance_indx].monitor_cbl_ptr),
  vt100_task_cbls[instance_indx].VT100_thread_stack,
  VT100_THREAD_STACK_SIZE,
  THREAD_PRIORITY_VT100,
  THREAD_PREEMPT_VT100,
  THREAD_TIME_SLICE_VT100,
  0);
  if (res == TX_SUCCESS)
  {
    EAPPLOG("VT100 task %d created", instance_indx);
  }
  else
  {
    EAPPLOG("VT100 task %d creation error: %04X", instance_indx, res);
    serial_drv_ptr->_deinit(&serial_drv_ptr->drv_cbl_ptr);
    goto err_exit;
  }

  *task_instance_index_ptr = instance_indx;
  tx_mutex_put(&vt100_mutex);
  return RES_OK;

err_exit:
  if (instance_indx != -1)
  {
    vt100_task_cbls[instance_indx].taken = 0;
    memset(vt100_task_cbls[instance_indx].task_name, 0, VT100_TASK_NAME_MAX_SZ);
    App_free(vt100_task_cbls[instance_indx].monitor_cbl_ptr);
    App_free(vt100_task_cbls[instance_indx].VT100_thread_prt);
    App_free(vt100_task_cbls[instance_indx].VT100_thread_stack);
    EAPPLOG("VT100 task %d creating error", instance_indx);
  }
  tx_mutex_put(&vt100_mutex);
  return RES_ERROR;
}

/*-----------------------------------------------------------------------------------------------------
  Start VT100 task

  Parameters:
    instance_indx - index of task

  Return:
    uint32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t Task_VT100_start(int32_t instance_indx)
{
  UINT res;

  if (instance_indx >= VT100_TASKS_MAX_NUM)
  {
    return RES_ERROR;
  }

  res = tx_thread_resume(vt100_task_cbls[instance_indx].VT100_thread_prt);
  if (res != TX_SUCCESS)
  {
    return RES_ERROR;
  }
  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Suspend VT100 task

  Parameters:
    instance_indx - index of task

  Return:
    uint32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t Task_VT100_suspend(int32_t instance_indx)
{
  UINT res;

  if (instance_indx >= VT100_TASKS_MAX_NUM)
  {
    return RES_ERROR;
  }

  res = tx_thread_suspend(vt100_task_cbls[instance_indx].VT100_thread_prt);
  if (res != TX_SUCCESS)
  {
    return RES_ERROR;
  }
  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Delete VT100 task

  Parameters:
    instance_indx - index of task

  Return:
    uint32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t Task_VT100_delete(int32_t instance_indx)
{
  UINT res;

  if (instance_indx >= VT100_TASKS_MAX_NUM)
  {
    return RES_ERROR;
  }

  if (tx_mutex_get(&vt100_mutex, MS_TO_TICKS(1000)) != TX_SUCCESS)
  {
    return RES_ERROR;
  }

  res = tx_thread_terminate(vt100_task_cbls[instance_indx].VT100_thread_prt);
  if (res == TX_SUCCESS)
  {
    res = tx_thread_delete(vt100_task_cbls[instance_indx].VT100_thread_prt);
    if (res == TX_SUCCESS)
    {
      EAPPLOG("VT100 task %d deleted", instance_indx);
    }
    else
    {
      EAPPLOG("VT100 task %d deleting error %d", instance_indx, res);
    }
  }
  else
  {
    EAPPLOG("VT100 task %d terminating error %d", instance_indx, res);
  }

  vt100_task_cbls[instance_indx].monitor_cbl_ptr->pdrv->_deinit(&(vt100_task_cbls[instance_indx].monitor_cbl_ptr->pdrv->drv_cbl_ptr));

  App_free(vt100_task_cbls[instance_indx].monitor_cbl_ptr);
  App_free(vt100_task_cbls[instance_indx].VT100_thread_prt);
  App_free(vt100_task_cbls[instance_indx].VT100_thread_stack);
  memset(vt100_task_cbls[instance_indx].task_name, 0, VT100_TASK_NAME_MAX_SZ);
  vt100_task_cbls[instance_indx].taken = 0;

  tx_mutex_put(&vt100_mutex);
  return RES_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Delete all tasks related to TELNET

  Parameters:
    void

  Return:
    uint32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t VT100_Telnet_sessions_delete(void)
{
  for (uint32_t i = 0; i < VT100_TASKS_MAX_NUM; i++)
  {
    if (vt100_task_cbls[i].taken != 0)
    {
      if (vt100_task_cbls[i].monitor_cbl_ptr->pdrv->driver_type == MN_NET_TELNET_DRIVER)
      {
        Task_VT100_delete(i);
      }
    }
  }
  return RES_OK;
}
