// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2019.08.06
// 9:30:25
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "App.h"
#include "USB_descriptors.h"

extern uint32_t usb_peri_usbx_initialize(uint32_t dcd_io);

// Размеры даны для случая с закоментированнными макросами UX_HOST_SIDE_ONLY и UX_DEVICE_SIDE_ONLY
#define USBX_REGULAR_MEMORY_SIZE    (25000)         // 12000 Минимальный размер определенный на основе статистики и тестирования при работе класса CDC ECM
                                                    // 24000 Минимальный размер определенный на основе статистики и тестирования при работе класса RNDIS
#define USBX_CACHE_SAFE_MEMORY_SIZE (38000)         // 32000 Минимальный размер определенный на основе статистики и тестирования при работе класса CDC ECM
                                                    // 38000 Минимальный размер определенный на основе статистики и тестирования при работе класса RNDIS

uint8_t usb_mem_regular[USBX_REGULAR_MEMORY_SIZE];  // Область для кэшируемой динамической памяти
uint8_t usb_mem_cache_safe[USBX_CACHE_SAFE_MEMORY_SIZE];

static uint8_t *usbd_hs_framework;
static uint8_t *usbd_fs_framework;

static uint8_t usb_1_interface_type;
static uint8_t usb_2_interface_type;

static T_usb_device_suspend_callback usb_device_suspend_callback;
/*-----------------------------------------------------------------------------------------------------


  \param void

  \return uint8_t
-----------------------------------------------------------------------------------------------------*/
uint8_t Get_usb_1_mode(void)
{
  return usb_1_interface_type;
}

/*-----------------------------------------------------------------------------------------------------


  \param void

  \return uint8_t
-----------------------------------------------------------------------------------------------------*/
uint8_t Get_usb_2_mode(void)
{
  return usb_2_interface_type;
}

