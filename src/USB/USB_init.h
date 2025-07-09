#ifndef USB_INIT_H
#define USB_INIT_H

#define USB_DEV_INTERFACE_FULL_SPEED_INTERFACE 0
#define USB_DEV_INTERFACE_HIGH_SPEED_INTERFACE 1
#define USB_INTERFACE_TYPE                     USB_DEV_INTERFACE_FULL_SPEED_INTERFACE

// Selector description:  USB1 interface mode
#define USB1_INTF_OFF                          0
#define USB1_INTF_VIRTUAL_COM_PORT             1
#define USB1_INTF_MASS_STORAGE_DEVICE          2
#define USB1_INTF_RNDIS_PORT                   3

// Selector description:  USB2 interface mode
#define USB2_INTF_OFF                          0
#define USB2_INTF_VIRTUAL_COM_PORT             1
#define USB2_INTF_MASS_STORAGE_DEVICE          2
#define USB2_INTF_FREEMASTER_PORT              3

typedef void (*T_cdc_acm_evt_callback)(void *);

typedef struct
{
  ULONG idVendor;
  ULONG idProduct;
  ULONG dev_state;
  ULONG interface_id;
  ULONG interface_num;

} T_usb_app_info;

typedef void (*T_usb_device_suspend_callback)(void);

extern T_usb_app_info g_uinf;

void    Init_USB_stack(void);
uint8_t Get_usb_1_mode(void);
uint8_t Get_usb_2_mode(void);
void    Set_usb_mode(void);
void    USB_device_set_suspend_callback(T_usb_device_suspend_callback callback_func);

#endif  // USB_INIT_H
