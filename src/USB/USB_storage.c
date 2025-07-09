// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2019.08.04
// 21:10:51
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include   "App.h"

static UX_SLAVE_CLASS_STORAGE_PARAMETER          storage_parms;

/*-----------------------------------------------------------------------------------------------------


  \param storage
  \param lun
  \param data_pointer
  \param number_blocks
  \param lba
  \param media_status

  \return UINT
-----------------------------------------------------------------------------------------------------*/
UINT ux_device_msc_media_read(VOID *storage, ULONG lun, UCHAR *data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status)
{
  fsp_err_t     res;

  res = g_rm_sdmmc_block_media.p_api->read(g_rm_sdmmc_block_media.p_ctrl, data_pointer, lba, number_blocks);
  if (res != FSP_SUCCESS)
  {
    *media_status = 0x00FFFF02; // UNKNOWN ERROR
    return UX_SUCCESS;
  }

  *media_status = 0x00;
  return UX_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------


  \param storage
  \param lun
  \param data_pointer
  \param number_blocks
  \param lba
  \param media_status

  \return UINT
-----------------------------------------------------------------------------------------------------*/
UINT ux_device_msc_media_write(VOID *storage, ULONG lun, UCHAR *data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status)
{
  UINT     res;

  if (lba == 0)
  {
    *media_status = 0x00002105; // LOGICAL BLOCK ADDRESS OUT OF RANGE
    return UX_SUCCESS;
  }
  res = g_rm_sdmmc_block_media.p_api->write(g_rm_sdmmc_block_media.p_ctrl, data_pointer, lba, number_blocks);
  if (res != FSP_SUCCESS)
  {
     *media_status = 0x00000303; // WRITE FAULT
    return UX_SUCCESS;
  }

  *media_status = 0x00;
  return UX_SUCCESS;
}

/*-----------------------------------------------------------------------------------------------------


  \param storage
  \param lun
  \param media_id
  \param media_status

  \return UINT
-----------------------------------------------------------------------------------------------------*/
UINT ux_device_msc_media_status(VOID *storage, ULONG lun, ULONG media_id, ULONG *media_status)
{
  *media_status = 0x00;
  return UX_SUCCESS;
}


/*-----------------------------------------------------------------------------------------------------


-----------------------------------------------------------------------------------------------------*/
uint32_t USB_storage_setup(void)
{
  UINT status;

 /* Auto setup for a simple media storage configuration with single Logical Unit Number (LUN). */
 /* Stores the number of LUN in this device storage instance.  */
 storage_parms.ux_slave_class_storage_parameter_number_lun = 1;
 storage_parms.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_last_lba       = fat_fs_media.fx_media_total_sectors;
 storage_parms.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_block_length   = fat_fs_media.fx_media_bytes_per_sector;
 storage_parms.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_type           = 0;
 storage_parms.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_removable_flag = 0x80;
 storage_parms.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_read           = ux_device_msc_media_read;
 storage_parms.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_write          = ux_device_msc_media_write;
 storage_parms.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_status         = ux_device_msc_media_status;

 /* Register user callback functions.  */
 storage_parms.ux_slave_class_storage_instance_activate         = NULL;
 storage_parms.ux_slave_class_storage_instance_deactivate       = NULL;
 storage_parms.ux_slave_class_storage_parameter_vendor_id       = (UCHAR *) "NULL";
 storage_parms.ux_slave_class_storage_parameter_product_id      = (UCHAR *) "NULL";
 storage_parms.ux_slave_class_storage_parameter_product_rev     = (UCHAR *) "NULL";
 storage_parms.ux_slave_class_storage_parameter_product_serial  = (UCHAR *) "NULL";

 /* Initializes the device storage class. The class is connected with interface 0 on configuration 1. */
 //status =  _ux_device_stack_class_register(_ux_system_slave_class_storage_name, ux_device_class_storage_entry, 1, 0x05, (VOID *)&storage_parms);
  return status;
}