/*-----------------------------------------------------------------------------------------------------


  \param callback_func
-----------------------------------------------------------------------------------------------------*/
void USB_device_set_suspend_callback(T_usb_device_suspend_callback callback_func)
{
  usb_device_suspend_callback = callback_func;
}
/*-----------------------------------------------------------------------------------------------------
  Функция вызыванется из прерывания usbhs_usb_int_resume_isr -> ux_dcd_synergy_interrupt_handler


  \param arg    UX_DEVICE_RESET
                UX_DEVICE_ATTACHED
                UX_DEVICE_ADDRESSED
                UX_DEVICE_CONFIGURED
                UX_DEVICE_SUSPENDED
                UX_DEVICE_RESUMED
                UX_DEVICE_SELF_POWERED_STATE
                UX_DEVICE_BUS_POWERED_STATE
                UX_DEVICE_REMOTE_WAKEUP
                UX_DEVICE_BUS_RESET_COMPLETED
                UX_DEVICE_REMOVED
                UX_DEVICE_FORCE_DISCONNECT

  При отключении кабеля функция вызывается с аргументом UX_DEVICE_SUSPENDED

  \return UINT
-----------------------------------------------------------------------------------------------------*/
static UINT ux_system_slave_change_callback(ULONG arg)
{
  switch (arg)
  {
    case UX_DEVICE_RESET:
      APPLOG("USB: event RESET");
      break;
    case UX_DEVICE_ATTACHED:
      APPLOG("USB: event ATTACHED");
      break;
    case UX_DEVICE_ADDRESSED:
      APPLOG("USB: event ADDRESSED");
      break;
    case UX_DEVICE_CONFIGURED:
      APPLOG("USB: event CONFIGURED");
      break;
    case UX_DEVICE_SUSPENDED:
      APPLOG("USB: event SUSPENDED");
      if (usb_device_suspend_callback != NULL) usb_device_suspend_callback();
      break;
    case UX_DEVICE_RESUMED:
      APPLOG("USB: event RESUMED");
      break;
    case UX_DEVICE_SELF_POWERED_STATE:
      APPLOG("USB: event SELF_POWERED_STATE");
      break;
    case UX_DEVICE_BUS_POWERED_STATE:
      APPLOG("USB: event BUS_POWERED_STATE");
      break;
    case UX_DEVICE_REMOTE_WAKEUP:
      APPLOG("USB: event REMOTE_WAKEUP");
      break;
    case UX_DEVICE_BUS_RESET_COMPLETED:
      APPLOG("USB: event BUS_RESET_COMPLETED");
      break;
    case UX_DEVICE_REMOVED:
      APPLOG("USB: event REMOVED");
      break;
    case UX_DEVICE_FORCE_DISCONNECT:
      APPLOG("USB: event FORCE_DISCONNECT");
      break;
    default:
      APPLOG("USB: unknown event: 0x%lX", arg);
      break;
  }
  return 0;
}
/*-----------------------------------------------------------------------------------------------------


  \param void
-----------------------------------------------------------------------------------------------------*/
void Set_usb_mode(void)
{
  if (g_file_system_ready == 0)
  {
    if (wvar.usb_mode == USB_MODE_MASS_STORAGE_)
    {
      wvar.usb_mode = USB_MODE_VCOM_PORT;
    }
    else if (wvar.usb_mode == USB_MODE_VCOM_AND_MASS_STORAGE)
    {
      wvar.usb_mode = USB_MODE_VCOM_PORT;
    }
  }

  switch (wvar.usb_mode)
  {
    case USB_MODE_NONE:
      usb_1_interface_type = USB1_INTF_OFF;
      usb_2_interface_type = USB2_INTF_OFF;
      break;
    case USB_MODE_VCOM_PORT:
      usb_1_interface_type = USB1_INTF_VIRTUAL_COM_PORT;
      usb_2_interface_type = USB2_INTF_OFF;
      break;
    case USB_MODE_MASS_STORAGE_:
      usb_1_interface_type = USB1_INTF_MASS_STORAGE_DEVICE;
      usb_2_interface_type = USB2_INTF_OFF;
      break;
    case USB_MODE_VCOM_AND_MASS_STORAGE:
      usb_1_interface_type = USB1_INTF_VIRTUAL_COM_PORT;
      usb_2_interface_type = USB2_INTF_MASS_STORAGE_DEVICE;
      break;
    case USB_MODE_VCOM_AND_FREEMASTER_PORT:
      usb_1_interface_type = USB1_INTF_VIRTUAL_COM_PORT;
      usb_2_interface_type = USB2_INTF_FREEMASTER_PORT;
      break;
    case USB_MODE_RNDIS:
      usb_1_interface_type = USB1_INTF_RNDIS_PORT;
      usb_2_interface_type = USB2_INTF_OFF;
      break;
    case USB_MODE_HOST_ECM:
      usb_1_interface_type = USB1_INTF_OFF;
      usb_2_interface_type = USB2_INTF_OFF;
      break;
    default:
      usb_1_interface_type = USB1_INTF_OFF;
      usb_2_interface_type = USB2_INTF_OFF;
      break;
  }
}

/*-----------------------------------------------------------------------------------------------------



  \return ULONG
-----------------------------------------------------------------------------------------------------*/
static ULONG _Get_string_descriptor_size(void)
{
  ULONG  size = 0;
  UCHAR *ptr  = (UCHAR *)usb_strings;
  if (NULL != ptr)
  {
    for (INT i = 0; i < 3; i++)
    {
      ptr  = ptr + 3; /* bLength at byte offset 3 */
      /* Counts bLength + Language code(2bytes) + bLength(1byte) */
      size = size + *ptr + 4;
      ptr  = ptr + (*ptr) + 1;
    }
  }
  return size;
}

