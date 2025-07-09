// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2024-09-12
// 22:20:19
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include   "App.h"

static UX_SLAVE_CLASS_CDC_ACM_PARAMETER          cdc0_parms;
static UX_SLAVE_CLASS_CDC_ACM_PARAMETER          cdc1_parms;


/*-----------------------------------------------------------------------------------------------------


  \param void
-----------------------------------------------------------------------------------------------------*/
void USB_cdc0_setup(T_cdc_acm_evt_callback init_callback, T_cdc_acm_evt_callback deinit_callback)
{
  cdc0_parms.ux_slave_class_cdc_acm_instance_activate   = init_callback;
  cdc0_parms.ux_slave_class_cdc_acm_instance_deactivate = deinit_callback;
  /* Initializes the device cdc class. */
  ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry, 1, 0x00, (VOID *)&cdc0_parms);
}

/*-----------------------------------------------------------------------------------------------------


  \param void
-----------------------------------------------------------------------------------------------------*/
void USB_cdc1_setup(T_cdc_acm_evt_callback init_callback, T_cdc_acm_evt_callback deinit_callback)
{
  cdc1_parms.ux_slave_class_cdc_acm_instance_activate   = init_callback;
  cdc1_parms.ux_slave_class_cdc_acm_instance_deactivate = deinit_callback;
  /* Initializes the device cdc class. */
  ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry, 1, 0x02, (VOID *)&cdc1_parms);
}

