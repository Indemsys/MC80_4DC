#ifndef USB_CDC_H
  #define USB_CDC_H


void USB_cdc0_setup(T_cdc_acm_evt_callback init_callback, T_cdc_acm_evt_callback deinit_callback);
void USB_cdc1_setup(T_cdc_acm_evt_callback init_callback, T_cdc_acm_evt_callback deinit_callback);




#endif 