/*-----------------------------------------------------------------------------------------------------


  \param void

  \return uint32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t _USB_get_hs_descr_sz(void)
{
  uint32_t sz = 0;

  sz += sizeof(device_hs_descriptor);
  sz += sizeof(config_hs_descriptor);

  switch (usb_1_interface_type)
  {
    case USB1_INTF_OFF:
      break;
    case USB1_INTF_VIRTUAL_COM_PORT:
      sz += sizeof(interface_cdc0_hs_descriptor);
      break;
    case USB1_INTF_MASS_STORAGE_DEVICE:
      sz += sizeof(interface_msd_hs_descriptor);
      break;
    case USB1_INTF_RNDIS_PORT:
      sz += sizeof(interface_rndis_hs_descriptor);
      break;
  }

  switch (usb_2_interface_type)
  {
    case USB2_INTF_OFF:
      break;
    case USB2_INTF_VIRTUAL_COM_PORT:
    case USB2_INTF_FREEMASTER_PORT:
      sz += sizeof(interface_cdc1_hs_descriptor);
      break;
    case USB2_INTF_MASS_STORAGE_DEVICE:
      sz += sizeof(interface_msd_hs_descriptor);
      break;
  }
  return sz;
}

/*-----------------------------------------------------------------------------------------------------



  \return uint32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t _USB_get_fs_descr_sz(void)
{
  uint32_t sz = 0;

  sz += sizeof(device_fs_descriptor);
  sz += sizeof(config_fs_descriptor);

  switch (usb_1_interface_type)
  {
    case USB1_INTF_OFF:
      break;
    case USB1_INTF_VIRTUAL_COM_PORT:
      sz += sizeof(interface_cdc0_fs_descriptor);
      break;
    case USB1_INTF_MASS_STORAGE_DEVICE:
      sz += sizeof(interface_msd_fs_descriptor);
      break;
    case USB1_INTF_RNDIS_PORT:
      sz += sizeof(interface_rndis_fs_descriptor);
      break;
  }

  switch (usb_2_interface_type)
  {
    case USB2_INTF_OFF:
      break;
    case USB2_INTF_VIRTUAL_COM_PORT:
    case USB2_INTF_FREEMASTER_PORT:
      sz += sizeof(interface_cdc1_fs_descriptor);
      break;
    case USB2_INTF_MASS_STORAGE_DEVICE:
      sz += sizeof(interface_msd_fs_descriptor);
      break;
  }
  return sz;
}

/*-----------------------------------------------------------------------------------------------------


  \param void
-----------------------------------------------------------------------------------------------------*/
void _USB_prepare_config_hs_descriptor(void)
{
  uint32_t sz   = 0;
  uint32_t icnt = 0;

  switch (usb_1_interface_type)
  {
    case USB1_INTF_OFF:
      break;
    case USB1_INTF_VIRTUAL_COM_PORT:
      sz += sizeof(interface_cdc0_hs_descriptor);
      icnt += 2;
      break;
    case USB1_INTF_MASS_STORAGE_DEVICE:
      sz += sizeof(interface_msd_hs_descriptor);
      icnt += 1;
      break;
    case USB1_INTF_RNDIS_PORT:
      sz += sizeof(interface_rndis_hs_descriptor);
      icnt += 2;
      break;
  }

  switch (usb_2_interface_type)
  {
    case USB2_INTF_OFF:
      break;
    case USB2_INTF_VIRTUAL_COM_PORT:
    case USB2_INTF_FREEMASTER_PORT:
      sz += sizeof(interface_cdc1_hs_descriptor);
      icnt += 2;
      break;
    case USB2_INTF_MASS_STORAGE_DEVICE:
      sz += sizeof(interface_msd_hs_descriptor);
      icnt += 1;
      break;
  }

  config_hs_descriptor.wTotalLength   = sizeof(config_hs_descriptor) + sz;
  config_hs_descriptor.bNumInterfaces = icnt;
}

