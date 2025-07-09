//----------------------------------------------------------------------
// File created on 2025-04-02
//----------------------------------------------------------------------
#include "App.h"

dmac_instance_ctrl_t g_usb_transfer_rx1_ctrl;
transfer_info_t      g_usb_transfer_rx1_info = {
       .transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED,
       .transfer_settings_word_b.repeat_area    = TRANSFER_REPEAT_AREA_SOURCE,
       .transfer_settings_word_b.irq            = TRANSFER_IRQ_END,
       .transfer_settings_word_b.chain_mode     = TRANSFER_CHAIN_MODE_DISABLED,
       .transfer_settings_word_b.src_addr_mode  = TRANSFER_ADDR_MODE_FIXED,
       .transfer_settings_word_b.size           = TRANSFER_SIZE_2_BYTE,
       .transfer_settings_word_b.mode           = TRANSFER_MODE_BLOCK,
       .p_dest                                  = (void *)0,
       .p_src                                   = (void const *)USB_SRC_ADDRESS,
       .num_blocks                              = 0,
       .length                                  = 0,
};
const dmac_extended_cfg_t g_usb_transfer_rx1_extend = {
  .offset          = 0,
  .src_buffer_size = 1,
#if defined(VECTOR_NUMBER_DMAC0_INT)
  .irq = VECTOR_NUMBER_DMAC0_INT,
#else
  .irq = FSP_INVALID_VECTOR,
#endif
  .ipl               = (3),
  .channel           = 0,
  .p_callback        = NULL,
  .p_context         = NULL,
  .activation_source = ELC_EVENT_USBFS_FIFO_0,
};
const transfer_cfg_t g_usb_transfer_rx1_cfg = {
  .p_info   = &g_usb_transfer_rx1_info,
  .p_extend = &g_usb_transfer_rx1_extend,
};
/* Instance structure to use this module. */
const transfer_instance_t g_usb_transfer_rx1 = {
  .p_ctrl = &g_usb_transfer_rx1_ctrl,
  .p_cfg  = &g_usb_transfer_rx1_cfg,
  .p_api  = &g_transfer_on_dmac
};
dmac_instance_ctrl_t g_usb_transfer_tx1_ctrl;
transfer_info_t      g_usb_transfer_tx_info = {
       .transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_FIXED,
       .transfer_settings_word_b.repeat_area    = TRANSFER_REPEAT_AREA_DESTINATION,
       .transfer_settings_word_b.irq            = TRANSFER_IRQ_END,
       .transfer_settings_word_b.chain_mode     = TRANSFER_CHAIN_MODE_DISABLED,
       .transfer_settings_word_b.src_addr_mode  = TRANSFER_ADDR_MODE_INCREMENTED,
       .transfer_settings_word_b.size           = TRANSFER_SIZE_2_BYTE,
       .transfer_settings_word_b.mode           = TRANSFER_MODE_BLOCK,
       .p_dest                                  = (void *)USB_DEST_ADDRESS,
       .p_src                                   = (void const *)0,
       .num_blocks                              = 0,
       .length                                  = 0,
};
const dmac_extended_cfg_t g_usb_transfer_tx_extend = {
  .offset          = 0,
  .src_buffer_size = 1,
#if defined(VECTOR_NUMBER_DMAC1_INT)
  .irq = VECTOR_NUMBER_DMAC1_INT,
#else
  .irq = FSP_INVALID_VECTOR,
#endif
  .ipl               = (3),
  .channel           = 1,
  .p_callback        = usb_ip0_d1fifo_callback,
  .p_context         = NULL,
  .activation_source = ELC_EVENT_USBFS_FIFO_1,
};
const transfer_cfg_t g_usb_transfer_tx1_cfg = {
  .p_info   = &g_usb_transfer_tx_info,
  .p_extend = &g_usb_transfer_tx_extend,
};
/* Instance structure to use this module. */
const transfer_instance_t g_usb_transfer_tx1 = {
  .p_ctrl = &g_usb_transfer_tx1_ctrl,
  .p_cfg  = &g_usb_transfer_tx1_cfg,
  .p_api  = &g_transfer_on_dmac
};
usb_instance_ctrl_t g_usb_basic_ctrl;

#if !defined(NULL)
extern usb_descriptor_t NULL;
#endif
#define RA_NOT_DEFINED (1)
const usb_cfg_t g_usb_basic_cfg = {
  .usb_mode      = USB_MODE_PERI,
  .usb_speed     = USB_SPEED_FS,
  .module_number = 0,
  .type          = USB_CLASS_PCDC,
#if defined(NULL)
  .p_usb_reg = NULL,
#else
  .p_usb_reg = &NULL,
#endif
  .usb_complience_cb = NULL,
#if defined(VECTOR_NUMBER_USBFS_INT)
  .irq = VECTOR_NUMBER_USBFS_INT,
#else
  .irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_USBFS_RESUME)
  .irq_r = VECTOR_NUMBER_USBFS_RESUME,
#else
  .irq_r = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_USBFS_FIFO_0)
  .irq_d0 = VECTOR_NUMBER_USBFS_FIFO_0,
#else
  .irq_d0 = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_USBFS_FIFO_1)
  .irq_d1 = VECTOR_NUMBER_USBFS_FIFO_1,
#else
  .irq_d1 = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_USBHS_USB_INT_RESUME)
  .hsirq = VECTOR_NUMBER_USBHS_USB_INT_RESUME,
#else
  .hsirq = FSP_INVALID_VECTOR,
#endif
  .irq_typec = FSP_INVALID_VECTOR,
#if defined(VECTOR_NUMBER_USBHS_FIFO_0)
  .hsirq_d0 = VECTOR_NUMBER_USBHS_FIFO_0,
#else
  .hsirq_d0 = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_USBHS_FIFO_1)
  .hsirq_d1 = VECTOR_NUMBER_USBHS_FIFO_1,
#else
  .hsirq_d1 = FSP_INVALID_VECTOR,
#endif
  .ipl       = (12),
  .ipl_r     = (12),
  .ipl_d0    = (12),
  .ipl_d1    = (12),
  .hsipl     = (BSP_IRQ_DISABLED),
  .ipl_typec = BSP_IRQ_DISABLED,
  .hsipl_d0  = (BSP_IRQ_DISABLED),
  .hsipl_d1  = (BSP_IRQ_DISABLED),
#if (BSP_CFG_RTOS == 0) && defined(USB_CFG_HMSC_USE)
  .p_usb_apl_callback = NULL,
#else
  .p_usb_apl_callback = NULL,
#endif
#if defined(NULL)
  .p_context = NULL,
#else
  .p_context = &NULL,
#endif
#if (RA_NOT_DEFINED == g_usb_transfer_tx1)
#else
  .p_transfer_tx = &g_usb_transfer_tx1,
#endif
#if (RA_NOT_DEFINED == g_usb_transfer_rx1)
#else
  .p_transfer_rx = &g_usb_transfer_rx1,
#endif
};
#undef RA_NOT_DEFINED

/* Instance structure to use this module. */
const usb_instance_t g_usb_basic = {
  .p_ctrl = &g_usb_basic_ctrl,
  .p_cfg  = &g_usb_basic_cfg,
  .p_api  = &g_usb_on_usb,
};
