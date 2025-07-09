#ifndef APP_USB_STORAGE_H
#define APP_USB_STORAGE_H

UINT     ux_device_msc_media_read(VOID *storage, ULONG lun, UCHAR *data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status);
UINT     ux_device_msc_media_write(VOID *storage, ULONG lun, UCHAR *data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status);
UINT     ux_device_msc_media_status(VOID *storage, ULONG lun, ULONG media_id, ULONG *media_status);
uint32_t USB_storage_setup(void);



#endif // APP_USB_STORAGE_H