/*-----------------------------------------------------------------------------------------------------


  \param void
-----------------------------------------------------------------------------------------------------*/
void _USB_prepare_config_fs_descriptor(void)
{
  uint32_t sz   = 0;
  uint32_t icnt = 0;

  switch (usb_1_interface_type)
  {
    case USB1_INTF_OFF:
      break;
    case USB1_INTF_VIRTUAL_COM_PORT:
      sz += sizeof(interface_cdc0_fs_descriptor);
      icnt += 2;
      break;
    case USB1_INTF_MASS_STORAGE_DEVICE:
      sz += sizeof(interface_msd_fs_descriptor);
      icnt += 1;
      break;
    case USB1_INTF_RNDIS_PORT:
      sz += sizeof(interface_rndis_fs_descriptor);
      icnt += 2;
      break;
  }

  switch (usb_2_interface_type)
  {
    case USB2_INTF_OFF:
      break;
    case USB2_INTF_VIRTUAL_COM_PORT:
    case USB2_INTF_FREEMASTER_PORT:
      sz += sizeof(interface_cdc1_fs_descriptor);
      icnt += 2;
      break;
    case USB2_INTF_MASS_STORAGE_DEVICE:
      sz += sizeof(interface_msd_fs_descriptor);
      icnt += 1;
      break;
  }

  config_fs_descriptor.wTotalLength   = sizeof(config_fs_descriptor) + sz;
  config_fs_descriptor.bNumInterfaces = icnt;
}

