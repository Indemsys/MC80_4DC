#ifndef FREEMASTER_USB_FS_DRV_H
  #define FREEMASTER_USB_FS_DRV_H


  #define   USB_DRV_STR_SZ               512
  #define   USB_DRV_RECV_THREAD_STACK_SZ 1024
  #define   FMUSBDRV_IN_BUF_QUANTITY     2           // Количество приемных буферов
  #define   FMUSBDRV_BUFFER_MAX_LENGTH   512


typedef struct
{
    const uint32_t   mark;
    const int        driver_type;
    int              (*_init)(void **pcbl, void *pdrv);
    int              (*_send_buf)(const void *buf, unsigned int len);
    int              (*_wait_char)(unsigned char *b,  int ticks); // ticks - время ожидания выражается в тиках (если 0 то без ожидания)
    int              (*_printf)(const char *, ...);               // Возвращает неопределенный результат
    int              (*_deinit)(void **pcbl);
    void              *drv_cbl_ptr;                               // Указатель на управляющую структуру необходимую для работы драйвера
} T_freemaster_serial_driver;


// Ведем прием циклически в N приемных буферов
typedef struct
{
    uint32_t  len;  // Длина пакета
    uint8_t   buff[FMUSBDRV_BUFFER_MAX_LENGTH]; // Буфер с пакетов

} T_fm_usb_rx_pack_cbl;


typedef struct
{
    volatile uint8_t       active;
    UX_SLAVE_CLASS_CDC_ACM *cdc;
    char                   str[USB_DRV_STR_SZ];
    TX_THREAD              recv_thread; // Задача прием
    char                   recv_task_name[32];
    uint8_t                recv_thread_stack[USB_DRV_RECV_THREAD_STACK_SZ];
    TX_EVENT_FLAGS_GROUP   event_flags;         // Группа флагов для взаимодействия с задачей приема
    char                   event_flags_name[32];
    void                  *dbuf;        // Указатель на буфер с принимаемыми данными
    uint32_t               dsz;         // Количество принимаемых байт

    volatile int32_t       head_buf_indx;     //  Индекс головы циклической очереди буферов приема                      
    volatile int32_t       tail_buf_indx;     //  Индекс хвоста циклической очереди буферов приема                      
    volatile int32_t       all_buffers_full;  //  Сигнал о том что все приемные буферы заняты принятыми данными         
    int32_t                tail_buf_rd_pos;   //  Позиция чтения в хвостовом буфере                                     
    T_fm_usb_rx_pack_cbl   in_bufs_ring[FMUSBDRV_IN_BUF_QUANTITY]; // Кольцо управляющих структур приема-обработки входящих пакетов


} T_fm_usb_drv_cbl;


T_freemaster_serial_driver* Get_FreeMaster_usb_vcom0_driver(void);
T_freemaster_serial_driver* Get_FreeMaster_usb_vcom1_driver(void);

VOID Freemaster_USB0_init_callback(VOID *cdc_instance);
VOID Freemaster_USB0_deinit_callback(VOID *cdc_instance);
VOID Freemaster_USB1_init_callback(VOID *cdc_instance);
VOID Freemaster_USB1_deinit_callback(VOID *cdc_instance);


#endif // MONITOR_USB_FS_DRV_H



