#ifndef USB_TRANSFER_OBJ_H
#define USB_TRANSFER_OBJ_H

extern const transfer_instance_t g_usb_transfer_rx1;
extern dmac_instance_ctrl_t      g_usb_transfer_rx1_ctrl;
extern const transfer_cfg_t      g_usb_transfer_rx1_cfg;
extern const transfer_instance_t g_usb_transfer_tx1;
extern dmac_instance_ctrl_t      g_usb_transfer_tx1_ctrl;
extern const transfer_cfg_t      g_usb_transfer_tx1_cfg;
#ifndef usb_ip0_d1fifo_callback
void usb_ip0_d1fifo_callback(transfer_callback_args_t* p_args);
#endif
extern const usb_instance_t g_usb_basic;
extern usb_instance_ctrl_t  g_usb_basic_ctrl;
extern const usb_cfg_t      g_usb_basic_cfg;

#endif