/*-----------------------------------------------------------------------------------------------------
   Initialization function that the user can choose to have called automatically during thread entry.
   The user can call this function at a later time if desired using the prototype below.

-----------------------------------------------------------------------------------------------------*/
static uint32_t _USB_dev_init(void)
{
  UINT status;

  /** Calculate the size of USBX String Framework. */
  ULONG  string_descr_sz;
  UCHAR *p_string_descr;

  {
    p_string_descr  = (UCHAR *)usb_strings;
    string_descr_sz = _Get_string_descriptor_size();
  }

  /** Calculate the size of USB Language Framework. */
  ULONG  lang_descr_sz;
  UCHAR *p_lang_descr;

  p_lang_descr  = (UCHAR *)usb_lang_descr;
  lang_descr_sz = 2;

  /** Initialize the USB Device stack. */

  uint8_t *pmem;
  uint32_t hs_descr_sz = _USB_get_hs_descr_sz();
  uint32_t fs_descr_sz = _USB_get_fs_descr_sz();

  if (USB_INTERFACE_TYPE == USB_DEV_INTERFACE_HIGH_SPEED_INTERFACE)
  {
    usbd_hs_framework = App_malloc_pending(hs_descr_sz, 10);
    if (usbd_hs_framework != NULL)
    {
      pmem = usbd_hs_framework;

      switch (usb_1_interface_type)
      {
        case USB1_INTF_VIRTUAL_COM_PORT:
        case USB1_INTF_MASS_STORAGE_DEVICE:
          memcpy(pmem, device_hs_descriptor, sizeof(device_hs_descriptor));
          pmem = pmem + sizeof(device_hs_descriptor);
          break;
        case USB1_INTF_RNDIS_PORT:
          memcpy(pmem, rndis_device_hs_descriptor, sizeof(rndis_device_hs_descriptor));
          pmem = pmem + sizeof(rndis_device_hs_descriptor);
          break;
      }

      _USB_prepare_config_hs_descriptor();
      memcpy(pmem, &config_hs_descriptor, sizeof(config_hs_descriptor));
      pmem = pmem + sizeof(config_hs_descriptor);

      switch (usb_1_interface_type)
      {
        case USB1_INTF_OFF:
          break;
        case USB1_INTF_VIRTUAL_COM_PORT:
          memcpy(pmem, interface_cdc0_hs_descriptor, sizeof(interface_cdc0_hs_descriptor));
          pmem = pmem + sizeof(interface_cdc0_hs_descriptor);
          break;
        case USB1_INTF_MASS_STORAGE_DEVICE:
          memcpy(pmem, interface_msd_hs_descriptor, sizeof(interface_msd_hs_descriptor));
          pmem = pmem + sizeof(interface_msd_hs_descriptor);
          break;
        case USB1_INTF_RNDIS_PORT:
          memcpy(pmem, interface_rndis_hs_descriptor, sizeof(interface_rndis_hs_descriptor));
          pmem = pmem + sizeof(interface_rndis_hs_descriptor);
          break;
      }

      switch (usb_2_interface_type)
      {
        case USB2_INTF_OFF:
          break;
        case USB2_INTF_VIRTUAL_COM_PORT:
        case USB2_INTF_FREEMASTER_PORT:
          memcpy(pmem, interface_cdc1_hs_descriptor, sizeof(interface_cdc1_hs_descriptor));
          pmem = pmem + sizeof(interface_cdc1_hs_descriptor);
          break;
        case USB2_INTF_MASS_STORAGE_DEVICE:
          memcpy(pmem, interface_msd_hs_descriptor, sizeof(interface_msd_hs_descriptor));
          pmem = pmem + sizeof(interface_msd_hs_descriptor);
          break;
      }
    }
  }

  usbd_fs_framework = App_malloc_pending(fs_descr_sz, 10);
  if (usbd_fs_framework != NULL)
  {
    pmem = usbd_fs_framework;

    switch (usb_1_interface_type)
    {
      case USB1_INTF_VIRTUAL_COM_PORT:
      case USB1_INTF_MASS_STORAGE_DEVICE:
        memcpy(pmem, device_fs_descriptor, sizeof(device_fs_descriptor));
        pmem = pmem + sizeof(device_fs_descriptor);
        break;
      case USB1_INTF_RNDIS_PORT:
        memcpy(pmem, rndis_device_fs_descriptor, sizeof(rndis_device_fs_descriptor));
        pmem = pmem + sizeof(rndis_device_fs_descriptor);
        break;
    }

    _USB_prepare_config_fs_descriptor();
    memcpy(pmem, &config_fs_descriptor, sizeof(config_fs_descriptor));
    pmem = pmem + sizeof(config_fs_descriptor);

    switch (usb_1_interface_type)
    {
      case USB1_INTF_OFF:
        break;
      case USB1_INTF_VIRTUAL_COM_PORT:
        memcpy(pmem, interface_cdc0_fs_descriptor, sizeof(interface_cdc0_fs_descriptor));
        pmem = pmem + sizeof(interface_cdc0_fs_descriptor);
        break;
      case USB1_INTF_MASS_STORAGE_DEVICE:
        memcpy(pmem, interface_msd_fs_descriptor, sizeof(interface_msd_fs_descriptor));
        pmem = pmem + sizeof(interface_msd_fs_descriptor);
        break;
      case USB1_INTF_RNDIS_PORT:
        memcpy(pmem, interface_rndis_fs_descriptor, sizeof(interface_rndis_fs_descriptor));
        pmem = pmem + sizeof(interface_rndis_fs_descriptor);
        break;
    }

    switch (usb_2_interface_type)
    {
      case USB2_INTF_OFF:
        break;
      case USB2_INTF_VIRTUAL_COM_PORT:
      case USB2_INTF_FREEMASTER_PORT:
        memcpy(pmem, interface_cdc1_fs_descriptor, sizeof(interface_cdc1_fs_descriptor));
        pmem = pmem + sizeof(interface_cdc1_fs_descriptor);
        break;
      case USB2_INTF_MASS_STORAGE_DEVICE:
        memcpy(pmem, interface_msd_fs_descriptor, sizeof(interface_msd_fs_descriptor));
        pmem = pmem + sizeof(interface_msd_fs_descriptor);
        break;
    }
  }

  if (USB_INTERFACE_TYPE == USB_DEV_INTERFACE_HIGH_SPEED_INTERFACE)
  {
    status = ux_device_stack_initialize((UCHAR *)usbd_hs_framework, hs_descr_sz, (UCHAR *)usbd_fs_framework, fs_descr_sz, p_string_descr, string_descr_sz, p_lang_descr, lang_descr_sz, ux_system_slave_change_callback);
  }
  else if (USB_INTERFACE_TYPE == USB_DEV_INTERFACE_FULL_SPEED_INTERFACE)
  {
    status = ux_device_stack_initialize(NULL, 0, (UCHAR *)usbd_fs_framework, fs_descr_sz, p_string_descr, string_descr_sz, p_lang_descr, lang_descr_sz, ux_system_slave_change_callback);
  }
  else
  {
    status = UX_ERROR;
  }

  return status;
}

/*-----------------------------------------------------------------------------------------------------
  The function initializes the USB device transport layer. It opens the USB driver and initializes the USB stack.

  \param void

  \return uint32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t _USB_device_transport_initialize(void)
{
  UINT      status;
  fsp_err_t usb_open_status;

  if (USB_INTERFACE_TYPE == USB_DEV_INTERFACE_HIGH_SPEED_INTERFACE)
  {
    status = usb_peri_usbx_initialize(R_USB_HS0_BASE);
    if (status != UX_SUCCESS)
    {
      APPLOG("USB: usb_peri_usbx_initialize (HS) failed: 0x%08lX", status);
      return status;  // Propagate the error
    }
  }
  else if (USB_INTERFACE_TYPE == USB_DEV_INTERFACE_FULL_SPEED_INTERFACE)
  {
    status = usb_peri_usbx_initialize(R_USB_FS0_BASE);
    if (status != UX_SUCCESS)
    {
      APPLOG("USB: usb_peri_usbx_initialize (FS) failed: 0x%08lX", status);
      return status;  // Propagate the error
    }
  }
  else
  {
    return UX_ERROR;
  }
  usb_open_status = R_USB_Open(&g_usb_basic_ctrl, &g_usb_basic_cfg);

  if (usb_open_status != FSP_SUCCESS)
  {
    APPLOG("USB: R_USB_Open failed: 0x%08lX", usb_open_status);
    return UX_ERROR;  // Indicate failure to open USB
  }

  return UX_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------


  \param void
-----------------------------------------------------------------------------------------------------*/
void Init_USB_stack(void)
{
  Set_usb_mode();

  if (wvar.usb_mode == USB_MODE_NONE) return;

  // Exit early if both USB interfaces are disabled
  if (usb_1_interface_type == USB1_INTF_OFF && usb_2_interface_type == USB2_INTF_OFF)
  {
    return;
  }

  memset(usb_mem_regular, 0, sizeof(usb_mem_regular));
  memset(usb_mem_cache_safe, 0, sizeof(usb_mem_cache_safe));

  ux_system_initialize((CHAR *)usb_mem_regular, USBX_REGULAR_MEMORY_SIZE, usb_mem_cache_safe, USBX_CACHE_SAFE_MEMORY_SIZE);

  _USB_dev_init();
  switch (usb_1_interface_type)
  {
    case USB1_INTF_OFF:
      break;
    case USB1_INTF_VIRTUAL_COM_PORT:
      USB_cdc0_setup(Monitor_USB_Init_callback, Monitor_USB_DeInit_callback);
      break;
    case USB1_INTF_MASS_STORAGE_DEVICE:
      USB_storage_setup();
      break;
    case USB1_INTF_RNDIS_PORT:
      // Register_rndis_class();
      break;
  }

  switch (usb_2_interface_type)
  {
    case USB2_INTF_OFF:
      break;
    case USB2_INTF_VIRTUAL_COM_PORT:
      USB_cdc1_setup(Monitor_USB_Init_callback, Monitor_USB_DeInit_callback);
      break;
    case USB2_INTF_FREEMASTER_PORT:
      USB_cdc1_setup(Freemaster_USB1_init_callback, Freemaster_USB1_deinit_callback);
      break;
    case USB2_INTF_MASS_STORAGE_DEVICE:
      USB_storage_setup();
      break;
  }
  _USB_device_transport_initialize();  // Инициализируем драйвер USB девайса

  if (wvar.usb_mode == USB_MODE_RNDIS)
  {
    // ux_network_driver_init();  // Запускаем сетевой драйвер чере USB
  }
  Wait_ms(10);
}
